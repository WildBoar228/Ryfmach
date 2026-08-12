#!/usr/bin/env bash

set -Eeuo pipefail

die() {
    echo "error: $*" >&2
    exit 1
}

resolve_site() {
    local site_argument="${1:-}"
    local env_argument="${2:-}"

    [[ -n "$site_argument" ]] || die "site directory is required"
    [[ -d "$site_argument" ]] || die "site directory does not exist: $site_argument"

    SITE_DIR="$(cd -- "$site_argument" && pwd -P)"
    SITE_NAME="$(basename -- "$SITE_DIR")"
    ENV_FILE="${env_argument:-$HOME/config/$SITE_NAME.env}"
    STATE_DIR="$SITE_DIR/.run"
    RELEASE_LINK="$SITE_DIR/Ryfmach"
    PYTHON_BIN="$SITE_DIR/.venv/bin/python"
    SERVER_PY="$SITE_DIR/server.py"

    export SITE_DIR SITE_NAME ENV_FILE STATE_DIR RELEASE_LINK PYTHON_BIN SERVER_PY
}

load_site_environment() {
    [[ -f "$ENV_FILE" ]] || die "environment file does not exist: $ENV_FILE"

    set -a
    # The environment file is private, account-owned, and shell-compatible.
    # shellcheck disable=SC1090
    source "$ENV_FILE"
    set +a
}

require_variable() {
    local variable_name="$1"
    [[ -n "${!variable_name:-}" ]] || die "$variable_name is required in $ENV_FILE"
}

runner_pid() {
    local pid_file="$STATE_DIR/runner.pid"
    [[ -f "$pid_file" ]] || return 1

    local pid
    read -r pid < "$pid_file"
    [[ "$pid" =~ ^[1-9][0-9]*$ ]] || return 1
    printf '%s\n' "$pid"
}

runner_is_alive() {
    local pid="${1:-}"
    [[ "$pid" =~ ^[1-9][0-9]*$ ]] || return 1
    kill -0 "$pid" 2>/dev/null
}

runner_matches_site() {
    local pid="$1"
    local command_line

    command_line="$(ps -p "$pid" -o args= 2>/dev/null || true)"
    [[ "$command_line" == *"site-runner.sh"* &&
       "$command_line" == *"$SITE_DIR"* ]]
}
