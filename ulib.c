#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
  {
    return -1;
  }
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}


//ASS 4 Itoa : 

//integer to address.
char * itoa(char buffer[] , int num) 
{ 
    char numbers[] = "0123456789";
    char * temp = 0;
    int multiply = 0;
    int base = 10; //decimal case
    //if the number is 0, return 48 in the first offset which represent the number 0.
    if (!num)
    {
        buffer[0] = 48;
        buffer[1] = 0;
        return buffer;
    }
    temp = buffer;
    // //if the number is negative , than we want to add '-' to represent negative.
    if (num < 0) 
    {
      num *= -1;
        *temp++ = '-'; 
    }
    temp++;
    multiply = num;
    //loop for counting the digits
    multiply = multiply / base;
    while(multiply != 0)
    {
      temp++;
      multiply = multiply / base;
    }
    *temp = '\0';
    //loop for finding a char that represent the digit
    *--temp = numbers[num % base];
    num = num / base;
    while(num != 0)
    { 
        *--temp = numbers [num % base];
        num = num / base;
    } 
    return buffer;
}



void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}
