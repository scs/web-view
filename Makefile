############################################################################
#
# Makefile for the Frame Capture Demo CGI
#
# 
############################################################################

# The executable name is suffix depending on the target
OUT = fcd.cgi
HOST_SUFFIX = _host
TARGET_SUFFIX = _target

# Host-Compiler executables and flags
HOST_CC = gcc 
HOST_CFLAGS = $(HOST_FEATURES) -Wall -pedantic -DOSC_HOST -g
HOST_LDFLAGS = -lm

# Cross-Compiler executables and flags
TARGET_CC = bfin-uclinux-gcc 
TARGET_CFLAGS = -Wall -pedantic -O2 -DOSC_TARGET
TARGETDBG_CFLAGS = -Wall -pedantic -ggdb3 -DOSC_TARGET
TARGET_LDFLAGS = -Wl,-elf2flt="-s 1048510"

# Source files of the application
SOURCES = cgi_fcd.c

# Files served by the web server
WEB_FILES = www/*

#Files needed for emulation on host
EMU_FILES = hostImgs.txt imgCapture.bmp

# Default target
all: $(OUT)

$(OUT): target host

# Compiles the executable
target: $(SOURCES) inc/*.h lib/libosc_target.a $(WEB_FILES)
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGET_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	@echo "Target CGI done."
	cp $(OUT)$(TARGET_SUFFIX) /tftpboot/$(OUT)
	@mkdir -p www/cgi-bin
	@cp $(OUT)$(TARGET_SUFFIX) www/cgi-bin/$(OUT)
	tar cfz targetFiles/www.tar.gz -C www .
	cp targetFiles/www.tar.gz /tftpboot

targetdbg: $(SOURCES) inc/*.h lib/libosc_target.a $(WEB_FILES)
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGETDBG_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	@echo "Target CGI done."
	cp $(OUT)$(TARGET_SUFFIX) /tftpboot/$(OUT)	
	
host: $(SOURCES) inc/*.h lib/libosc_host.a  $(WEB_FILES) $(EMU_FILES)
	@echo "Compiling for host.."
	$(HOST_CC) $(SOURCES) lib/libosc_host.a $(HOST_CFLAGS) \
	$(HOST_LDFLAGS) -o $(OUT)$(HOST_SUFFIX)
	@echo "Host executable done."
	@cp $(OUT)$(HOST_SUFFIX) $(OUT)
	@cp $(OUT) www/cgi-bin
	cp www/cgi-bin/* /var/www/cgi-bin/
	cp $(EMU_FILES) /var/www/cgi-bin/
	@chmod a+x /var/www/cgi-bin/$(OUT)
	cp  www/* /var/www -r

get:
	cp ../oscar/staging/* . -r
	@echo "Framework fetched."
	
# Cleanup
clean:	
	rm -f $(OUT)$(HOST_SUFFIX) $(OUT)$(TARGET_SUFFIX) $(OUT)
	rm -f *.o *.cgi *.gdb
	rm www/cgi-bin/$(OUT)
	rm www.tar
	@echo "Directory cleaned"

	
