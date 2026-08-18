/* prevent double include */
#pragma once

#if    defined (__BORLANDC__)
#define COMPILER_BORLAND

#elif defined (_MSC_VER)
#define COMPILER_MSVC

#elif defined (__CYGWIN32__)
#define COMPILER_CYGWIN

#elif defined (__MINGW32__)
#define COMPILER_MINGW

#elif defined (__LCC__)
#define COMPILER_LCC

#endif
