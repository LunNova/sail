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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sail-common.h"

#include "helpers.h"

/*  Compression types.  */
static const uint32_t SAIL_BI_RGB            = 0;
static const uint32_t SAIL_BI_RLE8           = 1;
static const uint32_t SAIL_BI_RLE4           = 2;
static const uint32_t SAIL_BI_BITFIELDS      = 3;
#if 0
static const uint32_t SAIL_BI_JPEG           = 4;
static const uint32_t SAIL_BI_PNG            = 5;
static const uint32_t SAIL_BI_ALPHABITFIELDS = 6;
static const uint32_t SAIL_BI_CMYK           = 11;
static const uint32_t SAIL_BI_CMYKRLE8       = 12;
static const uint32_t SAIL_BI_CMYKRLE4       = 13;
#endif

/* BMP identifiers. */
static const uint16_t SAIL_DDB_IDENTIFIER = 0x02;
static const uint16_t SAIL_DIB_IDENTIFIER = 0x4D42;

/* ICC profile types. */
#if 0
static const char SAIL_PROFILE_LINKED[4]   = { 'L', 'I', 'N', 'K' };
#endif
static const char SAIL_PROFILE_EMBEDDED[4] = { 'M', 'B', 'E', 'D' };

/* Sizes of DIB header structs. */
#define SAIL_BITMAP_DIB_HEADER_V2_SIZE 12
#define SAIL_BITMAP_DIB_HEADER_V3_SIZE 40
#define SAIL_BITMAP_DIB_HEADER_V4_SIZE 108
#define SAIL_BITMAP_DIB_HEADER_V5_SIZE 124

/*
 * Codec-specific state.
 */
struct bmp_state {
    struct sail_read_options *read_options;
    struct sail_write_options *write_options;

    enum SailPixelFormat source_pixel_format;

    enum SailBmpVersion version;

    struct SailBmpDdbFileHeader ddb_file_header;
    struct SailBmpDdbBitmap v1;

    struct SailBmpDibFileHeader dib_file_header;
    struct SailBmpDibHeaderV2 v2;
    struct SailBmpDibHeaderV3 v3;
    struct SailBmpDibHeaderV4 v4;
    struct SailBmpDibHeaderV5 v5;

    struct sail_iccp *iccp;

    sail_rgba8_t *palette;
    unsigned palette_count;
    unsigned bytes_in_row;
    /* Number of bytes to pad scan lines to 4-byte boundary. */
    unsigned pad_bytes;
    bool flipped;

    bool frame_read;
    bool frame_written;
};

static sail_status_t alloc_bmp_state(struct bmp_state **bmp_state) {

    void *ptr;
    SAIL_TRY(sail_malloc(sizeof(struct bmp_state), &ptr));
    *bmp_state = ptr;

    if (*bmp_state == NULL) {
        SAIL_LOG_AND_RETURN(SAIL_ERROR_MEMORY_ALLOCATION);
    }

    (*bmp_state)->read_options  = NULL;
    (*bmp_state)->write_options = NULL;
    (*bmp_state)->iccp          = NULL;
    (*bmp_state)->palette       = NULL;
    (*bmp_state)->palette_count = 0;
    (*bmp_state)->bytes_in_row  = 0;
    (*bmp_state)->pad_bytes     = 0;
    (*bmp_state)->flipped       = false;
    (*bmp_state)->frame_read    = false;
    (*bmp_state)->frame_written = false;

    return SAIL_OK;
}

static void destroy_bmp_state(struct bmp_state *bmp_state) {

    if (bmp_state == NULL) {
        return;
    }

    sail_destroy_read_options(bmp_state->read_options);
    sail_destroy_write_options(bmp_state->write_options);

    sail_destroy_iccp(bmp_state->iccp);

    sail_free(bmp_state->palette);

    sail_free(bmp_state);
}

/*
 * Decoding functions.
 */

