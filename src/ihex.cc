#include "ihex.h"
#include "errinfo.h"
#include <string>
#include <vector>

bool load_ihex (std::istream &is, page_map_512_t &pages)
{
  uint32_t addr_upper = 0;
  std::string line;
  unsigned lineno = 0;
  while (std::getline (is, line) && line.size () >= 11)
  {
    line.erase (line.find_last_not_of (" \n\r\t") +1);

    uint8_t sum = 0;

    uint8_t count;
    uint8_t addr_hi;
    uint8_t addr_lo;
    uint8_t type;
    if (sscanf (
          line.c_str (), ":%02hhx%02hhx%02hhx%02hhx",
          &count, &addr_hi, &addr_lo, &type) != 4)
      return_errinfoloc (false, "malformed ihex record at line", lineno);
    if (line.size () != (11 + 2*count))
      return_errinfoloc (false, "ihex record length error at line", lineno);

    sum += count;
    sum += addr_hi;
    sum += addr_lo;
    sum += type;

    std::vector<uint8_t> data;
    uint8_t byte;
    for (unsigned i = 0; i < count; ++i)
    {
      if (sscanf (line.c_str () + 9 + 2*i, "%02hhx", &byte) != 1)
        return_errinfoloc (false, "bad ihex data at line", lineno);
      data.push_back (byte);
      sum += byte;
    }
    uint8_t checksum;
    if (sscanf (line.c_str () + line.size () -2, "%02hhx", &checksum) != 1)
      return_errinfoloc (false, "failed to read checksum field at line", lineno);
    sum += checksum;
    if (sum)
      return_errinfoloc (false, "checksum mismatch at line", lineno);

    switch (type)
    {
      case 0x00:
      {
        uint16_t addr = ((uint16_t)addr_hi << 8) | addr_lo;
        int16_t  offs = addr % 512;
        uint32_t pgaddr = addr_upper + addr - offs;
        auto *pg = &pages[pgaddr];
        pg->addr = pgaddr;
        for (size_t i = 0; i < data.size (); ++i)
        {
          if (offs + i == 512) // argh, page boundary!
          {
            pgaddr += 512;
            offs -= 512;
            pg = &pages[pgaddr];
          }
          pg->data[offs + i] = data[i];
        }
        break;
      }
      case 0x01: return true; // EOF
      case 0x02: addr_upper = (data[0] << 12) | (data[1] << 4);  break;
      case 0x03: break; // cs:ip, ignore
      case 0x04: addr_upper = (data[0] << 24) | (data[1] << 16); break;
      case 0x05: break; // eip, ignore
    }

    ++lineno;
  }

  return_errinfo (false, "no EOF record found");
}
