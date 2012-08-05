/* cpu.c -- (C) Geoffrey Reynolds, March 2007.

   Detect CPU properties.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "sr5sieve.h"

#if (__APPLE__ && __ppc64__)
#include <sys/sysctl.h>
#endif

#if (__linux__ && __powerpc64__)
#include <dirent.h>
#define CPU_DIR_NAME "/proc/device-tree/cpus"
#define L1_FILE_NAME "d-cache-size"
#define L2_FILE_NAME "l2-cache/d-cache-size"

static unsigned read_cache_size_file(const char *dirname, const char *filename)
{
  char fn[FILENAME_MAX+1];
  FILE *fp;
  unsigned data = 0;

  snprintf(fn,FILENAME_MAX,"%s/%s/%s",CPU_DIR_NAME,dirname,filename);
  fn[FILENAME_MAX] = '\0';
  if ((fp = fopen(fn,"r")) != NULL)
  {
    fread(&data,sizeof(data),1,fp);
    fclose(fp);
  }

  return data/1024;
}
#endif

#if USE_ASM && defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
/* In misc-i386.S or misc-x86-64.S */
extern uint32_t my_cpuid(uint32_t op, uint32_t *R);
#endif

/* Set L1_cache_size, L2_cache_size in Kb.
 */
void set_cache_sizes(uint32_t L1_opt, uint32_t L2_opt)
{
  uint32_t L1 = 0, L2 = 0;

#if (USE_ASM && __GNUC__ && (__i386__ || __x86_64__))
  union {uint32_t regs[4]; uint8_t bytes[16]; } data;
  uint32_t i, j;
  if (my_cpuid(0,data.regs) >= 2 && data.regs[1]==0x756e6547
      && data.regs[2] == 0x6c65746e && data.regs[3] == 0x49656e69)
  {
    /* Intel method. */
    for (i = 0; i < (my_cpuid(2,data.regs) & 0xFF); i++)
      for (j = 1; j < 16; j++)
        if ((data.regs[j/4] & 0x80000000) == 0)
          switch (data.bytes[j])
          {
            case 0x0A: L1 = 8; break;
            case 0x0C: L1 = 16; break;
            case 0x2C: L1 = 32; break;
            case 0x39: L2 = 128; break;
            case 0x3A: L2 = 192; break;
            case 0x3B: L2 = 128; break;
            case 0x3C: L2 = 256; break;
            case 0x3D: L2 = 384; break;
            case 0x3E: L2 = 512; break;
            case 0x41: L2 = 128; break;
            case 0x42: L2 = 256; break;
            case 0x43: L2 = 512; break;
            case 0x44: L2 = 1024; break;
            case 0x45: L2 = 2048; break;
            case 0x49: L2 = 4096; break;
            case 0x60: L1 = 16; break;
            case 0x66: L1 = 8; break;
            case 0x67: L1 = 16; break;
            case 0x68: L1 = 32; break;
            case 0x78: L2 = 1024; break;
            case 0x79: L2 = 128; break;
            case 0x7A: L2 = 256; break;
            case 0x7B: L2 = 512; break;
            case 0x7C: L2 = 1024; break;
            case 0x7D: L2 = 2048; break;
            case 0x7F: L2 = 512; break;
            case 0x82: L2 = 256; break;
            case 0x83: L2 = 512; break;
            case 0x84: L2 = 1024; break;
            case 0x85: L2 = 2048; break;
            case 0x86: L2 = 512; break;
            case 0x87: L2 = 1024; break;
          }

    if (L2 == 0)
    {
      i = my_cpuid(0x80000000,data.regs);
      if (i >= 0x80000006)
      {
        my_cpuid(0x80000006,data.regs);
        L2 = data.regs[2] >> 16;
      }
    }
  }
  else /* Sensible method. */
  {
    i = my_cpuid(0x80000000,data.regs);
    if (i >= 0x80000005)
    {
      my_cpuid(0x80000005,data.regs);
      L1 = data.regs[2] >> 24;
    }
    if (i >= 0x80000006)
    {
      my_cpuid(0x80000006,data.regs);
      L2 = data.regs[2] >> 16;
    }
  }
#elif (__APPLE__ && __ppc64__)
  /* This was taken from code supplied by Alex Greenbank.
   */
  int64_t i;
  size_t len = 8;
  if (sysctlbyname( "hw.l1dcachesize", &i, &len, NULL, 0 ) != -1)
    L1 = i/1024;
  if (sysctlbyname( "hw.l2cachesize", &i, &len, NULL, 0 ) != -1)
    L2 = i/1024;
#elif (__linux__ && __powerpc64__)
  /* From code supplied by Ed (Sheep).
   */
  DIR *dp;
  struct dirent *ep;

  if ((dp = opendir(CPU_DIR_NAME)) != NULL)
  {
    while (L1 == 0 && L2 == 0 && (ep = readdir(dp)) != NULL)
    {
      L1 = read_cache_size_file(ep->d_name,L1_FILE_NAME);
      L2 = read_cache_size_file(ep->d_name,L2_FILE_NAME);
    }
    closedir(dp);
  }
#endif

  /* Sanity check. If cache sizes really are outside these ranges then we
     can't make effective use of them anyway.
  */
  if (L1 < 8 || L1 > 1024)
    L1 = 0;
  if (L2 < 64 || L2 > 8192)
    L2 = 0;

  /* If there are user-supplied sizes then use them instead.
   */
  if (L1_opt)
    for (L1 = 1; L1 < L1_opt; L1 *= 2)
      ;
  if (L2_opt)
    for (L2 = 1; L2 < L2_opt; L2 *= 2)
      ;

  /* Resort to using defaults otherwise.
   */
  L1_cache_size = (L1) ? MIN(L1,1024) : DEFAULT_L1_CACHE_SIZE;
  L2_cache_size = (L2) ? MIN(L2,8192) : DEFAULT_L2_CACHE_SIZE;

  if (verbose_opt)
    printf("L1 data cache %"PRIu32"Kb (%s), L2 cache %"PRIu32"Kb (%s).\n",
           L1_cache_size, L1_opt? "supplied" : L1? "detected" : "default",
           L2_cache_size, L2_opt? "supplied" : L2? "detected" : "default");

  return;
}