SAIL_EXPORT sail_status_t sail_codec_read_init_v4_bmp(struct sail_io *io, const struct sail_read_options *read_options, void **state) {

    SAIL_CHECK_STATE_PTR(state);
    *state = NULL;

    SAIL_CHECK_IO(io);
    SAIL_CHECK_READ_OPTIONS_PTR(read_options);

    SAIL_TRY(bmp_private_supported_read_output_pixel_format(read_options->output_pixel_format));

    /* Allocate a new state. */
    struct bmp_state *bmp_state;
    SAIL_TRY(alloc_bmp_state(&bmp_state));
    *state = bmp_state;

    /* Deep copy read options. */
    SAIL_TRY(sail_copy_read_options(read_options, &bmp_state->read_options));

    /* "BM" or 0x02. */
    uint16_t magic;
    SAIL_TRY(io->strict_read(io->stream, &magic, sizeof(magic)));
    SAIL_TRY(io->seek(io->stream, 0, SEEK_SET));

    if (magic == SAIL_DDB_IDENTIFIER) {
        bmp_state->version = SAIL_BMP_V1;

        SAIL_TRY(bmp_private_read_ddb_file_header(io, &bmp_state->ddb_file_header));
        SAIL_TRY(bmp_private_read_v1(io, &bmp_state->v1));

    } else if (magic == SAIL_DIB_IDENTIFIER) {
        SAIL_TRY(bmp_private_read_dib_file_header(io, &bmp_state->dib_file_header));

        size_t offset_of_bitmap_header;
        SAIL_TRY(io->tell(io->stream, &offset_of_bitmap_header));

        SAIL_TRY(bmp_private_read_v2(io, &bmp_state->v2));

        /* If the height is negative, the bitmap is top-to-bottom. */
        if (bmp_state->v2.height < 0) {
            bmp_state->v2.height = -bmp_state->v2.height;
            bmp_state->flipped = false;
        } else {
            bmp_state->flipped = true;
        }

        switch (bmp_state->v2.size) {
            case SAIL_BITMAP_DIB_HEADER_V2_SIZE: {
                bmp_state->version = SAIL_BMP_V2;
                break;
            }
            case SAIL_BITMAP_DIB_HEADER_V3_SIZE: {
                bmp_state->version = SAIL_BMP_V3;
                SAIL_TRY(bmp_private_read_v3(io, &bmp_state->v3));
                break;
            }
            case SAIL_BITMAP_DIB_HEADER_V4_SIZE: {
                bmp_state->version = SAIL_BMP_V4;
                SAIL_TRY(bmp_private_read_v3(io, &bmp_state->v3));
                SAIL_TRY(bmp_private_read_v4(io, &bmp_state->v4));
                break;
            }
            case SAIL_BITMAP_DIB_HEADER_V5_SIZE: {
                bmp_state->version = SAIL_BMP_V5;
                SAIL_TRY(bmp_private_read_v3(io, &bmp_state->v3));
                SAIL_TRY(bmp_private_read_v4(io, &bmp_state->v4));
                SAIL_TRY(bmp_private_read_v5(io, &bmp_state->v5));

                if (memcmp(&bmp_state->v4.color_space_type, SAIL_PROFILE_EMBEDDED, sizeof(SAIL_PROFILE_EMBEDDED)) == 0) {
                    SAIL_TRY(bmp_private_fetch_iccp(io, (long)(offset_of_bitmap_header + bmp_state->v5.profile_data), bmp_state->v5.profile_size, &bmp_state->iccp));
                }

                break;
            }
            default: {
                SAIL_LOG_ERROR("BMP: Unsupported file header size %u", bmp_state->dib_file_header.size);
                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_FORMAT);
            }
        }
    } else {
        SAIL_LOG_ERROR("BMP: 0x%x is not a valid magic number", magic);
        SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_FORMAT);
    }

    /* Check BMP restrictions. */
    if (bmp_state->version == SAIL_BMP_V1) {
        if (bmp_state->v1.type != 0) {
            SAIL_LOG_ERROR("BMP: DDB type must always be 0");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
        if (bmp_state->v1.planes != 1) {
            SAIL_LOG_ERROR("BMP: DDB planes must always be 1");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
        if (bmp_state->v1.pixels != 0) {
            SAIL_LOG_ERROR("BMP: DDB pixels must always be 0");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
        if (bmp_state->v1.bit_count != 1 && bmp_state->v1.bit_count != 4 && bmp_state->v1.bit_count != 8) {
            SAIL_LOG_ERROR("BMP: DDB bpp must be 1, 4, or 8");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
    } else if (bmp_state->version >= SAIL_BMP_V3) {
        if (bmp_state->v3.compression == SAIL_BI_BITFIELDS && bmp_state->v2.bit_count != 16 && bmp_state->v2.bit_count != 32) {
            SAIL_LOG_ERROR("BMP: BitFields compression is allowed only for 16 or 32 bpp");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
        if (bmp_state->v3.compression != SAIL_BI_RGB && bmp_state->v3.compression != SAIL_BI_RLE4 && bmp_state->v3.compression != SAIL_BI_RLE8) {
            SAIL_LOG_ERROR("BMP: Only RGB, RLE4, and RLE8 compressions are supported");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_COMPRESSION);
        }
        if (bmp_state->v3.compression == SAIL_BI_RLE4 && bmp_state->v2.bit_count != 4) {
            SAIL_LOG_ERROR("BMP: RLE4 compression must only be used with 4 bpp");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
        if (bmp_state->v3.compression == SAIL_BI_RLE8 && bmp_state->v2.bit_count != 8) {
            SAIL_LOG_ERROR("BMP: RLE8 compression must only be used with 8 bpp");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
        }
    }

    SAIL_TRY(bmp_private_bit_count_to_pixel_format(bmp_state->version == SAIL_BMP_V1 ? bmp_state->v1.bit_count : bmp_state->v2.bit_count,
                                                    &bmp_state->source_pixel_format));

    SAIL_LOG_DEBUG("BMP: Version is %d", bmp_state->version);

    /*  Read palette.  */
    if (bmp_state->version == SAIL_BMP_V1) {
        SAIL_TRY(bmp_private_fill_system_palette(bmp_state->v1.bit_count, &bmp_state->palette, &bmp_state->palette_count));
    } else if (bmp_state->v2.bit_count < 16) {
        size_t offset;
        SAIL_TRY(io->tell(io->stream, &offset));
        const unsigned palette_size = (unsigned)(bmp_state->dib_file_header.offset - offset);

        if (bmp_state->version == SAIL_BMP_V2) {
            bmp_state->palette_count = palette_size / 3;
        } else {
            bmp_state->palette_count = palette_size / 4;
        }

        if (bmp_state->palette_count == 0) {
            SAIL_LOG_ERROR("BMP: Indexed image has no palette");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_MISSING_PALETTE);
        }

        void *ptr;
        SAIL_TRY(sail_malloc(sizeof(sail_rgba8_t) * bmp_state->palette_count, &ptr));
        bmp_state->palette = ptr;

        if (bmp_state->version == SAIL_BMP_V2) {
            sail_rgb8_t rgb;

            for (unsigned i = 0; i < bmp_state->palette_count; i++) {
                SAIL_TRY(sail_read_pixel3_uint8(io, &rgb));

                bmp_state->palette[i].component1 = rgb.component1;
                bmp_state->palette[i].component2 = rgb.component2;
                bmp_state->palette[i].component3 = rgb.component3;
                bmp_state->palette[i].component4 = 255;
            }
        } else {
            sail_rgba8_t rgba;

            for (unsigned i = 0; i < bmp_state->palette_count; i++) {
                SAIL_TRY(sail_read_pixel4_uint8(io, &rgba));

                bmp_state->palette[i].component1 = rgba.component1;
                bmp_state->palette[i].component2 = rgba.component2;
                bmp_state->palette[i].component3 = rgba.component3;
                bmp_state->palette[i].component4 = 255;
            }
        }
    }

    /* Calculate the number of pad bytes to align scan lines to 4-byte boundary. */
    if (bmp_state->version == SAIL_BMP_V1) {
        SAIL_TRY(bmp_private_bytes_in_row(bmp_state->v1.width, bmp_state->v1.bit_count, &bmp_state->bytes_in_row));
        bmp_state->pad_bytes = bmp_state->v1.byte_width - bmp_state->bytes_in_row;
    } else {
        SAIL_TRY(bmp_private_bytes_in_row(bmp_state->v2.width, bmp_state->v2.bit_count, &bmp_state->bytes_in_row));
        bmp_state->pad_bytes = bmp_private_pad_bytes(bmp_state->bytes_in_row);
    }

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_seek_next_frame_v4_bmp(void *state, struct sail_io *io, struct sail_image **image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE_PTR(image);

    struct bmp_state *bmp_state = (struct bmp_state *)state;

    if (bmp_state->frame_read) {
        SAIL_LOG_AND_RETURN(SAIL_ERROR_NO_MORE_FRAMES);
    }

    bmp_state->frame_read = true;

    struct sail_image *image_local;
    SAIL_TRY(sail_alloc_image(&image_local));
    SAIL_TRY_OR_CLEANUP(sail_alloc_source_image(&image_local->source_image),
                        /* cleanup */ sail_destroy_image(image_local));

    image_local->source_image->compression = SAIL_COMPRESSION_NONE;
    image_local->source_image->pixel_format = bmp_state->source_pixel_format;
    image_local->source_image->properties = bmp_state->flipped ? SAIL_IMAGE_PROPERTY_FLIPPED_VERTICALLY : 0;
    image_local->width = (bmp_state->version == SAIL_BMP_V1) ? bmp_state->v1.width : bmp_state->v2.width;
    image_local->height = (bmp_state->version == SAIL_BMP_V1) ? bmp_state->v1.height : bmp_state->v2.height;

    if (bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_SOURCE) {
        image_local->pixel_format = bmp_state->source_pixel_format;
        image_local->bytes_per_line = bmp_state->bytes_in_row;

        if (bmp_state->palette != NULL) {
            SAIL_TRY_OR_CLEANUP(sail_alloc_palette_for_data(SAIL_PIXEL_FORMAT_BPP32_RGBA, bmp_state->palette_count, &image_local->palette),
                                /* cleanup */ sail_destroy_image(image_local));

            unsigned char *palette_ptr = image_local->palette->data;

            for (unsigned i = 0; i < bmp_state->palette_count; i++) {
                *palette_ptr++ = bmp_state->palette[i].component3;
                *palette_ptr++ = bmp_state->palette[i].component2;
                *palette_ptr++ = bmp_state->palette[i].component1;
                *palette_ptr++ = bmp_state->palette[i].component4;
            }
        }
    } else {
        if (bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_BPP32_RGBA) {
            image_local->pixel_format = SAIL_PIXEL_FORMAT_BPP32_RGBA;
        } else if (bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_BPP32_BGRA) {
            image_local->pixel_format = SAIL_PIXEL_FORMAT_BPP32_BGRA;
        } else {
            SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_PIXEL_FORMAT);
        }

        SAIL_TRY_OR_CLEANUP(sail_bytes_per_line(image_local->width, image_local->pixel_format, &image_local->bytes_per_line),
                            /* cleanup */ sail_destroy_image(image_local));
    }

    /* Resolution. */
    if (bmp_state->version >= SAIL_BMP_V3) {
        SAIL_TRY_OR_CLEANUP(
            sail_alloc_resolution_from_data(SAIL_RESOLUTION_UNIT_METER, bmp_state->v3.x_pixels_per_meter, bmp_state->v3.y_pixels_per_meter, &image_local->resolution),
                        /* cleanup */ sail_destroy_image(image_local));
    }

    /* Seek to the bitmap data. */
    if (bmp_state->version > SAIL_BMP_V1) {
        SAIL_TRY_OR_CLEANUP(io->seek(io->stream, bmp_state->dib_file_header.offset, SEEK_SET),
                            /* cleanup */ sail_destroy_image(image_local));
    }

    *image = image_local;

    const char *pixel_format_str = NULL;
    SAIL_TRY_OR_SUPPRESS(sail_pixel_format_to_string(image_local->source_image->pixel_format, &pixel_format_str));
    SAIL_LOG_DEBUG("BMP: Input pixel format is %s", pixel_format_str);
    SAIL_TRY_OR_SUPPRESS(sail_pixel_format_to_string(bmp_state->read_options->output_pixel_format, &pixel_format_str));
    SAIL_LOG_DEBUG("BMP: Output pixel format is %s", pixel_format_str);

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_seek_next_pass_v4_bmp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE(image);

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_frame_v4_bmp(void *state, struct sail_io *io, struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE(image);

    struct bmp_state *bmp_state = (struct bmp_state *)state;

    const unsigned r = bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_BPP32_RGBA ? 0 : 2;
    const unsigned g = 1;
    const unsigned b = bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_BPP32_RGBA ? 2 : 0;
    const unsigned a = 3;

    /* RLE-encoded images don't need to skip pad bytes. */
    bool skip_pad_bytes = true;
    const unsigned bit_count = (bmp_state->version == SAIL_BMP_V1) ? bmp_state->v1.bit_count : bmp_state->v2.bit_count;
    sail_rgba8_t rgba;

    /* Unobvious if/else branching: if+return. */
    if (bmp_state->read_options->output_pixel_format == SAIL_PIXEL_FORMAT_SOURCE) {
        switch (bit_count) {
            case 4: {
                if (bmp_state->version >= SAIL_BMP_V3 && bmp_state->v3.compression == SAIL_BI_RLE4) {
                    SAIL_LOG_ERROR("BMP: SOURCE pixel format is not compatible with RLE-encoded images");
                    SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_COMPRESSION);
                }
                break;
            }
            case 8: {
                if (bmp_state->version >= SAIL_BMP_V3 && bmp_state->v3.compression == SAIL_BI_RLE8) {
                    SAIL_LOG_ERROR("BMP: SOURCE pixel format is not compatible with RLE-encoded images");
                    SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_COMPRESSION);
                }
                break;
            }
        }

        for (unsigned i = image->height; i > 0; i--) {
            unsigned char *scan = (unsigned char *)image->pixels + image->bytes_per_line * (bmp_state->flipped ? (i - 1) : (image->height - i));

            for (unsigned pixel_index = 0; pixel_index < image->width;) {
                /* Read whole scan line. */
                SAIL_TRY(io->strict_read(io->stream, scan, bmp_state->bytes_in_row));
                pixel_index += image->width;
            }

            /* Skip pad bytes. */
            SAIL_TRY(io->seek(io->stream, bmp_state->pad_bytes, SEEK_CUR));
        }

        return SAIL_OK;
    }

    for (unsigned i = image->height; i > 0; i--) {
        unsigned char *scan = (unsigned char *)image->pixels + image->bytes_per_line * (bmp_state->flipped ? (i - 1) : (image->height - i));

        for (unsigned pixel_index = 0; pixel_index < image->width;) {
            switch (bit_count) {
                case 1: {
                    uint8_t byte;
                    SAIL_TRY(io->strict_read(io->stream, &byte, sizeof(byte)));

                    unsigned bit_shift = 7;
                    unsigned bit_mask = 1 << 7;

                    while (bit_mask > 0 && pixel_index < image->width) {
                        uint8_t index = (byte & bit_mask) >> bit_shift;
                        SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                        *(scan+r) = rgba.component3;
                        *(scan+g) = rgba.component2;
                        *(scan+b) = rgba.component1;
                        *(scan+a) = rgba.component4;
                        scan += 4;

                        bit_shift--;
                        bit_mask >>= 1;
                        pixel_index++;
                    }
                    break;
                }
                case 4: {
                    if (bmp_state->version >= SAIL_BMP_V3 && bmp_state->v3.compression == SAIL_BI_RLE4) {
                        skip_pad_bytes = false;

                        uint8_t marker;
                        SAIL_TRY(io->strict_read(io->stream, &marker, sizeof(marker)));

                        if (marker == SAIL_UNENCODED_RUN_MARKER) {
                            uint8_t count_or_marker;
                            SAIL_TRY(io->strict_read(io->stream, &count_or_marker, sizeof(count_or_marker)));

                            if (count_or_marker == SAIL_END_OF_SCAN_LINE_MARKER) {
                                /* Jump to the end of scan line. +1 to avoid reading end-of-scan-line marker twice below. */
                                pixel_index = image->width + 1;
                            } else if (count_or_marker == SAIL_END_OF_RLE_DATA_MARKER) {
                                SAIL_LOG_ERROR("BMP: Unexpected end-of-rle-data marker");
                                SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
                            } else if (count_or_marker == SAIL_DELTA_MARKER) {
                                SAIL_LOG_ERROR("BMP: Delta marker is not supported");
                                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_FORMAT);
                            } else {
                                bool read_byte = true;
                                uint8_t byte = 0;
                                uint8_t index;

                                for (uint8_t k = 0; k < count_or_marker; k++) {
                                    if (read_byte) {
                                        SAIL_TRY(io->strict_read(io->stream, &byte, sizeof(byte)));
                                        index = (byte >> 4) & 0xf;
                                        read_byte = false;
                                    } else {
                                        index = byte & 0xf;
                                        read_byte = true;
                                    }

                                    SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                                    *(scan+r) = rgba.component3;
                                    *(scan+g) = rgba.component2;
                                    *(scan+b) = rgba.component1;
                                    *(scan+a) = rgba.component4;
                                    scan += 4;
                                }

                                /* Odd number of bytes is accompanied with an additional byte. */
                                uint8_t number_of_unencoded_bytes = (count_or_marker + 1) / 2;
                                if ((number_of_unencoded_bytes % 2) != 0) {
                                    SAIL_TRY(io->seek(io->stream, 1, SEEK_CUR));
                                }

                                pixel_index += count_or_marker;
                            }
                        } else {
                            /* Normal RLE: count + value. */
                            bool high_4_bits = true;
                            uint8_t index;

                            uint8_t byte;
                            SAIL_TRY(io->strict_read(io->stream, &byte, sizeof(byte)));

                            for (uint8_t k = 0; k < marker; k++) {
                                if (high_4_bits) {
                                    index = (byte >> 4) & 0xf;
                                    high_4_bits = false;
                                } else {
                                    index = byte & 0xf;
                                    high_4_bits = true;
                                }

                                SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                                *(scan+r) = rgba.component3;
                                *(scan+g) = rgba.component2;
                                *(scan+b) = rgba.component1;
                                *(scan+a) = rgba.component4;
                                scan += 4;
                            }

                            pixel_index += marker;
                        }

                        /* Read a possible end-of-scan-line marker at the end of line. */
                        if (pixel_index == image->width) {
                            SAIL_TRY(bmp_private_skip_end_of_scan_line(io));
                        }
                    } else {
                        uint8_t byte;
                        SAIL_TRY(io->strict_read(io->stream, &byte, sizeof(byte)));

                        int bit_shift = 4;

                        while (bit_shift >= 0 && pixel_index < image->width) {
                            uint8_t index = (byte >> bit_shift) & 0xf;
                            SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                            *(scan+r) = rgba.component3;
                            *(scan+g) = rgba.component2;
                            *(scan+b) = rgba.component1;
                            *(scan+a) = rgba.component4;
                            scan += 4;

                            bit_shift -= 4;
                            pixel_index++;
                        }
                    }
                    break;
                }
                case 8: {
                    if (bmp_state->version >= SAIL_BMP_V3 && bmp_state->v3.compression == SAIL_BI_RLE8) {
                        skip_pad_bytes = false;

                        uint8_t marker;
                        SAIL_TRY(io->strict_read(io->stream, &marker, sizeof(marker)));

                        if (marker == SAIL_UNENCODED_RUN_MARKER) {
                            uint8_t count_or_marker;
                            SAIL_TRY(io->strict_read(io->stream, &count_or_marker, sizeof(count_or_marker)));

                            if (count_or_marker == SAIL_END_OF_SCAN_LINE_MARKER) {
                                /* Jump to the end of scan line. +1 to avoid reading end-of-scan-line marker twice below. */
                                pixel_index = image->width + 1;
                            } else if (count_or_marker == SAIL_END_OF_RLE_DATA_MARKER) {
                                SAIL_LOG_ERROR("BMP: Unexpected end-of-rle-data marker");
                                SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
                            } else if (count_or_marker == SAIL_DELTA_MARKER) {
                                SAIL_LOG_ERROR("BMP: Delta marker is not supported");
                                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_FORMAT);
                            } else {
                                for (uint8_t k = 0; k < count_or_marker; k++) {
                                    uint8_t index;
                                    SAIL_TRY(io->strict_read(io->stream, &index, sizeof(index)));
                                    SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                                    *(scan+r) = rgba.component3;
                                    *(scan+g) = rgba.component2;
                                    *(scan+b) = rgba.component1;
                                    *(scan+a) = rgba.component4;
                                    scan += 4;
                                }

                                /* Odd number of pixels is accompanied with an additional byte. */
                                if ((count_or_marker % 2) != 0) {
                                    SAIL_TRY(io->seek(io->stream, 1, SEEK_CUR));
                                }

                                pixel_index += count_or_marker;
                            }
                        } else {
                            /* Normal RLE: count + value. */
                            uint8_t index;
                            SAIL_TRY(io->strict_read(io->stream, &index, sizeof(index)));
                            SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                            for (uint8_t k = 0; k < marker; k++) {
                                *(scan+r) = rgba.component3;
                                *(scan+g) = rgba.component2;
                                *(scan+b) = rgba.component1;
                                *(scan+a) = rgba.component4;
                                scan += 4;
                            }

                            pixel_index += marker;
                        }

                        /* Read a possible end-of-scan-line marker at the end of line. */
                        if (pixel_index == image->width) {
                            SAIL_TRY(bmp_private_skip_end_of_scan_line(io));
                        }
                    } else {
                        uint8_t index;
                        SAIL_TRY(io->strict_read(io->stream, &index, sizeof(index)));
                        SAIL_TRY(bmp_private_get_palette_color(bmp_state->palette, bmp_state->palette_count, index, &rgba));

                        *(scan+r) = rgba.component3;
                        *(scan+g) = rgba.component2;
                        *(scan+b) = rgba.component1;
                        *(scan+a) = rgba.component4;
                        scan += 4;

                        pixel_index++;
                    }
                    break;
                }
                case 16: {
                    uint16_t rgb16;
                    SAIL_TRY(io->strict_read(io->stream, &rgb16, sizeof(rgb16)));

                    if (bmp_state->v3.compression == SAIL_BI_RGB) {
                        *(scan+r) = ((rgb16 >> 10) & 0x1f) << 3;
                        *(scan+g) = ((rgb16 >> 5)  & 0x1f) << 3;
                        *(scan+b) = ((rgb16 >> 0)  & 0x1f) << 3;
                    } else {
                        SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_COMPRESSION);
                    }

                    *(scan+a) = 255;
                    scan += 4;

                    pixel_index++;
                    break;
                }
                case 24: {
                    sail_rgb8_t rgb;
                    SAIL_TRY(sail_read_pixel3_uint8(io, &rgb));

                    *(scan+r) = rgb.component3;
                    *(scan+g) = rgb.component2;
                    *(scan+b) = rgb.component1;
                    *(scan+a) = 255;
                    scan += 4;

                    pixel_index++;
                    break;
                }
                case 32: {
                    sail_rgba8_t rgba1;
                    SAIL_TRY(sail_read_pixel4_uint8(io, &rgba1));

                    *(scan+r) = rgba1.component3;
                    *(scan+g) = rgba1.component2;
                    *(scan+b) = rgba1.component1;
                    *(scan+a) = rgba1.component4;
                    scan += 4;

                    pixel_index++;
                    break;
                }
                default: {
                    SAIL_LOG_AND_RETURN(SAIL_ERROR_UNSUPPORTED_BIT_DEPTH);
                }
            }
        }

        /* Skip pad bytes. */
        if (skip_pad_bytes) {
            SAIL_TRY(io->seek(io->stream, bmp_state->pad_bytes, SEEK_CUR));
        }
    }

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_finish_v4_bmp(void **state, struct sail_io *io) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);

    struct bmp_state *bmp_state = (struct bmp_state *)(*state);

    /* Subsequent calls to finish() will expectedly fail in the above line. */
    *state = NULL;

    destroy_bmp_state(bmp_state);

    return SAIL_OK;
}

/*
 * Encoding functions.
 */

SAIL_EXPORT sail_status_t sail_codec_write_init_v4_bmp(struct sail_io *io, const struct sail_write_options *write_options, void **state) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_WRITE_OPTIONS_PTR(write_options);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_seek_next_frame_v4_bmp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE(image);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_seek_next_pass_v4_bmp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE(image);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_frame_v4_bmp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);
    SAIL_CHECK_IMAGE(image);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_finish_v4_bmp(void **state, struct sail_io *io) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_CHECK_IO(io);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}
