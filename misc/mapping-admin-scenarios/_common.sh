#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8085}"
ADMIN_USER="${ADMIN_USER:-admin}"
ADMIN_PASS="${ADMIN_PASS:-admin}"
ADMIN_APP_NAME="${ADMIN_APP_NAME:-integrator}"
ADMIN_STORAGE_ROOT="${ADMIN_STORAGE_ROOT:-/tmp/mqttsuite/admin/${ADMIN_APP_NAME}}"

HTTP_STATUS=""
HTTP_BODY=""
HTTP_BODY_FILE=""
HTTP_HEADERS_FILE=""

log() {
    printf '[%s] %s\n' "$(date -u +%H:%M:%S)" "$*"
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    if [[ -n "${HTTP_STATUS}" ]]; then
        printf 'HTTP status: %s\n' "${HTTP_STATUS}" >&2
    fi
    if [[ -n "${HTTP_BODY}" ]]; then
        printf 'HTTP body: %s\n' "${HTTP_BODY}" >&2
    fi
    exit 1
}

require_tools() {
    local missing=0
    local tools=(curl jq mktemp awk date cmp)
    local t=""
    for t in "${tools[@]}"; do
        if ! command -v "${t}" >/dev/null 2>&1; then
            printf 'Missing required tool: %s\n' "${t}" >&2
            missing=1
        fi
    done

    if [[ "${missing}" -ne 0 ]]; then
        exit 1
    fi
}

cleanup_http_artifacts() {
    if [[ -n "${HTTP_BODY_FILE}" && -f "${HTTP_BODY_FILE}" ]]; then
        rm -f "${HTTP_BODY_FILE}"
    fi
    if [[ -n "${HTTP_HEADERS_FILE}" && -f "${HTTP_HEADERS_FILE}" ]]; then
        rm -f "${HTTP_HEADERS_FILE}"
    fi

    HTTP_BODY_FILE=""
    HTTP_HEADERS_FILE=""
    HTTP_BODY=""
    HTTP_STATUS=""
}

request_json() {
    local method="${1:?method required}"
    local path="${2:?path required}"
    local body="${3:-}"

    cleanup_http_artifacts

    HTTP_BODY_FILE="$(mktemp)"
    HTTP_HEADERS_FILE="$(mktemp)"

    local -a curl_args
    curl_args=(
        -sS
        -u "${ADMIN_USER}:${ADMIN_PASS}"
        -D "${HTTP_HEADERS_FILE}"
        -o "${HTTP_BODY_FILE}"
        -w "%{http_code}"
        -X "${method}"
        "${BASE_URL}${path}"
    )

    if [[ -n "${body}" ]]; then
        curl_args+=(
            -H "Content-Type: application/json"
            --data "${body}"
        )
    fi

    HTTP_STATUS="$(curl "${curl_args[@]}")"
    HTTP_BODY="$(cat "${HTTP_BODY_FILE}")"
}

assert_status() {
    local expected="${1:?expected status required}"
    local context="${2:-request}"

    if [[ "${HTTP_STATUS}" != "${expected}" ]]; then
        fail "${context} expected HTTP ${expected}, got ${HTTP_STATUS}"
    fi
}

json_get() {
    local jq_expr="${1:?jq expression required}"
    jq -r "${jq_expr}" "${HTTP_BODY_FILE}"
}

json_must_get() {
    local jq_expr="${1:?jq expression required}"
    local value
    if ! value="$(jq -er "${jq_expr}" "${HTTP_BODY_FILE}" 2>/dev/null)"; then
        fail "Missing JSON field: ${jq_expr}"
    fi
    printf '%s\n' "${value}"
}

json_contains() {
    local jq_expr="${1:?jq expression required}"
    local needle="${2:?needle required}"
    local haystack
    haystack="$(jq -r "${jq_expr}" "${HTTP_BODY_FILE}")"
    [[ "${haystack}" == *"${needle}"* ]]
}

header_get() {
    local name="${1:?header name required}"

    awk -F': ' -v key="${name}" '
        tolower($1) == tolower(key) {
            gsub("\\r", "", $2)
            print $2
        }
    ' "${HTTP_HEADERS_FILE}" | tail -n 1
}

now_ms() {
    date +%s%3N
}

