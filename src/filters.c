/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include <ctype.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <lustre/lustreapi.h>

#include <robinhood/backend.h>
#include <rbh-find/filters.h>

#include "filters.h"
#include "nodeset.h"

static const struct rbh_filter_field predicate2filter_field[] = {
    [PRED_HSM_STATE] = {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "hsm_state"},
    [PRED_FID] =       {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "fid"},
    [PRED_OST_INDEX] = {.fsentry = RBH_FP_NAMESPACE_XATTRS, .xattr = "ost"},
};

static int
str2uint64_t(const char *input, uint64_t *result)
{
    uint64_t value;
    char *end;

    errno = 0;
    value = strtoull(input, &end, 10);
    if (errno || (!value && input == end))
        return -1;

    *result = value;

    return 0;
}

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
        if (strcmp(&hsm_state[1], "eleased"))
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
    struct lu_fid lu_fid;
    int rc;

    rc = llapi_fid_parse(fid, &lu_fid, NULL);
    if (rc)
        error(EX_USAGE, 0, "invalid fid parsing: %s", strerror(-rc));

    filter = rbh_filter_compare_string_new(
            RBH_FOP_EQUAL, &predicate2filter_field[PRED_FID], fid
            );
    if (filter == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "fid2filter");

    return filter;
}

static struct rbh_filter *
filter_uint64_range_new(const struct rbh_filter_field *field, uint64_t start,
                        uint64_t end)
{
    struct rbh_filter *low, *high;

    low = rbh_filter_compare_uint64_new(RBH_FOP_GREATER_OR_EQUAL, field, start);
    if (low == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "filter_uint64_range_new");

    high = rbh_filter_compare_uint64_new(RBH_FOP_LOWER_OR_EQUAL, field, end);
    if (high == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "filter_uint64_range_new");

    return filter_and(low, high);
}

static struct rbh_filter *
nodeset2filter(const char *ost_index)
{
    struct nodeset *n = nodeset_from_str(ost_index);
    struct rbh_filter *filter = NULL;
    struct nodeset *tmp = n;

    if (!n)
        error(EX_USAGE, 0, "invalid ost index: `%s'", ost_index);

    while (tmp) {
        struct rbh_filter *left_filter = filter;

        switch (tmp->type) {
        case NODESET_VALUE:
            filter = rbh_filter_compare_uint64_new(
                    RBH_FOP_EQUAL, &predicate2filter_field[PRED_OST_INDEX],
                    tmp->data.value
                    );
            if (filter == NULL)
                error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                              "ost_index2filter");
            break;
        case NODESET_RANGE:
            filter = filter_uint64_range_new(
                    &predicate2filter_field[PRED_OST_INDEX],
                    tmp->data.range.begin, tmp->data.range.end
                    );
            if (filter == NULL)
                error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                              "ost_index2filter");
            break;
        case NODESET_EMPTY:
            error(EX_USAGE, 0, "invalid ost index: `%s'", ost_index);
            break;
        }

        if (left_filter) {
            filter = filter_or(left_filter, filter);
            if (filter == NULL)
                error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                              "ost_index2filter");
        }

        tmp = tmp->next;
    }

    nodeset_destroy(n);

    return filter;
}

static struct rbh_filter *
integer2filter(const char *ost_index)
{
    struct rbh_filter *filter;
    uint64_t index;
    int rc;

    rc = str2uint64_t(ost_index, &index);
    if (rc)
        error(EX_USAGE, errno, "invalid ost index: `%s'", ost_index);

    filter = rbh_filter_compare_uint64_new(
            RBH_FOP_EQUAL, &predicate2filter_field[PRED_OST_INDEX], index
            );
    if (filter == NULL)
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__,
                      "ost_index2filter");

    return filter;
}

struct rbh_filter *
ost_index2filter(const char *ost_index)
{
    struct rbh_filter *filter;

    if (isdigit(*ost_index))
        filter = integer2filter(ost_index);
    else
        filter = nodeset2filter(ost_index);

    return filter;
}
