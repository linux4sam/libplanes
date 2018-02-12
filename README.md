![Microchip](doc/microchip_logo.png)

# Microchip Hardware LCD Plane Library

libplanes is a library that provides support for working with LCD controller
hardware planes found on SAMA5 hardware.  It directly uses libdrm to manipulate
and control the hardware planes with support for using cairo for basic drawing
primitives.  It includes basic support for parsing a configuration file used to
describe the layout and configuration of the planes, but this can also be done
manually.

Also included are several applications that use libplanes to perform various
tasks to exercise the hardware and library.  These applications provide a set of
examples and evaluation tests related to using hardware planes.

## Terminology

The general terminology used to describe hardware planes varies.  The comparison
is as follows:

- Plane (Linux/DRM/KMS)
  - Primary Plane
  - Overlay Plane
  - Cursor Plane

- Layer (Datasheet/DirectFB)
  - Base Layer
  - Overlay Layer
  - High End Overlay (HEO)


## Dependencies

- libdrm >= 2.4.0
- cJSON >= 1.6.0
- lua >= 5.3.1
- cairo >= 1.14.6
- Python >= 2.7
- swig
- directfb >= 1.7.0 (optional)

## Building

Make sure you have the required dependencis listed above.  To cross compile, put
the cross gcc path in your environment PATH variable and use the appropriate
prefix for --host on the configure line.  For example:

    ./autogen.sh
    ./configure --host=arm-buildroot-linux-gnueabihf
    make

If you wish to statically link the applications, add the following to your
configure line:

    ./configure --enable-static --disable-shared LDFLAGS="-static"


## API Documentation

If you have doxygen installed, you can generate the API documentation by running:

    make doc

The resulting documentation will be in the doc directory.

## Python Bindings

Full Python bindings are provided, generated with swig.  You can find examples
in the `python` directory.

## Applications

### planes

Application that acts as a DRM master that takes a configuration and applies it
to hardware planes and then will run a minimal engine to perform basic animation
of the overlays.

    ./planes -v -c default.config

The config file is a simple JSON formatted file that specifies the configuration
of each plane.  See ``default.config`` for an example.

With this application running as a DRM master, other applications can then use
the created GEM names to manipulate the plane framebuffers externally from
another process.

### render

Application that uses GEM names created by the planes app that can be passed to
this process to write to a plane framebuffers independently.  It supports
setting a single background color or loading an image into the framebuffer. To
run this, the planes app must already be running.

    ./render -n 1 -c 0xff0000ff --width 800 --height 480
    ./render -n 1 -f ./frog.png --width 800 --height 480

### grab

Application that uses GEM names created by the planes app to capture and save
the framebuffer as a PNG. To run this, the planes app must already be running.

    ./grab -n 1 -f capture.png --width 800 --height 480

### dfblayers

Simple demo based on a DirectFB test that allocates hardware overlays using the
DirectFB API and moves them around on the screen showing little CPU usage in the
process.

    ./dfblayers

## License

libplanes is released under the terms of the `MIT` license. See the `COPYING`
file for more information.