check_api() {
    local status
    status="$(curl --max-time 2 -s -u "${ADMIN_USER:-admin}:${ADMIN_PASS:-admin}" -o /dev/null -w "%{http_code}" "${BASE_URL}/config" 2>/dev/null || true)"
    [[ "${status}" == "200" ]]
}

METRICS_FILE="${METRICS_FILE:-${LOGS_DIR:-/tmp/mqttsuite-scenario-logs}/metrics.jsonl}"

record_metric() {
    local scenario="${1:?scenario required}"
    local metric="${2:?metric name required}"
    local value="${3:?value required}"
    local unit="${4:-}"

    mkdir -p "$(dirname "${METRICS_FILE}")"

    # Prefer numeric JSON for the value, but fall back to a JSON string if it
    # isn't valid JSON (e.g. a computed "n/a") so a single degenerate metric
    # can never abort the whole scenario run.
    if jq -n --argjson value "${value}" '$value' >/dev/null 2>&1; then
        value_arg=(--argjson value "${value}")
    else
        value_arg=(--arg value "${value}")
    fi

    jq -nc \
        --arg scenario "${scenario}" \
        --arg metric "${metric}" \
        "${value_arg[@]}" \
        --arg unit "${unit}" \
        --arg ts "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
        '{scenario: $scenario, metric: $metric, value: $value, unit: $unit, ts: $ts}' \
        >>"${METRICS_FILE}"
}

unique_draft_id() {
    local prefix="${1:-draft}"
    printf '%s_%s_%04d\n' "${prefix}" "$(date +%s)" "$((RANDOM % 10000))"
}

get_active_revision() {
    request_json GET "/config"
    assert_status 200 "GET /config"
    jq -r '.meta.revision // 0' "${HTTP_BODY_FILE}"
}

get_active_config_to_file() {
    local out_file="${1:?output file required}"
    request_json GET "/config"
    assert_status 200 "GET /config"
    cp "${HTTP_BODY_FILE}" "${out_file}"
}

create_draft() {
    local draft_id="${1:?draft id required}"
    local payload
    payload="$(jq -nc --arg draft_id "${draft_id}" '{draft_id: $draft_id}')"

    request_json POST "/drafts/create" "${payload}"
}

get_draft() {
    local draft_id="${1:?draft id required}"
    local payload
    payload="$(jq -nc --arg draft_id "${draft_id}" '{draft_id: $draft_id}')"

    request_json POST "/drafts/get" "${payload}"
}

validate_draft() {
    local draft_id="${1:?draft id required}"
    local payload
    payload="$(jq -nc --arg draft_id "${draft_id}" '{draft_id: $draft_id}')"

    request_json POST "/drafts/validate" "${payload}"
}

replace_draft_from_file() {
    local draft_id="${1:?draft id required}"
    local expected_draft_revision="${2:?expected draft revision required}"
    local mapping_file="${3:?mapping file required}"

    if [[ ! -f "${mapping_file}" ]]; then
        fail "Mapping file not found: ${mapping_file}"
    fi

    local payload
    payload="$(jq -nc \
        --arg draft_id "${draft_id}" \
        --argjson expected_draft_revision "${expected_draft_revision}" \
        --slurpfile mapping "${mapping_file}" \
        '{draft_id: $draft_id, expected_draft_revision: $expected_draft_revision, mapping: $mapping[0]}')"

    request_json POST "/drafts/replace" "${payload}"
}

deploy_draft() {
    local draft_id="${1:?draft id required}"
    local expected_revision="${2:-}"
    local payload

    if [[ -n "${expected_revision}" ]]; then
        payload="$(jq -nc \
            --arg draft_id "${draft_id}" \
            --argjson expected_revision "${expected_revision}" \
            '{draft_id: $draft_id, expected_revision: $expected_revision}')"
    else
        payload="$(jq -nc --arg draft_id "${draft_id}" '{draft_id: $draft_id}')"
    fi

    request_json POST "/drafts/deploy" "${payload}"
}

delete_draft() {
    local draft_id="${1:?draft id required}"
    local payload
    payload="$(jq -nc --arg draft_id "${draft_id}" '{draft_id: $draft_id}')"

    request_json POST "/drafts/delete" "${payload}"
}
