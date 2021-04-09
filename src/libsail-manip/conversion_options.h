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

#ifndef SAIL_CONVERSION_OPTIONS_H
#define SAIL_CONVERSION_OPTIONS_H

#ifdef SAIL_BUILD
    #include "error.h"
    #include "export.h"
    #include "pixel.h"

    #include "manip_common.h"
#else
    #include <sail-common/error.h>
    #include <sail-common/export.h>
    #include <sail-common/pixel.h>

    #include <sail-manip/manip_common.h>
#endif

struct sail_conversion_options {

    /*
     * Or-ed SailConversionOption-s.
     */
    int options;

    /*
     * Background color to blend into other color components instead of alpha
     * when options has SAIL_CONVERSION_OPTION_BLEND_ALPHA.
     */
    sail_rgb48_t background;
};

#endif
