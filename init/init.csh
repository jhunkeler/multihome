# Set location of multihome to avoid PATH lookups
setenv MULTIHOME "%s"
if ( -x "$MULTIHOME" ) then
    # Save HOME
    setenv HOME_OLD "$HOME"
    # Redeclare HOME
    setenv HOME "`$MULTIHOME`"
    # Switch to new HOME
    if ( "$HOME" != "$HOME_OLD" ) then
        cd "$HOME"
    endif
endif
