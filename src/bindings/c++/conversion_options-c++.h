/*  This file is part of SAIL (https://github.com/HappySeaFox/sail)

    Copyright (c) 2020-2021 Dmitry Baryshev

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

#ifndef SAIL_CONVERSION_OPTIONS_CPP_H
#define SAIL_CONVERSION_OPTIONS_CPP_H

#include <memory>

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

struct sail_conversion_options;

namespace sail
{

/*
 * Image conversion options.
 */
class SAIL_EXPORT conversion_options
{
    friend class image;

public:
    /*
     * Constructs an empty conversion options object.
     */
    conversion_options();

    /*
     * Constructs a new conversion options object out of the or-ed SailConversionOption-s
     * and the 48-bit color to blend 48-bit images.
     * If the options argument is zero, SAIL_CONVERSION_OPTION_DROP_ALPHA is assumed.
     * Additionally, calculates and sets a new 24-bit background color to blend 24-bit images.
     */
    conversion_options(int options, const sail_rgb48_t &rgb48);

    /*
     * Constructs a new conversion options object out of the or-ed SailConversionOption-s
     * and the 24-bit color to blend 24-bit images.
     * If the options argument is zero, SAIL_CONVERSION_OPTION_DROP_ALPHA is assumed.
     * Additionally, calculates and sets a new 48-bit background color to blend 48-bit images.
     */
    conversion_options(int options, const sail_rgb24_t &rgb24);

    /*
     * Copies the conversion options object.
     */
    conversion_options(const conversion_options &co);

    /*
     * Copies the conversion options object.
     */
    conversion_options& operator=(const conversion_options &co);

    /*
     * Moves the conversion options object.
     */
    conversion_options(conversion_options &&co) noexcept;

    /*
     * Moves the conversion options object.
     */
    conversion_options& operator=(conversion_options &&co) noexcept;

    /*
     * Destroys the conversion options object.
     */
    ~conversion_options();

    /*
     * Returns the or-ed SailConversionOption-s.
     */
    int options() const;

    /*
     * Returns the 48-bit background color to blend 48-bit images.
     */
    sail_rgb48_t background48() const;

    /*
     * Returns the 24-bit background color to blend 24-bit images.
     */
    sail_rgb24_t background24() const;

    /*
     * Sets new or-ed SailConversionOption-s. If zero, SAIL_CONVERSION_OPTION_DROP_ALPHA is assumed.
     */
    void set_options(int options);

    /*
     * Sets a new 48-bit background color to blend 48-bit images.
     * Additionally, calculates and sets a new 24-bit background color to blend 24-bit images.
     */
    void set_background(const sail_rgb48_t &rgb48);

    /*
     * Sets a new 24-bit background color to blend 24-bit images.
     * Additionally, calculates and sets a new 48-bit background color to blend 48-bit images.
     */
    void set_background(const sail_rgb24_t &rgb24);

private:
    sail_status_t to_sail_conversion_options(sail_conversion_options **conversion_options) const;

private:
    class pimpl;
    std::unique_ptr<pimpl> d;
};

}

#endif
