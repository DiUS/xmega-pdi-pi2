#include "pdi.h"
#include <sys/signal.h>
#include <stdio.h>

void on_sig (int sig)
{
  signal (sig, SIG_DFL);
  pdi_stop ();
}


void on_done (bool success, pdi_sequence_t *seq)
{
  (void)seq;
  printf ("job %s\n", success ? "completed" : "FAILED");
}


int main (int argc, char *argv[])
{
  (void)argc; (void)argv;

  if (!pdi_init (11, 9)) // sclk, miso
    return 1;

  signal (SIGINT, on_sig);
  signal (SIGTERM, on_sig);
  signal (SIGQUIT, on_sig);

  #define STCS 0xC0
  #define LDCS 0x80
  #define KEY 0x0E

  char reset_cmd[] = { STCS | 0x01, 0x59 }; // pdi reset
  // KEY { data } command
  char key_cmd[] = { KEY, 0x12, 0x89, 0xAB, 0x45, 0xCD, 0xD8, 0x88, 0xFF };
  // read nvmen flag from status
  char nvmen_cmd[] = { LDCS | 0x00 };
  char status = 0x00;
  pdi_transfer_t xf[4];
  xf[0].len = sizeof (reset_cmd);
  xf[0].buf = reset_cmd;
  xf[0].dir = PDI_OUT;
  xf[1].len = sizeof (key_cmd);
  xf[1].buf = key_cmd;
  xf[1].dir = PDI_OUT;
  xf[2].len = sizeof (nvmen_cmd);
  xf[2].buf = nvmen_cmd;
  xf[2].dir = PDI_OUT;
  xf[3].len = 1;
  xf[3].buf = &status;
  xf[3].dir = PDI_IN;

  pdi_sequence_t seq[4];
  seq[0].next = &seq[1];
  seq[0].xfer = &xf[0];
  seq[1].next = &seq[2];
  seq[1].xfer = &xf[1];
  seq[2].next = &seq[3];
  seq[2].xfer = &xf[2];
  seq[3].next = 0;
  seq[3].xfer = &xf[3];

  if (!pdi_set_sequence (seq, on_done))
    return 2;

  pdi_run ();

  printf ("PDI status reg: 0x%02x\n", status);
  return 0;
}
