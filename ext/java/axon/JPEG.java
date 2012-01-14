package axon;

import java.io.IOException;
import java.util.Iterator;

import javax.imageio.ImageIO;
import javax.imageio.IIOImage;
import javax.imageio.IIOException;
import javax.imageio.ImageWriteParam;
import javax.imageio.ImageWriter;
import javax.imageio.stream.ImageOutputStream;

import org.jruby.Ruby;
import org.jruby.RubyFixnum;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubySymbol;
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
        IRubyObject img_in, ruby_io, rb_quality;
        RubyHash options;
        Iterator writers;
        ImageWriter writer;
        IOOutputStream jruby_io;
        ImageOutputStream ios;
        RubyImage img;
        ImageWriteParam iwp;
        int quality;
        
        writers = ImageIO.getImageWritersByFormatName("jpeg");
        writer = (ImageWriter)writers.next();
        img_in = args[0];
        ruby_io = args[1];
        rb_quality = null;
        
        if (args.length > 2) {
            options = (RubyHash)args[2];
            rb_quality = options.fastARef(RubySymbol.newSymbol(context.getRuntime(),
                    "quality"));
        }
        
        jruby_io = new IOOutputStream(ruby_io, false, false);
        ios = ImageIO.createImageOutputStream(jruby_io);
        writer.setOutput(ios);
        img = new RubyImage(img_in);
        iwp = writer.getDefaultWriteParam();

        if (rb_quality != null && !rb_quality.isNil()) {
            quality = RubyFixnum.num2int(rb_quality);
            if (quality < 1)
                quality = 1;
            else if (quality > 100)
                quality =  100;
            iwp.setCompressionMode(ImageWriteParam.MODE_EXPLICIT);
            iwp.setCompressionQuality(quality / 100.0f);
        }
        
        try {
            writer.write(null, new IIOImage(img, null, null), iwp);
//            writer.write(img);
        }
        catch(NegativeArraySizeException nas) {
            throw context.getRuntime().newRuntimeError("An exception occurred while writing.");            
        }
        catch(IIOException iioe) {
            throw context.getRuntime().newRuntimeError("An exception occurred while writing.");            
        }
        ios.flush();
        
        return context.getRuntime().newFixnum(ios.getStreamPosition());
    }

    static void initJPEG(Ruby runtime) {
        RubyModule axon = runtime.defineModule("Axon");
        RubyModule jpeg = axon.defineModuleUnder("JPEG");
        jpeg.defineAnnotatedMethods(JPEG.class);
    }
}
