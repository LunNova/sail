# SAIL: Squirrel Abstract Image Libraries

SAIL is a fast and lightweight cross-platform image decoding and encoding library providing multi-leveled APIs,
from one-liners to complex use cases with custom I/O sources. :sailboat:

SAIL is a fork of ksquirrel-libs, which was a set of C++ image codecs for the KSquirrel image viewer.
See [http://ksquirrel.sourceforge.net](http://ksquirrel.sourceforge.net).

Author: Dmitry Baryshev

## Target audience

- Image viewers.
- Game developers.
- Anyone who needs to load or save images in different image formats and who requires
  a lightweight and comprehensive API for that.

## Features overview

- Easy-to-use C and C++ interfaces
- Four levels of APIs, depending on your needs: `junior`, `advanced`, `deep diver`, and `technical diver`.
  See [EXAMPLES](EXAMPLES.md) for more.
- Reading images from file and memory
- Writing images to file and memory
- I/O abstraction for technical divers
- Image formats are supported by dynamically loaded codecs (plugins)
- It's guaranteed that every plugin is able to read and output to memory pixels in the `BPP24-RGB`
  and `BPP32-RGBA` formats. Supporting other output pixel formats is plugin-specific
- Reading and writing images in numerous plugin-specific pixel formats. For example, the JPEG plugin
  is able to read `RGB` and `YCbCr` images and output them to memory as `Grayscale` pixels, and vice versa
- Reading images and outputting them to memory in source pixel format for those who want to kick the hell
  out of images manually. For example, one may want to work with raw `CMYK` pixels in a print image.
  :warning: Some plugins might not support outputting source pixels
- Read and write meta information like JPEG comments
- Easily extensible with new image formats for those who want to implement a specific codec for his/her needs
- Qt, SDL, and pure C examples

## Features NOT provided

- Image editing capabilities (filtering, distortion, scaling, etc.)
- Color space conversion functions
- Color management functions (applying ICC profiles etc.)
- EXIF rotation

## Supported image formats

1. [JPEG](https://wikipedia.org/wiki/JPEG) (reading and writing, requires `libjpeg-turbo`)

## Supported platforms

Currently, SAIL supports the Windows and Linux platforms.

## Have questions or issues?

Opening a GitHub [issue](https://github.com/smoked-herring/sail/issues) is the preferred way
of communicating and solving problems.

## Architecture overview

SAIL is written in pure C11 w/o using any third-party libraries (except for codecs). It also provides
bindings to C++.

### SAIL plugins

SAIL plugins are the deepest level. This is a set of standalone, dynamically loaded codecs (SO on Linux
and DLL on Windows). They implement actual decoding and encoding capabilities. End-users never work with
plugins directly. They always use abstract, high-level APIs for that.

### libsail-common

libsail-common holds common data types (images, pixel formats, I/O abstractions etc.) and a small set
of functions shared between SAIL plugins and the high-level APIs.

### libsail

libsail is a feature-rich, high-level API. It provides comprehensive and lightweight interfaces to decode
and encode images. End-users implementing C applications always work with libsail.

### libsail-c++

libsail-c++ is a C++ binding to libsail. End-users implementing C++ applications may choose
between libsail and libsail-c++. Using libsail-c++ is always recommended, as it's much more simple
to use in C++ applications.

## License

- libsail-common, libsail, C++ bindings, and plugins are under LGPLv3+.
- Examples and tests are under the MIT license.

## APIs overview

SAIL provides four levels of APIs, depending on your needs. Let's have a quick look at them.

### 1. `Junior` - "I just want to load this damn JPEG."

#### C:
```C
struct sail_image *image;
unsigned char *image_bits;

/*
 * sail_read() reads the image and outputs pixels in BPP24-RGB pixel format for image formats
 * without transparency support and BPP32-RGBA otherwise. If you need to control output pixel
 * formats, consider switching to the deep diver API.
 */
SAIL_TRY(sail_read(path,
                   &image,
                   (void **)&image_bits));

/*
 * Handle the image bits here.
 * Use image->width, image->height, image->bytes_per_line,
 * and image->pixel_format for that.
 */

free(image_bits);
sail_destroy_image(image);
```

#### C++:
```C++
sail::image_reader reader;
sail::image image;

// read() reads the image and outputs pixels in BPP24-RGB pixel format for image formats
// without transparency support and BPP32-RGBA otherwise. If you need to control output pixel
// formats, consider switching to the deep diver API.
//
SAIL_TRY(reader.read(path, &image));

// Handle the image and its bits here.
// Use image.width(), image.height(), image.bytes_per_line(),
// image.pixel_format(), and image.bits() for that.
```

It's pretty easy, isn't it? :smile: See [EXAMPLES](EXAMPLES.md) for more.

### 2. `Advanced` - "I want to load this damn animated GIF."

See [EXAMPLES](EXAMPLES.md) for more.

### 3. `Deep diver` - "I want to load this damn possibly multi-paged image from memory and have comprehensive control over selected plugins and output pixel formats."

See [EXAMPLES](EXAMPLES.md) for more.

### 4. `Technical diver` - "I want everything above and my custom I/O source."

See [EXAMPLES](EXAMPLES.md) for more.
