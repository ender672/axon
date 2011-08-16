#include <ruby.h>

static ID id_image, id_components, id_width_ratio, id_width;

/*
*  call-seq:
*     image.interpolate_scanline(original_scanlines, q) -> interpolated_scanline
*/
static VALUE
interpolate_scanline(VALUE self, VALUE rb_scanline)
{
    VALUE rb_components, rb_width_ratio, rb_dest_sl, rb_image, rb_width;
    double width_ratio;
    unsigned char *scanline, *dest_sl;
    size_t width, components, i, j;

    rb_width = rb_ivar_get(self, id_width);
    width = FIX2INT(rb_width);

    rb_image = rb_ivar_get(self, id_image);
    rb_components = rb_funcall(rb_image, id_components, 0);
    components = FIX2INT(rb_components);

    rb_width_ratio = rb_ivar_get(self, id_width_ratio);
    width_ratio = NUM2DBL(rb_width_ratio);

    scanline = RSTRING_PTR(rb_scanline);
    
    rb_dest_sl = rb_str_new(0, width * components);
    dest_sl = RSTRING_PTR(rb_dest_sl);

    for (i = 0; i < width; i++)
        for (j = 0; j < components; j++)
            dest_sl[i * components + j] = scanline[(int)(i / width_ratio)];
    
    return rb_dest_sl;
}

void
Init_nearest_neighbor_interpolation()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE mNearestNeighborScaling = rb_define_module_under(mAxon, "NearestNeighborScaling");
    rb_define_method(mNearestNeighborScaling, "interpolate_scanline", interpolate_scanline, 1);
    
    id_width_ratio = rb_intern("@width_ratio");
    id_image = rb_intern("@image");
    id_components = rb_intern("components");
    id_width = rb_intern("@width");
}
