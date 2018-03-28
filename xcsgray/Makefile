rootdir = ..
src = $(wildcard src/*.c) $(wildcard src/unix/*.c) \
	  $(filter-out $(rootdir)/src/main.c, $(wildcard $(rootdir)/src/*.c))
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = xcsgray

sys := $(shell uname -s | sed 's/MINGW32.*/mingw/')

warn = -pedantic -Wall
dbg = -g
#opt = -O3 -ffast-math
inc = -Isrc -I$(rootdir)/src

CFLAGS = $(warn) $(dbg) $(opt) $(inc) -fopenmp
LDFLAGS = $(libgl_$(sys)) -lm -ltreestore -lresman -lgomp

libgl_Linux = -lGL -lglut
libgl_Darwin = -framework OpenGL -framework GLUT
libgl_mingw = -lopengl32 -lglut32

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
