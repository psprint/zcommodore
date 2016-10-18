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

[[ -z "${fg_bold[green]}" ]] && builtin autoload -Uz colors && colors

#
# Compile myctags
#

if [ ! -e "${ZCMDR_REPO_DIR}/myctags/ctags" ]; then
    print "${fg_bold[magenta]}psprint${reset_color}/${fg_bold[yellow]}Zcommodore${reset_color} is building custom exuberant ctags for you..."
    ( cd "${ZCMDR_REPO_DIR}/myctags"; [[ ! -e Makefile ]] && ./configure )
    command make -C "${ZCMDR_REPO_DIR}/myctags"

    () {
        local ts="${EPOCHSECONDS}"
        [[ -z "$ts" ]] && ts="$( date +%s )"
        echo "$ts" >! "${ZCMDR_REPO_DIR}/myctags/COMPILED_AT"
    }
elif [[ ! -f "${ZCMDR_REPO_DIR}/myctags/COMPILED_AT" || ( "${ZCMDR_REPO_DIR}/myctags/COMPILED_AT" -ot "${ZCMDR_REPO_DIR}/myctags/RECOMPILE_REQUEST" ) ]]; then
    # Final verification via file read
    () {
        # Don't trust access times and verify hard stored values
        local compiled_at_ts="$(<${ZCMDR_REPO_DIR}/myctags/COMPILED_AT)"
        local recompile_request_ts="$(<${ZCMDR_REPO_DIR}/myctags/RECOMPILE_REQUEST)"
        if [[ "$recompile_request_ts" -gt "${compiled_at_ts:-0}" ]]; then
            echo "${fg_bold[red]}RECOMPILETION OF CTAGS REQUESTED BY PLUGIN'S UPDATE${reset_color}"
            ( cd "${ZCMDR_REPO_DIR}/myctags"; ./configure )
            command make -C "${ZCMDR_REPO_DIR}/myctags" clean
            command make -C "${ZCMDR_REPO_DIR}/myctags"

            local ts="${EPOCHSECONDS}"
            [[ -z "$ts" ]] && ts="$( date +%s )"
            echo "$ts" >! "${ZCMDR_REPO_DIR}/myctags/COMPILED_AT"
        fi
    }
fi

#
# Setup
#

autoload -- tag-search-multi-word zcm -zcmdr-list -zcmdr-list-draw -zcmdr-list-input -zcmdr-list-wrapper
autoload -- -zcmdr-process-buffer -zcmdr-usetty-wrapper uizcm zcm-feature
zle -N tag-search-multi-word
zle -N tag-search-multi-word-backwards tag-search-multi-word
zle -N tag-search-multi-word-pbackwards tag-search-multi-word
zle -N tag-search-multi-word-pforwards tag-search-multi-word
bindkey "^O^K" tag-search-multi-word

# uizcm autoloads
autoload -- -zcmdr-process-buffer -zcmdr-usetty-wrapper -zcmdr-list -zcmdr-list-input -zcmdr-list-draw
autoload -- -zcmdr_uizcm_last_n_git_log -zcmdr_uizcm_git_modified -zcmdr_action_git_open_commit
autoload -- -zcmdr_uizcm_last_nprojects -zcmdr_uizcm_git_header -zcmdr_uizcm_hg_header
autoload -- -zcmdr_uizcm_last_n_hg_log -zcmdr_action_hg_open_commit -zcmdr_uizcm_hg_modified
autoload -- -zcmdr_uizcm_tree -zcmdr_uizcm_load_tree -zcmdr_uizcm_features

zle -N uizcm
bindkey "^O^U" uizcm

#
# Global parameters
#

typeset -gAH ZCMDR
ZCMDR[current_project]=""
ZCMDR[current_repo]=""
ZCMDR[current_tag_file]=""
ZCMDR[ctags_bin]="${ZCMDR_REPO_DIR}/myctags/ctags"
typeset -gAH ZCMDR_ACTION_IDS_TO_HANDLERS

if [[ -f "$ZCMDR_CONFIG_DIR/current_project" ]]; then
    local -a input_data
    input_data=( "${(@f)"$(<$ZCMDR_CONFIG_DIR/current_project)"}" )

    ZCMDR[current_project]="${input_data[1]}"
    ZCMDR[current_repo]="${input_data[2]}"
    ZCMDR[current_tag_file]="${input_data[3]}"

    # That is not consistent, what is in file that is in
    # file, but I think that this kind of robustness is fine
    if [[ -z "${ZCMDR[current_tag_file]}" && -f "${ZCMDR[current_project]}/.zcmdr_tags" ]]; then
        ZCMDR[current_tag_file]="${ZCMDR[current_project]}/.zcmdr_tags"
    fi
fi

[[ "${ZCMDR[cmdline_sourced]}" != "1" ]] && source "${ZCMDR_REPO_DIR}/lib/cmdline.lcmdr"

#
# Load modules
#

zmodload -F zsh/stat b:zstat && ZCMDR[stat_available]="1" || ZCMDR[stat_available]="0"

#
# Functions
#

#
# CD to current project's directory
#
function zcm-cd() {
    if [[ -n "${ZCMDR[current_project]}" ]]; then
        if [[ -d "${ZCMDR[current_project]}" ]]; then
            builtin cd "${ZCMDR[current_project]}"
            print "Current directory is now: ${fg_bold[green]}${ZCMDR[current_project]:t}${reset_color}"
        else
            print "The project's directory doesn't exist: ${ZCMDR[current_project]}"
        fi
    else
        print "No Current-Project set (it is done with command: zcm -g, from project's directory)"
    fi
}

#
# Switch session to current project
#
function zcm-refresh() {
    local -a input_data
    input_data=( "${(@f)"$(<$ZCMDR_CONFIG_DIR/current_project)"}" )
    ZCMDR[current_project]="${input_data[1]}"
    ZCMDR[current_repo]="${input_data[2]}"
    ZCMDR[current_tag_file]="${input_data[3]}"

    print "Obtained data from $ZCMDR_CONFIG_DIR/current_project"

    local p="${ZCMDR[current_repo]:t}"
    [[ -z "$p" ]] && p="${ZCMDR[current_project]:t}"
    print "Switched to ${fg_bold[green]}$p${reset_color}"

    cd "${ZCMDR[current_project]}"
}

# vim:ft=zsh