#if USE_ASM && __i386__ && MULTI_PATH
int have_sse2(void)
{
  uint32_t regs[4];

  if (my_cpuid(0,regs) >= 1)
  {
    my_cpuid(1,regs);
    if (regs[3] & (1<<26))
      return 1;
  }

  return 0;
}

int have_cmov(void)
{
  uint32_t regs[4];

  if (my_cpuid(0,regs) >= 1)
  {
    my_cpuid(1,regs);
    if (regs[3] & (1<<15))
      return 1;
  }

  return 0;
}
#endif

#if USE_ASM && __GNUC__ && ((__i386__ && MULTI_PATH) || __x86_64__)
int is_amd(void)
{
  uint32_t regs[4];

  my_cpuid(0,regs);
  if (regs[1]==0x68747541 && regs[2]==0x444d4163 && regs[3]==0x69746e65)
    return 1;

  return 0;
}
#endif

#if USE_ASM && __GNUC__ && (ARM_CPU && MULTI_PATH)
int is_neon(void)
{
  FILE* file = NULL;
  int neon = 0;
  char line[257];
  
  file = fopen("/proc/cpuinfo","rb");
  
  while ((fgets(line,256,file))!= NULL)
  {
    if (strstr(line, "Features\t: ") != line)
      continue;
    if (strstr(line,"vfpv3d16")!=NULL)
      neon=0;
    else if (strstr(line,"neon")!=NULL)
      neon=1;
  }
  fclose(file);
  return neon;
}
#endif