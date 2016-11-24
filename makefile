export PREFIX?=/usr
export DOCDIR?=$(DESTDIR)$(PREFIX)/share/man/man1

.PHONY: all clean
CFLAGS?=-g -Wall
LDLIBS?=-lm -lXext -lX11 -lXrender -lGL

all: xlaser xlaser.1.gz

install:
	install -m 0755 xlaser "$(DESTDIR)$(PREFIX)/bin"
	install -g 0 -o 0 -m 0644 xlaser.1.gz "$(DOCDIR)"

xlaser.1.gz:
	gzip -c < xlaser.1 > $@

clean:
	$(RM) xlaser xlaser.1.gz

run:
	valgrind -v --leak-check=full --track-origins=yes --show-reachable=yes ./xlaser sample.conf
