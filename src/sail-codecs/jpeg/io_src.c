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

#include <jerror.h>

#include "sail-common.h"

#include "io_src.h"

#define INPUT_BUF_SIZE  4096    /* choose an efficiently fread'able size */

/*
 * Most of this file was copied from libjpeg-turbo 2.0.4 and adapted to SAIL.
 */

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */
static void init_source(j_decompress_ptr cinfo)
{
    struct sail_jpeg_source_mgr *src = (struct sail_jpeg_source_mgr *)cinfo->src;

    /* We reset the empty-input-file flag for each image,
     * but we don't clear the input buffer.
     * This is correct behavior for reading a series of images from one source.
     */
    src->start_of_file = TRUE;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */
static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
    struct sail_jpeg_source_mgr *src = (struct sail_jpeg_source_mgr *)cinfo->src;
    size_t nbytes;

    sail_status_t err = src->io->tolerant_read(src->io->stream, src->buffer, INPUT_BUF_SIZE, &nbytes);

    if (err != SAIL_OK || nbytes == 0) {
        if (src->start_of_file)     /* Treat empty input file as fatal error */
            ERREXIT(cinfo, JERR_INPUT_EMPTY);

        WARNMS(cinfo, JWRN_JPEG_EOF);
        /* Insert a fake EOI marker */
        src->buffer[0] = (JOCTET)0xFF;
        src->buffer[1] = (JOCTET)JPEG_EOI;
        nbytes = 2;
    }

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = nbytes;
    src->start_of_file = FALSE;

    return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */
static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr *src = cinfo->src;

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes. Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0) {
        while (num_bytes > (long)src->bytes_in_buffer) {
            num_bytes -= (long)src->bytes_in_buffer;
            (void)(*src->fill_input_buffer) (cinfo);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
            */
        }

        src->next_input_byte += (size_t)num_bytes;
        src->bytes_in_buffer -= (size_t)num_bytes;
    }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
static void term_source(j_decompress_ptr cinfo)
{
    /* no work necessary here */
    (void)cinfo;
}

/*
 * Prepare for input from a SAIL I/O stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */
void jpeg_private_sail_io_src(j_decompress_ptr cinfo, struct sail_io *io) {

    struct sail_jpeg_source_mgr *src;

    /* The source object and input buffer are made permanent so that a series
     * of JPEG images can be read from the same file by calling jpeg_stdio_src
     * only before the first one.  (If we discarded the buffer at the end of
     * one image, we'd likely lose the start of the next one.)
     */
    if (cinfo->src == NULL) {     /* first time for this JPEG object? */
        cinfo->src = cinfo->mem->alloc_small((j_common_ptr)cinfo,
                                                JPOOL_PERMANENT,
                                                sizeof(struct sail_jpeg_source_mgr));
        src = (struct sail_jpeg_source_mgr *)cinfo->src;
        src->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
                                                                            JPOOL_PERMANENT,
                                                                            INPUT_BUF_SIZE * sizeof(JOCTET));
    } else if (cinfo->src->init_source != init_source) {
        /* It is unsafe to reuse the existing source manager unless it was created
         * by this function.  Otherwise, there is no guarantee that the opaque
         * structure is the right size.  Note that we could just create a new
         * structure, but the old structure would not be freed until
         * jpeg_destroy_decompress() was called.
         */
        ERREXIT(cinfo, JERR_BUFFER_SIZE);
    }

    src = (struct sail_jpeg_source_mgr *)cinfo->src;

    src->pub.init_source       = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data   = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source       = term_source;
    src->io                    = io;
    src->pub.bytes_in_buffer   = 0;    /* forces fill_input_buffer on first read */
    src->pub.next_input_byte   = NULL; /* until buffer loaded */
}
