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

    (c) 2011 Erik Tews <e_tews at cdc.informatik.tu-darmstadt.de>
*/

#include <time.h>
#include <stdio.h>
#include <sys/time.h>

#include <dvbcsa/dvbcsa.h>
#include "dvbcsa_pv.h"

#ifdef __linux__
#include <sched.h>
#endif


#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#define KEYS 5000000

int
main (void)
{
  struct dvbcsa_key_s *key = dvbcsa_key_alloc();
  unsigned int i;
  uint8_t cw[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  struct timeval        t0, t1;


#ifdef HAVE_ASSERT_H
  assert(key != NULL);
#endif

#ifdef __linux__
  struct sched_param sp = { .sched_priority = 1 };

  sched_setscheduler (0, SCHED_FIFO, &sp);
#endif

  printf("* DVBCSA keysetup benchmark *\n");
  gettimeofday(&t0, NULL); 
  for (i = 0; i < KEYS; i++) {
    cw[0] = i;
    cw[1] = i>>8;
    cw[2] = i>>16;
    cw[3] = i>>24;
    dvbcsa_key_set (cw, key);
  }
  gettimeofday(&t1, NULL);

  printf(" - %u keys proceded, %.1f Mkeys/s\n\n", KEYS,
         (float)(KEYS) / (float)((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec))
         );

  dvbcsa_key_free(key);

  return (0);
}

