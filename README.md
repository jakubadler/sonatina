# Sonatina

Sonatina is a client for [MPD](https://musicpd.org/) written in C using GTK+.
Its elegant graphical layout is inspired by [Sonata](http://www.nongnu.org/sonata/).

## Dependencies

 * GTK+ 3.6
 * libmpdclient

## Installation

Sonatina uses GNU make to build and install sources, just run
```
make install
```
in the source directory. You can use PREFIX variable to specify installation
prefix:
```
PREFIX=/opt/sonatina
```
Default value is `/usr/local/`; for more info see `config.mk`.

For Gentoo users, there is an ebuild in [my overlay](https://github.com/jakubadler/adl-overlay).
