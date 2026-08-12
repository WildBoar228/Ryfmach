#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=site-common.sh
source "$SCRIPT_DIR/site-common.sh"

resolve_site "${1:-}" "${2:-}"
load_site_environment

require_variable RYFMACH_API_HOST
require_variable RYFMACH_API_PORT
require_variable RYFMACH_API_LOG_PATH
require_variable RYFMACH_WEB_LOG_PATH

[[ -L "$RELEASE_LINK" ]] || die "release link does not exist: $RELEASE_LINK"
[[ -x "$PYTHON_BIN" ]] || die "site Python does not exist: $PYTHON_BIN"
[[ -f "$SERVER_PY" ]] || die "Gunicorn launcher does not exist: $SERVER_PY"

RELEASE_DIR="$(readlink -f -- "$RELEASE_LINK")"
[[ -d "$RELEASE_DIR" ]] || die "release target does not exist: $RELEASE_DIR"
[[ -f "$RELEASE_DIR/python/main.py" ]] ||
    die "release has no python/main.py: $RELEASE_DIR"

mkdir -p \
    "$STATE_DIR" \
    "$(dirname -- "$RYFMACH_API_LOG_PATH")" \
    "$(dirname -- "$RYFMACH_WEB_LOG_PATH")"

exec 9>"$STATE_DIR/runner.lock"
flock -n 9 || die "site runner is already active for $SITE_DIR"

printf '%s\n' "$$" > "$STATE_DIR/runner.pid.tmp"
mv -f -- "$STATE_DIR/runner.pid.tmp" "$STATE_DIR/runner.pid"
rm -f -- "$STATE_DIR/ready"

api_pid=""
web_pid=""
shutdown_requested=0

wait_for_http() {
    local host="$1"
    local port="$2"
    local path="$3"
    local timeout="$4"
    local required_status="${5:-}"

    "$PYTHON_BIN" - "$host" "$port" "$path" "$timeout" "$required_status" <<'PY'
import http.client
import sys
import time

host, port_text, path, timeout_text, required_status_text = sys.argv[1:]
port = int(port_text)
deadline = time.monotonic() + float(timeout_text)
required_status = int(required_status_text) if required_status_text else None

while time.monotonic() < deadline:
    connection = http.client.HTTPConnection(host, port, timeout=1)
    try:
        connection.request("GET", path)
        response = connection.getresponse()
        response.read()
        if required_status is None or response.status == required_status:
            raise SystemExit(0)
    except OSError:
        pass
    finally:
        connection.close()
    time.sleep(0.25)

raise SystemExit(1)
PY
}

stop_child() {
    local pid="${1:-}"
    [[ -n "$pid" ]] || return 0
    kill -TERM "$pid" 2>/dev/null || true
}

cleanup() {
    local pid
    local deadline

    trap - EXIT INT TERM HUP
    rm -f -- "$STATE_DIR/ready"

    stop_child "$web_pid"
    stop_child "$api_pid"

    deadline=$((SECONDS + 15))
    for pid in "$web_pid" "$api_pid"; do
        [[ -n "$pid" ]] || continue
        while kill -0 "$pid" 2>/dev/null && ((SECONDS < deadline)); do
            sleep 0.2
        done
        if kill -0 "$pid" 2>/dev/null; then
            kill -KILL "$pid" 2>/dev/null || true
        fi
        wait "$pid" 2>/dev/null || true
    done

    if [[ -f "$STATE_DIR/runner.pid" ]] &&
       [[ "$(cat "$STATE_DIR/runner.pid")" == "$$" ]]; then
        rm -f -- "$STATE_DIR/runner.pid"
    fi
}

handle_signal() {
    shutdown_requested=1
    exit 0
}

trap cleanup EXIT
trap handle_signal INT TERM HUP

maintenance=0
if [[ "$(basename -- "$RELEASE_DIR")" == "maintenance" ]]; then
    maintenance=1
fi

if ((maintenance == 0)); then
    require_variable RYFMACH_HOST_NAME
    require_variable RYFMACH_SOUND_COMPATIBILITY_PATH
    require_variable SLOUNIK_DB_PATH
    require_variable RHYME_LIKES_DB_PATH

    [[ -x "$RELEASE_DIR/bin/ryfmach" ]] ||
        die "release API binary is not executable: $RELEASE_DIR/bin/ryfmach"

    "$RELEASE_DIR/bin/ryfmach" >>"$RYFMACH_API_LOG_PATH" 2>&1 &
    api_pid=$!

    api_start_timeout="${RYFMACH_API_START_TIMEOUT:-30}"
    if ! wait_for_http \
        "$RYFMACH_API_HOST" \
        "$RYFMACH_API_PORT" \
        "/health" \
        "$api_start_timeout" \
        "200"; then
        die "C++ API did not become healthy within ${api_start_timeout}s"
    fi

    runner_is_alive "$api_pid" || die "C++ API exited during startup"
fi

"$PYTHON_BIN" "$SERVER_PY" >>"$RYFMACH_WEB_LOG_PATH" 2>&1 &
web_pid=$!

web_host="${INSTANCE_HOST:-${HOST:-127.0.0.1}}"
web_port="${PORT:-20006}"
web_start_timeout="${RYFMACH_WEB_START_TIMEOUT:-30}"
if ! wait_for_http "$web_host" "$web_port" "/" "$web_start_timeout"; then
    die "Gunicorn did not become reachable within ${web_start_timeout}s"
fi

runner_is_alive "$web_pid" || die "Gunicorn exited during startup"
touch "$STATE_DIR/ready"

set +e
if ((maintenance == 1)); then
    wait "$web_pid"
    child_status=$?
else
    wait -n "$api_pid" "$web_pid"
    child_status=$?
fi
set -e

if ((shutdown_requested == 1)); then
    exit 0
fi

echo "site child exited unexpectedly with status $child_status" >&2
if ((child_status == 0)); then
    exit 1
fi
exit "$child_status"
