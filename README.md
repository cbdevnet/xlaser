# XLaser

XLaser emulates a scanner lighting fixture within an X11 window, useful for
example for creating light shows with computer projectors.

## Control
Control data is input via the Art-Net protocol, generated by most lighting control
desks and software.

## Gobos
Up to 255 projection gobos are supported via a configurable folder containing
images named ``value``.png (eg. ``127.png``). All non-transparent pixels of
a gobo image will be colored in the resulting image.

## Channels
One XLaser instance uses 16 channels, some of which are reserved but not yet mapped
to functionality.

* Channel 1/2: Pan coarse/fine
* Channel 3/4: Tilt coarse/fine
* Channel 5/6/7: Red/Green/Blue color mix
* Channel 8: Dimmer
* Channel 9: Shutter (not yet implemented)
* Channel 10: Gobo
* Channel 11: Gobo zoom
* Channel 12: Gobo rotation absolute
* Channel 13: Gobo rotation speed (not yet implemented)
* Channel 14: Gobo focus (experimental)

## Building
Simply run make.

### Prerequisites (debian)
* A C compiler
* libx11-dev
* x11proto-xext-dev
* x11proto-render-dev

## Running

* Edit sample.conf to match your setup
* Run ``xlaser /path/to/configuration.file``
* Optionally override the starting address with the ``-d <address>`` parameter (useful to start multiple instances from one configuration file)
