
INCDIR += -I /usr/local/kaezip/include/
# Include Libs.
LIBDIR := -L/usr/local/kaezip/lib

# The flags
CFLAGS    := -g -lz -Wall
LDFLAGS   := $(LIBDIR)

all: kuafzip_perf zip_perf
objects = kuafzip_perf.c

kuafzip_perf : $(objects)
	gcc $(CFLAGS) $(LDFLAGS) $(INCDIR) -Wl,-rpath=/usr/local/kaezip/lib -o kuafzip_perf $(objects)
zip_perf : $(objects)
	gcc $(CFLAGS) -o zip_perf $(objects)

.PHONY : clean
clean :
	-rm -f kuafzip_perf zip_perf
