#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./_common.sh
source "${SCRIPT_DIR}/_common.sh"

require_tools

tmp_dir="$(mktemp -d)"
orphan_tmp=""
cleanup() {
    cleanup_http_artifacts
    if [[ -n "${orphan_tmp}" && -f "${orphan_tmp}" ]]; then
        rm -f "${orphan_tmp}"
    fi
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

REPO_ROOT="${REPO_ROOT:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
LOGS_DIR="${LOGS_DIR:-/tmp/mqttsuite-scenario-logs}"
INTEGRATOR_CMD_DEFAULT="./build/mqttintegrator/mqttintegrator integrator --mqtt-mapping-file mapfile.json"
INTEGRATOR_CMD="${INTEGRATOR_CMD:-${INTEGRATOR_CMD_DEFAULT}}"
PID_FILE="${LOGS_DIR}/mqttintegrator-wrapper.pid"
WRAPPER_LOG="${LOGS_DIR}/mqttintegrator-wrapper.log"

# Mirrors the --mqtt-mapping-file argument in INTEGRATOR_CMD, resolved against
# the integrator's cwd (REPO_ROOT), since ConfigApplication::persistMapping()
# writes to whatever path was passed on the command line.
MAPPING_FILE="${MAPPING_FILE:-${REPO_ROOT}/mapfile.json}"

log "Scenario 10: Active-config atomic persistence and restart recovery (NFR-04 / FR-04)"

if [[ ! -f "${MAPPING_FILE}" ]]; then
    fail "Mapping file not found: ${MAPPING_FILE}"
fi

# --- Part A: a real deploy exercises persistMapping() and lands on disk ----
active_revision_before="$(get_active_revision)"
draft_id="$(unique_draft_id "s10crash")"

log "Creating draft ${draft_id} from active revision ${active_revision_before}"
create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"
draft_revision="$(json_must_get '.draft_revision')"

get_draft "${draft_id}"
assert_status 200 "POST /drafts/get"

mapping_file="${tmp_dir}/mapping.json"
updated_mapping_file="${tmp_dir}/mapping.updated.json"
jq '.mapping' "${HTTP_BODY_FILE}" >"${mapping_file}"
marker_a="scenario10-${draft_id}-$(date -u +%Y%m%dT%H%M%SZ)"
jq --arg marker "${marker_a}" '.meta.comment = $marker' "${mapping_file}" >"${updated_mapping_file}"

replace_draft_from_file "${draft_id}" "${draft_revision}" "${updated_mapping_file}"
assert_status 200 "POST /drafts/replace"

deploy_draft "${draft_id}" "${active_revision_before}"
assert_status 200 "POST /drafts/deploy"

deploy_status="$(json_must_get '.status')"
deployed_revision_a="$(json_must_get '.revision')"
if [[ "${deploy_status}" != "persist-ack" ]]; then
    fail "Expected deploy status=persist-ack (mappingPersisted=true), got '${deploy_status}'"
fi

log "Deploy A acknowledged: status=${deploy_status}, revision=${deployed_revision_a}"

# Round-trip check: confirm the atomic write in persistMapping() actually put
# the new content on disk, not just in memory. Prior scenarios never opened
# the mapping file directly, so this is new coverage.
on_disk_comment_a="$(jq -r '.meta.comment // ""' "${MAPPING_FILE}")"
on_disk_revision_a="$(jq -r '.meta.revision // 0' "${MAPPING_FILE}")"
if [[ "${on_disk_comment_a}" != "${marker_a}" ]]; then
    fail "On-disk mapping file comment '${on_disk_comment_a}' does not match deployed marker '${marker_a}'"
fi
if [[ "${on_disk_revision_a}" -ne "${deployed_revision_a}" ]]; then
    fail "On-disk mapping file revision ${on_disk_revision_a} does not match deployed revision ${deployed_revision_a}"
fi

log "On-disk mapping file confirmed to match deployed revision ${deployed_revision_a}"

# --- Part B: simulate a crash between temp-file creation and rename --------
# ConfigApplication::persistMapping() writes to <mappingFile>.tmp, fsyncs it,
# then atomically renames it onto the target. A process killed in that window
# leaves exactly this artifact behind: an orphaned .tmp file next to an
# untouched, still-valid canonical file.
canonical_backup="${tmp_dir}/canonical_mapfile.backup.json"
cp "${MAPPING_FILE}" "${canonical_backup}"

orphan_tmp="${MAPPING_FILE}.tmp"
printf '{ "connection": { "client_id": "truncated-mid-write"' >"${orphan_tmp}"

if ! cmp -s "${MAPPING_FILE}" "${canonical_backup}"; then
    fail "Canonical mapping file changed unexpectedly while simulating a partial write"
fi

log "Simulated orphan temp file: ${orphan_tmp}"
log "Verifying canonical mapping file is untouched and the live process is unaffected"

request_json GET "/config"
assert_status 200 "GET /config after simulated crash"
live_comment="$(json_must_get '.meta.comment')"
live_revision="$(json_must_get '.meta.revision')"
if [[ "${live_comment}" != "${marker_a}" || "${live_revision}" -ne "${deployed_revision_a}" ]]; then
    fail "Live active config changed unexpectedly after simulating a crash artifact on disk"
fi

# --- Part C: the orphaned temp file must not block the next real deploy ----
draft_id_b="$(unique_draft_id "s10crash2")"
create_draft "${draft_id_b}"
assert_status 201 "POST /drafts/create (second draft)"
draft_revision_b="$(json_must_get '.draft_revision')"

get_draft "${draft_id_b}"
assert_status 200 "POST /drafts/get (second draft)"

mapping_file_b="${tmp_dir}/mapping.b.json"
updated_mapping_file_b="${tmp_dir}/mapping.b.updated.json"
jq '.mapping' "${HTTP_BODY_FILE}" >"${mapping_file_b}"
marker_b="scenario10-${draft_id_b}-$(date -u +%Y%m%dT%H%M%SZ)"
jq --arg marker "${marker_b}" '.meta.comment = $marker' "${mapping_file_b}" >"${updated_mapping_file_b}"

replace_draft_from_file "${draft_id_b}" "${draft_revision_b}" "${updated_mapping_file_b}"
assert_status 200 "POST /drafts/replace (second draft)"

deploy_draft "${draft_id_b}" "${deployed_revision_a}"
assert_status 200 "POST /drafts/deploy (second draft, orphan tmp still present)"

deployed_revision_b="$(json_must_get '.revision')"
deploy_status_b="$(json_must_get '.status')"
if [[ "${deploy_status_b}" != "persist-ack" ]]; then
    fail "Expected second deploy status=persist-ack, got '${deploy_status_b}'"
fi

on_disk_comment_b="$(jq -r '.meta.comment // ""' "${MAPPING_FILE}")"
if [[ "${on_disk_comment_b}" != "${marker_b}" ]]; then
    fail "On-disk mapping file comment '${on_disk_comment_b}' does not reflect the second deploy '${marker_b}'"
fi

# The orphaned .tmp is overwritten (same fixed name) by the next real write,
# so it should no longer contain the truncated garbage from Part B.
if [[ -f "${orphan_tmp}" ]] && grep -q "truncated-mid-write" "${orphan_tmp}" 2>/dev/null; then
    fail "Orphaned temp file still contains simulated-crash garbage after a subsequent successful deploy"
fi
orphan_tmp=""

log "Second deploy succeeded with orphan tmp present; revision now ${deployed_revision_b}"

record_metric "scenario10_recovery" "fr04_active_config_atomic_persistence_verified" "1" "bool"

# --- Part D: restart recovery (NFR-04) --------------------------------------
# Only meaningful when this scenario is driven by run_all_scenarios.sh, which
# tracks the managed integrator pid the same way scenario9 does.
if [[ ! -f "${PID_FILE}" ]]; then
    log "SKIP: ${PID_FILE} not found; restart-recovery check requires the"
    log "server to have been started by run_all_scenarios.sh (START_INTEGRATOR=1)."
    printf '\nScenario 10 PARTIAL\n'
    printf 'Atomic persistence of the active mapping file verified (Parts A-C).\n'
    printf 'Restart-recovery check skipped (no managed pid file found).\n'
    exit 0
fi

fine_wait_for_api() {
    local max_wait_ms="${1:-30000}"
    local waited_ms=0
    while ((waited_ms < max_wait_ms)); do
        if check_api; then
            return 0
        fi
        sleep 0.02
        waited_ms=$((waited_ms + 20))
    done
    return 1
}

old_pid="$(cat "${PID_FILE}")"
if ! kill -0 "${old_pid}" >/dev/null 2>&1; then
    fail "Tracked mqttintegrator pid ${old_pid} is not running; cannot verify restart recovery"
fi

log "Stopping mqttintegrator (pid=${old_pid}) to verify cold-boot recovery"
kill "${old_pid}" >/dev/null 2>&1 || true

stop_waited=0
while kill -0 "${old_pid}" >/dev/null 2>&1 && ((stop_waited < 10000)); do
    sleep 0.05
    stop_waited=$((stop_waited + 50))
done
if kill -0 "${old_pid}" >/dev/null 2>&1; then
    fail "mqttintegrator pid ${old_pid} did not exit within 10s of SIGTERM"
fi

nohup /usr/bin/env bash -c "cd \"${REPO_ROOT}\" && exec ${INTEGRATOR_CMD}" >>"${WRAPPER_LOG}" 2>&1 &
new_pid="$!"
echo "${new_pid}" >"${PID_FILE}"

if ! fine_wait_for_api 30000; then
    tail -n 40 "${WRAPPER_LOG}" || true
    fail "mqttintegrator did not become ready within 30s of restart"
fi

log "mqttintegrator restarted (pid=${new_pid}); verifying recovered active config"

request_json GET "/config"
assert_status 200 "GET /config after restart"
recovered_comment="$(json_must_get '.meta.comment')"
recovered_revision="$(json_must_get '.meta.revision')"

if [[ "${recovered_comment}" != "${marker_b}" ]]; then
    fail "After restart, expected active config marker '${marker_b}', got '${recovered_comment}'"
fi
if [[ "${recovered_revision}" -ne "${deployed_revision_b}" ]]; then
    fail "After restart, expected active revision ${deployed_revision_b}, got ${recovered_revision}"
fi

record_metric "scenario10_recovery" "nfr04_restart_recovery_verified" "1" "bool"

printf '\nScenario 10 PASS\n'
printf 'Simulated crash temp file did not corrupt the canonical active mapping file.\n'
printf 'A subsequent deploy succeeded normally with the orphan tmp file present.\n'
printf 'After a real process restart, the active config recovered to revision %s (marker: %s).\n' \
    "${deployed_revision_b}" "${marker_b}"
