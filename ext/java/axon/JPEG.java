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

public class JPEG {
    
    /*
     * OpenJDK (IcedTea6 1.9.10), has an implementation of JPEGImageReader that
     * loads the entire image into memory via read() before compressing it. Not
     * sure why it does this -- the PNGImageReader is much nicer and only
     * requests one scanline at a time.
     * 
     * Be warned -- large images can take up lots of memory here.
     */
    
    @JRubyMethod(meta = true, required = 2, optional = 1)
    public static IRubyObject write(ThreadContext context, IRubyObject self,
                                   IRubyObject[] args) throws IOException {
        IRubyObject img_in, ruby_io;
        Iterator writers = ImageIO.getImageWritersByFormatName("jpeg");
        ImageWriter writer = (ImageWriter)writers.next();
        img_in = args[0];
        ruby_io = args[1];
        IOOutputStream jruby_io = new IOOutputStream(ruby_io, false, false);
        ImageOutputStream ios = ImageIO.createImageOutputStream(jruby_io);
        writer.setOutput(ios);
        RubyImage img = new RubyImage(img_in);
        writer.write(img);
        ios.flush();
        
        return context.getRuntime().newFixnum(ios.getStreamPosition());
    }

    static void initJPEG(Ruby runtime) {
        RubyModule axon = runtime.defineModule("Axon");
        RubyModule jpeg = axon.defineModuleUnder("JPEG");
        jpeg.defineAnnotatedMethods(JPEG.class);
    }
}
