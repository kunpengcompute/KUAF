# Library directories
LIBDIR := -L/usr/local/kuaf/lib -L/usr/local/openssl1.1-debug/lib

# The flags
CFLAGS    := -g -lz -Wall
LDFLAGS   := $(LIBDIR)

# Include directories
INCDIR := -I/usr/local/openssl1.1-debug/include

# Libraries
LIBS := -lcrypto -lssl -ldl -lpthread

# Runtime library path
RPATH := -Wl,-rpath=/usr/local/kuaf/lib:/usr/local/openssl1.1-debug/lib

all: rsa_perf sm2_perf

# RSA performance test
rsa_perf: rsa_kae_test.c
	gcc $(CFLAGS) $(LDFLAGS) $(INCDIR) $(RPATH) -o rsa_perf rsa_kae_test.c $(LIBS)

# SM2 performance test  
sm2_perf: sm2_kae_test.c
	g++ -O3 -march=native -o sm2_perf sm2_kae_test.c $(LIBS)

.PHONY : clean
clean :
	-rm -f rsa_perf sm2_perf