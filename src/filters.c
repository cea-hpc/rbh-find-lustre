/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include <error.h>
#include <stdlib.h>
#include <sysexits.h>

#include <lustre/lustreapi.h>

#include <robinhood/backend.h>
#include <rbh-find/filters.h>

#include "filters.h"

static const struct rbh_filter_field predicate2filter_field[] = {
    [PRED_HSM_STATE] = {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "hsm_state"},
    [PRED_FID] =       {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "fid"},
};

static enum hsm_states
str2hsm_states(const char *hsm_state)
{
    switch (hsm_state[0]) {
    case 'a':
        if (strcmp(&hsm_state[1], "rchived"))
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_ARCHIVED;
    case 'd':
        if (strcmp(&hsm_state[1], "irty"))
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_DIRTY;
    case 'e':
        if (strcmp(&hsm_state[1], "xists"))
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_EXISTS;
    case 'l':
        if (strcmp(&hsm_state[1], "ost"))
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_LOST;
    case 'n':
        if (hsm_state[1] != 'o')
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        switch (hsm_state[2]) {
        case 'a':
            if (strcmp(&hsm_state[3], "rchive"))
                error(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NOARCHIVE;
        case 'n':
            if (hsm_state[3] != 'e' || hsm_state[4] != 0)
                error(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NONE;
        case 'r':
            if (strcmp(&hsm_state[3], "elease"))
                error(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NORELEASE;
        default:
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
            __builtin_unreachable();
        }
    case 'r':
        if (strcmp(&hsm_state[1], "elease"))
            error(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_RELEASED;
    }

    error(EX_USAGE, 0, "unknown hsm-state: `%s'", hsm_state);
    __builtin_unreachable();
}

struct rbh_filter *
hsm_state2filter(const char *hsm_state)
{
    enum rbh_filter_operator op = RBH_FOP_BITS_ANY_SET;
    enum hsm_states state = str2hsm_states(hsm_state);
    struct rbh_filter *filter;

    if (state == HS_NONE)
        op = RBH_FOP_EQUAL;

    filter = rbh_filter_compare_uint32_new(
            op, &predicate2filter_field[PRED_HSM_STATE], state
            );
    if (filter == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "hsm_state2filter");

    return filter;
}

struct rbh_filter *
fid2filter(const char *fid)
{
    struct rbh_filter *filter;
    const char *ptr = fid;
    int count = 0;

    do {
        if (ptr[0] != '0' || ptr[1] != 'x')
            error(EX_USAGE, 0, "fid sections must start with '0x'");

        if (++count > 3)
            error(EX_USAGE, 0, "fid requires 3 sections separated by ':'");

        ptr = strchr(ptr + 1, ':');
        if (ptr == NULL)
            break;

        ptr++;
    } while (true);

    if (count != 3)
        error(EX_USAGE, 0, "fid requires 3 sections separated by ':'");

    filter = rbh_filter_compare_string_new(
            RBH_FOP_EQUAL, &predicate2filter_field[PRED_FID], fid
            );
    if (filter == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "fid2filter");

    return filter;
}
