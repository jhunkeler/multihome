# Set location of multihome to avoid PATH lookups
MULTIHOME="%s"
if [ -x "$MULTIHOME" ]; then
    # Save HOME
    HOME_OLD="$HOME"
    # Redeclare HOME
    HOME="$($MULTIHOME)"
    # Switch to new HOME
    if [ "$HOME" != "$HOME_OLD" ]; then
        cd "$HOME"
    fi
fi
