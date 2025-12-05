AC_PREREQ([2.69])
AC_INIT([uadk_engine], [1.1], [dev@example.com])
AC_CONFIG_SRCDIR([src/e_kuaf.c])
AM_INIT_AUTOMAKE([subdir-objects foreign 1.15])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# 工具链检查
AC_PROG_CC
AC_PROG_LIBTOOL
LT_INIT([dlopen])
AM_PROG_AR

# 定义命令行参数
AC_ARG_ENABLE([kae], [AS_HELP_STRING([--enable-kae], [Enable kae support])], [enable_kae="yes"], [enable_kae="no"])
AC_ARG_ENABLE([coverage], [AS_HELP_STRING([--enable-coverage], [Enable coverage support])], [enable_coverage="yes"], [enable_coverage="no"])
AC_ARG_ENABLE(engine, AS_HELP_STRING([--enable-engine],[Enable engine in OpenSSL 3.0]))

# 设置条件变量
AM_CONDITIONAL([WD_KAE], [test "$enable_kae" = "yes"])
AM_CONDITIONAL([WD_ENGINE], [test "$enable_engine" = "yes"])
AM_CONDITIONAL([COVER], [test "$enable_coverage" = "yes"])

# 通过命令传入OpenSSL的安装路径，优先使用此路径
AC_ARG_WITH([openssl_install_dir],
    [AS_HELP_STRING([--with-openssl_install_dir=DIR],
        [Path to where OpenSSL is installed. The Engine will be installed to the lib/engines subdirectory of this path.])],
    [openssl_dir="$withval"],
    [openssl_dir=""])
AC_ARG_ENABLE([coverage], 
    [AS_HELP_STRING([--enable-cover], [Enable code coverage support with gcov])],
    [enable_coverage="$enableval"],
    [enable_coverage="no"])
AC_SUBST([with_openssl_install_dir], [$openssl_dir])

# OpenSSL路径检测逻辑
AS_IF([test "x$openssl_dir" != "x"], [
    # 使用用户指定的OpenSSL路径
    AC_MSG_NOTICE([Using user-specified OpenSSL path: $openssl_dir])
    
    # 检查头文件路径
    openssl_include="$openssl_dir/include"
    AS_IF([test -f "$openssl_include/openssl/opensslv.h"], [
        # 检测OpenSSL版本
        AC_MSG_CHECKING([OpenSSL version at $openssl_include])
        AS_IF([grep -q "define OPENSSL_VERSION_NUMBER  0x1010" "$openssl_include/openssl/opensslv.h"], [
            AC_MSG_RESULT([OpenSSL 1.1.1])
            AC_SUBST([CFLAGS], ["-Wl,-z,relro,-z,now -fstack-protector-strong"])
        ], [
            AC_MSG_RESULT([OpenSSL 3.0+])
            AC_SUBST([cflags_openssl_3], ["-DKAE_OPENSSL_3 -DOPENSSL_SUPPRESS_DEPRECATED"])
        ])
        
        AC_SUBST([includes_openssl], ["-I$openssl_include"])
    ], [
        AC_MSG_ERROR([OpenSSL headers not found in $openssl_include/openssl/])
    ])
], [
    # 使用系统默认的OpenSSL
    AC_PATH_TOOL([PKGCONFIG], [pkg-config])
    AS_IF([test "x$PKGCONFIG" = "x"], [AC_MSG_ERROR([pkg-config not found.])])
    
    # 获取系统OpenSSL信息
    AS_IF([$PKGCONFIG --exists libcrypto], [
        openssl_include=`$PKGCONFIG --variable=includedir libcrypto`
        AC_MSG_CHECKING([system OpenSSL version])
        
        AS_IF([grep -q "define OPENSSL_VERSION_NUMBER  0x1010" "$openssl_include/openssl/opensslv.h" 2>/dev/null], [
            AC_MSG_RESULT([OpenSSL 1.1.1])
        ], [
            AC_MSG_RESULT([OpenSSL 3.0+])
            AC_SUBST([cflags_openssl_3], ["-DKAE_OPENSSL_3 -DOPENSSL_SUPPRESS_DEPRECATED"])
        ])
        
        AC_SUBST([includes_openssl], ["-I$openssl_include"])
        AC_MSG_NOTICE([Using system OpenSSL from: $openssl_include])
    ], [
        AC_MSG_ERROR([libcrypto not found by pkg-config])
    ])
])

AS_IF([test "x$with_openssl_install_dir" != "x"], [
    # 检查指定路径的libcrypto
    AC_CHECK_LIB([crypto], [OPENSSL_init], [
        openssl_lib_dir="$with_openssl_install_dir/lib"
        AC_MSG_NOTICE([Using OpenSSL library from: $openssl_lib_dir])
        LIBS="-L$openssl_lib_dir -lcrypto $LIBS"
        LDFLAGS="-L$openssl_lib_dir -Wl,-rpath,$openssl_lib_dir $LDFLAGS"
    ], [
        AC_MSG_ERROR([OpenSSL library not found in $with_openssl_install_dir/lib])
    ], [-L$with_openssl_install_dir/lib])
])
# 检查OpenSSL头文件
AC_CHECK_HEADERS([openssl/engine.h], [], [
    AC_MSG_ERROR([OpenSSL engine.h header not found. Please ensure OpenSSL development packages are installed.])
])

# 检查WD库
PKG_CHECK_MODULES([WD], [libwd libwd_crypto], 
    [with_wd=yes], 
    [with_wd=no]
)
AM_CONDITIONAL([HAVE_WD], [test "$with_wd" != "no"])

# 检查OpenSSL库版本
PKG_CHECK_MODULES([libcrypto], [libcrypto < 3.0 libcrypto >= 1.1],
    [with_crypto=yes], 
    [with_crypto=no]
)
AM_CONDITIONAL([HAVE_CRYPTO], [test "$with_crypto" != "no"])

PKG_CHECK_MODULES([libcrypto], [libcrypto >= 3.0],
    [with_crypto3=yes], 
    [with_crypto3=no]
)
AM_CONDITIONAL([HAVE_CRYPTO3], [test "$with_crypto3" != "no"])

# 输出配置信息
AC_MSG_NOTICE([OpenSSL include flags: $includes_openssl])
AC_MSG_NOTICE([OpenSSL install dir: $openssl_dir])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT