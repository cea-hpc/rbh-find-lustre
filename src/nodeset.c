/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#include "nodeset.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

struct nodeset *
nodeset_new()
{
    struct nodeset *nodeset;

    nodeset = malloc(sizeof(*nodeset));
    if (!nodeset) {
        errno = ENOMEM;
        return NULL;
    }

    nodeset->type = NODESET_EMPTY;
    nodeset->next = NULL;

    return nodeset;
}

void
nodeset_destroy(struct nodeset *n)
{
    while (n) {
        struct nodeset *tmp = n->next;

        free(n);
        n = tmp;
    }
}

/* Return a pointer to the last charecter of the parsed integer */
char *
nodeset_parse_value(struct nodeset *n, const char *input)
{

    unsigned long res;
    char *end;

    res = strtoul(input, &end, 10);

    if ((res == ULONG_MAX && errno == ERANGE) ||
        res > UINT_MAX)
        return NULL;

    if (n->type == NODESET_EMPTY) {
        n->type = NODESET_VALUE;
        n->data.value = (unsigned int)res;
    }

    if (n->type == NODESET_RANGE) {
        n->data.range.begin = n->data.value;
        n->data.range.end = res;
    }

    return end - 1;
}

struct nodeset *
nodeset_from_str(const char *input)
{
    const char *iter = input;
    struct nodeset *nodeset;
    struct nodeset *base;

    if (*iter != '[') {
        errno = EINVAL;
        return NULL;
    }

    iter++;
    nodeset = nodeset_new();
    if (!nodeset)
        return NULL;

    errno = 0;
    base = nodeset;

    while (*iter) {
        if (isdigit(*iter)) {
            iter = nodeset_parse_value(nodeset, iter);
            if (!iter)
                break;

        } else if (*iter == ',') {
            if (nodeset->type == NODESET_EMPTY)
                break;

            nodeset->next = nodeset_new();
            if (!nodeset->next)
                break;

            nodeset = nodeset->next;

        } else if (*iter == '-') {
            if (nodeset->type == NODESET_EMPTY)
                break;

            nodeset->type = NODESET_RANGE;

        } else if (*iter == ']') {
            return base;

        } else {
            break;
        }

        iter++;
    }

    nodeset_destroy(base);
    errno = EINVAL;
    return NULL;
}
