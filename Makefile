# Makefile for Applications using the Oscar Framework
# Copyright (C) 2008 Supercomputing Systems AG
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty. This file is offered as-is,
# without any warranty.

# Disable make's built-in rules.
MAKE += -RL --no-print-directory
SHELL := $(shell which bash)

# Include the file generated by te configuration process.
-include .config
ifeq '$(filter .config, $(MAKEFILE_LIST))' ''
$(error Please configure the application using './configure' prior to compilation.)
endif

##--------------------------------------------------------------------------
#  Application specific part
##--------------------------------------------------------------------------

# Name for the application to produce.
APP_NAME := web-view

# Name of the directory the application should be deployed to on the target.
DEPLOY_DIR := /mnt/app/

# Binary executables to generate (make sure it's the same as in the run.sh script).
PRODUCTS := app
# subproduct folders: for each folder, make will be called
SUB_PRODUCTS := cgi

# Listings of source files for the different executables.
SOURCES_app := $(wildcard *.cpp) $(wildcard *.c)

# Listings of source files for the different applications.
SOURCES_$(APP_NAME) := $(wildcard *.cpp) $(wildcard *.c)

# statically linked libraries
LIBS_host := oscar/library/libosc_host
LIBS_target := oscar/library/libosc_target

##--------------------------------------------------------------------------
#  Common part
##--------------------------------------------------------------------------

# Generic flags for the C/CPP compiler.
CFLAGS := -c -Wall -Ioscar/include

ifeq '$(CONFIG_USE_OPENCV)' 'y'
CFLAGS += -I$(CONFIG_OPENCV_PATH)/include/opencv
OPENCV_LIBS_host := -L$(CONFIG_OPENCV_PATH)/build-host/src/.libs -lcvaux -lcv -lcxcore -lpthread -ldl -lrt -lz
OPENCV_LIBS_target := -L$(CONFIG_OPENCV_PATH)/build-target/src/.libs -lbfdsp -lcvaux -lcv -lcxcore -lpthread -lrt
else
OPENCV_LIBS_host := 
OPENCV_LIBS_target := 
endif

#Oscar CC
ifeq '$(CONFIG_USE_OSCAR_CC)' 'y'

IS_ABSOLUTE_PATH := $(shell expr "$(CONFIG_FRAMEWORK_PATH)" : '/')
ifeq '$(IS_ABSOLUTE_PATH)' '0'
ABS_OSCAR_PATH := $(shell pwd)/$(CONFIG_FRAMEWORK_PATH)
else
ABS_OSCAR_PATH := $(CONFIG_FRAMEWORK_PATH)
endif

-include $(CONFIG_OSCAR_CC_PATH)/.config
#M_INC := -I$(CONFIG_OSCAR_CC_PATH)/modules/$mod/include
CFLAGS += $(addsuffix /include, $(addprefix -I$(CONFIG_OSCAR_CC_PATH)/modules/, $(CONFIG_CC_MODULES)))
OSC_CC_LIBS_host := $(addsuffix _host, $(addprefix -losc-cc_, $(CONFIG_CC_MODULES)))
OSC_CC_LIBS_target := $(addsuffix _target, $(addprefix -losc-cc_, $(CONFIG_CC_MODULES)))
OSC_CC_LIBS_INC := -L$(CONFIG_OSCAR_CC_PATH)/library
else
OSC_CC_LIBS_host := 
OSC_CC_LIBS_target := 
OSC_CC_LIBS_INC := 
endif # '$(CONFIG_USE_OSCAR_CC)' 'y'



ifeq '$(CONFIG_USE_GPP_COMPILER)' 'y'
GCC := g++
else
CFLAGS += -std=gnu99
GCC := gcc
endif

CC_host := $(GCC) $(CFLAGS) -DOSC_HOST -D'APP_NAME="$(APP_NAME)"'
CC_target := bfin-uclinux-$(GCC) $(CFLAGS) -DOSC_TARGET -D'APP_NAME="$(APP_NAME)"'
LD_host := $(GCC) -fPIC
LD_target := bfin-uclinux-$(GCC) -elf2flt="-s 1048576"

ifeq '$(CONFIG_ENABLE_DEBUG)' 'y'
CC_host +=  -g
CC_target +=  -ggdb3
else
CC_host +=  -O2
CC_target +=  -O2
endif
ifeq '$(CONFIG_ENABLE_SIMULATION)' 'y'
CC_host += -DOSC_SIM
CC_target += -DOSC_SIM
endif

ifeq '$(CONFIG_PRIVATE_FRAMEWORK)' 'y'
CONFIG_FRAMEWORK_PATH := ./oscar
endif

APPS := $(patsubst SOURCES_%, %, $(filter SOURCES_%, $(.VARIABLES)))

