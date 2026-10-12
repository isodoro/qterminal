# QTerminal managed-variable helper.
# Source this file from an interactive Zsh session.

typeset -gA QTERMINAL_MANAGED_VARIABLES
# Interactive V.S.M. state.
typeset -g QTERMINAL_VAR_UI_ACTION=''
typeset -gi QTERMINAL_VAR_UI_PENDING=0
typeset -g QTERMINAL_VAR_UI_STATE=''
typeset -g QTERMINAL_VAR_UI_NAME=''
typeset -g QTERMINAL_VAR_UI_ORIGINAL_BUFFER=''
typeset -gi QTERMINAL_VAR_UI_ORIGINAL_CURSOR=0
typeset -ga QTERMINAL_VAR_UI_NAMES
typeset -g QTERMINAL_VAR_UI_ORIGINAL_KEYMAP=''

# Finish an interactive V.S.M. input session.
qterminal_var_ui_finish() {
    zle -K "$QTERMINAL_VAR_UI_ORIGINAL_KEYMAP"

    BUFFER=$QTERMINAL_VAR_UI_ORIGINAL_BUFFER
    CURSOR=$QTERMINAL_VAR_UI_ORIGINAL_CURSOR

    QTERMINAL_VAR_UI_STATE=''
    QTERMINAL_VAR_UI_ACTION=''
    QTERMINAL_VAR_UI_NAME=''
    QTERMINAL_VAR_UI_NAMES=()
    QTERMINAL_VAR_UI_PENDING=0

    zle -R
}

