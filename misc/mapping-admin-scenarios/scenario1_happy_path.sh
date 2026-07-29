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

log "Scenario 1: Initial State -> Draft Created -> Schema Validated -> Deployed"

initial_revision="$(get_active_revision)"
draft_id="$(unique_draft_id "s1happy")"

log "Initial active revision: ${initial_revision}"
log "Creating draft ${draft_id}"

t_initial="$(now_ms)"
create_draft "${draft_id}"
t_created="$(now_ms)"
assert_status 201 "POST /drafts/create"

draft_revision="$(json_must_get '.draft_revision')"
base_revision="$(json_must_get '.base_revision')"

if [[ "${base_revision}" != "${initial_revision}" ]]; then
    fail "Draft base revision ${base_revision} does not match active revision ${initial_revision}"
fi

log "Draft created with draft_revision=${draft_revision}, base_revision=${base_revision}"

get_draft "${draft_id}"
assert_status 200 "POST /drafts/get"

mapping_file="${tmp_dir}/mapping.json"
updated_mapping_file="${tmp_dir}/mapping.updated.json"

jq '.mapping' "${HTTP_BODY_FILE}" >"${mapping_file}"
marker="scenario1-${draft_id}-$(date -u +%Y%m%dT%H%M%SZ)"
jq --arg marker "${marker}" '.meta.comment = $marker' "${mapping_file}" >"${updated_mapping_file}"

replace_draft_from_file "${draft_id}" "${draft_revision}" "${updated_mapping_file}"
assert_status 200 "POST /drafts/replace"

updated_draft_revision="$(json_must_get '.draft_revision')"
if [[ "${updated_draft_revision}" -le "${draft_revision}" ]]; then
    fail "Expected draft revision to increase after replace"
fi

log "Draft updated to draft_revision=${updated_draft_revision}"

validate_draft "${draft_id}"
t_validated="$(now_ms)"
assert_status 200 "POST /drafts/validate"

is_valid="$(json_must_get '.valid')"
if [[ "${is_valid}" != "true" ]]; then
    fail "Draft validation did not return valid=true"
fi

log "Draft validated successfully"

deploy_draft "${draft_id}" "${initial_revision}"
t_deployed="$(now_ms)"
assert_status 200 "POST /drafts/deploy"

deployed_revision="$(json_must_get '.revision')"
expected_revision=$((initial_revision + 1))
if [[ "${deployed_revision}" -ne "${expected_revision}" ]]; then
    fail "Expected deployed revision ${expected_revision}, got ${deployed_revision}"
fi

reload_mode="$(json_get '.reload_mode // ""')"
instances="$(json_get '.instances // 0')"
subscribed="$(json_get '.subscribed // 0')"
unsubscribed="$(json_get '.unsubscribed // 0')"

log "Deploy acknowledged with reload_mode=${reload_mode}, instances=${instances}, subscribed=${subscribed}, unsubscribed=${unsubscribed}"

request_json GET "/config"
assert_status 200 "GET /config after deploy"

active_revision_after="$(json_must_get '.meta.revision')"
source_draft_id="$(json_get '.meta.source_draft_id // ""')"
comment_after="$(json_get '.meta.comment // ""')"

if [[ "${active_revision_after}" -ne "${deployed_revision}" ]]; then
    fail "Active revision after deploy ${active_revision_after} does not match deploy response ${deployed_revision}"
fi

if [[ "${source_draft_id}" != "${draft_id}" ]]; then
    fail "Expected deployed config meta.source_draft_id=${draft_id}, got ${source_draft_id}"
fi

if [[ "${comment_after}" != "${marker}" ]]; then
    fail "Expected deployed config marker '${marker}', got '${comment_after}'"
fi

latency_create_ms=$((t_created - t_initial))
latency_validate_ms=$((t_validated - t_created))
latency_deploy_ms=$((t_deployed - t_validated))
latency_total_ms=$((t_deployed - t_initial))

record_metric "scenario1_happy_path" "latency_create_ms" "${latency_create_ms}" "ms"
record_metric "scenario1_happy_path" "latency_validate_ms" "${latency_validate_ms}" "ms"
record_metric "scenario1_happy_path" "latency_deploy_ms" "${latency_deploy_ms}" "ms"
record_metric "scenario1_happy_path" "latency_total_ms" "${latency_total_ms}" "ms"

printf '\nScenario 1 PASS\n'
printf 'Draft ID: %s\n' "${draft_id}"
printf 'Latency Initial->Draft Created: %d ms\n' "${latency_create_ms}"
printf 'Latency Draft Created->Schema Validated: %d ms\n' "${latency_validate_ms}"
printf 'Latency Schema Validated->Deployed: %d ms\n' "${latency_deploy_ms}"
printf 'Total lifecycle latency: %d ms\n' "${latency_total_ms}"
printf 'Immediate GET /config confirmed active mapping update without restart.\n'
