#ifndef FREESTANDING_STDDEF_H
#define FREESTANDING_STDDEF_H

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
typedef signed int ptrdiff_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
typedef signed int wchar_t;
#endif

#define offsetof(type, member) ((size_t)&(((type*)0)->member))

#endif // FREESTANDING_STDDEF_H
