/*
   Copyright (C) 2004, Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#ifndef COMMON_H
#define COMMON_H 1

#if HAVE_CONFIG_H
#   include <config.h>
#endif

#include <stdio.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if STDC_HEADERS
#   include <stdlib.h>
#   include <stddef.h>
#else
#   if HAVE_STDLIB_H
#     include <stdlib.h>
#   endif
#endif /*STDC_HEADERS*/

#if HAVE_STRING_H
#   if !STDC_HEADERS && HAVE_MEMORY_H
#     include <memory.h>
#   endif
#   include <string.h>
#endif

#if HAVE_STRINGS_H
#   include <strings.h>
#endif

#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#if HAVE_ERRNO_H
#   include <errno.h>
#endif /*HAVE_ERRNO_H*/

#ifndef errno
extern int errno;
#endif

#include "xmalloc.h"

#if HAVE_LIMITS_H
#   include <limits.h>
#endif

#if HAVE_INTTYPES_H
#   include <inttypes.h>
#endif

#define EXIT_SUCCESS        0     
#define EXIT_DIFFER         1
#define EXIT_FATAL          2
#define EXIT_FAILED         3
#define EXIT_IDENTICAL      4

#define DEBUG 0

#endif /* !COMMON_H */
