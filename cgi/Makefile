
SOURCES := $(wildcard *.cpp) $(wildcard *.c)

PRODUCT := cgi

PRODUCT_host := $(addsuffix _host, $(PRODUCT))
PRODUCT_target := $(addsuffix _target, $(PRODUCT))


all:
	gcc $(SOURCES) -o $(PRODUCT_host) ../oscar/library/libosc_host.a -Os -I../oscar/include -DOSC_HOST
	bfin-uclinux-gcc -Os $(SOURCES) -o $(PRODUCT_target) ../oscar/library/libosc_target.a -elf2flt="-s 4096" -I../oscar/include -DOSC_TARGET
clean: 
	rm -f $(PRODUCT_host)
	rm -f $(PRODUCT_target)

	



