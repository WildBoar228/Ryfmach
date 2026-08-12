#!/usr/bin/env bash

set -Eeuo pipefail

die() {
    echo "error: $*" >&2
    exit 1
}

resolve_account_home() {
    local passwd_entry=""
    local passwd_home=""

    if [[ -n "${HOME:-}" && -d "$HOME" ]]; then
        ACCOUNT_HOME="$(cd -- "$HOME" && pwd -P)"
    elif command -v getent >/dev/null 2>&1; then
        passwd_entry="$(getent passwd "$EUID" || true)"
        IFS=: read -r _ _ _ _ _ passwd_home _ <<<"$passwd_entry"
        [[ -n "$passwd_home" && -d "$passwd_home" ]] ||
            die "cannot determine the account home directory"
        ACCOUNT_HOME="$(cd -- "$passwd_home" && pwd -P)"
    else
        die "HOME is unset and getent is unavailable"
    fi

    # Cron implementations may omit HOME. Export the resolved value for pip,
    # Gunicorn, and all other child processes as well as these scripts.
    HOME="$ACCOUNT_HOME"
    export ACCOUNT_HOME HOME
}

resolve_site() {
    local site_argument="${1:-}"
    local env_argument="${2:-}"

    [[ -n "$site_argument" ]] || die "site directory is required"
    [[ -d "$site_argument" ]] || die "site directory does not exist: $site_argument"

    resolve_account_home

    SITE_DIR="$(cd -- "$site_argument" && pwd -P)"
    SITE_NAME="$(basename -- "$SITE_DIR")"
    ENV_FILE="${env_argument:-$ACCOUNT_HOME/config/$SITE_NAME.env}"
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
