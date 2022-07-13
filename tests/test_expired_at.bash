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

test_dir=$(dirname $(readlink -e $0))
. $test_dir/test_utils.bash

LUSTRE_DIR=/mnt/lustre/
cd "$LUSTRE_DIR"

################################################################################
#                                    TESTS                                     #
################################################################################

test_invalid()
{
    rbh_lfind "rbh:mongo:$testdb" -expired-at -1 &&
        error "find with a negative expired-at epoch should have failed"

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
    local fileB="expired_file_B"
    local fileC="expired_file_C"

    touch "$fileA"
    setfattr -n user.ccc_expires_at -v 111 "$fileA"

    touch "$fileB"
    setfattr -n user.ccc_expires_at -v 222 "$fileB"

    touch "$fileC"
    setfattr -n user.ccc_expires_at -v 333 "$fileC"

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"

    rbh_lfind "rbh:mongo:$testdb" -expired-at 001 | sort |
        difflines ""
    rbh_lfind "rbh:mongo:$testdb" -expired-at 112 | sort |
        difflines "/$fileA"
    rbh_lfind "rbh:mongo:$testdb" -expired-at 223 | sort |
        difflines "/$fileA" "/$fileB"
    rbh_lfind "rbh:mongo:$testdb" -expired-at 333 | sort |
        difflines "/$fileA" "/$fileB" "/$fileC"
}

################################################################################
#                                     MAIN                                     #
################################################################################

declare -a tests=(test_invalid test_expired_files)

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
trap "rm -r $tmpdir" EXIT

cd "$tmpdir"

run_tests "${tests[@]}"
