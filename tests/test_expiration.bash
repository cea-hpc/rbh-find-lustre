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

test_invalid_generic()
{
    rbh_lfind "rbh:mongo:$testdb" "$1" $(echo 2^64 | bc) &&
        error "find with an expired-at epoch too big should have failed"

    rbh_lfind "rbh:mongo:$testdb" "$1" 42blob &&
        error "find with an invalid expired-at epoch should have failed"

    rbh_lfind "rbh:mongo:$testdb" "$1" invalid &&
        error "find with an invalid expired-at epoch should have failed"

    return 0
}

test_invalid()
{
    test_invalid_generic "-expired-at"
    test_invalid_generic "-expired-in"
}

test_expired_setup()
{
    fileA="expired_file_A"
    timeA="$(($(date +%s) $1 30))"
    fileB="expired_file_B"
    timeB="$(($(date +%s) $1 60))"
    fileC="expired_file_C"
    timeC="$(($(date +%s) $1 80))"

    touch "$fileA"
    setfattr -n user.ccc_expires_at -v "$timeA" "$fileA"

    touch "$fileB"
    setfattr -n user.ccc_expires_at -v "$timeB" "$fileB"

    touch "$fileC"
    setfattr -n user.ccc_expires_at -v "$timeC" "$fileC"

    sleep 1

    rbh-sync "rbh:lustre:." "rbh:mongo:$testdb"
}

test_expired_at_files()
{
    test_expired_setup "-"

    rbh_lfind "rbh:mongo:$testdb" -expired-at 0 | sort |
        difflines "/$fileA"
    rbh_lfind "rbh:mongo:$testdb" -expired-at 1 | sort |
        difflines "/$fileB" "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-at -1 | sort |
        difflines "/$fileA"
    rbh_lfind "rbh:mongo:$testdb" -expired-at -2 | sort |
        difflines "/$fileA" "/$fileB" "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-at +1 | sort |
        difflines "/$fileB" "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-at +2 | sort |
        difflines
}

test_expired_in_files()
{
    test_expired_setup "+"

    rbh_lfind "rbh:mongo:$testdb" -expired-in 0 | sort |
        difflines "/$fileA" "/$fileB"
    rbh_lfind "rbh:mongo:$testdb" -expired-in 1 | sort |
        difflines "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-in -1 | sort |
        difflines "/$fileA" "/$fileB"
    rbh_lfind "rbh:mongo:$testdb" -expired-in -2 | sort |
        difflines "/$fileA" "/$fileB" "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-in +1 | sort |
        difflines "/$fileC"
    rbh_lfind "rbh:mongo:$testdb" -expired-in +2 | sort |
        difflines
}

################################################################################
#                                     MAIN                                     #
################################################################################

declare -a tests=(test_invalid test_expired_at_files test_expired_in_files)

tmpdir=$(mktemp --directory --tmpdir=$LUSTRE_DIR)
trap "rm -r $tmpdir" EXIT

cd "$tmpdir"

run_tests "${tests[@]}"
