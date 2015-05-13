default: pdi

OBJS=\
  main.o \
  pdi.o \
  nvm.o \
  ihex.o \
  errinfo.o \

VPATH=src

CFLAGS+=-O3 -g -std=c99 -Wall -Wextra -Isrc
CXXFLAGS+=-O3 -g -std=c++0x -Wall -Wextra -Isrc
LDFLAGS+=-lbcm2835

pdi: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	-rm -f pdi *.o
