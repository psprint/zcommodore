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
    print "${fg_bold[magenta]}psprint${reset_color}/${fg_bold[yellow]}Zcommodore${reset_color} is building custom exuberant ctags for you..."
    ( cd "${ZCMDR_REPO_DIR}/myctags"; [[ ! -e Makefile ]] &&  ./configure )
    make -C "${ZCMDR_REPO_DIR}/myctags"
fi

#
# Setup
#

autoload tag-search-multi-word
zle -N tag-search-multi-word
zle -N tag-search-multi-word-backwards tag-search-multi-word
zle -N tag-search-multi-word-pbackwards tag-search-multi-word
zle -N tag-search-multi-word-pforwards tag-search-multi-word
bindkey "^O^K" tag-search-multi-word

[[ -z "${fg_bold[green]}" ]] && builtin autoload -Uz colors && colors

#
# Global parameters
#

typeset -gAH ZCMDR
ZCMDR[current_project]=""
ZCMDR[current_tag_file]=""
ZCMDR[ctags_bin]="${ZCMDR_REPO_DIR}/myctags/ctags"

#
# Functions
#

function zcmdr() {
    if [[ "$1" = "-h" || "$1" = "--help" ]]; then
        print -- "${fg_bold[green]}Usage: zcmdr [-g] [-h|--help]${reset_color}"
        print -- "-h|--help    this message"
        print -- "-g           regenerate .zcmdr_tags file (it contains exuberant ctags output)"
        return 0
    fi

    if [[ "$PWD" = "$HOME" ]]; then
        print "Cannot set current project to home directory"
        return 1
    fi

    # Always set current project if not $HOME
    ZCMDR[current_project]="$PWD"

    # Next establish the tags file, this can be bumpy

    if [[ "$1" = "-g" ]]; then
        (
            command rm -f ".zcmdr_tags"
            "${ZCMDR[ctags_bin]}" -R --langmap=sh:.,sh:+.sh:+.zsh -f ".zcmdr_tags" .
        )
        ZCMDR[current_tag_file]="$PWD/.zcmdr_tags"
        print "Done generation of .zcmdr_tags file (-g request)"
    else
        if [[ -e ".zcmdr_tags" ]]; then
            ZCMDR[current_tag_file]="$PWD/.zcmdr_tags"
        elif [[ -e "TAGS" ]]; then
            ZCMDR[current_tag_file]="$PWD/TAGS"
        else
            print "There are no tags generated (no .zcmdr_tags file and no standard TAGS file)"
            print "Do you want to generate .zcmdr_tags file now? [y/n]"
            local answer
            read -qs answer
            if [[ "$answer" = "y" ]]; then
                (
                    "${ZCMDR[ctags_bin]}" -R --langmap=sh:.,sh:+.sh:+.zsh -f ".zcmdr_tags" .
                )
                ZCMDR[current_tag_file]="$PWD/.zcmdr_tags"
                print "${fg_bold[yellow]}Done.${reset_color}"
            else
                ZCMDR[current_tag_file]=""
            fi
        fi
    fi

    print "Zcommodore Current-Project is now: ${PWD:h}/${fg_bold[green]}${PWD:t}${reset_color}"

    return 0
}

# vim:ft=zsh
