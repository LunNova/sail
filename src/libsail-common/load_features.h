/*  This file is part of SAIL (https://github.com/HappySeaFox/sail)

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

#ifndef SAIL_LOAD_FEATURES_H
#define SAIL_LOAD_FEATURES_H

#ifdef SAIL_BUILD
    #include "error.h"
    #include "export.h"
#else
    #include <sail-common/error.h>
    #include <sail-common/export.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct sail_string_node;

/*
 * Load features. Use this structure to determine what a codec can actually load.
 */
struct sail_load_features {

    /* Supported or-ed features of loading operations. See SailCodecFeature. */
    int features;

    /*
     * Codec-specific tuning options. For example, a hypothetical ABC image codec
     * can allow disabling filtering with setting the "abc-filtering" tuning option
     * to 0 in load options. Tuning options' names start with the codec name
     * to avoid confusing.
     *
     * The list of possible values for every tuning option is not current available
     * programmatically. Every codec must document them in the codec info.
     *
     * It's not guaranteed that tuning options and their values are backward
     * or forward compatible.
     */
    struct sail_string_node *tuning;
};

typedef struct sail_load_features sail_load_features_t;

/*
 * Allocates load features.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_alloc_load_features(struct sail_load_features **load_features);

/*
 * Destroys the specified load features object and all its internal allocated memory buffers. The load features
 * MUST NOT be used anymore after calling this function. Does nothing if the load features is NULL.
 */
SAIL_EXPORT void sail_destroy_load_features(struct sail_load_features *load_features);

/* extern "C" */
#ifdef __cplusplus
}
#endif

#endif
