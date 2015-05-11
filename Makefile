default: pdi


SRCS=$(addprefix src/, \
  main.c \
  pdi.c \
)

CFLAGS+=-O2 -g -std=c99 -Wall -Wextra -Isrc
LDFLAGS+=-lbcm2835

pdi: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	-rm -f pdi
