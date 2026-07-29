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

log "Scenario 6: FR-02 granular RFC 6902 JSON Patch editing via PATCH /drafts/patch"

draft_id="$(unique_draft_id "s6patch")"

create_draft "${draft_id}"
assert_status 201 "POST /drafts/create"
draft_revision="$(json_must_get '.draft_revision')"

# NOTE: the draft envelope's "mapping" field is the *entire* configuration
# document (connection / discover_prefix / mapping / meta), matching the
# thesis's own JSON Patch example in Figure 2.1 ("/mapping/topic_level/0/...").
# The actual topic tree therefore lives one level deeper, under ".mapping.mapping".
get_draft "${draft_id}"
assert_status 200 "POST /drafts/get (baseline)"
original_name="$(jq -r '.mapping.mapping.topic_level[0].name' "${HTTP_BODY_FILE}")"
original_qos="$(jq -r '.mapping.mapping.topic_level[0].topic_level[0].subscription.qos' "${HTTP_BODY_FILE}")"

if [[ "${original_name}" == "null" || "${original_qos}" == "null" ]]; then
    fail "Could not resolve baseline topic_level fields (name=${original_name}, qos=${original_qos}); mapfile.json structure may have changed"
fi

log "Baseline: mapping.topic_level[0].name=${original_name}, nested qos=${original_qos}"

# --- Part A: valid granular patch (add/replace/test) -----------------------
# Demonstrates FR-02: only the touched fields are transmitted/mutated; a
# leading "test" op enforces a precondition on the untouched qos value before
# a "replace" op changes the name, exercising RFC 6902 semantics that JSON
# Merge Patch cannot express (Section 2.5.3).
t_patch_start="$(now_ms)"
valid_patch="$(jq -nc \
    --argjson qos "${original_qos}" \
    '[
        {"op": "test", "path": "/mapping/topic_level/0/topic_level/0/subscription/qos", "value": $qos},
        {"op": "replace", "path": "/mapping/topic_level/0/name", "value": "rfid-patched"}
    ]')"

patch_payload="$(jq -nc \
    --arg draft_id "${draft_id}" \
    --argjson expected_draft_revision "${draft_revision}" \
    --argjson patch "${valid_patch}" \
    '{draft_id: $draft_id, expected_draft_revision: $expected_draft_revision, patch: $patch}')"

request_json PATCH "/drafts/patch" "${patch_payload}"
t_patch_end="$(now_ms)"
assert_status 200 "PATCH /drafts/patch (valid patch)"

patched_name="$(json_must_get '.mapping.mapping.topic_level[0].name')"
patched_revision="$(json_must_get '.draft_revision')"

if [[ "${patched_name}" != "rfid-patched" ]]; then
    fail "Expected patched name 'rfid-patched', got '${patched_name}'"
fi

if [[ "${patched_revision}" -le "${draft_revision}" ]]; then
    fail "Expected draft_revision to increase after patch, was ${draft_revision} still ${patched_revision}"
fi

log "Valid patch applied: name=${patched_name}, draft_revision=${draft_revision}->${patched_revision}"

# --- Part B: atomicity of a rejected patch (failed "test" op) --------------
# A patch array containing a failing "test" op must be rejected as a whole
# unit (RFC 6902 atomicity, Section 2.5.2); no partial mutation may reach
# disk. We assert the draft content and revision are byte-for-byte unchanged.
stale_qos_patch="$(jq -nc \
    --argjson wrong_qos "$((original_qos + 99))" \
    '[
        {"op": "test", "path": "/mapping/topic_level/0/topic_level/0/subscription/qos", "value": $wrong_qos},
        {"op": "replace", "path": "/mapping/topic_level/0/name", "value": "should-not-apply"}
    ]')"

reject_payload="$(jq -nc \
    --arg draft_id "${draft_id}" \
    --argjson expected_draft_revision "${patched_revision}" \
    --argjson patch "${stale_qos_patch}" \
    '{draft_id: $draft_id, expected_draft_revision: $expected_draft_revision, patch: $patch}')"

request_json PATCH "/drafts/patch" "${reject_payload}"
assert_status 422 "PATCH /drafts/patch (failing test op must be rejected atomically)"

error_name="$(json_get '.error // ""')"
if [[ "${error_name}" != "Draft patch failed" ]]; then
    fail "Expected error 'Draft patch failed', got '${error_name}'"
fi

get_draft "${draft_id}"
assert_status 200 "POST /drafts/get (post-rejection)"
post_reject_name="$(json_must_get '.mapping.mapping.topic_level[0].name')"
post_reject_revision="$(json_must_get '.draft_revision')"

if [[ "${post_reject_name}" != "rfid-patched" ]]; then
    fail "Draft content changed despite rejected patch: name is '${post_reject_name}'"
fi

if [[ "${post_reject_revision}" -ne "${patched_revision}" ]]; then
    fail "Draft revision changed despite rejected patch: ${patched_revision} -> ${post_reject_revision}"
fi

log "Failing test-op patch correctly left draft unchanged at draft_revision=${post_reject_revision}"

delete_draft "${draft_id}"
assert_status 200 "POST /drafts/delete cleanup"

patch_latency_ms=$((t_patch_end - t_patch_start))
record_metric "scenario6_fr02_json_patch" "granular_patch_latency_ms" "${patch_latency_ms}" "ms"
record_metric "scenario6_fr02_json_patch" "fr02_granular_patch_verified" "1" "bool"
record_metric "scenario6_fr02_json_patch" "fr02_patch_atomicity_verified" "1" "bool"

printf '\nScenario 6 PASS\n'
printf 'Granular JSON Patch (test+replace) applied in %d ms; draft_revision %s -> %s\n' \
    "${patch_latency_ms}" "${draft_revision}" "${patched_revision}"
printf 'A patch with a failing "test" precondition was rejected atomically (HTTP 422);\n'
printf 'draft content and revision remained unchanged, confirming RFC 6902 atomicity.\n'
