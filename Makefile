default: pdi


SRCS=$(addprefix src/, \
  main.c \
  pdi.c \
  nvm.c \
)

CFLAGS+=-O3 -g -std=c99 -Wall -Wextra -Isrc
LDFLAGS+=-lbcm2835

pdi: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	-rm -f pdi
