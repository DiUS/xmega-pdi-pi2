/* Copyright (C) 2015 DiUS Computing Pty. Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "nvm.h"
#include "pdi.h"

#define PAGE_SIZE 512
#define WAIT_ATTEMPTS 2000

enum {
  NVM_NOP                           = 0x00,
  NVM_CHIP_ERASE                    = 0x40, // cmdex
  NVM_READ                          = 0x43, // pdi read

  NVM_LOAD_PAGE_BUF                 = 0x23, // pdi write
  NVM_ERASE_PAGE_BUF                = 0x26, // cmdex

  NVM_ERASE_FLASH_PAGE              = 0x2B, // pdi write
  NVM_WRITE_FLASH_PAGE              = 0x2E, // pdi write
  NVM_ERASE_WRITE_FLASH_PAGE        = 0x2F, // pdi write
  NVM_FLASH_CRC                     = 0x78, // cmdex

  NVM_ERASE_APP_SECTION             = 0x20, // pdi write
  NVM_ERASE_APP_SECTION_PAGE        = 0x22, // pdi write
  NVM_WRITE_APP_SECTION_PAGE        = 0x24, // pdi write
  NVM_ERASE_WRITE_APP_SECTION_PAGE  = 0x25, // pdi write
  NVM_APP_SECTION_CRC               = 0x38, // cmdex

  NVM_ERASE_BOOT_SECTION            = 0x68, // pdi write
  NVM_ERASE_BOOT_SECTION_PAGE       = 0x2A, // pdi write
  NVM_WRITE_BOOT_SECTION_PAGE       = 0x2C, // pdi write
  NVM_ERASE_WRITE_BOOT_SECTION_PAGE = 0x2D, // pdi write
  NVM_BOOT_SECTION_CRC              = 0x39, // nvmaa???

  NVM_READ_USERSIG_ROW              = 0x03, // pdi read
  NVM_ERASE_USERSIG_ROW             = 0x18, // pdi write
  NVM_WRITE_USERSIG_ROW             = 0x1A, // pdi write
  NVM_READ_CALIBRATION_ROW          = 0x02, // pdi read

  NVM_READ_FUSE                     = 0x07, // pdi read
  NVM_WRITE_FUSE                    = 0x4C, // pdi write
  NVM_WRITE_LOCK_BITS               = 0x08, // cmdex

  NVM_LOAD_EEPROM_PAGE_BUF          = 0x33, // pdi write
  NVM_ERASE_EEPROM_PAGE_BUF         = 0x36, // cmdex

  NVM_ERASE_EEPROM                  = 0x30, // cmdex
  NVM_ERASE_EEPROM_PAGE             = 0x32, // pdi write
  NVM_WRITE_EEPROM_PAGE             = 0x34, // pdi write
  NVM_ERASE_WRITE_EEPROM_PAGE       = 0x35, // pdi write
  NVM_READ_EEPROM                   = 0x06  // pdi read
};

#define NVM_REG_BASE    0x010001C0
#define NVM_REG_CMD_OFFS      0x0A
#define NVM_REG_CTRLA_OFFS    0x0B
#define NVM_REG_STATUS_OFFS   0x0F
#define NVM_REG_LOCKBITS_OFFS 0x10

#define NVM_CTRLA_CMDEX_bm (1 << 0)
#define NVM_STATUS_BUSY_bm (1 << 7)

#define PDI_NVMEN_bm 0x02



// --- Helper functions --------------------------------------------

static inline bool nvm_cmdex (void)
{
  static const char cmds[] = {
    STS | (SZ_4 << 2) | SZ_1,
    ((NVM_REG_BASE + NVM_REG_CTRLA_OFFS)      ) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CTRLA_OFFS) >>  8) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CTRLA_OFFS) >> 16) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CTRLA_OFFS) >> 24) & 0xff,
    NVM_CTRLA_CMDEX_bm
  };
  return pdi_send (cmds, sizeof (cmds));
}


static inline bool nvm_loadcmd (uint8_t cmd)
{
  char cmds[] = {
    STS | (SZ_4 << 2) | SZ_1,
    ((NVM_REG_BASE + NVM_REG_CMD_OFFS)      ) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CMD_OFFS) >>  8) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CMD_OFFS) >> 16) & 0xff,
    ((NVM_REG_BASE + NVM_REG_CMD_OFFS) >> 24) & 0xff,
    cmd
  };
  return pdi_send (cmds, sizeof (cmds));
}


static inline bool nvm_controller_busy_wait (void)
{
  static const char cmds[] = {
    ST | PTR | SZ_4,
    ((NVM_REG_BASE + NVM_REG_STATUS_OFFS)      ) & 0xff,
    ((NVM_REG_BASE + NVM_REG_STATUS_OFFS) >>  8) & 0xff,
    ((NVM_REG_BASE + NVM_REG_STATUS_OFFS) >> 16) & 0xff,
    ((NVM_REG_BASE + NVM_REG_STATUS_OFFS) >> 24) & 0xff
  };

  if (!pdi_send (cmds, sizeof (cmds)))
    return false;

  const char status_cmd = LD | xPTR | SZ_1;
  char status = 0;
  int max_attempts = WAIT_ATTEMPTS;
  do
  {
    if (!pdi_sendrecv (&status_cmd, 1, &status, 1))
      return false;
    if (--max_attempts == 0)
      return false;
  } while (status & NVM_STATUS_BUSY_bm);

  return true;
}


// --- API functions -----------------------------------------------

bool nvm_wait_enabled (void)
{
  const char read_status = LDCS | PDI_REG_STATUS;
  char status = 0x00;
  int max_attempts = WAIT_ATTEMPTS;
  while (!(status & PDI_NVMEN_bm))
  {
    if (--max_attempts == 0)
      return false;
    if (!pdi_sendrecv (&read_status, 1, &status, 1))
      return false;
  }
  return true;
}


bool nvm_read (uint32_t addr, char *buf, uint32_t len)
{
  uint32_t rpt = len -1;
  char cmds[] = {
    ST | PTR | SZ_4,
    (addr      ) & 0xff,
    (addr >>  8) & 0xff,
    (addr >> 16) & 0xff,
    (addr >> 24) & 0xff,

    REPEAT | SZ_4,
    (rpt      ) & 0xff,
    (rpt >>  8) & 0xff,
    (rpt >> 16) & 0xff,
    (rpt >> 24) & 0xff,

    LD | xPTRpp | SZ_1
  };

  return
    nvm_controller_busy_wait () &&
    nvm_loadcmd (NVM_READ) &&
    pdi_sendrecv (cmds, sizeof (cmds), buf, len);
}


bool nvm_rewrite_page (uint32_t addr, const char *buf, uint16_t len)
{
  if (len > PAGE_SIZE)
    return false;

  if (!nvm_controller_busy_wait () ||
      !nvm_loadcmd (NVM_ERASE_PAGE_BUF) ||
      !nvm_cmdex ())
    return false;

  if (!nvm_controller_busy_wait () ||
      !nvm_loadcmd (NVM_LOAD_PAGE_BUF))
    return false;

  // I would guess only the lower PAGE_SIZE part of the address is relevant
  // while writing to the page buffer, but the application note is very unclear
  uint16_t rpt = len -1;
  char buf_cmds[] = {
    ST | PTR | SZ_4,
    (addr      ) & 0xff,
    (addr >>  8) & 0xff,
    (addr >> 16) & 0xff,
    (addr >> 24) & 0xff,

    REPEAT | SZ_2,
    (rpt     ) & 0xff,
    (rpt >> 8) & 0xff,

    ST | xPTRpp | SZ_1
  };
  if (!pdi_send (buf_cmds, sizeof (buf_cmds)) || !pdi_send (buf, len))
    return false;

  char page_cmds[] = {
    ST | PTR | SZ_4,
    (addr      ) & 0xff,
    (addr >>  8) & 0xff,
    (addr >> 16) & 0xff,
    (addr >> 24) & 0xff,
    ST | xPTRpp | SZ_1, // dummy write to trigger erase+program from page buf
    0
  };

  return
    nvm_loadcmd (NVM_ERASE_WRITE_FLASH_PAGE) &&
    pdi_send (page_cmds, sizeof (page_cmds)) &&
    nvm_controller_busy_wait ();
}


bool nvm_chip_erase (void)
{
  return
    nvm_controller_busy_wait () &&
    nvm_loadcmd (NVM_CHIP_ERASE) &&
    nvm_cmdex () &&
    nvm_wait_enabled () &&
    nvm_controller_busy_wait ();
}
