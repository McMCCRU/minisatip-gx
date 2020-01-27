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
#include <stdio.h>

#include <dvbcsa/dvbcsa.h>
#include "dvbcsa_pv.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#define TS_SIZE		184	/* stream size generation */

static void
hexdump (const char *str, const void *data, uint32_t len)
{
  const uint8_t *p = data;
  uint32_t i;

  printf ("- %s - %u bytes\n", str, len);
  for (i = 0; i < len; i++)
    printf ("0x%02x,%c", p[i], (i + 1) % 16 ? ' ' : '\n');
  puts ("");
}

static int
memtest (uint8_t *data, unsigned int val, unsigned int len)
{
  unsigned int	i;

  for (i = 0; i < len; i++)
    if (data[i] != val)
      return -1;

  return 0;
}

int
main (void)
{
  struct dvbcsa_bs_key_s *ffkey = dvbcsa_bs_key_alloc();
  struct dvbcsa_key_s *key = dvbcsa_key_alloc();
  unsigned int i;
  uint8_t cw[8] = {0x12, 0x34, 0x56, 0x78, 0x1e, 0xd4, 0xf6, 0xab};
  unsigned int			gs = dvbcsa_bs_batch_size();
  struct dvbcsa_bs_batch_s	pcks[gs + 1];
  uint8_t	data[gs][184];
  unsigned int	rnd[gs];

#ifdef HAVE_ASSERT_H
  assert(ffkey != NULL);
  assert(key != NULL);
#endif

  //  srand(time(0));

  printf("* DVBCSA test *\n");

  dvbcsa_key_set (cw, key);
  dvbcsa_bs_key_set (cw, ffkey);

  printf(" - Generating batch with %i randomly sized packets\n", gs);

  for (i = 0; i < gs; i++)
    {
#if 0
      rnd[i] = 0xa5;

      pcks[i].data = data[i];
      pcks[i].len = 184;
#else
      rnd[i] = rand() & 0xff;

      pcks[i].data = data[i] + rand() % 10;
      pcks[i].len = 100 + rand() % 75;

#endif
      memset(pcks[i].data, rnd[i], pcks[i].len);
    }

  pcks[i].data = NULL;

  /* Bitslice decrypt test */

  printf(" - Encrypting each packet using dvbcsa_encrypt()\n");

  for (i = 0; pcks[i].data; i++)
    dvbcsa_encrypt (key, pcks[i].data, pcks[i].len);

  printf(" - Decrypting batch using _bitslice_ dvbcsa_bs_decrypt()\n");

  dvbcsa_bs_decrypt(ffkey, pcks, 184);

  printf(" - Checking results...\n");

  for (i = 0; pcks[i].data; i++)
    {
      if (memtest(pcks[i].data, rnd[i], pcks[i].len))
	{
	  printf (" - #%u Failed !\n", i);
	  hexdump("failed", pcks[i].data, pcks[i].len);
	  return -1;
	}
    }

  /* Bitslice encrypt test */

  printf(" - Decrypting each packet using dvbcsa_decrypt()\n");

  for (i = 0; pcks[i].data; i++)
    dvbcsa_decrypt (key, pcks[i].data, pcks[i].len);

  printf(" - Encrypting batch using _bitslice_ dvbcsa_bs_encrypt()\n");

  dvbcsa_bs_encrypt(ffkey, pcks, 184);

  printf(" - Checking results...\n");

  for (i = 0; pcks[i].data; i++)
    {
      if (memtest(pcks[i].data, rnd[i], pcks[i].len))
	{
	  puts (" - Failed !");
	  hexdump("failed", pcks[i].data, pcks[i].len);
	  return -1;
	}
    }

  dvbcsa_key_free(key);
  dvbcsa_bs_key_free(ffkey);

  puts (" - Ok !");

  return (0);
}

