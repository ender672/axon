package axon;

import java.io.IOException;
import java.util.Iterator;

import javax.imageio.ImageIO;
import javax.imageio.ImageWriter;
import javax.imageio.stream.ImageOutputStream;

import org.jruby.Ruby;
import org.jruby.RubyModule;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.IOOutputStream;

public class PNG {
    @JRubyMethod(meta = true)
    public static IRubyObject write(ThreadContext context, IRubyObject self,
                                   IRubyObject img_in, IRubyObject ruby_io) throws IOException {
        Iterator writers = ImageIO.getImageWritersByFormatName("png");
        ImageWriter writer = (ImageWriter)writers.next();
        IOOutputStream jruby_io = new IOOutputStream(ruby_io, false, false);
        ImageOutputStream ios = ImageIO.createImageOutputStream(jruby_io);
        writer.setOutput(ios);
        RubyImage img = new RubyImage(img_in);
        try {
            writer.write(img);
        }
        catch(IllegalArgumentException iae) {
            throw context.getRuntime().newRuntimeError("An Illegal Argument exception occurred while writing.");
        }
        catch(ArrayIndexOutOfBoundsException oob) {
            throw context.getRuntime().newRuntimeError("An Out of Bounds exception occurred while writing.");            
        }
        catch(NegativeArraySizeException nas) {
            throw context.getRuntime().newRuntimeError("An exception occurred while writing.");            
        }
        return context.getRuntime().newFixnum(ios.getStreamPosition());
    }

    static void initPNG(Ruby runtime) {
        RubyModule axon = runtime.defineModule("Axon");
        RubyModule png = axon.defineModuleUnder("PNG");
        png.defineAnnotatedMethods(PNG.class);
    }
}
