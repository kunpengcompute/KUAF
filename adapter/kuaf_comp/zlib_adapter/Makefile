#
# Date:   2025/5/10

# Description:
# compile for Zlib Adapter
#
# Usage:
#   $ make           compile and link the program.
#   $ make rebuild   rebuild the program. The same as make clean && make all.
#   $ make clean     clean the objective, dependent and executable files.
#   $ make install   copy to the system directory.
#   $ make uninstall clean the executable file from the system directory.
#==============================================================================

WORK_PATH := .
INSTALL_PATH := /usr/local/kuaf

CC=gcc

LIBNAME := libz.so
VERSION = 1.2.11
TARGET = ${LIBNAME}.${VERSION}
SOFTLINK = libz.so

ifndef SILENCE
	SILENCE = @
endif

# Src
SRCDIRS   += ${WORK_PATH}/src
SRCEXTS   := .c # C program

# Include
INCDIR += -I $(WORK_PATH)/include
INCDIR += -I ${WORK_PATH}/../../../open_source/zlib
INCDIR += -I /usr/local/kaezip/include
INCDIR += -I /usr/local/kuaf/include

# Include Libs.
LIBDIR += -L/usr/local/kuaf/lib
LIBS := -lz_sw -lkuaf -lkaezip
LIBS += -lc_nonshared

# The flags
CFLAGS    := -Wall -Werror -fstack-protector-all -fPIC -D_GNU_SOURCE -shared -fstack-protector-strong -ftrapv -D_FORTIFY_SOURCE=2 -O2
LDFLAGS   := $(LIBDIR)
LDFLAGS   += $(LIBS) 
LDFLAGS   += -Wl,-z,relro,-z,now,-z,noexecstack -s  #safe link option

# The command used to delete file.
RM        = rm -f
LN        = ln -sf

SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))

OBJS    = $(foreach x,$(SRCEXTS), \
      $(patsubst %$(x), %.o, $(filter %$(x),$(SOURCES))))

.PHONY : all objs clean cleanall rebuild

all : $(TARGET)

# Rules for creating the dependency files (.d).
%.d : %.c
	$(CC) -MM -MD $(CFLAGS)  $<

# Rules for producing the objects.
objs : $(OBJS)
%.o : %.c
	@echo compiling $(notdir $<)
	$(SILENCE) $(CC) -c $(CFLAGS) $(INCDIR) $(LDFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	@echo Linking $@
	$(SILENCE) $(CC) $(CFLAGS) -Wl,-soname,libz.so.1,--version-script,zlib.map $(INCDIR) -o ./$(TARGET) $(OBJS) $(LDFLAGS)
rebuild: clean all

clean :
	@-$(RM) *.d *.a *.so *.symbol $(TARGET)
	@-$(RM)
	@find ${WORK_PATH} -name '*.o' -exec $(RM) {} \;
	@echo all clean

install :
	mkdir -p $(INSTALL_PATH)/include
	mkdir -p $(INSTALL_PATH)/lib
	install -m 755 $(TARGET) $(INSTALL_PATH)/lib
	$(LN) $(INSTALL_PATH)/lib/$(TARGET)  $(INSTALL_PATH)/lib/$(SOFTLINK)
	$(LN) $(INSTALL_PATH)/lib/$(TARGET)  $(INSTALL_PATH)/lib/$(SOFTLINK).1