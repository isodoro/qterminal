# QTerminal Zsh startup overlay.
# Forward the user's own interactive Zsh configuration.

ZDOTDIR="${QTERMINAL_VSM_USER_ZDOTDIR:-$HOME}"

if [[ -r "$ZDOTDIR/.zshrc" ]]; then
    source "$ZDOTDIR/.zshrc"
fi

QTERMINAL_VSM_USER_ZDOTDIR="${ZDOTDIR:-$HOME}"
ZDOTDIR="$QTERMINAL_VSM_USER_ZDOTDIR"

# Load the QTerminal Variable Session Manager after user configuration.
if [[ -o interactive && -n ${QTERMINAL_VARIABLE_SOCKET:-} ]]; then
    source "${QTERMINAL_VSM_OVERLAY_DIR:h}/qterminal-variables.zsh"
fi

unset QTERMINAL_VSM_OVERLAY_DIR
unset QTERMINAL_VSM_USER_ZDOTDIR
unset QTERMINAL_VSM_ORIGINAL_ZDOTDIR
