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
}

test_none()
{
    local file=test_none

    lfs setstripe -i 1 -c 2 $file

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    rbh_lfind "rbh:mongo:$testdb" -ost 1 | sort |
        difflines "/$file"
}

################################################################################
#                                     MAIN                                     #
################################################################################

declare -a tests=(test_invalid test_none)

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
cd "$tmpdir"

for test in "${tests[@]}"; do
    (
    # trap -- "teardown" EXIT
    setup

    ("$test") && echo "$test: ✔" || error "$test: ✖"
    )
done
