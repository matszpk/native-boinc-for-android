/* legendre.c -- (C) Geoffrey Reynolds, September 2006.

   Generate (or load from cache file) lookup tables to replace the
   Legendre(-ckb^n,p) function.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include "sr5sieve.h"
#include "bitmap.h"

#if NO_LOOKUP_OPT
int no_lookup_opt = 0;  /* Set to disable lookup table generation. */

/*
  Return the value of the Legendre symbol (a/p), where gcd(a,p)=1, p prime.
  If gcd(a,p)!=1 then return value undefined.
*/
int legendre32_64(int32_t a, uint64_t p)
{
  uint32_t x, y, t;
  int sign;

  if (a < 0)
    a = -a, sign = (p % 4 == 1) ? 1 : -1;
  else
    sign = 1;

  for (y = a; y % 2 == 0; y /= 2)
    if (p % 8 == 3 || p % 8 == 5)
      sign = -sign;

  if (p % 4 == 3 && y % 4 == 3)
    sign = -sign;

  for (x = p % y; x > 0; x %= y)
  {
    for ( ; x % 2 == 0; x /= 2)
      if (y % 8 == 3 || y % 8 == 5)
        sign = -sign;

    t = x, x = y, y = t;

    if (x % 4 == 3 && y % 4 == 3)
      sign = -sign;
  }

  return sign;
}
#endif

#if LEGENDRE_CACHE
#include <stdio.h>
#include <stdlib.h>
#if HAVE_MMAP
#include <sys/mman.h>
static void *mmap_data;
#endif

static FILE *cache_file;
static const char *cache_file_name;
static uint32_t loaded_from_cache;

typedef struct
{
  uint32_t k;
  int32_t c;
  uint32_t m;
  uint32_t parity;
  uint32_t len;
  uint32_t off;
  uint_fast32_t checksum;
} rec_t;

static rec_t *REC;
static uint32_t rec_count;
static uint32_t data_off;

#define CACHE_VERSION 1

#define THIS_BYTE_ORDER  0x44332211
#define OTHER_BYTE_ORDER1 0x11223344
#define OTHER_BYTE_ORDER2 0x33441122
#define OTHER_BYTE_ORDER3 0x22114433


static void xread(void *data, size_t size, size_t count)
{
  if (fread(data,size,count,cache_file) < count)
    error("Failed read from cache file `%s'.",cache_file_name);
}

static uint32_t xread32(void)
{
  uint32_t tmp;

  xread(&tmp,sizeof(tmp),1);

  return tmp;
}

static int legendre_cache_init(const char *file_name)
{
  if ((cache_file = fopen(file_name,"rb")) == NULL)
    return 0;

  cache_file_name = file_name;
  switch(xread32())
  {
    case THIS_BYTE_ORDER:
      break;
    case OTHER_BYTE_ORDER1:
    case OTHER_BYTE_ORDER2:
    case OTHER_BYTE_ORDER3:
      error("Incompatible (%s) cache file `%s'.","wrong byte order",file_name);
    default:
      error("Unrecognised cache file `%s'.",file_name);
  }
  if (xread32() != CACHE_VERSION)
    error("Incompatible (%s) cache file `%s'.","wrong version",file_name);
#if (BASE == 0)
  if (xread32() != b_term)
    error("Incompatible (%s) cache file `%s'.","wrong base",file_name);
#else
  if (xread32() != BASE)
    error("Incompatible (%s) cache file `%s'.","wrong base",file_name);
#endif
  if (xread32() != sizeof(uint_fast32_t))
    error("Incompatible (%s) cache file `%s'.","wrong width",file_name);
  if (xread32() != sizeof(rec_t))
    error("Incompatible (%s) cache file `%s'.","wrong alignment",file_name);

  report(0,"Opening version %u cache file `%s'...",
         (unsigned int)CACHE_VERSION,file_name);

  rec_count = xread32();
  REC = xmalloc(rec_count*sizeof(rec_t));
  xread(REC,sizeof(rec_t),rec_count);
  data_off = ftell(cache_file);
  assert(data_off % sizeof(uint_fast32_t) == 0);
  loaded_from_cache = 0;

#if HAVE_MMAP
 {
   uint32_t i, len;

   for (i = len = 0; i < rec_count; i++)
     len += REC[i].len;

   mmap_data = mmap(NULL,data_off+len*sizeof(uint_fast32_t),
                    PROT_READ,MAP_PRIVATE,fileno(cache_file),0);
   if (mmap_data == (void *)-1)
     warning("Failed to mmap() cache file `%s', will use malloc() instead.",
             file_name);
 }
#endif

  return 1;
}

