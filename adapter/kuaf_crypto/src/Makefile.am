# Makefile for building /usr/local/kuaf/lib/kae.so
DEBUG_CFLAGS = -g -O0 -fno-omit-frame-pointer -Wall -Wextra
DEBUG_LDFLAGS = -g
HARDEN_CFLAGS = -fPIC -fstack-protector -D_FORTIFY_SOURCE=2 -ftrapv -fvisibility=hidden
HARDEN_LDFLAGS = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -s

DEPSHEADDIR = /usr/local/include/

# Dependency paths for the KAF library
WD_CFLAGS = -I/usr/local/kuaf/include
WD_CFLAGS += -I$(DEPSHEADDIR)/uadk/v1
WD_CFLAGS += -I$(DEPSHEADDIR)/uadk
WD_CFLAGS += -I/usr/local/lib
WD_CFLAGS += -I/usr/local/openssl1.1-debug/include

# Linker options
AM_LDFLAGS = $(DEBUG_LDFLAGS)
AM_LDFLAGS += -L/usr/local/kuaf/lib
AM_LDFLAGS += -L/usr/local/lib
AM_LDFLAGS += -L/usr/local/openssl1.1-debug/lib
AM_LDFLAGS += -L/usr/local/lib/engines-1.1
AM_LDFLAGS += -Wl,-rpath,/usr/local/kuaf/lib
AM_LDFLAGS += $(HARDEN_LDFLAGS)

# KAF library configuration - generate /usr/local/kuaf/lib/kae.so
lib_LTLIBRARIES = libkae.la

libkae_la_SOURCES = \
    e_kuaf.c \
    sm2_hard.c \
    rsa_hard.c
    
libkae_la_CFLAGS = $(WD_CFLAGS) -DKAF_LIBRARY $(DEBUG_CFLAGS) $(HARDEN_CFLAGS)

libkae_la_LDFLAGS = -avoid-version -shared $(AM_LDFLAGS)

# Library dependencies for the KAF engine
libkae_la_LIBADD = -l:kae.so -lkuaf -lkaezip -lwd -lwd_crypto -lcrypto -lnuma -ldl -lpthread

if COVER
libkae_la_CFLAGS += -fprofile-arcs -ftest-coverage -g -O0
libkae_la_LIBADD += -lgcov
endif
# Install to the KAF directory
kafdir = /usr/local/kuaf/lib
