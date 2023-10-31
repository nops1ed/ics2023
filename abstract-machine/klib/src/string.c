#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
  size_t len = 0;
  char *t = (char *)s;
  while (*t != '\0')
  {
    len++;
    t++;
  }
  return len;
}

char *strcpy(char *dst, const char *src)
{
  int i;
  for (i = 0; src[i] != '\0'; ++i)
  {
    dst[i] = src[i];
  }
  dst[i] = '\0';

  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  panic("Not implemented");
}

char *strcat(char *dst, const char *src)
{
  size_t len = strlen(dst);
  char *s = (char *)src;
  size_t i = 0;
  while (s[i] != '\0')
  {
    dst[len + i] = s[i];
    i++;
  }
  dst[len + i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  char *t1 = (char *)s1;
  char *t2 = (char *)s2;
  while (*t1 != '\0' && *t2 != '\0')
  {
    if (*t1 != *t2)
    {
      return (int)(*t1) - (int)(*t2);
    }
    t1++;
    t2++;
  }

  return (int)(*t1) - (int)(*t2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  size_t i = 0;
  char *t1 = (char *)s1;
  char *t2 = (char *)s2;

  while (*t1 != '\0' && *t2 != '\0' && i < n)
  {
    if (*t1 != *t2)
    {
      return (int)(*t1) - (int)(*t2);
    }
    t1++;
    t2++;
    i++;
  }
  if (i == n)
  {
    return 0;
  }
  return (int)(*t1) - (int)(*t2);
}

void *memset(void *s, int c, size_t n)
{
  char *t = (char *)s;
  for (size_t i = 0; i < n; i++)
  {
    t[i] = (char)(c);
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
  char *char_dst = (char *)dst;
  char *char_src = (char *)src;

  if (dst < src)
  {
    for (int i = 0; i < n; ++i)
      char_dst[i] = char_src[i];
  }
  else if (dst > src)
  {
    for (int i = n - 1; i >= 0; --i)
      char_dst[i] = char_src[i];
  } // else 不需要复制

  return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
  char *o = (char *)out;
  char *i = (char *)in;
  for (size_t j = 0; j < n; j++)
  {
    *(o + j) = *(i + j);
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  char *t1 = (char *)s1;
  char *t2 = (char *)s2;

  size_t i = 0;
  for (i = 0; i < n && t1[i] != '\0' && t2[i] != '\0'; i++)
  {
    if (t1[i] != t2[i])
    {
      return (int)(t1[i]) - (int)(t2[i]);
    }
  }

  if (i == n)
  {
    return 0;
  }
  return (int)(t1[i]) - (int)(t2[i]);
}

size_t strnlen(const char *s, size_t n)
{
  size_t cnt = 0;
  while (cnt < n && *s++ != '\0')
  {
    cnt++;
  }
  return cnt;
}

#endif
