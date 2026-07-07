include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC) -I$(COMPAT_INC)
LIBS     = $(COMPAT_LIB) $(NIO_ALIB)

TARGET = compat_test
SRCS   = $(COMPAT)/test/compat_test.c

APP_NAME         = compat_test
APP_BINARY       = $(TARGET)
# AppStoreRead on the FileDevice — the appkey read-back is the LAST wire
# command compat_test issues, so matching it proves the HTTP test and the
# appkey write→read round-trip all made it onto the bus.
EMU_PASS_PATTERN = dev=0xFE cmd=0x21
EMU_TIMEOUT      = 90

all: $(TARGET)

include ../../emu/template/emu.mk

$(TARGET): $(SRCS) $(COMPAT_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LIBS)

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
