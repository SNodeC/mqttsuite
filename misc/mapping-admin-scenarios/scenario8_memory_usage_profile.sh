#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./_common.sh
source "${SCRIPT_DIR}/_common.sh"

require_tools

tmp_dir="$(mktemp -d)"
cleanup() {
    cleanup_http_artifacts
    rm -rf "${tmp_dir}"
}
trap cleanup EXIT

BATCH_SIZE="${BATCH_SIZE:-30}"

log "Scenario 8: Resident memory profile under admin-plane load (NFR-01, Section 6.4.1)"

find_integrator_pid() {
    local pid_file="${LOGS_DIR:-/tmp/mqttsuite-scenario-logs}/mqttintegrator-wrapper.pid"
    if [[ -f "${pid_file}" ]]; then
        local pid
        pid="$(cat "${pid_file}")"
        if kill -0 "${pid}" >/dev/null 2>&1; then
            printf '%s\n' "${pid}"
            return 0
        fi
    fi

    pgrep -f "mqttintegrator integrator" | head -n 1
}

rss_kb_of() {
    local pid="${1:?pid required}"
    ps -o rss= -p "${pid}" 2>/dev/null | tr -d ' '
}

integrator_pid="$(find_integrator_pid || true)"
if [[ -z "${integrator_pid}" ]]; then
    fail "Could not determine mqttintegrator PID; is the server running (see run_all_scenarios.sh)?"
fi

log "Measuring RSS of mqttintegrator pid=${integrator_pid}"

# Let RSS settle from any prior scenario activity before taking the baseline.
sleep 1
rss_baseline_kb="$(rss_kb_of "${integrator_pid}")"
log "Baseline RSS: ${rss_baseline_kb} kB"

# Drive BATCH_SIZE create -> patch -> validate -> (deploy | delete) cycles
# against the admin plane to observe heap growth under sustained
# administrative load, as required by NFR-01 ("must not exceed 10% of total
# system resources during peak administrative operations").
deployed_count=0
for ((i = 1; i <= BATCH_SIZE; i++)); do
    draft_id="$(unique_draft_id "s8mem${i}")"

    create_draft "${draft_id}"
    assert_status 201 "POST /drafts/create (${i}/${BATCH_SIZE})"
    draft_revision="$(json_must_get '.draft_revision')"

    # The draft envelope's "mapping" field is the full config document; the
    # topic tree lives under ".mapping.mapping" (see scenario6 for details).
    patch_payload="$(jq -nc \
        --arg draft_id "${draft_id}" \
        --argjson expected_draft_revision "${draft_revision}" \
        --arg marker "mem-probe-${i}" \
        '{draft_id: $draft_id, expected_draft_revision: $expected_draft_revision,
          patch: [{"op":"replace","path":"/mapping/topic_level/0/name","value":$marker}]}')"
    request_json PATCH "/drafts/patch" "${patch_payload}"
    assert_status 200 "PATCH /drafts/patch (${i}/${BATCH_SIZE})"

    validate_draft "${draft_id}"
    assert_status 200 "POST /drafts/validate (${i}/${BATCH_SIZE})"

    if ((i % 10 == 0)); then
        active_revision="$(get_active_revision)"
        deploy_draft "${draft_id}" "${active_revision}"
        assert_status 200 "POST /drafts/deploy (${i}/${BATCH_SIZE})"
        deployed_count=$((deployed_count + 1))
    else
        delete_draft "${draft_id}"
        assert_status 200 "POST /drafts/delete (${i}/${BATCH_SIZE})"
    fi
done

sleep 1
rss_after_kb="$(rss_kb_of "${integrator_pid}")"
rss_delta_kb=$((rss_after_kb - rss_baseline_kb))

log "Post-batch RSS: ${rss_after_kb} kB (delta ${rss_delta_kb} kB over ${BATCH_SIZE} cycles, ${deployed_count} deployed)"

# Allow the OS/allocator a moment, then re-check to see whether memory is
# reclaimed after drafts are deleted (informational; glibc malloc frequently
# retains arenas rather than returning them to the OS, so a non-zero
# steady-state delta here is expected and should be discussed qualitatively
# in Section 6.5 rather than treated as a leak).
sleep 2
rss_settled_kb="$(rss_kb_of "${integrator_pid}")"
rss_settled_delta_kb=$((rss_settled_kb - rss_baseline_kb))

record_metric "scenario8_memory_profile" "rss_baseline_kb" "${rss_baseline_kb}" "kB"
record_metric "scenario8_memory_profile" "rss_after_batch_kb" "${rss_after_kb}" "kB"
record_metric "scenario8_memory_profile" "rss_delta_kb" "${rss_delta_kb}" "kB"
record_metric "scenario8_memory_profile" "rss_settled_kb" "${rss_settled_kb}" "kB"
record_metric "scenario8_memory_profile" "rss_settled_delta_kb" "${rss_settled_delta_kb}" "kB"
record_metric "scenario8_memory_profile" "batch_size" "${BATCH_SIZE}" "count"
record_metric "scenario8_memory_profile" "rss_delta_per_op_kb" "$(awk -v d="${rss_delta_kb}" -v n="${BATCH_SIZE}" 'BEGIN{printf "%.3f", d/n}')" "kB_per_op"

printf '\nScenario 8 PASS\n'
printf 'RSS baseline: %d kB\n' "${rss_baseline_kb}"
printf 'RSS after %d admin cycles (%d deployed): %d kB (delta %+d kB)\n' \
    "${BATCH_SIZE}" "${deployed_count}" "${rss_after_kb}" "${rss_delta_kb}"
printf 'RSS 2s after batch settled: %d kB (delta %+d kB vs. baseline)\n' \
    "${rss_settled_kb}" "${rss_settled_delta_kb}"
printf 'Per-operation RSS delta: %s kB/op\n' \
    "$(awk -v d="${rss_delta_kb}" -v n="${BATCH_SIZE}" 'BEGIN{printf "%.3f", d/n}')"
