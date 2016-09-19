#
# No plugin manager is needed to use this file. All that is needed is adding:
#   source {where-zcommodore-is}/zcommodore.plugin.zsh
#
# to ~/.zshrc.
#

0="${(%):-%N}" # this gives immunity to functionargzero being unset
ZCMDR_REPO_DIR="${0%/*}"
ZCMDR_CONFIG_DIR="$HOME/.config/zcommodore"

#
# Update FPATH if:
# 1. Not loading with Zplugin
# 2. Not having fpath already updated (that would equal: using other plugin manager)
#

if [[ -z "$ZPLG_CUR_PLUGIN" && "${fpath[(r)$ZCMDR_REPO_DIR]}" != $ZCMDR_REPO_DIR ]]; then
    fpath+=( "$ZCMDR_REPO_DIR" )
fi

#
# Compile myctags
#

if [ ! -e "${ZCMDR_REPO_DIR}/myctags/ctags" ]; then
    echo "\033[1;35m""psprint\033[0m/\033[1;33m""Zcommodore\033[0m is building custom ctags for you..."
    ( cd "${ZCMDR_REPO_DIR}/myctags"; [ ! -e Makefile ] &&  ./configure )
    make -C "${ZCMDR_REPO_DIR}/myctags"
fi

#
# Setup
#

autoload tag-search-multi-word
zle -N tag-search-multi-word
zle -N tag-search-multi-word-backwards tag-search-multi-word
bindkey "^O^K" tag-search-multi-word

[[ -z "${fg_bold[green]}" ]] && builtin autoload -Uz colors && colors
