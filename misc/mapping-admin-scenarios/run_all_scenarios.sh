#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
export REPO_ROOT

BASE_URL="${BASE_URL:-http://127.0.0.1:8085}"
LOGS_DIR="${LOGS_DIR:-./tmp/mqttsuite-scenario-logs}"
START_INTEGRATOR="${START_INTEGRATOR:-1}"
KEEP_INTEGRATOR="${KEEP_INTEGRATOR:-0}"
INTEGRATOR_CMD_DEFAULT="./build/mqttintegrator/mqttintegrator integrator --mqtt-mapping-file mapfile.json"
INTEGRATOR_CMD="${INTEGRATOR_CMD:-${INTEGRATOR_CMD_DEFAULT}}"
export BASE_URL LOGS_DIR INTEGRATOR_CMD

INTEGRATOR_PID=""

log() {
    printf '[%s] %s\n' "$(date -u +%H:%M:%S)" "$*"
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

cleanup() {
    # scenario9 may have killed and respawned mqttintegrator to measure
    # restart latency, so the tracked PID can go stale. Re-read the pid file
    # (which scenario9 keeps up to date) before deciding what to stop.
    local pid_file="${LOGS_DIR}/mqttintegrator-wrapper.pid"
    local current_pid="${INTEGRATOR_PID}"
    if [[ -f "${pid_file}" ]]; then
        current_pid="$(cat "${pid_file}" 2>/dev/null || printf '%s' "${INTEGRATOR_PID}")"
    fi

    if [[ -n "${current_pid}" && "${KEEP_INTEGRATOR}" != "1" ]]; then
        if kill -0 "${current_pid}" >/dev/null 2>&1; then
            log "Stopping mqttintegrator (pid=${current_pid})"
            kill "${current_pid}" >/dev/null 2>&1 || true
            wait "${current_pid}" 2>/dev/null || true
        fi
    fi
}
trap cleanup EXIT

ensure_jq() {
    if command -v jq >/dev/null 2>&1; then
        return
    fi

    if [[ -x "${REPO_ROOT}/.tools/jq" ]]; then
        export PATH="${REPO_ROOT}/.tools:${PATH}"
        return
    fi

    fail "jq is required. Install jq or place an executable at ${REPO_ROOT}/.tools/jq"
}

check_api() {
    local status
    status="$(curl --max-time 2 -s -u "${ADMIN_USER:-admin}:${ADMIN_PASS:-admin}" -o /dev/null -w "%{http_code}" "${BASE_URL}/config" 2>/dev/null || true)"
    [[ "${status}" == "200" ]]
}

wait_for_api() {
    local max_attempts="${1:-30}"
    local integrator_pid="${2:-}"
    local i

    for ((i = 1; i <= max_attempts; i++)); do
        if check_api; then
            return 0
        fi

        if [[ -n "${integrator_pid}" ]] && ! kill -0 "${integrator_pid}" >/dev/null 2>&1; then
            return 2
        fi

        sleep 1
    done

    return 1
}

start_integrator_if_needed() {
    if check_api; then
        log "Admin API already reachable at ${BASE_URL}"
        return
    fi

    if [[ "${START_INTEGRATOR}" != "1" ]]; then
        fail "Admin API is down and START_INTEGRATOR=0"
    fi

    if [[ ! -x "${REPO_ROOT}/build/mqttintegrator/mqttintegrator" ]]; then
        fail "Binary not found: ${REPO_ROOT}/build/mqttintegrator/mqttintegrator"
    fi

    log "Starting mqttintegrator with command: ${INTEGRATOR_CMD}"
    nohup /usr/bin/env bash -c "cd \"${REPO_ROOT}\" && exec ${INTEGRATOR_CMD}" >"${LOGS_DIR}/mqttintegrator-wrapper.log" 2>&1 &
    INTEGRATOR_PID="$!"
    echo "${INTEGRATOR_PID}" >"${LOGS_DIR}/mqttintegrator-wrapper.pid"

    if ! kill -0 "${INTEGRATOR_PID}" >/dev/null 2>&1; then
        tail -n 80 "${LOGS_DIR}/mqttintegrator-wrapper.log" || true
        fail "mqttintegrator failed immediately after launch. See ${LOGS_DIR}/mqttintegrator-wrapper.log"
    fi

    local wait_rc=0
    if wait_for_api 45 "${INTEGRATOR_PID}"; then
        log "mqttintegrator is ready (pid=${INTEGRATOR_PID})"
        return
    else
        wait_rc=$?
    fi

    tail -n 80 "${LOGS_DIR}/mqttintegrator-wrapper.log" || true
    if [[ "${wait_rc}" -eq 2 ]] || ! kill -0 "${INTEGRATOR_PID}" >/dev/null 2>&1; then
        fail "mqttintegrator exited before API became ready. See ${LOGS_DIR}/mqttintegrator-wrapper.log"
    fi

    fail "mqttintegrator did not become ready at ${BASE_URL}. See ${LOGS_DIR}/mqttintegrator-wrapper.log"
}

run_script() {
    local script_path="${1:?script required}"
    local name
    local log_file

    name="$(basename "${script_path}" .sh)"
    log_file="${LOGS_DIR}/${name}.log"

    printf '===== RUN %s =====\n' "${script_path}"

    if "${script_path}" >"${log_file}" 2>&1; then
        printf 'result=PASS log=%s\n' "${log_file}"
        printf '%s\n' "${name}:PASS" >>"${LOGS_DIR}/summary.txt"
    else
        printf 'result=FAIL log=%s\n' "${log_file}"
        printf '%s\n' "${name}:FAIL" >>"${LOGS_DIR}/summary.txt"
        printf -- '--- log tail (%s) ---\n' "${name}"
        tail -n 60 "${log_file}" || true
        return 1
    fi
}

print_metrics_table() {
    local metrics_file="${LOGS_DIR}/metrics.jsonl"
    local summary_file="${LOGS_DIR}/metrics_summary.txt"

    if [[ ! -s "${metrics_file}" ]]; then
        log "No metrics recorded at ${metrics_file}"
        return
    fi

    {
        printf 'Metrics summary (raw records in %s)\n' "${metrics_file}"
        printf '%-28s %-32s %14s %10s\n' "scenario" "metric" "value" "unit"
        printf -- '-%.0s' {1..90}
        printf '\n'
        jq -r '[.scenario, .metric, (.value|tostring), .unit] | @tsv' "${metrics_file}" \
            | awk -F'\t' '{printf "%-28s %-32s %14s %10s\n", $1, $2, $3, $4}'
    } | tee "${summary_file}"
}

print_summary() {
    local pass_count
    local fail_count

    pass_count="$(grep -c ':PASS$' "${LOGS_DIR}/summary.txt" || true)"
    fail_count="$(grep -c ':FAIL$' "${LOGS_DIR}/summary.txt" || true)"

    printf '\nSummary\n'
    cat "${LOGS_DIR}/summary.txt"
    printf '\nTotal: %s pass, %s fail\n' "${pass_count}" "${fail_count}"

    if [[ "${fail_count}" -ne 0 ]]; then
        return 1
    fi

    return 0
}

main() {
    local scripts=()

    mkdir -p "${LOGS_DIR}"
    : >"${LOGS_DIR}/summary.txt"
    : >"${LOGS_DIR}/metrics.jsonl"

    ensure_jq

    scripts=(
        "${SCRIPT_DIR}/scenario1_happy_path.sh"
        "${SCRIPT_DIR}/scenario2_occ_layer1_draft_conflict.sh"
        "${SCRIPT_DIR}/scenario3_occ_layer2_stale_deploy.sh"
        "${SCRIPT_DIR}/scenario4_invalid_configuration_rejection.sh"
        "${SCRIPT_DIR}/scenario5_atomicity_and_persistence.sh"
        "${SCRIPT_DIR}/scenario6_fr02_json_patch.sh"
        "${SCRIPT_DIR}/scenario7_fr05_history_observability.sh"
        "${SCRIPT_DIR}/scenario8_memory_usage_profile.sh"
        "${SCRIPT_DIR}/scenario9_hot_reload_vs_restart_latency.sh"
        "${SCRIPT_DIR}/scenario10_active_config_crash_recovery.sh"
    )

    start_integrator_if_needed

    local failed=0
    local s=""
    for s in "${scripts[@]}"; do
        if ! run_script "${s}"; then
            failed=1
        fi
    done

    if ! print_summary; then
        failed=1
    fi

    print_metrics_table

    if [[ "${failed}" -ne 0 ]]; then
        exit 1
    fi

    log "All scenarios passed. Logs in ${LOGS_DIR}"
}

main "$@"
