# QTerminal Zsh startup overlay.
# Preserve and forward the user's own Zsh configuration.

typeset -g QTERMINAL_VSM_OVERLAY_DIR="${ZDOTDIR}"
typeset -g QTERMINAL_VSM_USER_ZDOTDIR="${QTERMINAL_VSM_ORIGINAL_ZDOTDIR:-$HOME}"

ZDOTDIR="$QTERMINAL_VSM_USER_ZDOTDIR"

if [[ -r "$ZDOTDIR/.zshenv" ]]; then
    source "$ZDOTDIR/.zshenv"
fi

QTERMINAL_VSM_USER_ZDOTDIR="${ZDOTDIR:-$HOME}"
ZDOTDIR="$QTERMINAL_VSM_OVERLAY_DIR"
