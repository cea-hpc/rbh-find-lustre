/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include <error.h>
#include <stdlib.h>

#include <lustre/lustreapi.h>

#include <robinhood/backend.h>
#include <rbh-find/filters.h>

#include "filters.h"

static const struct rbh_filter_field predicate2filter_field[] = {
    [PRED_HSM_STATE] = {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "hsm_state"},
};

static enum hsm_states
str2hsm_states(const char *hsm_state)
{
    switch (hsm_state[0]) {
    case 'a':
        if (strcmp(&hsm_state[1], "rchived"))
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_ARCHIVED;
    case 'd':
        if (strcmp(&hsm_state[1], "irty"))
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_DIRTY;
    case 'e':
        if (strcmp(&hsm_state[1], "xists"))
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_EXISTS;
    case 'l':
        if (strcmp(&hsm_state[1], "ost"))
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_LOST;
    case 'n':
        if (hsm_state[1] != 'o')
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        switch (hsm_state[2]) {
        case 'a':
            if (strcmp(&hsm_state[3], "rchive"))
                ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NOARCHIVE;
        case 'n':
            if (hsm_state[3] != 'e')
                ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NONE;
        case 'r':
            if (strcmp(&hsm_state[3], "elease"))
                ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
            return HS_NORELEASE;
        default:
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
            __builtin_unreachable();
        }
    case 'r':
        if (strcmp(&hsm_state[1], "elease"))
            ERROR(EX_USAGE, 0, "invalid argument to -hsm-state");
        return HS_RELEASE;
    }

    ERROR(EX_USAGE, 0, "unknown hsm-state: `%s'", hsm_state);
    __builtin_unreachable();
}

struct rbh_filter *
hsm_state2filter(const char *hsm_state)
{
    struct rbh_filter *filter;

    filter = rbh_filter_compare_uint32_new(
            RBH_FOP_BITS_ANY_SET, &predicate2filter_field[PRED_HSM_STATE],
            str2hsm_states(hsm_state)
            );
    if (filter == NULL)
        ERROR_AT_LINE(EXIT_FAILURE, errno, "hsm_state2filter");

    return filter;
}
