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

int foo = 0;

int main (int argc, char *argv[])
{
  (void)argc; (void)argv;

  signal (SIGINT, on_sig);
  signal (SIGTERM, on_sig);
  signal (SIGQUIT, on_sig);

  srand (time (0));
  int r = rand ();

  if (!pdi_init (11, 9, 0)) // sclk, miso, 0us
    return 1;

  static const char init[] = {
    STCS | 0x02, 0x07, // 2 idle bits
    STCS | 0x01, 0x59, // hold device in reset
    KEY, 0xFF, 0x88, 0xD8, 0xCD, 0x45, 0xAB, 0x89, 0x12, // enable NVM
  };

  if (!pdi_send (init, sizeof (init)) || !nvm_wait_enabled ())
    return 2;

#if 0
  if (!nvm_chip_erase ())
  {
    printf ("failed to perform chip erase\n");
    return 6;
  }
#endif

  uint32_t addr = 0x800200;

  #if 1
  static char wpage[512];
  for (unsigned i = 0x0; i < sizeof (wpage); ++i)
    wpage[i] = i + r;
  if (!nvm_rewrite_page (addr, wpage, sizeof (wpage)))
  {
    printf ("failed to rewrite nvm, foo is %d\n", foo);
  }
#endif

  static char page[512] = { 0, };
  if (!nvm_read (addr, page, sizeof (page)))
  {
    printf ("failed to read nvm, foo is %d\n", foo);
    return 5;
  }

  dump (addr, page, sizeof (page));

  return 0;
}
