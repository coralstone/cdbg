


#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>
/**/
#ifdef __cplusplus
extern "C" {
#endif


static void _log(const char *format,...)
{
    char buf[4096], *p = buf;
    va_list args;
    va_start(args, format);
    p += _vsnprintf(p, sizeof buf - 1, format, args);
    va_end(args);
    /*
        while ( p > buf && isspace(p[-1]) )
        {
        *--p = '\0';
        *p++ = '\r';
        *p++ = '\n';*p = '\0';
        }
    */
    //printf(buf);
    ::OutputDebugString(buf) ;
}

static const char* pre_fmt(const char* file,int line,void* buff,const char* fmt)
{
   int len = strlen(file);
   const char* p = file + len;
   while(len-->0) {
    if(*p =='\\' || *p == '/' )
    {p++; break;}
    p--;
   }

   sprintf((char*)buff,"%s:%d:%s\n",p,line,fmt);
   return (char*)buff;
}
#define pf(fmt) pre_fmt(__FILE__,__LINE__,(char*)alloca(strlen(fmt)+128),fmt)
#define log_d(fmt,...) _log(pf(fmt),##__VA_ARGS__)
#define log_i(fmt,...) printf(pf(fmt),##__VA_ARGS__)
#define get_err() GetLastError()
//#define log(fmt,...) __log(fmt,##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

