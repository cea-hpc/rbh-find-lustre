/* This file is part of rbh-find-lustre
 * Copyright (C) 2022 Commissariat a l'energie atomique et aux energies
 *                    alternatives
 *
 * SPDX-License-Identifer: LGPL-3.0-or-later
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <sysexits.h>

#include <robinhood/backend.h>
#include <robinhood/sstack.h>

#include "filters.h"

struct rbh_filter *
placeholder2filter(const char *placeholder_field)
{
    (void) placeholder_field;

    error_at_line(EXIT_FAILURE, ENOTSUP, __FILE__, __LINE__,
                  "placeholder2filter");

    return NULL;
}