static void legendre_cache_fini(void)
{
  if (cache_file)
  {
    fclose(cache_file);
    cache_file = NULL;
    free(REC);
    report(1,"Loaded Legendre symbol lookup tables for %"PRIu32
           " sequences from `%s'.", loaded_from_cache, cache_file_name);
  }
}

static uint_fast32_t *load_table(uint32_t seq, uint32_t m)
{
  uint_fast32_t *B, checksum;
  uint32_t i, j, k;
  int32_t c;

  if (cache_file == NULL)
    return NULL;

  k = SEQ[seq].k;
  c = SEQ[seq].c;

  for (i = 0; i < rec_count; i++)
    if (REC[i].k == k && REC[i].c == c && REC[i].parity == seq_parity(seq))
    {
      if (REC[i].m != m)
        error("Invalid record for %s in `%s'.",seq_str(seq),cache_file_name);
      break;
    }
  if (i == rec_count)
    return NULL; /* Not in cache */

#ifndef NDEBUG
  report(0,"Loading Legendre symbol lookup table for %s ...",seq_str(seq));
#endif

#if HAVE_MMAP
  if (mmap_data != (void *)-1)
    B = (uint_fast32_t *)(mmap_data+data_off)+REC[i].off;
  else
  {
    /* mmap() failed, fall back to using fseek/malloc/read instead. */
#endif
    if (fseek(cache_file,data_off+REC[i].off*sizeof(uint_fast32_t),SEEK_SET))
      error("Seek failed in cache file `%s'.", cache_file_name);
    B = xmalloc(REC[i].len*sizeof(uint_fast32_t));
    xread(B,sizeof(uint_fast32_t),REC[i].len);
#if HAVE_MMAP
  }
#endif

  for (checksum = 0, j = 0; j < REC[i].len; j++)
    checksum += B[j];
  if (REC[i].checksum != checksum)
    error("Bad checksum for %s in `%s'.",seq_str(seq),cache_file_name);

  loaded_from_cache++;

  return B;
}

static void xwrite(const void *data, size_t size, size_t count)
{
  if (fwrite(data,size,count,cache_file) < count)
    error("Failed write to cache file `%s'.",cache_file_name);
}

static void xwrite32(uint32_t data)
{
  xwrite(&data,sizeof(data),1);
}

void write_legendre_cache(const char *file_name)
{
  uint32_t i, j, off;

  assert(seq_count > 0);

  if ((cache_file = fopen(file_name,"wb")) == NULL)
  {
    warning("Cannot create cache file `%s'.",file_name);
    return;
  }

  cache_file_name = file_name;
  xwrite32(THIS_BYTE_ORDER);
  xwrite32(CACHE_VERSION);
#if (BASE == 0)
  xwrite32(b_term);
#else
  xwrite32(BASE);
#endif
  xwrite32(sizeof(uint_fast32_t));
  xwrite32(sizeof(rec_t));
  xwrite32(seq_count);

  for (off = 0, i = 0; i < seq_count; i++)
  {
    rec_t tmp;

    tmp.k = SEQ[i].k;
    tmp.c = SEQ[i].c;
    tmp.m = SEQ[i].mod;
    tmp.parity = seq_parity(i);
    tmp.len = bitmap_size(SEQ[i].mod);
    tmp.off = off;
    off += tmp.len;
    tmp.checksum = 0;
    for (j = 0; j < tmp.len; j++)
      tmp.checksum += SEQ[i].map[j];
    xwrite(&tmp,sizeof(tmp),1);
  }

  assert(sizeof(rec_t) % sizeof(uint_fast32_t) == 0);

  for (i = j = 0; i < seq_count; i++)
    xwrite(SEQ[i].map,sizeof(uint_fast32_t),bitmap_size(SEQ[i].mod));

  if (fclose(cache_file))
    warning("Problem closing cache file `%s'.",cache_file_name);
  cache_file = NULL;

  report(1,"Wrote %"PRIu32" Legendre symbol lookup tables to version %u"
         " cache file `%s'.", i, (unsigned int)CACHE_VERSION, cache_file_name);

#if HAVE_MMAP
  /* Reload cache file using mmap() so that memory can be shared with other
     processes.
  */
  for (i = 0; i < seq_count; i++)
    free(SEQ[i].map);
  if (legendre_cache_init(file_name) == 0)
    error("Failed to reload cache file `%s'.", file_name); 
  for (i = 0; i < seq_count; i++)
    if ((SEQ[i].map = load_table(i,SEQ[i].mod)) == NULL)
      error("Failed to reload table for %s from `%s'.",seq_str(i),file_name);
  legendre_cache_fini();
#endif
}
#endif /* LEGENDRE_CACHE */


