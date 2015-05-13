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
