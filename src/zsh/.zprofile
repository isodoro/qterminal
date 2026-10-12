# QTerminal Zsh startup overlay.
# Forward the user's own Zsh login-profile configuration.

ZDOTDIR="${QTERMINAL_VSM_USER_ZDOTDIR:-$HOME}"

if [[ -r "$ZDOTDIR/.zprofile" ]]; then
    source "$ZDOTDIR/.zprofile"
fi

QTERMINAL_VSM_USER_ZDOTDIR="${ZDOTDIR:-$HOME}"
ZDOTDIR="$QTERMINAL_VSM_OVERLAY_DIR"
