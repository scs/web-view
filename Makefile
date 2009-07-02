# Makefile for simple modules in the Oscar Framework.
# Copyright (C) 2008 Supercomputing Systems AG
# 
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2.1 of the License, or (at your option)
# any later version.
# 
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
# 
# You should have received a copy of the GNU Lesser General Public License along
# with this library; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Disable make's built-in rules.
MAKE += -RL --no-print-directory
SHELL := $(shell which bash)

# Generic flags for the C compiler.
CFLAGS := -c -std=gnu99 -Wall -Werror -Ioscar/include

# Include the file generated by te configuration process.
-include .config
ifeq '$(filter .config, $(MAKEFILE_LIST))' ''
$(error Please configure the application using './configure' prior to compilation.)
endif

# Name for the application to produce.
APP_NAME := web-view

# Name of the directory the application should be deployed to on the target.
DEPLOY_DIR := /mnt/app/

# Binary executables to generate.
PRODUCTS := app cgi/cgi

# Listings of source files for the different executables.
SOURCES_app := $(wildcard *.c)
SOURCES_cgi/cgi := $(wildcard cgi/*.c)

ifeq '$(CONFIG_ENABLE_DEBUG)' 'y'
CC_host := gcc $(CFLAGS) -DOSC_HOST -g
CC_target := bfin-uclinux-gcc $(CFLAGS) -DOSC_TARGET -ggdb3
else
CC_host := gcc $(CFLAGS) -DOSC_HOST -O2
CC_target := bfin-uclinux-gcc $(CFLAGS) -DOSC_TARGET -O2
endif
ifeq '$(CONFIG_ENABLE_SIMULATION)' 'y'
CC_host += -DOSC_SIM
CC_target += -DOSC_SIM
endif
LD_host := gcc -fPIC
LD_target := bfin-uclinux-gcc -elf2flt="-s 1048576"

# Listings of source files for the different applications.
SOURCES_$(APP_NAME) := $(wildcard *.c)
SOURCES_cgi/template.cgi := $(wildcard cgi/*.c)

APPS := $(patsubst SOURCES_%, %, $(filter SOURCES_%, $(.VARIABLES)))

LIBS_host := oscar/library/libosc_host
LIBS_target := oscar/library/libosc_target
ifeq '$(CONFIG_ENABLE_SIMULATION)' 'y'
LIBS_target := $(LIBS_target)_sim
endif
ifeq '$(CONFIG_ENABLE_DEBUG)' 'y'
LIBS_host := $(LIBS_host)_dbg
LIBS_target := $(LIBS_target)_dbg
endif
LIBS_host := $(LIBS_host).a
LIBS_target := $(LIBS_target).a

BINARIES := $(addsuffix _host, $(PRODUCTS)) $(addsuffix _target, $(PRODUCTS))

.PHONY: all clean host target install deploy run reconfigure
all: $(BINARIES)
host target: %: $(addsuffix _%, $(PRODUCTS))

deploy: $(APP_NAME).app
	tar c $< | ssh root@$(CONFIG_TARGET_IP) 'rm -rf $< && tar x -C $(DEPLOY_DIR)' || true

run:
	ssh root@$(CONFIG_TARGET_IP) $(DEPLOY_DIR)$(APP_NAME).app/run.sh || true

install: cgi/cgi_host
	cp -RL cgi/www/* /var/www
	cp $< /var/www/cgi-bin/cgi
	chmod -Rf a+rX /var/www/ || true

reconfigure:
ifeq '$(CONFIG_PRIVATE_FRAMEWORK)' 'n'
	@ ! [ -e "oscar" ] || [ -h "oscar" ] && ln -sfn $(CONFIG_FRAMEWORK_PATH) oscar || echo "The symlink to the lgx module could not be created as the file ./lgx already exists and is something other than a symlink. Pleas remove it and run 'make reconfigure' to create the symlink."
endif
	@ ! [ -d "oscar" ] || $(MAKE) -C oscar config

oscar/%:
	$(MAKE) -C oscar $*

# Including depency files and optional local Makefile.
-include build/*.d

# Build targets.
build/%_host.o: %.c $(filter-out %.d, $(MAKEFILE_LIST))
	@ mkdir -p $(dir $@)
	$(CC_host) -MD $< -o $@
	@ grep -oE '[^ \\]+' < $(@:.o=.d) | sed -r '/:$$/d; s|^.*$$|$@: \0\n\0:|' > $(@:.o=.d~) && mv -f $(@:.o=.d){~,}
build/%_target.o: %.c $(filter-out %.d, $(MAKEFILE_LIST))
	@ mkdir -p $(dir $@)
	$(CC_target) -MD $< -o $@
	@ grep -oE '[^ \\]+' < $(@:.o=.d) | sed -r '/:$$/d; s|^.*$$|$@: \0\n\0:|' > $(@:.o=.d~) && mv -f $(@:.o=.d){~,}

# Link targets.
define LINK
$(1)_host: $(patsubst %.c, build/%_host.o, $(SOURCES_$(1))) $(LIBS_host)
	$(LD_host) -o $$@ $$^
$(1)_target: $(patsubst %.c, build/%_target.o, $(SOURCES_$(1))) $(LIBS_target)
	$(LD_target) -o $$@ $$^ -lm -lbfdsp
endef
$(foreach i, $(PRODUCTS), $(eval $(call LINK,$i)))

.PHONY: $(APP_NAME).app
$(APP_NAME).app: $(addsuffix _target, $(PRODUCTS))
	rm -rf $@
	cp -rL app $@
	tar c -h -C cgi/www . | gzip > $@/www.tar.gz

# Cleans the module.
clean:
	rm -rf build *.gdb $(BINARIES) $(APP_NAME).app
