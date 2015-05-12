#ifndef _NVM_H_
#define _NVM_H_

#include <stdbool.h>
#include <stdint.h>

bool nvm_wait_enabled (void);
bool nvm_read (uint32_t addr, char *buf, uint32_t len);
bool nvm_rewrite_page (uint32_t addr, const char *buf, uint16_t len);
bool nvm_chip_erase (void);

#endif