/* Return the least factor d of n such that n/d is a square.
 */
static uint32_t core32(uint32_t n)
{
  uint32_t c = 1, d, q, r;

  assert(n > 0);

  while (n % 2 == 0)
  {
    n /= 2;
    if (n % 2 != 0)
      c *= 2;
    else
      n /= 2;
  }
  while (n % 3 == 0)
  {
    n /= 3;
    if (n % 3 != 0)
      c *= 3;
    else
      n /= 3;
  }
  r = sqrt(n);
  if (r*r == n)
    return c;
  for (q = 5, d = 4; q <= r; d = 6-d, q += d)
    if (n % q == 0)
    {
      do
      {
        n /= q;
        if (n % q != 0)
          c *= q;
        else
          n /= q;
      }
      while (n % q == 0);
      r = sqrt(n);
      if (r*r == n)
        return c;
    }

  return c * n;
}

/* Return (a/p) if gcd(a,p)==1, 0 otherwise.
 */
static int jacobi32(int32_t a, uint32_t p)
{
  uint32_t x, y, t;
  int sign;

  if (a < 0)
    x = -a, sign = (p % 4 == 1) ? 1 : -1;
  else
    x = a, sign = 1;

  for (y = p; x > 0; x %= y)
  {
    for ( ; x % 2 == 0; x /= 2)
      if (y % 8 == 3 || y % 8 == 5)
        sign = -sign;

    t = x, x = y, y = t;

    if (x % 4 == 3 && y % 4 == 3)
      sign = -sign;
  }

  return (y == 1) ? sign : 0;
}

#if CHECK_FOR_GFN
/* Return the greatest value y < 6 such that x = A^(2^y).
 */
static uint32_t squares(uint32_t x)
{
  uint32_t r;
  uint32_t y;

  for (y = 0; y < 5; y++)
  {
    r = sqrt(x);
    if (r*r != x)
      break;
    x = r;
  }

  return y;
}

/* Return the greatest value y < 6 such that x = M*(2^y).
 */
static uint32_t twos(uint32_t x)
{
  uint32_t y;

  for (y = 0; y < 5; y++)
  {
    if (x % 2)
      break;
    x /= 2;
  }

  return y;
}

/* Find the greatest value y < 6 such that every term is of the form A^(2^y)+1.
*/
static uint32_t gen_fermat_y(uint32_t seq)
{
  uint32_t i, y;

  assert (SEQ[seq].c > 0);

  /* k*b^n+1 must satisfy: */

  /* 1.  n = M*2^y.  */
  for (i = SEQ[seq].first, y = 5; i <= SEQ[seq].last; i++)
    y = MIN(y,MIN(twos(SUBSEQ[i].a),twos(SUBSEQ[i].b)));
#if (BASE == 0)
  y += squares(b_term);
#else
  y += squares(BASE);
#endif

  /* 2.  k = A^(2^y) */
  y = MIN(y,squares(SEQ[seq].k));

  return y;
}
#endif

