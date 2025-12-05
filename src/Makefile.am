VERSION = 1:0:0
ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects
CFLAGS =-g 
CFLAGS +=-I/usr/local/include/uadk/v1 -I/usr/local/kaezip/include
CFLAGS +=-I$(top_srcdir)/huawei_secure_c/include
CFLAGS +=-ftrapv -fPIC -fPIE -pie -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O2

CFLAGS +=-L$(top_srcdir)/huawei_secure_c/lib
CFLAGS +=-L/usr/local/kaezip/lib -L/usr/local/kuaf/lib

CFLAGS +=-Wl,-z,now -Wl,-z,relro -Wl,--disable-new-dtags

libkuaf_la_LIBADD = -lpthread -lnuma -lwd -lwd_comp -lkaezip -lz_sw -lsecurec
libkuaf_la_LDFLAGS = -module -version-number $(VERSION)
lib_LTLIBRARIES=libkuaf.la

if COVER
CFLAGS += -fprofile-arcs -ftest-coverage -g
libkuaf_la_LIBADD += -lgcov
endif

libkuaf_includedir = /usr/local/kuaf/include
libkuaf_include_HEADERS = core/scheduler/kuaf_sc.h \
						core/algorithm/kuaf_alg.h

kuaf_crypto_includedir = $(libkuaf_includedir)/crypto
kuaf_crypto_include_HEADERS = core/algorithm/crypto/kuaf_rsa.h

kuaf_comp_includedir = $(libkuaf_includedir)/comp
kuaf_comp_include_HEADERS = core/algorithm/comp/kuaf_zlib.h

libkuaf_la_SOURCES = common/kuaf_config.c \
		common/kuaf_log.c    \
		core/device/kuaf_dev.c \
		core/scheduler/kuaf_sc.c \
		core/algorithm/kuaf_alg.c \
		core/strategy/kuaf_shm.c \
		core/strategy/bandwidth/kuaf_bw.c \
		core/strategy/ratio/kuaf_ratio.c \
		core/strategy/parallel/kuaf_paral.c \
		core/process/kuaf_comp.c \
		core/process/kuaf_crypto.c \
		core/process/kuaf_process.c