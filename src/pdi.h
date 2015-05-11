#ifndef _PDI_H_
#define _PDI_H_

#include <stdint.h>
#include <stdbool.h>

// --- Initialisation (including pushing the device into PDI mode) ---
bool pdi_init (uint8_t clk_pin, uint8_t data_pin, uint16_t delay_us);

// --- Low-level API -------------------------------------------------
typedef enum { PDI_OUT, PDI_IN } pdi_dir_t;

typedef struct pdi_transfer
{
  uint32_t len;
  char *buf;
  pdi_dir_t dir;
} pdi_transfer_t;

typedef struct pdi_sequence
{
  pdi_transfer_t *xfer;
  struct pdi_sequence *next;
} pdi_sequence_t;

typedef void (*pdi_sequence_done_fn_t) (bool success, pdi_sequence_t *seq);


// returns false if a job is already in progress
// null ptrs or zero-length transfers NOT supported
bool pdi_set_sequence (pdi_sequence_t *sequence, pdi_sequence_done_fn_t fn);

// sends the double-break indication (unless a sequence is in progress)
bool pdi_break (void);

void pdi_run (void);

void pdi_stop (void);


// --- High-level API - be mindful of clock gaps - no printf'ing! -----
bool pdi_send (const char *buf, uint32_t len);
bool pdi_recv (char *buf, uint32_t len);
bool pdi_sendrecv (const char *cmd, uint32_t cmdlen, char *buf, uint32_t rxlen);

#endif
