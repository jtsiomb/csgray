src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = csgray

sys := $(shell uname -s | sed 's/MINGW32.*/mingw/')

CFLAGS = -pedantic -Wall -g -O3 -fopenmp
LDFLAGS = -lm -ltreestore -lgomp

ifeq ($(sys), mingw)
	bin = csgray.exe
endif

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)
