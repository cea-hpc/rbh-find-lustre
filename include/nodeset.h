/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#ifndef NODESET_H
#define NODESET_H

#include <stdio.h>

enum nodeset_type {
    NODESET_EMPTY,
    NODESET_VALUE,
    NODESET_RANGE,
};

struct nodeset {
    enum nodeset_type type;
    union {
        unsigned int value;
        struct {
            unsigned int begin;
            unsigned int end;
        } range;
    } data;
    struct nodeset *next;
};

struct nodeset *
nodeset_from_str(const char *input);

struct nodeset *
nodeset_new();

void
nodeset_destroy(struct nodeset *nodeset);

#endif
