#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *str) {
  const char *char_ptr;
  const unsigned long int *longword_ptr;
  unsigned long int longword, himagic, lomagic;
  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  for (char_ptr = str; ((unsigned long int) char_ptr
			& (sizeof (longword) - 1)) != 0;
       ++char_ptr)
    if (*char_ptr == '\0')
      return char_ptr - str;
  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to 8-byte longwords.  */
  longword_ptr = (unsigned long int *) char_ptr;
  /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
     the "holes."  Note that there is a hole just to the left of
     each byte, with an extra at the end:
     bits:  01111110 11111110 11111110 11111111
     bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD
     The 1-bits make sure that carries propagate to the next 0-bit.
     The 0-bits provide holes for carries to fall into.  */
  himagic = 0x80808080L;
  lomagic = 0x01010101L;
  if (sizeof (longword) > 4) {
      /* 64-bit version of the magic.  */
      /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
    himagic = ((himagic << 16) << 16) | himagic;
    lomagic = ((lomagic << 16) << 16) | lomagic;
  }
  if (sizeof (longword) > 8)
    assert(0);
  /* Instead of the traditional loop which tests each character,
     we will test a longword at a time.  The tricky part is testing
     if *any of the four* bytes in the longword in question are zero.  */
  for (;;) {
    longword = *longword_ptr++;
    if (((longword - lomagic) & ~longword & himagic) != 0) {
      /* Which of the bytes was the zero?  If none of them were, it was
        a misfire; continue the search.  */
      const char *cp = (const char *) (longword_ptr - 1);
      if (cp[0] == 0)
        return cp - str;
      if (cp[1] == 0)
        return cp - str + 1;
      if (cp[2] == 0)
        return cp - str + 2;
      if (cp[3] == 0)
        return cp - str + 3;
      if (sizeof (longword) > 4) {
        if (cp[4] == 0)
          return cp - str + 4;
        if (cp[5] == 0)
          return cp - str + 5;
        if (cp[6] == 0)
          return cp - str + 6;
        if (cp[7] == 0)
          return cp - str + 7;
      }
	  }
  }
}

char *strcpy(char *dst, const char *src) {
  return memcpy (dst, src, strlen(src) + 1);
}

/* Any UB should not be protected here. */
char *strncpy(char *dst, const char *src, size_t n) {
  return memcpy (dst, src, n);
}

char *strcat(char *dst, const char *src) {
  strcpy (dst + strlen (dst), src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  /* Avoid potential symbol-related issues. */
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;
  do {
    c1 = (unsigned char) *p1++;
    c2 = (unsigned char) *p2++;
    if (c1 == '\0')   return c1 - c2;
  } while (c1 == c2);
  return c1 - c2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  long int dstp = (long int)s;
  /* Here we still need page copy instead of byte copy
  * otherwise it performed too bad 
  */
  //PAGE_COPY_FWD();

  while (n > 0) {
      ((__BYTE *) dstp)[0] = c;
      dstp += 1;
      n -= 1;
  }
  return s; 
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

/* Not Thread-safe */
void *memcpy(void *dstpp, const void *srcpp, size_t n) {
  unsigned long int dstp = (long int) dstpp;
  unsigned long int srcp = (long int) srcpp;
  /* Lots of thing to be done here...
  * Here we need page copy and enable byte align 
  */


  /* Trivial Implement.
  * Just implement byte copy 
  */
  BYTE_COPY_FWD(dstp, srcp, n);
  /* Byte copy may not need align. */
  return dstpp;
}


int memcmp(const void *s1, const void *s2, size_t n) {
  long int srcp1 = (long int)s1;
  long int srcp2 = (long int)s2;
  long int res;
  long int a0, b0;
  /* Still lots of thing to do... */
  // TODO
  while (n != 0) {
      a0 = ((__BYTE *) srcp1)[0];
      b0 = ((__BYTE *) srcp2)[0];
      srcp1 += 1;
      srcp2 += 1;
      res = a0 - b0;
      if (res != 0)  return res;
      n -= 1;
  }
  return 0;
}

#endif
