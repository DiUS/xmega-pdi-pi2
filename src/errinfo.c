/* Copyright (C) 2015 DiUS Computing Pty. Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "errinfo.h"

static const char *errstr = 0;
static int errloc = -1;

void set_errinfo (const char *str, int loc)
{
  errstr = str;
  errloc = loc;
}

void get_errinfo (const char **pstr, int *ploc)
{
  if (pstr)
    *pstr = errstr;
  if (ploc)
    *ploc = errloc;
}
