package axon;

import org.jruby.Ruby;
import org.jruby.RubyFixnum;
import org.jruby.RubyModule;
import org.jruby.RubyString;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;

public class Interpolation {
    @JRubyMethod(required = 5, meta = true)
    public static IRubyObject bilinear(ThreadContext context, IRubyObject self,
                                       IRubyObject[] args) {
        ByteList scanline1, scanline2;
        double ty;
        int width, src_width, src_line_size, components;
        byte[] scanline_out;
        
        width = RubyFixnum.num2int(args[2]);
        components = RubyFixnum.num2int(args[4]);
        ty = RubyFixnum.num2int(args[3]);
        
        scanline1 = args[0].convertToString().getByteList();
        scanline2 = args[1].convertToString().getByteList();
        
        src_line_size = scanline1.getRealSize();
        
        src_width = src_line_size / components - 1;

        scanline_out = calc_bilinear(width, src_width, components, ty,
                scanline1.getUnsafeBytes(), scanline2.getUnsafeBytes());
        
        return(new RubyString(context.getRuntime(), context.getRuntime().getString(), scanline_out));
    }
    
    private static byte[] calc_bilinear(int width, int src_width,
            int components, double ty, byte[] scanline1, byte[] scanline2) {
        byte[] dest_sl;
        double width_ratio_inv, sample_x, tx, _tx, p00, p10, p01, p11;
        int c0, c1, dest_pos, sample_x_i;
        short c00, c10, c01, c11;
        
        dest_sl = new byte[width * components];
        width_ratio_inv = (double)src_width / width;
        
        dest_pos = 0;
        for (int i = 0; i < width; i++) {
            sample_x = i * width_ratio_inv;
            sample_x_i = (int)sample_x;
            
            tx = sample_x - sample_x_i;
            _tx = 1 - tx;
            
            p11 = tx * ty;
            p01 = _tx * ty;
            p10 =  tx - p11;
            p00 = _tx - p01;

            c0 = sample_x_i * components;
            c1 = c0 + components;
            
/*            if (i == 0) {
                System.out.println(
                    "width_ratio_inv: " + width_ratio_inv +
                    "width: " + width +
                    ", src_width: " + src_width +
                    ", components: " + components +
                    ", ty: " + ty +
                    ", sample_x: " + sample_x +
                    ", sample_x_i: " + sample_x_i +
                    ", tx: " + tx +
                    ", p11: " + p11 + ", p01: " + p01 +
                    ", p10: " + p10 + ", p00: " + p00
                );
                
            }
*/            
            for (int j = 0; j < components; j++) {
                c00 = (short)(0x000000FF & (int)scanline1[c0 + j]);
                c10 = (short)(0x000000FF & (int)scanline1[c1 + j]);
                c01 = (short)(0x000000FF & (int)scanline2[c0 + j]);
                c11 = (short)(0x000000FF & (int)scanline2[c1 + j]);

                if (i == 0) {
//                    System.out.println("c00: " + c00 + ", p11: " + p11 + ", p01: " + p01 + ", p10: " + p10 + ", p00: " + p00);
                }

                if (i == 0) {
                    System.out.println("calculated: " + (p00 * c00 + p10 * c10 + p01 * c01 + p11 * c11));
                }

//                dest_sl[dest_pos] = p00 * c00 + p10 * c10 + p01 * c01 + p11 * c11;
                dest_sl[dest_pos] = (byte)(((short)(p00 * c00 + p10 * c10 + p01 * c01 + p11 * c11)) & 0xFF);
                dest_pos += 1;
            }
        }
        
        return dest_sl;
        
    }
    
    @JRubyMethod(meta = true)
    public static IRubyObject nearest(ThreadContext context, IRubyObject self,
                                      IRubyObject rb_scanline,
                                      IRubyObject rb_width,
                                      IRubyObject rb_components) {
        ByteList scanline_in;
        byte[] scanline_out;
        int width, src_width, src_line_size, components;
        
        width = RubyFixnum.num2int(rb_width);
        components = RubyFixnum.num2int(rb_components);
        
        scanline_in = rb_scanline.convertToString().getByteList();
        src_line_size = scanline_in.getRealSize();
        
        src_width = src_line_size / components;
        scanline_out = calc_nearest(width, src_width, components, scanline_in.getUnsafeBytes());
        
        return(new RubyString(context.getRuntime(), context.getRuntime().getString(), scanline_out));
    }
    
    private static byte[] calc_nearest(int width, int src_width,
                                       int components, byte[] scanline) {
        double inv_scale_x;
        byte[] dest_sl;
        int src_pos, dest_pos, i, j;
        
        inv_scale_x = (double)src_width / width;
        dest_sl = new byte[width * components];
        
        dest_pos = 0;
        for (i = 0; i < width; i++) {
            src_pos = (int)(i * inv_scale_x) * components;
            for (j = 0; j < components; j++) {
                dest_sl[dest_pos] = scanline[src_pos + j];
                dest_pos += 1;
            }
        }
        
        return dest_sl;
    }
    
    static void initInterpolation(Ruby runtime) {
        RubyModule axon = runtime.defineModule("Axon");
        RubyModule interpolation = axon.defineModuleUnder("Interpolation");
        interpolation.defineAnnotatedMethods(Interpolation.class);        
    }
}
