## Zconvey's Zstyles

The values being set are the defaults.

```zsh
zstyle ":plugin:zcommodore" page-size "$(( LINES / 2 ))"        # Value of $LINES / 2 that is evaluated at runtime
zstyle ":plugin:zcommodore" highlight-color "fg=magenta,bold"   # Zsh style format: color magenta, font style: bold
zstyle ":plugin:zcommodore" editor "$EDITOR"                    # Value of $EDITOR evaluated at runtime. Supported editors:
                                                                # vim (gvim, mvim, etc.), emacs
zstyle ":plugin:zcommodore" auto_run "yes"                      # Automatically run the editor command for selected tag. If "no",
                                                                # "off", "0" or "false" - only put the command into command line
```
