#ifndef  AXON_JPEG_COMMON_H
#define  AXON_JPEG_COMMON_H

#include <ruby.h>
#include <jpeglib.h>

#ifndef HAVE_RB_BLOCK_CALL
#define rb_block_call(arg1, arg2, arg3, arg4, arg5, arg6) rb_iterate(rb_each, arg1, arg5, arg6)
#endif

#ifdef JCS_EXTENSIONS
#define JPEG_LIB_TURBO 1
#else
#define JPEG_LIB_TURBO 0
#endif

/*
 * Marker size is defined by two bytes, so the maximum is 65,535 bytes.
 * Two of those bytes are used by the size indicator bytes themselves, leaving
 * 65,533 bytes.
 */

#define MAX_MARKER_LEN 65533

/*
 * The Exif marker eats 4 bytes for "Exif", and 2 bytes for NULL terminators.
 */

#define EXIF_OVERHEAD_LEN 6
#define MAX_EXIF_DATA_LEN MAX_MARKER_LEN - EXIF_OVERHEAD_LEN
#define EXIF_MARKER (JPEG_APP0 + 1)

extern struct jpeg_error_mgr jerr;

ID j_color_space_to_id(J_COLOR_SPACE cs);
J_COLOR_SPACE id_to_j_color_space(ID rb);
int sym_to_marker_code(ID rb);

void Init_jpeg();
void Init_jpeg_native_writer();
void Init_jpeg_native_reader();

#endif
