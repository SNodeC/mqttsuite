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

REPO_ROOT="${REPO_ROOT:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
LOGS_DIR="${LOGS_DIR:-/tmp/mqttsuite-scenario-logs}"
INTEGRATOR_CMD_DEFAULT="./build/mqttintegrator/mqttintegrator integrator --mqtt-mapping-file mapfile.json"
INTEGRATOR_CMD="${INTEGRATOR_CMD:-${INTEGRATOR_CMD_DEFAULT}}"
PID_FILE="${LOGS_DIR}/mqttintegrator-wrapper.pid"
WRAPPER_LOG="${LOGS_DIR}/mqttintegrator-wrapper.log"

HOT_RELOAD_TRIALS="${HOT_RELOAD_TRIALS:-8}"
RESTART_TRIALS="${RESTART_TRIALS:-3}"

log "Scenario 9: Hot-reload latency vs. full service restart latency (Section 6.4.2)"

# --- Part A: hot-reload latency (mapping-only deploy, no reconnect) --------
mkdir -p "${LOGS_DIR}"
hot_latencies_file="${tmp_dir}/hot_latencies.txt"
: >"${hot_latencies_file}"

for ((i = 1; i <= HOT_RELOAD_TRIALS; i++)); do
    active_revision="$(get_active_revision)"
    draft_id="$(unique_draft_id "s9hot${i}")"

    create_draft "${draft_id}"
    assert_status 201 "POST /drafts/create (hot ${i}/${HOT_RELOAD_TRIALS})"
    draft_revision="$(json_must_get '.draft_revision')"

    get_draft "${draft_id}"
    assert_status 200 "POST /drafts/get (hot ${i}/${HOT_RELOAD_TRIALS})"
    mapping_file="${tmp_dir}/hot_mapping_${i}.json"
    jq --arg marker "hot-reload-probe-${i}" '.mapping + {meta_probe: $marker}' \
        "${HTTP_BODY_FILE}" >"${mapping_file}"

    replace_draft_from_file "${draft_id}" "${draft_revision}" "${mapping_file}"
    assert_status 200 "POST /drafts/replace (hot ${i}/${HOT_RELOAD_TRIALS})"

    t0="$(now_ms)"
    deploy_draft "${draft_id}" "${active_revision}"
    t1="$(now_ms)"
    assert_status 200 "POST /drafts/deploy (hot ${i}/${HOT_RELOAD_TRIALS})"

    reload_mode="$(json_get '.reload_mode // ""')"
    if [[ "${reload_mode}" != "hot" ]]; then
        log "WARNING: expected reload_mode=hot on trial ${i}, got '${reload_mode}' (connection/plugins may have changed)"
    fi

    latency=$((t1 - t0))
    if ((latency < 0)); then
        # date +%s%3N reads the wall clock; a backward jump (NTP/VM clock
        # resync, common under WSL2) can yield a negative delta despite the
        # deploy having genuinely succeeded above. Discard as measurement
        # noise rather than polluting the reported statistics.
        log "WARNING: discarding hot reload trial ${i}/${HOT_RELOAD_TRIALS}: negative latency ${latency} ms (wall-clock jump, not a real regression)"
    else
        printf '%s\n' "${latency}" >>"${hot_latencies_file}"
    fi
    log "Hot reload trial ${i}/${HOT_RELOAD_TRIALS}: ${latency} ms (mode=${reload_mode})"
done

if [[ ! -s "${hot_latencies_file}" ]]; then
    fail "All hot reload latency samples were discarded as clock-jump noise; re-run the scenario"
fi

hot_stats="$(awk '{sum+=$1; if(min==""||$1<min)min=$1; if($1>max)max=$1; n++} END{printf "%.2f %d %d %d", sum/n, min, max, n}' "${hot_latencies_file}")"
hot_mean_ms="$(awk '{print $1}' <<<"${hot_stats}")"
hot_min_ms="$(awk '{print $2}' <<<"${hot_stats}")"
hot_max_ms="$(awk '{print $3}' <<<"${hot_stats}")"
hot_sample_count="$(awk '{print $4}' <<<"${hot_stats}")"

log "Hot reload latency: mean=${hot_mean_ms}ms min=${hot_min_ms}ms max=${hot_max_ms}ms over ${hot_sample_count}/${HOT_RELOAD_TRIALS} valid trials"

record_metric "scenario9_latency" "hot_reload_mean_ms" "${hot_mean_ms}" "ms"
record_metric "scenario9_latency" "hot_reload_min_ms" "${hot_min_ms}" "ms"
record_metric "scenario9_latency" "hot_reload_max_ms" "${hot_max_ms}" "ms"
record_metric "scenario9_latency" "hot_reload_trials" "${hot_sample_count}" "count"

