/* Copyright (C) 2015 DiUS Computing Pty. Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _NVM_H_
#define _NVM_H_

#include <stdbool.h>
#include <stdint.h>

bool nvm_wait_enabled (void);
bool nvm_read (uint32_t addr, char *buf, uint32_t len);
bool nvm_rewrite_page (uint32_t addr, const char *buf, uint16_t len);
bool nvm_chip_erase (void);

#endif
