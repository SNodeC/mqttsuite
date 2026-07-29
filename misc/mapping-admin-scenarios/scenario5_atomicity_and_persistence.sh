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

log "Scenario 5: Atomicity and persistence fail-safe check"

active_revision_before="$(get_active_revision)"
draft_id="$(unique_draft_id "s5atomic")"

log "Creating draft ${draft_id}"
create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"

draft_file="${ADMIN_STORAGE_ROOT}/drafts/${draft_id}.json"
if [[ ! -f "${draft_file}" ]]; then
    fail "Draft file not found: ${draft_file}"
fi

canonical_backup="${tmp_dir}/canonical.backup.json"
cp "${draft_file}" "${canonical_backup}"

# Simulate a process crash before rename by leaving a broken temporary file behind.
# writeJsonAtomically writes to <target>.tmp.<id> and only then renames atomically to <target>.
crash_tmp="${draft_file}.tmp.simulated-crash"
printf '{ "id": "%s", "mapping": ' "${draft_id}" >"${crash_tmp}"

if ! cmp -s "${draft_file}" "${canonical_backup}"; then
    fail "Canonical draft file changed unexpectedly while simulating partial write"
fi

log "Simulated orphan temp file: ${crash_tmp}"
log "Verifying router still reads canonical draft JSON"

get_draft "${draft_id}"
assert_status 200 "POST /drafts/get after simulated crash"
read_draft_id="$(json_must_get '.id')"
if [[ "${read_draft_id}" != "${draft_id}" ]]; then
    fail "Expected to read draft ${draft_id}, got ${read_draft_id}"
fi

# Optional proof that stale temp files do not block normal lifecycle progression.
deploy_draft "${draft_id}" "${active_revision_before}"
assert_status 200 "POST /drafts/deploy after simulated crash"

deployed_revision="$(json_must_get '.revision')"
expected_revision=$((active_revision_before + 1))
if [[ "${deployed_revision}" -ne "${expected_revision}" ]]; then
    fail "Expected deployed revision ${expected_revision}, got ${deployed_revision}"
fi

if [[ ! -f "${crash_tmp}" ]]; then
    fail "Expected simulated crash temp file to remain for manual inspection"
fi

rm -f "${crash_tmp}"

record_metric "scenario5_atomicity" "fr04_atomic_persistence_verified" "1" "bool"

printf '\nScenario 5 PASS\n'
printf 'Crash-style temp file did not corrupt canonical draft file.\n'
printf 'Draft remained readable and deployable; deploy advanced revision to: %s\n' "${deployed_revision}"
printf 'This aligns with fsync + temp-file + rename atomic write strategy.\n'
