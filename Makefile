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

# Disable make's built-in rules
MAKEFLAGS += -r

# this includes the framework configuration
-include .config

# decide whether we are building or dooing something other like cleaning or configuring
ifeq ($(filter $(MAKECMDGOALS), clean distclean config), )
  # check whether a .config file has been found
  ifeq ($(filter .config, $(MAKEFILE_LIST)), )
    $(error "Cannot make the target '$(MAKECMDGOALS)' without configuring the application. Please run make config to do this.")
  endif
endif

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
	! [ -d /tftpboot ] && true || cp $(OUT)$(TARGET_SUFFIX) /tftpboot/$(OUT)
	@mkdir -p www/cgi-bin
	@cp $(OUT)$(TARGET_SUFFIX) www/cgi-bin/$(OUT)
	tar cfz targetFiles/www.tar.gz -C www .
	! [ -d /tftpboot ] && true || cp targetFiles/www.tar.gz /tftpboot

targetdbg: $(SOURCES) inc/*.h lib/libosc_target.a $(WEB_FILES)
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGETDBG_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	@echo "Target CGI done."
	! [ -d /tftpboot ] && true || cp $(OUT)$(TARGET_SUFFIX) /tftpboot/$(OUT)	

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
	cp www/* /var/www -r


# Target to explicitly start the configuration process
.PHONY : config
config :
	@ ./configure
	@ $(MAKE) --no-print-directory get

# Set symlinks to the framework
.PHONY : get
get :
	@ rm -rf inc lib
	@ ln -s $(CONFIG_FRAMEWORK)/staging/inc ./inc
	@ ln -s $(CONFIG_FRAMEWORK)/staging/lib ./lib
	@ echo "Configured Oscar framework."

# deploying to the device
.PHONY : deploy
deploy : $(OUT)$(TARGET_SUFFIX)
	@ scp -rp targetFiles/www.tar.gz root@$(CONFIG_TARGET_IP):/mnt/app/ || echo -n ""
	@ echo "Application deployed."

# Cleanup
.PHONY : clean
clean :	
	rm -f $(OUT)$(HOST_SUFFIX) $(OUT)$(TARGET_SUFFIX)
	rm -f *.o *.gdb
	@ echo "Directory cleaned"

# Cleans everything not intended for source distribution
.PHONY : distclean
distclean : clean
	rm -f .config
	rm -rf inc lib