/* Set .mod and .map fields for sequence k*b^n+c so that bit (p/2)%mod of
   map is set if and only if:
   (-ck/p)=1 for sequences with all n even,
   (-bck/p)=1 for sequences with all n odd,
   (-ck/p)=1 or (-bck/p)=1 for sequences with both odd and even n.

   In the worst case the table for k*b^n+c could be 4*b*k bits long.

   TODO: Consider the case where gcd(k,b) > 1.
*/
static void generate_legendre_lookup_table(uint32_t seq)
{
  uint_fast32_t *B;
  uint32_t k, i, m, b;
  int32_t c, r;

  k = SEQ[seq].k;
  c = SEQ[seq].c;
  m = core32(k);

#if CHECK_FOR_GFN
  if (c == 1 && m == 1 && (i = gen_fermat_y(seq)) > 0)
  {
    /* Only need to check primes p = 1 (mod 2^(i+1)) */

    m = 1 << i;
    B = make_bitmap(m);
    set_bit(B,0);
    if (verbose_opt)
      report(1,"GFN sequence: %s = A^%"PRIu32"+B^%"PRIu32,seq_str(seq),m,m);
    SEQ[seq].mod = m;
    SEQ[seq].map = B;
    return;
  }
#endif

#if (BASE == 0)
  b = core32(b_term);
#else
  b = core32(BASE);
#endif

  /* We may need the signed product c*b*m, and the unsigned product m*b*2.
  */
  if (m >= INT32_MAX/b)
    error("%s: Square-free part of k or c is too large in k*b^n+c.",
          seq_str(seq));
  r = (c < 0) ? m : -m;

  switch (seq_parity(seq))
  {
    default:
    case 1: /* odd n, test for (-bck/p)==1 */
      m *= b;
      r *= b;
      /* Fall through */
    case 0: /* even n, test for (-ck/p)==1 */
      if ((r < 0 && (-r) % 4 != 3) || (r > 0 && r % 4 != 1))
        m *= 2;
#if LEGENDRE_CACHE
      B = load_table(seq,m);
      if (B != NULL)
        break;
#endif
     report(0,"Building Legendre symbol lookup table for %s ...",seq_str(seq));
      B = make_bitmap(m);
      for (i = 0; i < m; i++)
        if (jacobi32(r,2*i+1) == 1)
          set_bit(B,i);
      break;
#if ALLOW_MIXED_PARITY_SEQUENCES
    case 2: /* even and odd n, test for (-ck/p)==1 or (-bck/p)==1 */
      m = m*2*b;
# if LEGENDRE_CACHE
      B = load_table(seq,m);
      if (B != NULL)
        break;
# endif
     report(0,"Building Legendre symbol lookup table for %s ...",seq_str(seq));
      B = make_bitmap(m);
      for (i = 0; i < m; i++)
        if (jacobi32(r,2*i+1) == 1 || jacobi32(r*b,2*i+1) == 1)
          set_bit(B,i);
      break;
#endif
  }

#if 0
  if (1)
  {
    uint32_t pop, check;
    for (pop = check = i = 0; i < m; i++)
      if (test_bit(B,i))
      {
        pop++;
        check += i;
      }
    report(1,"%s: length=%"PRIu32", popcount=%"PRIu32", checksum=%"PRIu32,
           seq_str(seq), m, pop, check);
  }
#endif

  SEQ[seq].mod = m;
  SEQ[seq].map = B;
}


void generate_legendre_tables(const char *file_name)
{
  uint32_t i, size;

#if NO_LOOKUP_OPT
  if (no_lookup_opt)
  {
    for (i = 0; i < seq_count; i++)
    {
      uint32_t m = core32(SEQ[i].k);
      if (m >= INT32_MAX)
        error("%s: Square-free part %"PRIu32" of k too large.",seq_str(i),m);
      SEQ[i].kc_core = m*(-SEQ[i].c);
      SEQ[i].parity = seq_parity(i);
    }
    return;
  }
#endif

#if LEGENDRE_CACHE
  if (file_name != NULL)
    legendre_cache_init(file_name);
#endif

  for (i = 0, size = 0; i < seq_count; i++)
  {
    generate_legendre_lookup_table(i);
    size += SEQ[i].mod;
  }

#if LEGENDRE_CACHE
  legendre_cache_fini();
#endif

  if (verbose_opt)
    report(1,"Using %"PRIu32"Kb for Legendre symbol lookup tables.",
           size/8/1024);
}
