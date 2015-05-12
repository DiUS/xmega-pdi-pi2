#ifndef _PDI_H_
#define _PDI_H_

#include <stdint.h>
#include <stdbool.h>

#define PDI_REG_STATUS  0x00
#define PDI_REG_RESET   0x01
#define PDI_REG_CONTROL 0x02

// --- Initialisation (including pushing the device into PDI mode) ---
bool pdi_init (uint8_t clk_pin, uint8_t data_pin, uint16_t delay_us);

// --- Low-level API -------------------------------------------------

// PDI commands
enum {
  LDS    = 0x00, // lower 4 bits: address-size; data-size (byte, word, 3, long)
  STS    = 0x40, // "-"
  LD     = 0x20, // lower 4 bits: ptr mode; size (byte, word, 3, long)
  ST     = 0x60, // -"-
  LDCS   = 0x80, // lower 2 bits: register no
  STCS   = 0xC0, // -"-
  KEY    = 0xE0,
  REPEAT = 0xA0  // lower 2 bits indicate length field following
};

// pointer modes (*ptr, *ptr++, ptr, ptr++), for LD/ST commands
enum {
  xPTR   = (0 << 2),
  xPTRpp = (1 << 2),
  PTR    = (2 << 2),
  PTRpp  = (3 << 2)
};

// sizes
enum {
  SZ_1 = 0,
  SZ_2 = 1,
  SZ_3 = 2,
  SZ_4 = 3
};

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
