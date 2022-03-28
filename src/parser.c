/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sysexits.h>

#include <rbh-find/parser.h>

#include "parser.h"

int
str2lustre_predicate(const char *string)
{
    assert(string[0] == '-');

    if (strcmp(&string[1], "placeholder") == 0)
        error(EX_USAGE, ENOTSUP, "placeholder predicate");

    return str2predicate(string);
}

static const char *__lustre_predicate2str[] = {
    [PRED_PLACEHOLDER]  = "placeholder",
};

const char *
lustre_predicate2str(int predicate)
{
    if (predicate >= PRED_PLACEHOLDER)
        return __lustre_predicate2str[predicate];

    return predicate2str(predicate);
}
