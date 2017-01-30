include config.mk

clib := lem/gzip/core.so

all: $(clib)

$(clib): lem/gzip/core.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ 

install: $(clib)
	install -D -m 644 $< $(cmoddir)/$(clib)

clean:
	rm -f $(clib)
