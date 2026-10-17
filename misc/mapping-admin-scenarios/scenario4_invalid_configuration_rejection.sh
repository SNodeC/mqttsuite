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

log "Scenario 4: Invalid configuration rejection (validation gatekeeper)"

active_revision_before="$(get_active_revision)"
draft_id="$(unique_draft_id "s4invalid")"

log "Active revision before test: ${active_revision_before}"
log "Creating draft ${draft_id}"

create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"

# The router validates on /drafts/replace, so to test deploy-time gatekeeping we intentionally
# tamper the persisted draft envelope directly on disk with semantically invalid mapping JSON.
draft_file="${ADMIN_STORAGE_ROOT}/drafts/${draft_id}.json"
if [[ ! -f "${draft_file}" ]]; then
    fail "Draft file not found: ${draft_file}"
fi

cp "${draft_file}" "${tmp_dir}/original-draft.json"
jq '.mapping = {"connection": {"client_id": "broken-only"}}' "${draft_file}" >"${tmp_dir}/tampered-draft.json"
mv "${tmp_dir}/tampered-draft.json" "${draft_file}"

log "Tampered draft file written: ${draft_file}"

validate_draft "${draft_id}"
assert_status 422 "POST /drafts/validate for invalid draft"

if [[ "$(json_get '.valid // false')" != "false" ]]; then
    fail "Expected valid=false for tampered draft"
fi

log "Deploying invalid draft ${draft_id}"
deploy_draft "${draft_id}"
assert_status 422 "POST /drafts/deploy invalid draft"

error_name="$(json_get '.error // ""')"
if [[ "${error_name}" != "Deploy validation failed" ]]; then
    fail "Expected Deploy validation failed error, got '${error_name}'"
fi

active_revision_after="$(get_active_revision)"
if [[ "${active_revision_after}" -ne "${active_revision_before}" ]]; then
    fail "Active revision changed unexpectedly (${active_revision_before} -> ${active_revision_after})"
fi

delete_draft "${draft_id}"
assert_status 200 "POST /drafts/delete cleanup"

record_metric "scenario4_invalid_config" "class3_conflict_detected" "1" "bool"
record_metric "scenario4_invalid_config" "validate_status_code" "422" "http_status"
record_metric "scenario4_invalid_config" "deploy_status_code" "422" "http_status"

printf '\nScenario 4 PASS\n'
printf 'Invalid draft was rejected at both validation and deploy time with HTTP 422,\n'
printf 'consistent with thesis Section 5.4.2 ("Schema validation failure results in\n'
printf 'HTTP 422 everywhere in the API"). MappingAdminRouter.cpp::handleDeployRequest\n'
printf 'now catches std::invalid_argument (thrown by the schema validator'"'"'s\n'
printf 'throwing_error_handler) explicitly and maps it to 422, ahead of the generic\n'
printf 'catch-all that previously produced a misleading HTTP 500.\n'
printf 'Active revision remained unchanged at: %s\n' "${active_revision_after}"
