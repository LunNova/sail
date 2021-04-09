/*  This file is part of SAIL (https://github.com/smoked-herring/sail)

    Copyright (c) 2021 Dmitry Baryshev

    The MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef SAIL_MANIP_UTILS_H
#define SAIL_MANIP_UTILS_H

#include <stdint.h>

#include "error.h"
#include "export.h"

struct sail_conversion_options;

SAIL_HIDDEN void fill_rgba32_pixel_from_uint8_values(uint8_t rv, uint8_t gv, uint8_t bv, uint8_t av, uint8_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options);

SAIL_HIDDEN void fill_rgba32_pixel_from_uint16_values(uint16_t rv, uint16_t gv, uint16_t bv, uint16_t av, uint8_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options);

SAIL_HIDDEN void fill_rgba64_pixel_from_uint8_values(uint8_t rv, uint8_t gv, uint8_t bv, uint8_t av, uint16_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options);

SAIL_HIDDEN void fill_rgba64_pixel_from_uint16_values(uint16_t rv, uint16_t gv, uint16_t bv, uint16_t av, uint16_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options);

#endif
