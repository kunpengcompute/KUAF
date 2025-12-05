# Library directories
LIBDIR := -L/usr/local/kuaf/lib -L/usr/local/openssl1.1-debug/lib

# The flags
CFLAGS    := -g -lz -Wall
LDFLAGS   := $(LIBDIR)

# Include directories
INCDIR := -I/usr/local/openssl1.1-debug/include

# Libraries
LIBS := -lkae -lcrypto -lssl -ldl -lpthread

# Runtime library path
RPATH := -Wl,-rpath=/usr/local/kuaf/lib:/usr/local/openssl1.1-debug/lib

all: rsa_func sm2_func

# RSA performance test
rsa_func: rsa_func_test.c
	g++ $(CFLAGS) $(LDFLAGS) $(INCDIR) $(RPATH) -o rsa_func rsa_func_test.c $(LIBS)

# SM2 performance test  
sm2_func: sm2_func_test.c
	g++ $(CFLAGS) $(LDFLAGS) $(INCDIR) $(RPATH) -o sm2_func sm2_func_test.c $(LIBS)

.PHONY : clean
clean :
	-rm -f rsa_func sm2_func