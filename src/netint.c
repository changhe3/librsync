/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
 *
 * Copyright (C) 1999, 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 1999 by Andrew Tridgell <tridge@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

                            /*=
                             | Ummm, well, OK.  The network's the
                             | network, the computer's the
                             | computer.  Sorry for the confusion.
                             |        -- Sun Microsystems
                             */

/** \file netint.c
 * Network-byte-order output to the tube.
 *
 * All the `suck' routines return a result code. The most common values are
 * RS_DONE if they have enough data, or RS_BLOCKED if there is not enough input
 * to proceed.
 *
 * The `squirt` routines also return a result code which in theory could be
 * RS_BLOCKED if there is not enough output space to proceed, but in practice
 * is always RS_DONE. */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include "librsync.h"
#include "netint.h"
#include "stream.h"

#define RS_MAX_INT_BYTES 8

/** Write a single byte to a stream output. */
rs_result rs_squirt_byte(rs_job_t *job, rs_byte_t val)
{
    rs_tube_write(job, &val, 1);
    return RS_DONE;
}

/** Write a variable-length integer to a stream.
 *
 * \param job - Job of data.
 *
 * \param val - Value to write out.
 *
 * \param len - Length of integer, in bytes. */
rs_result rs_squirt_netint(rs_job_t *job, rs_long_t val, int len)
{
    rs_byte_t buf[RS_MAX_INT_BYTES];
    int i;

    assert(len <= RS_MAX_INT_BYTES);
    /* Fill the output buffer with a bigendian representation of the number. */
    for (i = len - 1; i >= 0; i--) {
        buf[i] = val;             /* truncated */
        val >>= 8;
    }
    rs_tube_write(job, buf, len);
    return RS_DONE;
}

rs_result rs_squirt_n4(rs_job_t *job, int val)
{
    return rs_squirt_netint(job, val, 4);
}

rs_result rs_suck_byte(rs_job_t *job, rs_byte_t *val)
{
    rs_result result;
    rs_byte_t *buf;

    if ((result = rs_scoop_read(job, 1, (void **)&buf)) == RS_DONE)
        *val = *buf;
    return result;
}

rs_result rs_suck_netint(rs_job_t *job, rs_long_t *val, int len)
{
    rs_result result;
    rs_byte_t *buf;
    int i;

    assert(len <= RS_MAX_INT_BYTES);
    if ((result = rs_scoop_read(job, len, (void **)&buf)) == RS_DONE) {
        *val = 0;
        for (i = 0; i < len; i++)
            *val = *val << 8 | buf[i];
    }
    return result;
}

rs_result rs_suck_n4(rs_job_t *job, int *val)
{
    rs_result result;
    rs_long_t buf;

    if ((result = rs_suck_netint(job, &buf, 4)) == RS_DONE)
        *val = buf;
    return result;
}

int rs_int_len(rs_long_t val)
{
    assert(val >= 0);
    if (!(val & ~(rs_long_t)0xff))
        return 1;
    if (!(val & ~(rs_long_t)0xffff))
        return 2;
    if (!(val & ~(rs_long_t)0xffffffff))
        return 4;
    assert(!(val & ~(rs_long_t)0xffffffffffffffff));
    return 8;
}
