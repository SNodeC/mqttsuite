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

log "Scenario 2: OCC Layer 1 Conflict during draft authoring"

draft_id="$(unique_draft_id "s2occ")"
log "Creating shared draft ${draft_id}"

create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"

client_a_expected_rev="$(json_must_get '.draft_revision')"

get_draft "${draft_id}"
assert_status 200 "Client A POST /drafts/get"
client_a_expected_rev="$(json_must_get '.draft_revision')"
client_a_mapping_file="${tmp_dir}/client_a_mapping.json"
jq '.mapping' "${HTTP_BODY_FILE}" >"${client_a_mapping_file}"

get_draft "${draft_id}"
assert_status 200 "Client B POST /drafts/get"
client_b_expected_rev="$(json_must_get '.draft_revision')"
client_b_mapping_file="${tmp_dir}/client_b_mapping.json"
jq '.mapping' "${HTTP_BODY_FILE}" >"${client_b_mapping_file}"

if [[ "${client_a_expected_rev}" != "${client_b_expected_rev}" ]]; then
    fail "Expected both clients to fetch same draft_revision, got A=${client_a_expected_rev}, B=${client_b_expected_rev}"
fi

jq --arg stamp "client-a-$(date -u +%Y%m%dT%H%M%SZ)" '.meta.client_a_update = $stamp' "${client_a_mapping_file}" >"${tmp_dir}/client_a_mapping.updated.json"
jq --arg stamp "client-b-$(date -u +%Y%m%dT%H%M%SZ)" '.meta.client_b_update = $stamp' "${client_b_mapping_file}" >"${tmp_dir}/client_b_mapping.updated.json"

log "Client A replaces draft with expected_draft_revision=${client_a_expected_rev}"
replace_draft_from_file "${draft_id}" "${client_a_expected_rev}" "${tmp_dir}/client_a_mapping.updated.json"
assert_status 200 "Client A POST /drafts/replace"

client_a_new_rev="$(json_must_get '.draft_revision')"
expected_after_a=$((client_a_expected_rev + 1))
if [[ "${client_a_new_rev}" -ne "${expected_after_a}" ]]; then
    fail "Expected draft revision ${expected_after_a} after Client A update, got ${client_a_new_rev}"
fi

log "Client B attempts stale replace using expected_draft_revision=${client_b_expected_rev}"
replace_draft_from_file "${draft_id}" "${client_b_expected_rev}" "${tmp_dir}/client_b_mapping.updated.json"
assert_status 412 "Client B stale POST /drafts/replace"

error_name="$(json_get '.error // ""')"
if [[ "${error_name}" != "Draft modified concurrently" ]]; then
    fail "Expected Draft modified concurrently error, got '${error_name}'"
fi

if ! json_contains '.details // ""' 'Draft revision conflict'; then
    fail "Expected details to mention draft revision conflict"
fi

current_draft_revision="$(json_get '.current_draft_revision // -1')"
if [[ "${current_draft_revision}" -ne "${client_a_new_rev}" ]]; then
    fail "Expected current_draft_revision=${client_a_new_rev}, got ${current_draft_revision}"
fi

delete_draft "${draft_id}"
assert_status 200 "POST /drafts/delete"

record_metric "scenario2_occ_layer1" "class1_conflict_detected" "1" "bool"

printf '\nScenario 2 PASS\n'
printf 'Client A updated revision to: %s\n' "${client_a_new_rev}"
printf 'Client B stale update rejected with HTTP 412 as expected.\n'
