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

################################################################################
#                                  UTILITIES                                   #
################################################################################

SUITE=${BASH_SOURCE##*/}
SUITE=${SUITE%.*}

LUSTRE_DIR=/mnt/lustre/
cd "$LUSTRE_DIR"

__rbh_lfind=$(PATH="$PWD:$PATH" which rbh-lfind)
rbh_lfind()
{
    "$__rbh_lfind" "$@"
}

__mongo=$(which mongosh || which mongo)
mongo()
{
    "$__mongo" "$@"
}

setup()
{
    # Create a test directory and `cd` into it
    testdir=$SUITE-$test
    mkdir "$testdir"
    echo "testdir = $testdir"
    cd "$testdir"

    # Create test database's name
    testdb=$SUITE-$test-$$
}

teardown()
{
     mongo --quiet "$testdb" --eval "db.dropDatabase()" >/dev/null
     rm -rf "$testdir"
}

error()
{
    echo "$*"
    exit 1
}

difflines()
{
    diff -y - <([ $# -eq 0 ] && printf '' || printf '%s\n' "$@")
}

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
    local length=${#states[@]}
    length=$((length - 1))

    for i in $(eval echo {0..$length}); do
        local state="${states[$i]}"

        touch "${state}"
        sudo lfs hsm_archive "$state"
        sleep 1

        if [ "$state" == "released" ]; then
            sudo lfs hsm_release "$state"
        else
            sudo lfs hsm_set "--$state" "$state"
        fi
    done

    ls

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in $(eval echo {0..$length}); do
        local state="${states[$i]}"

        rbh_lfind "rbh:mongo:$testdb" -hsm-state "$state" | sort |
            difflines "/$state"
    done
}

test_independant_states()
{
    local states=("archived" "exists" "norelease" "noarchive")
    local length=${#states[@]}
    length=$((length - 1))


    for i in $(eval echo {0..$length}); do
        local state="${states[$i]}"

        touch "${state}"
        sudo lfs hsm_set "--$state" "$state"
    done

    ls

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in $(eval echo {0..$length}); do
        local state="${states[$i]}"

        rbh_lfind "rbh:mongo:$testdb" -hsm-state "$state" | sort |
            difflines "/$state"
    done
}

test_multiple_states()
{
    local states=("archived" "exists" "noarchive" "norelease")
    local length=${#states[@]}
    length=$((length - 1))

    for i in $(eval echo {0..$length}); do
        local state=${states[$i]}
        touch "$state"

        for j in $(eval echo {${i}..$length}); do
            local flag=${states[$j]}

            sudo lfs hsm_set "--$flag" "$state"
        done
    done

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in $(eval echo {0..$length}); do
        local state=${states[$i]}
        local sublength=$((i + 1))
        local substates=("${states[@]:0:sublength}")
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

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
cd "$tmpdir"

for test in "${tests[@]}"; do
    (
    trap -- "teardown" EXIT
    setup

    ("$test") && echo "$test: ✔" || error "$test: ✖"
    )
done
