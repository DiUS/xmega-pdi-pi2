#include "pdi.h"
#include <sys/signal.h>
#include <stdio.h>

#define STCS 0xC0
#define LDCS 0x80
#define KEY 0xE0

void on_sig (int sig)
{
  signal (sig, SIG_DFL);
  pdi_stop ();
}


int main (int argc, char *argv[])
{
  (void)argc; (void)argv;

  signal (SIGINT, on_sig);
  signal (SIGTERM, on_sig);
  signal (SIGQUIT, on_sig);

  if (!pdi_init (11, 9, 0)) // sclk, miso, 0us
    return 1;

  static const char init[] = {
    STCS | 0x02, 0x07, // 2 idle bits
    STCS | 0x01, 0x59, // hold device in reset
    KEY, 0xFF, 0x88, 0xD8, 0xCD, 0x45, 0xAB, 0x89, 0x12, // enable NVM
  };

  if (!pdi_send (init, sizeof (init)))
    return 2;

  const char read_status = LDCS | 0x00;
  char status = 0x00;
  while (status == 0x00)
  {
    if (!pdi_sendrecv (&read_status, 1, &status, 1))
      return 4;
  }
  printf ("Read status reg: 0x%02x, NVM controller %s\n", status, (status & 0x02) ? "ENABLED" : "disabled");

  return 0;
}
