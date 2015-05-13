/* Copyright (C) 2015 DiUS Computing Pty. Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _IHEX_H_
#define _IHEX_H_

#include "page_map.h"
#include <iostream>

typedef page_t<512>::container_t page_map_512_t;

bool load_ihex (std::istream &is, page_map_512_t &pages);

#endif
