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

#ifndef SAIL_SAVE_OPTIONS_H
#define SAIL_SAVE_OPTIONS_H

#ifdef SAIL_BUILD
    #include "common.h"
    #include "error.h"
    #include "export.h"
#else
    #include <sail-common/common.h>
    #include <sail-common/error.h>
    #include <sail-common/export.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct sail_hash_map;
struct sail_save_features;

/*
 * Options to modify saving operations.
 */
struct sail_save_options {

    /* Or-ed manipulation options. See SailOption. */
    int options;

    /*
     * Compression type. For example: SAIL_COMPRESSION_RLE. See SailCompression.
     * Use sail_save_features to determine what compression types or values are supported by a particular codec.
     *
     * If a codec supports more than two compression types, compression levels are ignored in this case.
     *
     * For example:
     *
     *     1. The JPEG codec supports only one compression, JPEG. save_features->compression_level can be used
     *        to select a compression level.
     *     2. The TIFF codec supports more than two compression types (PACKBITS, JPEG, etc.). Compression levels
     *        are ignored.
     */
    enum SailCompression compression;

    /*
     * Requested compression level. Must be in the range specified in save_features.compression_level.
     */
    double compression_level;

    /* Codec-specific tuning options. */
    struct sail_hash_map *tuning;
};

typedef struct sail_save_options sail_save_options_t;

/*
 * Allocates save options.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_alloc_save_options(struct sail_save_options **save_options);

/*
 * Destroys the specified save options object and all its internal allocated memory buffers.
 * The save options MUST NOT be used anymore after calling this function. It does nothing
 * if the save options is NULL.
 */
SAIL_EXPORT void sail_destroy_save_options(struct sail_save_options *save_options);

/*
 * Allocates and builds default save options from the save features.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_alloc_save_options_from_features(const struct sail_save_features *save_features, struct sail_save_options **save_options);

/*
 * Makes a deep copy of the specified save options object.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_copy_save_options(const struct sail_save_options *source, struct sail_save_options **target);

/* extern "C" */
#ifdef __cplusplus
}
#endif

#endif
