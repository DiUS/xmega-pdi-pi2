/* Copyright (C) 2015 DiUS Computing Pty. Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _ERRINFO_H_
#define _ERRINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

void set_errinfo (const char *str, int loc);

void get_errinfo (const char **pstr, int *ploc);

#define return_errinfoloc(retval, msg, loc) \
  do { set_errinfo (msg, loc); return retval; } while (0)

#define return_errinfo(retval, msg) \
  return_errinfoloc (retval, msg, -1)

#ifdef __cplusplus
}
#endif
#endif
