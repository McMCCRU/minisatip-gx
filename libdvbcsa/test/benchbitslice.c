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

int
main (void)
{
  struct timeval		t0, t1;
  struct dvbcsa_bs_key_s	*ffkey = dvbcsa_bs_key_alloc();
  unsigned int			n, i, c = 0;
  unsigned int			gs = dvbcsa_bs_batch_size();
  uint8_t			data[gs + 1][184];
  struct dvbcsa_bs_batch_s	pcks[gs + 1];
  uint8_t			cw[8] = { 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, };

#ifdef HAVE_ASSERT_H
  assert(ffkey != NULL);
#endif

#ifdef __linux__
  struct sched_param sp = { .sched_priority = 1 };

  sched_setscheduler (0, SCHED_FIFO, &sp);
#endif

  puts("* DVBCSA bench *");

  srand(time(0));

  dvbcsa_bs_key_set (cw, ffkey);

  printf(" - Generating batch with %i randomly sized packets\n\n", gs);

  for (i = 0; i < gs; i++)
    {
      pcks[i].data = data[i];
      pcks[i].len = 100 + rand() % 85;

      memset(data[i], rand(), pcks[i].len);
    }

  pcks[i].data = NULL;

  gettimeofday(&t0, NULL);

  for (n = (1 << 12) / gs; n < (1 << 19) / gs; n *= 2)
    {
      printf(" - decrypting %u TS packets\n", n * gs);

#ifdef __linux__
      sched_yield();
#endif

      for (i = 0; i < n; i++)
	dvbcsa_bs_decrypt(ffkey, pcks, 184);

      c += n * gs;
    }

  gettimeofday(&t1, NULL);

  printf(" - %u packets proceded, %.1f Mbits/s\n\n", c,
	 (float)(c * 184 * 8) / (float)((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec))
	 );

  gettimeofday(&t0, NULL);

  c = 0;

  for (n = (1 << 12) / gs; n < (1 << 19) / gs; n *= 2)
    {
      printf(" - encrypting %u TS packets\n", n * gs);

#ifdef __linux__
      sched_yield();
#endif

      for (i = 0; i < n; i++)
	dvbcsa_bs_encrypt(ffkey, pcks, 184);

      c += n * gs;
    }

  gettimeofday(&t1, NULL);

  printf(" - %u packets proceded, %.1f Mbits/s\n", c,
	 (float)(c * 184 * 8) / (float)((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec))
	 );

  dvbcsa_bs_key_free(ffkey);

  puts("* Done *");

  return (0);
}

