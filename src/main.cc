extern "C" {
#include "pdi.h"
#include "nvm.h"
}
#include "ihex.h"
#include "errinfo.h"
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>

void on_sig (int sig)
{
  signal (sig, SIG_DFL);
  pdi_stop ();
}

void dump (uint32_t addr, char *p, uint32_t len)
{
  uint32_t at = (addr & ~15u);
  printf ("%08x: ", at);
  for (unsigned sp = 0; sp < (addr % 16); ++sp)
  {
    if (sp == 8)
      putchar (' ');
    printf ("   ");
  }
  for (int i = (addr % 16) + 1; len; ++i, --len, ++p)
  {
    printf ("%02x ", *p);
    if (i == 8)
      putchar (' ');
    if (i == 16 && len != 1)
    {
      i = 0;
      at += 16;
      printf ("\n%08x: ", at);
    }
  }
  printf ("\n");
}


int error_out (int code)
{
  const char *e;
  int l;
  get_errinfo (&e, &l);
  if (!e)
    e = "unknown error";


  fprintf (stderr, "error: %s", e);
  if (l != -1)
    fprintf (stderr, " %d\n", l);
  else
    fprintf (stderr, "\n");

  return code;
}

void syntax (const char *name)
{
  fprintf (stderr,
    "syntax: %s [-h] [-q] [-a baseaddr] [-b] [-c clkpin] [-d datapin] [-s pdidelay] [-D len@offs] [-E] [-F ihexfile]\n\n"
    "  -q             quiet mode\n"
    "  -a baseaddr    override base address (note: PDI address space)\n"
    "  -b             use default boot flash instead of app flash address\n"
    "  -c clkpin      set gpio pin to use as PDI_CLK\n"
    "  -d datapin     set gpio pin to use as PDI_DATA\n"
    "  -s pdidelay    set PDI clock delay, in us\n"
    "  -D len@offs    dump memory, len bytes from (baseaddr + offs)\n"
    "  -E             perform chip erase\n"
    "  -F ihexfile    write ihexfile\n"
    "  -h             show this help\n"
    "\n"
    , name);
  exit (-1);
}

#define bail_out(retval) \
  do { ret = retval; goto out; } while (0)


int main (int argc, char *argv[])
{
  (void)argc; (void)argv;

  int ret = 0;

  signal (SIGINT, on_sig);
  signal (SIGTERM, on_sig);
  signal (SIGQUIT, on_sig);

  bool quiet = false;
  uint32_t flash_base = 0x800000;
  uint8_t  clk_pin = 24, data_pin = 21; // j8.18, j8.40
  uint32_t pdi_delay_us = 0;

  bool dump_mem = false;
  uint32_t dump_addr = 0, dump_len = 0;
  const char *fname = 0;
  bool chip_erase = false;

  page_map_512_t page_map;

  int opt;
  while ((opt = getopt (argc, argv, "a:bc:d:h:s:qD:F:E")) != -1)
  {
    switch (opt)
    {
      case 'a': flash_base = strtoul (optarg, 0, 0); break;
      case 'b': flash_base = 0x840000; break; // bootarea for x256
      case 'c': clk_pin = atoi (optarg); break;
      case 'd': data_pin = atoi (optarg); break;
      case 's': pdi_delay_us = strtoul (optarg, 0, 0); break;
      case 'q': quiet = true; break;
      case 'D':
      {
        dump_mem = true;
        dump_len = strtoul (optarg, 0, 0);
        const char *addr = strchr (optarg, '@');
        if (addr)
          dump_addr = strtoul (addr + 1, 0, 0);
        else
          syntax (argv[0]);
        break;
      }
      case 'F': fname = optarg; break;
      case 'E': chip_erase = true; break;
      case 'h': // fall through
      default: syntax (argv[0]); break;
    }
  }

  if (!dump_mem && !fname && !chip_erase)
    syntax (argv[0]);

  if (dump_mem && (fname || chip_erase))
  {
    set_errinfo ("dumping not supported in conjunction with write/erase", -1);
    return error_out (1);
  }

  if (fname)
  {
    std::ifstream in (fname);
    if (!load_ihex (in, page_map))
      return error_out (2);
  }

  if (!quiet)
  {
    const char *hint = "unknown";
    if (flash_base == 0x800000)
      hint = "app-flash";
    if (flash_base == 0x840000)
      hint = "boot-flash";
    printf (
      "Using: clk=gpio%d, data=gpio%d, delay: %dus, baseaddr: 0x%08x (%s)\n",
      clk_pin, data_pin, pdi_delay_us, flash_base, hint);
    printf ("Actions: ");
    if (dump_mem)
      printf ("dump-memory ");
    if (chip_erase)
      printf ("chip-erase ");
    if (fname)
      printf ("program:%s ", fname);
    printf ("\n");
  }

  // Okay, all the slow stuff is done, now we're entering PDI programming mode

  if (!pdi_init (clk_pin, data_pin, pdi_delay_us))
    return error_out (3);

  // from here on we need to bail_out(n) instead of error_out, so we pdi_close()
  if (!pdi_open () || !nvm_wait_enabled ())
    bail_out (4);

  if (dump_mem)
  {
    uint32_t keep_addr = dump_addr;
    uint32_t keep_len = dump_len;
    while (dump_len)
    {
      uint16_t offs = dump_addr % 512;
      uint16_t len = (dump_len > 512) ? 512 : dump_len;
      uint32_t pgaddr = dump_addr - offs;
      auto &pg = page_map[pgaddr];
      pg.addr = pgaddr;
      if (!nvm_read (flash_base + pgaddr, pg.data, 512))
        bail_out (10);

      dump_len -= len;
      dump_addr += len;
    }
    dump_addr = keep_addr;
    dump_len = keep_len;
  }

  if (chip_erase)
  {
    if (!nvm_chip_erase ())
    {
      set_errinfo ("failed to perform chip erase", -1);
      bail_out (11);
    }
  }

  if (fname)
  {
    for (auto &i : page_map)
    {
      auto &p = i.second;
      if (!nvm_rewrite_page (flash_base + p.addr, p.data, sizeof (p.data)))
      {
        set_errinfo ("failed to rewrite page at address", p.addr);
        bail_out (12);
      }
    }
  }

out:
  pdi_close ();

  // ...and we're back to being allowed to go a bit slower *phew*

  if (!ret && dump_mem)
  {
    uint16_t start_offs = dump_addr % 512;
    uint16_t end_offs = (dump_addr + dump_len) % 512;
    auto p = page_map.begin ();
    for (size_t i = 0; i < page_map.size (); ++i, ++p)
    {
      uint16_t offs = 0;
      uint16_t end = 512;
      if (i == 0)
        offs = start_offs;
      if (i == (page_map.size () -1))
        end = end_offs;

      dump (p->first + offs, p->second.data + offs, end - offs);
    }
  }

  if (ret)
    return error_out (ret);
  else
  {
    printf ("ok\n");
    return 0;
  }
}
