#pragma once
typedef unsigned long long U64;
typedef unsigned int   U32;
typedef   signed int   S32;
typedef unsigned short U16;
typedef   signed short S16;
typedef unsigned char  U8;

struct complex    {double real, imag;};
struct pixel      {U8 b, g, r, a;};
struct escape     {int iter; double distance;};
struct frac_value {escape e[9];};




