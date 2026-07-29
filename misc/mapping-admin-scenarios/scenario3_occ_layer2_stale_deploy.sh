#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./_common.sh
source "${SCRIPT_DIR}/_common.sh"

require_tools

cleanup() {
    cleanup_http_artifacts
}
trap cleanup EXIT

log "Scenario 3: OCC Layer 2 Conflict for stale deployment"

active_revision_before="$(get_active_revision)"
draft_a="$(unique_draft_id "s3a")"
draft_b="$(unique_draft_id "s3b")"

log "Initial active revision: ${active_revision_before}"
log "Creating draft A=${draft_a} and draft B=${draft_b} from same active revision"

create_draft "${draft_a}"
assert_status 201 "POST /drafts/create for draft A"
base_a="$(json_must_get '.base_revision')"

create_draft "${draft_b}"
assert_status 201 "POST /drafts/create for draft B"
base_b="$(json_must_get '.base_revision')"

if [[ "${base_a}" -ne "${active_revision_before}" || "${base_b}" -ne "${active_revision_before}" ]]; then
    fail "Both drafts must be based on active revision ${active_revision_before}"
fi

log "Deploying draft B to advance active revision"
deploy_draft "${draft_b}" "${active_revision_before}"
assert_status 200 "POST /drafts/deploy for draft B"

active_revision_after_b="$(json_must_get '.revision')"
expected_after_b=$((active_revision_before + 1))
if [[ "${active_revision_after_b}" -ne "${expected_after_b}" ]]; then
    fail "Expected revision ${expected_after_b} after draft B deploy, got ${active_revision_after_b}"
fi

log "Attempting to deploy stale draft A without expected_revision to trigger base_revision guard"
deploy_draft "${draft_a}"
assert_status 412 "POST /drafts/deploy stale draft A"

error_name="$(json_get '.error // ""')"
if [[ "${error_name}" != "Revision conflict" ]]; then
    fail "Expected Revision conflict error, got '${error_name}'"
fi

if ! json_contains '.details // ""' 'Draft base revision does not match active revision'; then
    fail "Expected details to mention draft base revision mismatch"
fi

current_revision="$(json_get '.current_revision // -1')"
if [[ "${current_revision}" -ne "${active_revision_after_b}" ]]; then
    fail "Expected current_revision=${active_revision_after_b}, got ${current_revision}"
fi

delete_draft "${draft_a}"
assert_status 200 "POST /drafts/delete draft A"

record_metric "scenario3_occ_layer2" "class2_conflict_detected" "1" "bool"

printf '\nScenario 3 PASS\n'
printf 'Draft A based on stale active revision was rejected with HTTP 412.\n'
printf 'Current active revision remained: %s\n' "${active_revision_after_b}"
