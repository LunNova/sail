/*  This file is part of SAIL (https://github.com/smoked-herring/sail)

    Copyright (c) 2020 Dmitry Baryshev

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sail-common.h"

int sail_alloc_read_options(struct sail_read_options **read_options) {

    SAIL_TRY(sail_malloc(read_options, sizeof(struct sail_read_options)));

    (*read_options)->output_pixel_format = SAIL_PIXEL_FORMAT_UNKNOWN;
    (*read_options)->io_options          = 0;

    return 0;
}

void sail_destroy_read_options(struct sail_read_options *read_options) {

    if (read_options == NULL) {
        return;
    }

    sail_free(read_options);
}

sail_error_t sail_read_options_from_features(const struct sail_read_features *read_features, struct sail_read_options *read_options) {

    SAIL_CHECK_READ_FEATURES_PTR(read_features);
    SAIL_CHECK_READ_OPTIONS_PTR(read_options);

    read_options->output_pixel_format = read_features->preferred_output_pixel_format;

    read_options->io_options = 0;

    if (read_features->features & SAIL_PLUGIN_FEATURE_META_INFO) {
        read_options->io_options |= SAIL_IO_OPTION_META_INFO;
    }

    if (read_features->features & SAIL_PLUGIN_FEATURE_INTERLACED) {
        read_options->io_options |= SAIL_IO_OPTION_INTERLACED;
    }

    if (read_features->features & SAIL_PLUGIN_FEATURE_ICCP) {
        read_options->io_options |= SAIL_IO_OPTION_ICCP;
    }

    return 0;
}

sail_error_t sail_alloc_read_options_from_features(const struct sail_read_features *read_features, struct sail_read_options **read_options) {

    SAIL_TRY(sail_alloc_read_options(read_options));
    SAIL_TRY_OR_CLEANUP(sail_read_options_from_features(read_features, *read_options),
                        /* cleanup */ sail_destroy_read_options(*read_options));

    return 0;
}

sail_error_t sail_copy_read_options(const struct sail_read_options *source, struct sail_read_options **target) {

    SAIL_CHECK_READ_OPTIONS_PTR(source);
    SAIL_CHECK_READ_OPTIONS_PTR(target);

    SAIL_TRY(sail_malloc(target, sizeof(struct sail_read_options)));

    memcpy(*target, source, sizeof(struct sail_read_options));

    return 0;
}
