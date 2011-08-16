#include <ruby.h>

#include "jpeg_common.h"
#include "png_common.h"
#include "interpolation.h"

void
Init_axon()
{
    Init_jpeg();
    Init_jpeg_reader();
    Init_jpeg_native_writer();

    Init_png();
    Init_png_reader();
    Init_png_native_writer();

    Init_bilinear_interpolation();
    Init_nearest_neighbor_interpolation();
}