# Start an interactive V.S.M. input session.
qterminal_var_ui_start() {
    QTERMINAL_VAR_UI_PENDING=0

    QTERMINAL_VAR_UI_ORIGINAL_BUFFER=$BUFFER
    QTERMINAL_VAR_UI_ORIGINAL_CURSOR=$CURSOR
    QTERMINAL_VAR_UI_ORIGINAL_KEYMAP=$KEYMAP

    BUFFER=''
    CURSOR=0

    case $QTERMINAL_VAR_UI_ACTION in
        add)
            QTERMINAL_VAR_UI_STATE='name'
            zle -M 'VSM: Variable Name to Add:'
            ;;

        edit)
            if (( ${#QTERMINAL_MANAGED_VARIABLES} == 0 )); then
                qterminal_var_ui_finish
                return
            fi

            QTERMINAL_VAR_UI_NAMES=(
                "${(@ok)QTERMINAL_MANAGED_VARIABLES}"
            )

            local message='VSM: Variable Number to Edit: '

            local i
            for (( i = 1; i <= ${#QTERMINAL_VAR_UI_NAMES}; i++ )); do
                message+="$i. ${QTERMINAL_VAR_UI_NAMES[$i]}  "
            done

            QTERMINAL_VAR_UI_STATE='edit_number'
            zle -M "$message"
            ;;
        delete)
            if (( ${#QTERMINAL_MANAGED_VARIABLES} == 0 )); then
                qterminal_var_ui_finish
                return
            fi

            QTERMINAL_VAR_UI_NAMES=(
                "${(@ok)QTERMINAL_MANAGED_VARIABLES}"
            )

            local message='VSM: Variable Number to Delete: '

            local i
            for (( i = 1; i <= ${#QTERMINAL_VAR_UI_NAMES}; i++ )); do
                message+="$i. ${QTERMINAL_VAR_UI_NAMES[$i]}  "
            done

            QTERMINAL_VAR_UI_STATE='delete_number'
            zle -M "$message"
            ;;

        *)
            qterminal_var_ui_finish
            return
            ;;
    esac

    zle -K qterminal-vsm-map
}

# Start a pending V.S.M. operation when ZLE initialises.
qterminal_var_line_init() {
    if (( QTERMINAL_VAR_UI_PENDING )) &&
       [[ $QTERMINAL_VAR_UI_ACTION == add ||
          $QTERMINAL_VAR_UI_ACTION == edit ]]; then
        qterminal_var_ui_start
    fi
}

# Process each input stage.
qterminal_var_ui_accept() {
    local entered=$BUFFER

    BUFFER=''
    CURSOR=0

    case $QTERMINAL_VAR_UI_STATE in
        name)
            if [[ ! $entered =~ '^[a-zA-Z_][a-zA-Z0-9_]*$' ]]; then
                zle -M 'V.S.M.: invalid variable name. Press Enter to retry'
                return
            fi

            QTERMINAL_VAR_UI_NAME=$entered
            QTERMINAL_VAR_UI_STATE='value'
            zle -M "VSM: Enter Value for $entered:"
            ;;

        edit_number)
            if [[ ! $entered =~ '^[0-9]+$' ]] ||
               (( entered < 1 || entered > ${#QTERMINAL_VAR_UI_NAMES} )); then
                zle -M 'V.S.M.: invalid number. Enter a listed number'
                return
            fi

            QTERMINAL_VAR_UI_NAME=${QTERMINAL_VAR_UI_NAMES[$entered]}
            QTERMINAL_VAR_UI_STATE='edit_value'
            zle -M "VSM: Enter New Value for $QTERMINAL_VAR_UI_NAME:"
            ;;

        delete_number)
            if [[ ! $entered =~ '^[0-9]+$' ]] ||
               (( entered < 1 || entered > ${#QTERMINAL_VAR_UI_NAMES} )); then
                zle -M 'V.S.M.: invalid number. Enter a listed number'
                return
            fi

            local deleted_name=${QTERMINAL_VAR_UI_NAMES[$entered]}

            qterminal_var_delete "$deleted_name"

            qterminal_var_ui_finish
            zle -M "V.S.M.: deleted $deleted_name"
            ;;

        value)
            local added_name=$QTERMINAL_VAR_UI_NAME

            if [[ ${QTERMINAL_MANAGED_VARIABLES[$added_name]+registered} ]]; then
                qterminal_var_set "$added_name" "$entered"
            else
                qterminal_var_register "$added_name" &&
                    qterminal_var_set "$added_name" "$entered"
            fi

            qterminal_var_ui_finish
            zle -M "V.S.M.: added $added_name"
            ;;

        edit_value)
            local edited_name=$QTERMINAL_VAR_UI_NAME

            qterminal_var_set "$edited_name" "$entered"

            qterminal_var_ui_finish
            zle -M "V.S.M.: updated $edited_name"
            ;;
    esac
}

# Cancel the current V.S.M. interaction.
qterminal_var_ui_cancel() {
    qterminal_var_ui_finish
    zle -M 'V.S.M.: operation cancelled'
}

# Create a separate keymap so normal Enter behaviour is preserved.
if [[ -o interactive ]]; then
    # Keep Zsh Autosuggestions from wrapping V.S.M. widgets.
    typeset -ga ZSH_AUTOSUGGEST_IGNORE_WIDGETS
    ZSH_AUTOSUGGEST_IGNORE_WIDGETS+=(
        qterminal_var_ui_start
        qterminal_var_ui_accept
    )

    bindkey -N qterminal-vsm-map main
    bindkey -M qterminal-vsm-map '^M' qterminal_var_ui_accept
    bindkey -M qterminal-vsm-map '^J' qterminal_var_ui_accept
    bindkey -M qterminal-vsm-map '^C' qterminal_var_ui_cancel

    zle -N qterminal_var_ui_start
    zle -N qterminal_var_ui_accept

    add-zle-hook-widget line-init qterminal_var_line_init

    bindkey -M main '^G' qterminal_var_ui_start
fi

qterminal_var_register() {
    local name=${1-}

    if [[ ! $name =~ '^[a-zA-Z_][a-zA-Z0-9_]*$' ]]; then
        printf 'Invalid variable name: %s\n' "$name" >&2
        return 2
    fi

    QTERMINAL_MANAGED_VARIABLES[$name]=1
}

qterminal_var_set() {
    local name=${1-}
    local value=${2-}

    if [[ ! $name =~ '^[a-zA-Z_][a-zA-Z0-9_]*$' ]]; then
        printf 'Invalid variable name: %s\n' "$name" >&2
        return 2
    fi

    if [[ ! ${QTERMINAL_MANAGED_VARIABLES[$name]+registered} ]]; then
        printf 'Variable is not registered: %s\n' "$name" >&2
        return 2
    fi

    typeset -g "$name=$value"

    qterminal_var_request changed >/dev/null
}

qterminal_var_delete() {
    local name=${1-}

    if [[ ! $name =~ '^[a-zA-Z_][a-zA-Z0-9_]*$' ]]; then
        printf 'Invalid variable name: %s\n' "$name" >&2
        return 2
    fi

    if [[ ! ${QTERMINAL_MANAGED_VARIABLES[$name]+registered} ]]; then
        printf 'Variable is not registered: %s\n' "$name" >&2
        return 2
    fi

    unset "$name"
    unset "QTERMINAL_MANAGED_VARIABLES[$name]"

    qterminal_var_request changed >/dev/null
}

qterminal_var_list() {
    local name

    for name in ${(k)QTERMINAL_MANAGED_VARIABLES}; do
        if (( ${+parameters[$name]} )); then
            printf '%s=%s\n' "$name" "${(P)name}"
        fi
    done
}

# Send one JSON request to this terminal's QTerminal socket.
qterminal_var_request() {
    if [[ -z ${QTERMINAL_VARIABLE_SOCKET:-} ]]; then
        print -u2 -- "QTerminal variable socket is not configured."
        return 1
    fi

    if (( $# < 1 || $# > 3 )); then
        print -u2 -- "Usage: qterminal_var_request operation [name] [value]"
        return 2
    fi

    python3 -c '
import json
import socket
import sys

path = sys.argv[1]
request = {"op": sys.argv[2]}

if len(sys.argv) > 3:
    request["name"] = sys.argv[3]
if len(sys.argv) > 4:
    request["value"] = sys.argv[4]

try:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(3)
        client.connect(path)
        client.sendall(
            json.dumps(request, ensure_ascii=True).encode("utf-8") + b"\n"
        )

        response = bytearray()
        while b"\n" not in response:
            chunk = client.recv(4096)
            if not chunk:
                break
            response.extend(chunk)
            if len(response) > 65536:
                raise ValueError("response too large")

        if not response:
            raise ValueError("server returned no response")

        line = bytes(response).split(b"\n", 1)[0]
        print(json.dumps(json.loads(line), ensure_ascii=True))

except (OSError, ValueError, json.JSONDecodeError) as exc:
    print(f"QTerminal variable request failed: {exc}", file=sys.stderr)
    sys.exit(1)
' "$QTERMINAL_VARIABLE_SOCKET" "$@"
}

# Poll for one queued request and execute it in this shell.

qterminal_var_poll() {
    if [[ -z ${QTERMINAL_VARIABLE_SOCKET:-} ]]; then
        return 1
    fi

    local response
    response=$(qterminal_var_request poll) || return 1

    local parsed
    parsed=$(print -r -- "$response" | python3 -c '
import json
import sys

try:
    response = json.load(sys.stdin)
    if response.get("ok") is not True:
        raise ValueError("poll failed")

    if response.get("status") == "idle":
        print("idle")
        sys.exit(0)

    request = response.get("request")
    if not isinstance(request, dict):
        raise ValueError("missing request")

    op = request.get("op")
    if op not in ("list", "register", "set", "delete", "ui_add", "ui_edit", "ui_delete", "ui_list"):
        raise ValueError("unsupported queued operation")

    name = request.get("name", "")
    value = request.get("value", "")

    if not isinstance(name, str) or not isinstance(value, str):
        raise ValueError("invalid request fields")

    print(json.dumps([op, name, value], ensure_ascii=True))
except (ValueError, TypeError, json.JSONDecodeError) as exc:
    print(f"Invalid QTerminal poll response: {exc}", file=sys.stderr)
    sys.exit(1)
') || return 1
    if [[ $parsed == idle ]]; then
        return 2
    fi

    local -a fields
    fields=("${(@f)$(print -r -- "$parsed" | python3 -c '
import json, sys
for value in json.load(sys.stdin):
    print(value)
')}") || return 1

    case ${fields[1]-} in
        ui_add)
            if (( QTERMINAL_VAR_UI_PENDING )); then
                return 0
            fi

            QTERMINAL_VAR_UI_PENDING=1
            QTERMINAL_VAR_UI_ACTION='add'
            ;;

        ui_edit)
            if (( QTERMINAL_VAR_UI_PENDING )); then
                return 0
            fi

            if (( ${#QTERMINAL_MANAGED_VARIABLES} == 0 )); then
                return 0
            fi

            QTERMINAL_VAR_UI_PENDING=1
            QTERMINAL_VAR_UI_ACTION='edit'
            ;;
                ui_delete)
            if (( QTERMINAL_VAR_UI_PENDING )); then
                return 0
            fi

            if (( ${#QTERMINAL_MANAGED_VARIABLES} == 0 )); then
                return 0
            fi

            QTERMINAL_VAR_UI_PENDING=1
            QTERMINAL_VAR_UI_ACTION='delete'
            ;;

        ui_list)
            local -a names
            names=("${(@ok)QTERMINAL_MANAGED_VARIABLES}")

            if (( ${#names} > 0 )); then
                zle -I
                print -r -- 'V.S.M. Registered variables:'

                local i name
                for (( i = 1; i <= ${#names}; i++ )); do
                    name=${names[$i]}

                    if (( ${+parameters[$name]} )); then
                        printf '%d. %s=%s\n' "$i" "$name" "${(P)name}"
                    fi
                done

                zle -R
            fi
            ;;

        list)
            local result
            result=$(qterminal_var_list)

            local result_json
            result_json=$(python3 -c '
import json, sys
print(json.dumps({
    "op": "result",
    "variables": sys.argv[1].splitlines() if sys.argv[1] else []
}, ensure_ascii=True))
' "$result") || return 1

            qterminal_var_send_result "$result_json"
            ;;
        register)
            qterminal_var_register "${fields[2]-}"
            ;;
        set)
            qterminal_var_set "${fields[2]-}" "${fields[3]-}"
            ;;
        delete)
            qterminal_var_delete "${fields[2]-}"
            ;;
    esac
}



# Send a structured result object to QTerminal.
qterminal_var_send_result() {
    if [[ -z ${QTERMINAL_VARIABLE_SOCKET:-} ]]; then
        return 1
    fi

    python3 -c '
import json
import socket
import sys

path = sys.argv[1]
request = json.loads(sys.argv[2])

try:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(3)
        client.connect(path)
        client.sendall(
            json.dumps(request, ensure_ascii=True).encode("utf-8") + b"\n"
        )
        response = bytearray()
        while b"\n" not in response:
            chunk = client.recv(4096)
            if not chunk:
                break
            response.extend(chunk)
            if len(response) > 65536:
                raise ValueError("response too large")
        if not response:
            raise ValueError("server returned no response")
        bytes(response).split(b"\n", 1)[0]
except (OSError, ValueError, json.JSONDecodeError) as exc:
    print(f"QTerminal result request failed: {exc}", file=sys.stderr)
    sys.exit(1)
' "$QTERMINAL_VARIABLE_SOCKET" "$1"
}

# Persistent notification connection for asynchronous sidebar refresh.
qterminal_var_notification_stream() {
    python3 -c '
import json
import socket
import sys

path = sys.argv[1]

try:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(path)
        client.sendall(b"{\"op\":\"subscribe\"}\n")

        stream = client.makefile("rb")
        acknowledgement = stream.readline()
        if not acknowledgement:
            raise RuntimeError("subscription acknowledgement missing")

        response = json.loads(acknowledgement)
        if response.get("status") != "subscribed":
            raise RuntimeError("subscription failed")

        print("QTERMINAL_READY", flush=True)

        for line in stream:
            print(line.decode("utf-8").rstrip("\n"), flush=True)

except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
    print(f"QTerminal notification stream failed: {exc}", file=sys.stderr)
' "$QTERMINAL_VARIABLE_SOCKET"
}

# Called by ZLE when QTerminal sends a notification.
qterminal_var_zle_handler() {
    local fd=$1
    local line
    local poll_status


    if ! read -r -u "$fd" line; then
        zle -F "$fd"
        return
    fi


    while true; do
        qterminal_var_poll
        poll_status=$?


        if (( poll_status == 2 )); then
            break
        elif (( poll_status != 0 )); then
            break
        fi

        if (( QTERMINAL_VAR_UI_PENDING )) &&
           [[ $QTERMINAL_VAR_UI_ACTION == add ||
              $QTERMINAL_VAR_UI_ACTION == edit ||
              $QTERMINAL_VAR_UI_ACTION == delete ]]; then
            qterminal_var_ui_start
            return
        fi
    done

    zle -R
}

# Start the notification stream once in an interactive Zsh session.
qterminal_var_enable_async() {
    [[ -o interactive && -n ${QTERMINAL_VARIABLE_SOCKET:-} ]] || return 0
    (( ${+commands[python3]} )) || return 0

    if [[ -n ${QTERMINAL_VAR_NOTIFY_FD:-} ]]; then
        return 0
    fi

    exec {QTERMINAL_VAR_NOTIFY_FD}< <(
        qterminal_var_notification_stream
    ) || return 1

    local ready
    if ! read -r -t 5 -u "$QTERMINAL_VAR_NOTIFY_FD" ready; then
        exec {QTERMINAL_VAR_NOTIFY_FD}<&-
        unset QTERMINAL_VAR_NOTIFY_FD
        print -u2 -- "QTerminal: notification subscription timed out."
        return 1
    fi

    if [[ "$ready" != "QTERMINAL_READY" ]]; then
        exec {QTERMINAL_VAR_NOTIFY_FD}<&-
        unset QTERMINAL_VAR_NOTIFY_FD
        print -u2 -- "QTerminal: notification subscription failed."
        return 1
    fi

    zle -N qterminal_var_zle_handler
    zle -F -w "$QTERMINAL_VAR_NOTIFY_FD" qterminal_var_zle_handler
}

if [[ -o interactive ]]; then
    qterminal_var_enable_async
fi