ifeq '$(CONFIG_ENABLE_SIMULATION)' 'y'
LIBS_target := $(LIBS_target)_sim
endif
ifeq '$(CONFIG_ENABLE_DEBUG)' 'y'
LIBS_host := $(addsuffix _dbg, $(LIBS_host))
LIBS_target := $(addsuffix _dbg, $(LIBS_target))
OSC_CC_LIBS_host := $(addsuffix _dbg, $(OSC_CC_LIBS_host))
OSC_CC_LIBS_target := $(addsuffix _dbg, $(OSC_CC_LIBS_target))
endif
LIBS_host := $(addsuffix .a, $(LIBS_host))
LIBS_target := $(addsuffix .a, $(LIBS_target))

BINARIES := $(addsuffix _host, $(PRODUCTS)) $(addsuffix _target, $(PRODUCTS))

.PHONY: all clean host target install deploy run reconfigure opencv
all: $(BINARIES)
	$(foreach i, $(SUB_PRODUCTS), make -C $i APP_NAME=$(APP_NAME))
host target: %: $(addsuffix _%, $(PRODUCTS))

opencv:
ifeq '$(CONFIG_USE_OPENCV)' 'y'
	cd $(CONFIG_OPENCV_PATH) && ./do-build
endif

deploy: $(APP_NAME).app
	tar c $< | ssh root@$(CONFIG_TARGET_IP) 'rm -rf $< && tar x -C $(DEPLOY_DIR)' || true

run:
	ssh root@$(CONFIG_TARGET_IP) $(DEPLOY_DIR)$(APP_NAME).app/run.sh || true

install: cgi/cgi_host
#install only if folder exists
ifneq "$(wildcard www )" ""
	cp -RL www/* /var/www
	cp $< /var/www/cgi-bin/cgi
	chmod -Rf a+rX /var/www/ || true
endif

reconfigure:
ifeq '$(CONFIG_USE_OSCAR_CC)' 'y'
	cd $(CONFIG_OSCAR_CC_PATH) && ./configure CONFIG_CC_PRIVATE_FRAMEWORK=n CONFIG_CC_FRAMEWORK_PATH='$(ABS_OSCAR_PATH)'
endif

ifeq '$(CONFIG_PRIVATE_FRAMEWORK)' 'n'
	@ ! [ -e "oscar" ] || [ -h "oscar" ] && ln -sfn $(CONFIG_FRAMEWORK_PATH) oscar || echo "The symlink to the lgx module could not be created as the file ./lgx already exists and is something other than a symlink. Pleas remove it and run 'make reconfigure' to create the symlink."
endif
	@ ! [ -d "oscar" ] || $(MAKE) -C oscar config
oscar/%:
	$(MAKE) -C oscar $*

osc-cc:
ifeq '$(CONFIG_USE_OSCAR_CC)' 'y'
	$(MAKE) -C $(CONFIG_OSCAR_CC_PATH) $*
endif

# Including depency files and optional local Makefile.
-include build/*.d

# Build targets.
build/%_host.o: $(filter-out %.d, $(MAKEFILE_LIST))
	@ mkdir -p $(dir $@)
	$(CC_host) -MD $(filter $*.c $*.cpp,$($(addsuffix $(PRODUCTS), SOURCES_))) -o $@
	@ grep -oE '[^ \\]+' < $(@:.o=.d) | sed -r '/:$$/d; s|^.*$$|$@: \0\n\0:|' > $(@:.o=.d~) && mv -f $(@:.o=.d){~,}
build/%_target.o: $(filter-out %.d, $(MAKEFILE_LIST))
	@ mkdir -p $(dir $@)
	$(CC_target) -MD $(filter $*.c $*.cpp,$($(addsuffix $(PRODUCTS), SOURCES_))) -o $@
	@ grep -oE '[^ \\]+' < $(@:.o=.d) | sed -r '/:$$/d; s|^.*$$|$@: \0\n\0:|' > $(@:.o=.d~) && mv -f $(@:.o=.d){~,}

# Link targets.
define LINK
$(1)_host: $(patsubst %.cpp, build/%_host.o, $(patsubst %.c, build/%_host.o, $(SOURCES_$(1)))) $(LIBS_host)
	$(LD_host) -o $$@ $$^ $(OSC_CC_LIBS_INC) -lm $(OSC_CC_LIBS_host) $(OPENCV_LIBS_host)
$(1)_target: $(patsubst %.cpp, build/%_target.o, $(patsubst %.c, build/%_target.o, $(SOURCES_$(1)))) $(LIBS_target)
	$(LD_target) -o $$@ $$^ $(OSC_CC_LIBS_INC) -lm -lbfdsp $(OSC_CC_LIBS_target) $(OPENCV_LIBS_target)
endef
$(foreach i, $(PRODUCTS), $(eval $(call LINK,$i)))

.PHONY: $(APP_NAME).app
$(APP_NAME).app: $(addsuffix _target, $(PRODUCTS))
	rm -rf $@
	cp -rL app $@
ifneq "$(wildcard www )" ""
	tar c -h -C www . | gzip > $@/www.tar.gz
endif

# Cleans the module.
clean:
	rm -rf build *.gdb $(BINARIES) $(APP_NAME).app
	$(foreach i, $(SUB_PRODUCTS), make -C $i clean)
