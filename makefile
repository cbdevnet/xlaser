export PREFIX?=/usr
export DOCDIR?=$(DESTDIR)$(PREFIX)/share/man/man1

.PHONY: all clean
CFLAGS?=-g -Wall #$(shell freetype-config --cflags)
LDLIBS?=-lm -lXext -lX11 -lXrender #$(shell freetype-config --libs) -lXft

all: xlaser xlaser.1.gz

install:
	install -m 0755 xlaser "$(DESTDIR)$(PREFIX)/bin"
	install -g 0 -o 0 -m 0644 xlaser.1.gz "$(DOCDIR)"

xlaser.1.gz:
	gzip -c < xlaser.1 > $@

clean:
	$(RM) xlaser xlaser.1.gz

displaytest:
	valgrind -v --leak-check=full --track-origins=yes --show-reachable=yes ./xlaser 16
