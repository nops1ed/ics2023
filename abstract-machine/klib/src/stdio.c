#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define MAX_IBUF 64

enum {
  NUM_DEC = 10, NUM_OCT = 8,  NUM_HEX = 16,
};

/* Write a single char to file stream or call putch() */
static void _writeC(char *out, char c) {
  if(out) *out = c;
  else putch(c);
}

/* Write an interger into the buffer according to type we wanna convert to. */

/* Well, this is not a good practice 
 * But putch is a default function so we have to treat it differently instead of a file stream
 */
static int _writeI(char *out, uint32_t _offset_, int num, size_t *n, uint32_t width, 
                       uint32_t type) {
  long int _num = num;
  /* This should be enough, or we consider it as overflow and cut it down */
  char buf[MAX_IBUF];
  uint32_t offset = 0;  
  if(_num == 0) buf[offset++] = '0';
  else 
    while(_num) {
      buf[offset++] = (_num % type) > 9? 'a' + (_num % type) - 10: _num % type + '0';
      _num /= type;
    }     
  for(int j = offset; j < width; j++) {
    if (out)  _writeC(out + _offset_ + j, '0'); 
    else _writeC(out, '0');
  }
  int i;
  for(i = 0; i < offset && *n > 0; i++, (*n)--)  
    if (out)  _writeC(out + _offset_ + i, *(buf + offset - 1 - i)); 
    else _writeC(out, *(buf + offset - 1 - i));
    //*(out + offset - 1 - i) = buf[i];
  return i; 
}

/* Write a string to buffer */
static int _writeS(char *out, uint32_t _offset_, const char *buffer, size_t *n, int len) {
  uint32_t offset;
  for (offset = 0; offset < len && *n > 0; offset++, (*n)--) {
    if(out) _writeC(out + _offset_ + offset, *(buffer + offset));
    else _writeC(out, *(buffer + offset));
    //*(out + _offset_ + offset) = *(buffer + offset);
  }
  return offset;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap , fmt);
  int ret = vsprintf(NULL, fmt, ap);
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

/* Trivial implement , We didn`t call _vsprintf_internal 
 * The behavior of copy we implement is just byte copy 
 */
int sprintf(char *out, const char *fmt, ...) {
  va_list ap;  
  va_start(ap, fmt);
  int ret = vsnprintf(out, -1, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;  
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  uint32_t offset = 0, len, width;
  for (const char *p = fmt; *p != '\0'; p++) {
    if (*p == '%') {  
      width = 0;
      p++;  
      for ( ; *p != '\0' && *p >= '0' && *p <= '9'; ++p) {
        width *= 10;
        width += *p - '0';
      }
			if (*p == 'd') {
        /* Some functions should be implemented here 
        * which returns a value: len, it symbolizes the length we write into 
        * the buffer
        */
        int num = va_arg(ap, int);
        //offset += _writeI(out + offset, va_arg(ap, int), &n, NUM_DEC);
        offset += _writeI(out, offset, num, &n, width, NUM_DEC);
			}
			else if (*p == 'x' || *p == 'X') {
        int num = va_arg(ap, int);
        offset += _writeI(out, offset, num, &n, width, NUM_HEX);
			}
			else if(*p == 's') {
        char *buf = va_arg(ap, char *);  
				len = strlen(buf);
        offset += _writeS(out, offset, buf, &n, len);
			}
			else if(*p == 'c') {
        char buf[8];
        *buf = (char)va_arg(ap, int);  
        offset += _writeS(out, offset, buf, &n, 1);
		  }
			else {
				char *buf = "%%";
        offset += _writeS(out, offset, buf, &n, 2);
		  }
    }  
		else {  
			char buf[8];
			buf[0] = *p;
      offset += _writeS(out, offset, buf, &n, 1);
    }  
  }  
  if(out && out + offset) _writeC(out + offset, '\0'); 
  else _writeC(out, '\0');
  //*(out + offset) = '\0'; 
  va_end(ap);
  return offset;
}

#endif
