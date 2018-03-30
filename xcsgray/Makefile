rootdir = ..
src = $(wildcard src/*.c) \
	  $(filter-out $(rootdir)/src/main.c, $(wildcard $(rootdir)/src/*.c))
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = xcsgray

sys := $(shell uname -s | sed 's/MINGW32.*/mingw/')

warn = -pedantic -Wall
dbg = -g
opt = -O3 -ffast-math
inc = -Isrc -I$(rootdir)/src

CFLAGS = $(warn) $(dbg) $(opt) $(inc) -fopenmp
LDFLAGS = $(libgl) -lm -ltreestore -lresman -lpthread -lgomp

ifeq ($(sys), mingw)
	src += $(wildcard src/win32/*.c)
	bin = xcsgray.exe
	libgl = -lopengl32 -lfreeglut
	LDFLAGS += -lwinmm
else
	src += $(wildcard src/unix/*.c)
	libgl = -lGL -lglut
endif

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
