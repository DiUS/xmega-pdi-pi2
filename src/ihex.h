#ifndef _IHEX_H_
#define _IHEX_H_

#include "page_map.h"
#include <iostream>

typedef page_t<512>::container_t page_map_512_t;

bool load_ihex (std::istream &is, page_map_512_t &pages);

#endif
