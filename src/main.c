#include "pdi.h"
#include "nvm.h"
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void on_sig (int sig)
{
  signal (sig, SIG_DFL);
  pdi_stop ();
}

void dump (uint32_t addr, char *p, uint32_t len)
{
  uint32_t at = (addr & ~15u);
  printf ("%08x: ", at);
  for (unsigned sp = 0; sp < (addr % 16); ++sp)
    printf ("   ");
  for (int i = (addr % 16) + 1; len; ++i, --len, ++p)
  {
    printf ("%02x ", *p);
    if (i == 8)
      putchar (' ');
    if (i == 16 && len != 1)
    {
      i = 0;
      at += 16;
      printf ("\n%08x: ", at);
    }
  }
  printf ("\n");
}


int main (int argc, char *argv[])
{
  (void)argc; (void)argv;

  signal (SIGINT, on_sig);
  signal (SIGTERM, on_sig);
  signal (SIGQUIT, on_sig);

  srand (time (0));
  int r = rand ();

  int ret = 0;

  if (!pdi_init (11, 9, 0) || // sclk, miso, 0us
      !pdi_open () ||
      !nvm_wait_enabled ())
    return 1;

#if 0
  if (!nvm_chip_erase ())
  {
    printf ("failed to perform chip erase\n");
    return 6;
  }
#endif

  uint32_t addr = 0x800200;

  static char wpage[512];
  for (unsigned i = 0x0; i < sizeof (wpage); ++i)
    wpage[i] = i + r;
  if (!nvm_rewrite_page (addr, wpage, sizeof (wpage)))
  {
    ret = 2;
    goto out;
  }

  static char page[512] = { 0, };
  if (!nvm_read (addr, page, sizeof (page)))
  {
    ret = 3;
    goto out;
  }

  printf ("ok\n");

out:
  pdi_close ();

  dump (addr, page, sizeof (page));

  return ret;
}
