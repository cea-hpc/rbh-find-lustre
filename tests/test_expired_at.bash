#!/usr/bin/env bash

# This file is part of rbh-find-lustre
# Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
#                    alternatives
#
# SPDX-License-Identifer: LGPL-3.0-or-later

set -e

if ! command -v rbh-sync &> /dev/null; then
    echo "This test requires rbh-sync to be installed" >&2
    exit 1
fi

test_dir=$(dirname $(readlink -e $0))
. $test_dir/test_utils.bash

LUSTRE_DIR=/mnt/lustre/
cd "$LUSTRE_DIR"

################################################################################
#                                    TESTS                                     #
################################################################################

test_invalid()
{
    rbh_lfind "rbh:mongo:$testdb" -expired-at $(echo 2^64 | bc) &&
        error "find with an expired-at epoch too big should have failed"

    rbh_lfind "rbh:mongo:$testdb" -expired-at 42blob &&
        error "find with an invalid expired-at epoch should have failed"

    rbh_lfind "rbh:mongo:$testdb" -expired-at invalid &&
        error "find with an invalid expired-at epoch should have failed"

    return 0
}

test_expired_files()
{
    local fileA="expired_file_A"
    local timeA="$(($(date +%s) - 111))"
    local fileB="expired_file_B"
    local timeB="$(($(date +%s) - 222))"
    local fileC="expired_file_C"
    local timeC="$(($(date +%s) - 333))"

    touch "$fileA"
    setfattr -n user.ccc_expires_at -v "$timeA" "$fileA"

    touch "$fileB"
    setfattr -n user.ccc_expires_at -v "$timeB" "$fileB"

    touch "$fileC"
    setfattr -n user.ccc_expires_at -v "$timeC" "$fileC"

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    rbh_lfind "rbh:mongo:$testdb" -expired-at 001 | sort |
        difflines
    rbh_lfind "rbh:mongo:$testdb" -expired-at 111 | sort |
        difflines "/$fileA"
    rbh_lfind "rbh:mongo:$testdb" -expired-at -222 | sort |
        difflines "/$fileA"
    rbh_lfind "rbh:mongo:$testdb" -expired-at -223 | sort |
        difflines "/$fileA" "/$fileB"
    rbh_lfind "rbh:mongo:$testdb" -expired-at +1 | sort |
        difflines "/$fileA" "/$fileB" "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-at +333 | sort |
        difflines
}

################################################################################
#                                     MAIN                                     #
################################################################################

declare -a tests=(test_invalid test_expired_files)

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
trap "rm -r $tmpdir" EXIT

cd "$tmpdir"

run_tests "${tests[@]}"
