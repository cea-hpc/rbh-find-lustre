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

################################################################################
#                                  UTILITIES                                   #
################################################################################

SUITE=${BASH_SOURCE##*/}
SUITE=${SUITE%.*}

LUSTRE_DIR=/mnt/lustre/

__rbh_lfind=$(PATH="$PWD:$PATH" which rbh-lfind)
rbh_lfind()
{
    "$__rbh_lfind" "$@"
}
cd "$LUSTRE_DIR"

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

lfs_find_ost()
{
    lfs find . -ost $1 | sed 's/^.//' | sort | xargs
}

################################################################################
#                                    TESTS                                     #
################################################################################

test_invalid()
{
    if rbh_lfind "rbh:mongo:$testdb" -ost -1; then
        error "find with a negative ost index should have failed"
    fi

    if rbh_lfind "rbh:mongo:$testdb" -ost $(echo 2^64 | bc); then
        error "find with an ost index too big should have failed"
    fi

    if rbh_lfind "rbh:mongo:$testdb" -ost []; then
        error "find with an empty nodeset should have failed"
    fi
}

test_single_match()
{
    local file=test_single_match

    lfs setstripe -i 1 -c 1 $file

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    rbh_lfind "rbh:mongo:$testdb" -ost 1 | sort |
        difflines "/$file"
}

test_nodeset()
{
    local files=()

    for i in {0..5}; do
        lfs setstripe -i $i -c 1 test_nodeset$i
    done

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in {0..5}; do
        rbh_lfind "rbh:mongo:$testdb" -ost [$i] | sort |
            difflines "/test_nodeset$i"

        rbh_lfind "rbh:mongo:$testdb" -ost $i | sort |
            difflines "/test_nodeset$i"
    done

    rbh_lfind "rbh:mongo:$testdb" -ost [4-6,1] | sort |
        difflines $(lfs_find_ost 1,4,5,6)
}

test_pfl()
{
    lfs setstripe \
        -E 64k  -c 1 \
        -E 128k -c 4 \
        -E -1   -c 6 \
        test_pfl

    dd if=/dev/urandom of=test_pfl bs=4096 count=256

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    for i in {0..9}; do
        rbh_lfind "rbh:mongo:$testdb" -ost [$i] | sort |
            difflines $(lfs_find_ost $i)
    done
}

################################################################################
#                                     MAIN                                     #
################################################################################

# declare -a tests=(test_invalid test_single_match test_nodeset test_pfl)
declare -a tests=(test_pfl)

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
cd "$tmpdir"

for test in "${tests[@]}"; do
    (
    trap -- "teardown" EXIT
    setup

    "$test"
    )
    rc=$?

    if [[ $rc = 0 ]]; then
        echo "$test: ✔"
    else
        error "$test: ✖"
    fi
done

cd ..
rm -rf "$tmpdir"
