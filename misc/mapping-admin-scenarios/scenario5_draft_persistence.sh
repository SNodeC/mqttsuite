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

log "Scenario 5: Save and read a draft"

draft_id="$(unique_draft_id "s5persist")"
create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"

if [[ "$(json_must_get '.id')" != "${draft_id}" ]]; then
    fail "Created draft ID does not match ${draft_id}"
fi

expected="${tmp_dir}/expected.json"
jq -S . "${HTTP_BODY_FILE}" >"${expected}"

draft_file="${ADMIN_STORAGE_ROOT}/drafts/${draft_id}.json"
if [[ ! -f "${draft_file}" ]]; then
    fail "Draft file not found: ${draft_file}"
fi
jq -S . "${draft_file}" >"${tmp_dir}/saved.json"
if ! cmp -s "${expected}" "${tmp_dir}/saved.json"; then
    fail "Saved draft does not match the created draft"
fi

get_draft "${draft_id}"
assert_status 200 "POST /drafts/get"
jq -S . "${HTTP_BODY_FILE}" >"${tmp_dir}/read.json"
if ! cmp -s "${expected}" "${tmp_dir}/read.json"; then
    fail "Read draft does not match the saved draft"
fi

record_metric "scenario5_persistence" "draft_persistence_verified" "1" "bool"

printf '\nScenario 5 PASS\n'
printf 'Saved draft contents match both the creation response and the read response.\n'