# --- Part B: full service restart latency (kill + respawn + become ready) --
if [[ ! -f "${PID_FILE}" ]]; then
    log "SKIP: ${PID_FILE} not found; restart-latency comparison requires the"
    log "server to have been started by run_all_scenarios.sh (START_INTEGRATOR=1)."
    printf '\nScenario 9 PARTIAL\n'
    printf 'Hot reload latency: mean=%sms min=%sms max=%sms (n=%d)\n' \
        "${hot_mean_ms}" "${hot_min_ms}" "${hot_max_ms}" "${hot_sample_count}"
    printf 'Restart latency comparison skipped (no managed pid file found).\n'
    exit 0
fi

fine_wait_for_api() {
    local max_wait_ms="${1:-30000}"
    local waited_ms=0
    while ((waited_ms < max_wait_ms)); do
        if check_api; then
            return 0
        fi
        sleep 0.02
        waited_ms=$((waited_ms + 20))
    done
    return 1
}

restart_latencies_file="${tmp_dir}/restart_latencies.txt"
: >"${restart_latencies_file}"

for ((i = 1; i <= RESTART_TRIALS; i++)); do
    old_pid="$(cat "${PID_FILE}")"
    if ! kill -0 "${old_pid}" >/dev/null 2>&1; then
        fail "Tracked mqttintegrator pid ${old_pid} is not running; cannot measure restart latency"
    fi

    log "Restart trial ${i}/${RESTART_TRIALS}: stopping pid=${old_pid}"
    kill "${old_pid}" >/dev/null 2>&1 || true

    stop_waited=0
    while kill -0 "${old_pid}" >/dev/null 2>&1 && ((stop_waited < 10000)); do
        sleep 0.05
        stop_waited=$((stop_waited + 50))
    done
    if kill -0 "${old_pid}" >/dev/null 2>&1; then
        fail "mqttintegrator pid ${old_pid} did not exit within 10s of SIGTERM"
    fi

    t0="$(now_ms)"
    nohup /usr/bin/env bash -c "cd \"${REPO_ROOT}\" && exec ${INTEGRATOR_CMD}" >>"${WRAPPER_LOG}" 2>&1 &
    new_pid="$!"
    echo "${new_pid}" >"${PID_FILE}"

    if ! fine_wait_for_api 30000; then
        tail -n 40 "${WRAPPER_LOG}" || true
        fail "mqttintegrator did not become ready within 30s of restart (trial ${i})"
    fi
    t1="$(now_ms)"

    latency=$((t1 - t0))
    printf '%s\n' "${latency}" >>"${restart_latencies_file}"
    log "Restart trial ${i}/${RESTART_TRIALS}: ${latency} ms (new pid=${new_pid})"
done

restart_stats="$(awk '{sum+=$1; if(min==""||$1<min)min=$1; if($1>max)max=$1; n++} END{printf "%.2f %d %d", sum/n, min, max}' "${restart_latencies_file}")"
restart_mean_ms="$(awk '{print $1}' <<<"${restart_stats}")"
restart_min_ms="$(awk '{print $2}' <<<"${restart_stats}")"
restart_max_ms="$(awk '{print $3}' <<<"${restart_stats}")"

speedup="$(awk -v h="${hot_mean_ms}" -v r="${restart_mean_ms}" 'BEGIN{ if (h>0) printf "%.1f", r/h; else print "n/a" }')"

record_metric "scenario9_latency" "restart_mean_ms" "${restart_mean_ms}" "ms"
record_metric "scenario9_latency" "restart_min_ms" "${restart_min_ms}" "ms"
record_metric "scenario9_latency" "restart_max_ms" "${restart_max_ms}" "ms"
record_metric "scenario9_latency" "restart_trials" "${RESTART_TRIALS}" "count"
record_metric "scenario9_latency" "restart_vs_hot_speedup_factor" "${speedup}" "ratio"

printf '\nScenario 9 PASS\n'
printf 'Hot reload latency:   mean=%sms  min=%sms  max=%sms  (n=%d)\n' \
    "${hot_mean_ms}" "${hot_min_ms}" "${hot_max_ms}" "${hot_sample_count}"
printf 'Full restart latency: mean=%sms  min=%sms  max=%sms  (n=%d)\n' \
    "${restart_mean_ms}" "${restart_min_ms}" "${restart_max_ms}" "${RESTART_TRIALS}"
printf 'Hot reload is ~%sx faster than a full service restart.\n' "${speedup}"
