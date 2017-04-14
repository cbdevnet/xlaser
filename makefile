export PREFIX ?= /usr
export DOCDIR ?= $(DESTDIR)$(PREFIX)/share/man/man1

.PHONY: all clean shaders run
CFLAGS ?= -g -Wall
LDLIBS ?= -lm -lX11

opengl: CFLAGS += -DOPENGL
opengl: LDLIBS += -lGLEW -lGL
xrender: LDLIBS += -lXext -lXrender

all: opengl xlaser.1.gz

shaders:
	$(MAKE) -C shaders

opengl: shaders xlaser

xrender: xlaser

install:
	install -m 0755 xlaser "$(DESTDIR)$(PREFIX)/bin"
	install -g 0 -o 0 -m 0644 xlaser.1.gz "$(DOCDIR)"

xlaser.1.gz:
	gzip -c < xlaser.1 > $@

clean:
	$(RM) xlaser xlaser.1.gz
	$(MAKE) -C shaders clean

run:
	valgrind -v --leak-check=full --track-origins=yes --show-reachable=yes ./xlaser configs/sample.conf
