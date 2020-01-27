/*

    This file is part of libdvbcsa.

    libdvbcsa is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdvbcsa is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdvbcsa; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#ifdef __linux__
#include <sched.h>
#endif

#include <dvbcsa/dvbcsa.h>
#include "dvbcsa_pv.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#define TS_SIZE		184

int
main		(void)
{
  struct timeval	t0, t1;
  struct dvbcsa_key_s	*key = dvbcsa_key_alloc();
  unsigned int		n, i, c = 0;
  uint8_t		data[256];
  dvbcsa_cw_t		cw = { 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, };

#ifdef HAVE_ASSERT_H
  assert(key != NULL);
#endif

#ifdef __linux__
  struct sched_param sp = { .sched_priority = 1 };

  sched_setscheduler (0, SCHED_FIFO, &sp);
#endif

  puts("* CSA encryption bench *");

  for (i = 0; i < sizeof(dvbcsa_cw_t); i++)
    cw[i] = i * 3 ^ 0x55;

  memset(data, 0, TS_SIZE);

  dvbcsa_key_set(cw, key);

  gettimeofday(&t0, NULL);

  for (n = 256; n < 1 << 16; n *= 2)
    {
      printf(" benchmarking with %u TS packets\n", n);

#ifdef __linux__
      sched_yield();
#endif

      for (i = 0; i < n; i++)
	dvbcsa_encrypt(key, data, TS_SIZE);

	c += n;
    }

  gettimeofday(&t1, NULL);

  printf(" - %u packets proceded, %.1f Mbits/s\n\n", c,
	 (float)(c * 184 * 8) / (float)((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec))
	 );

  dvbcsa_key_free(key);

  puts("* Done *");

  return (0);
}

