#!/usr/bin/env bash

# This file is part of rbh-find-lustre
# Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
#                    alternatives
#
# SPDX-License-Identifer: LGPL-3.0-or-later

set -xe

if ! command -v rbh-sync &> /dev/null; then
    echo "This test requires rbh-sync to be installed" >&2
    exit 1
fi

if ! lctl get_param mdt.*.hsm_control | grep "enabled"; then
    echo "At least 1 MDT needs to have HSM control enabled" >&2
    exit 1
fi

test_dir=$(dirname $(readlink -e $0))
. $test_dir/test_utils.bash

################################################################################
#                                    TESTS                                     #
################################################################################

test_none()
{
    touch "none"
    touch "archived"
    lfs hsm_set --archived archived
    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    rbh_lfind "rbh:mongo:$testdb" -hsm-state none | sort |
        difflines "/none"
}

test_archived_states()
{
    local states=("dirty" "lost" "released")

    for state in "${states[@]}"; do
        touch "${state}"
        sudo lfs hsm_archive "$state"

        while ! lfs hsm_state "$state" | grep "archived"; do
            sleep 0.5;
        done

        if [ "$state" == "released" ]; then
            sudo lfs hsm_release "$state"
        else
            sudo lfs hsm_set "--$state" "$state"
        fi
    done

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for state in "${states[@]}"; do
        rbh_lfind "rbh:mongo:$testdb" -hsm-state "$state" | sort |
            difflines "/$state"
    done
}

test_independant_states()
{
    local states=("archived" "exists" "norelease" "noarchive")

    for state in "${states[@]}"; do
        touch "${state}"
        sudo lfs hsm_set "--$state" "$state"
    done

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for state in "${states[@]}"; do
        rbh_lfind "rbh:mongo:$testdb" -hsm-state "$state" | sort |
            difflines "/$state"
    done
}

test_multiple_states()
{
    local states=("archived" "exists" "noarchive" "norelease")
    local length=${#states[@]}
    length=$((length - 1))

    for i in $(seq 0 $length); do
        local state=${states[$i]}
        touch "$state"

        for j in $(seq $i $length); do
            local flag=${states[$j]}

            sudo lfs hsm_set "--$flag" "$state"
        done
    done

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in $(seq 0 $length); do
        local state=${states[$i]}
        local sublength=$((i + 1))

        # This will create an array from $states, of length $sublength,
        # starting from index 0
        local substates=("${states[@]:0:sublength}")
        # This will create an array containing all the elements in $substates
        # prefixed with a "/"
        substates=("${substates[@]/#/\/}")

        rbh_lfind "rbh:mongo:$testdb" -hsm-state "$state" | sort |
            difflines "${substates[@]}"
    done
}

################################################################################
#                                     MAIN                                     #
################################################################################

declare -a tests=(test_none test_archived_states test_independant_states
                  test_multiple_states)

LUSTRE_DIR=/mnt/lustre/
cd "$LUSTRE_DIR"

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
cd "$tmpdir"

for test in "${tests[@]}"; do
    (
    trap -- "teardown" EXIT
    setup

    ("$test") && echo "$test: ✔" || error "$test: ✖"
    )
done
