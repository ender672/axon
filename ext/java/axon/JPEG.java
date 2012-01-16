package axon;

import java.io.IOException;
import java.util.Iterator;

import javax.imageio.ImageIO;
import javax.imageio.IIOImage;
import javax.imageio.IIOException;
import javax.imageio.ImageTypeSpecifier;
import javax.imageio.ImageWriteParam;
import javax.imageio.ImageWriter;
import javax.imageio.stream.ImageOutputStream;

import java.awt.image.ColorModel;
import java.awt.image.SampleModel;

import org.jruby.Ruby;
import org.jruby.RubyFixnum;
import org.jruby.RubyHash;
import org.jruby.RubyString;
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
     * buffers one scanline at a time.
     * 
     * Be warned -- large images can take up lots of memory here.
     */
    
    @JRubyMethod(meta = true, required = 2, optional = 1)
    public static IRubyObject write(ThreadContext context, IRubyObject self,
                                   IRubyObject[] args) throws IOException {
        IRubyObject img_in, ruby_io, rb_quality, rb_icc_profile;
        RubyHash options;
        Iterator writers;
        ImageWriter writer;
        IOOutputStream jruby_io;
        ImageOutputStream ios;
        RubyImage img;
        ImageWriteParam iwp;
        ImageTypeSpecifier dest_type;
        ColorModel cm;
        SampleModel sm;
        int quality;
        byte[] icc_profile;
        
        writers = ImageIO.getImageWritersByFormatName("jpeg");
        writer = (ImageWriter)writers.next();
        img_in = args[0];
        ruby_io = args[1];
        rb_quality = null;
        rb_icc_profile = null;
        
        if (args.length > 2) {
            options = (RubyHash)args[2];
            rb_quality = options.fastARef(RubySymbol.newSymbol(context.getRuntime(),
                    "quality"));
            rb_icc_profile = options.fastARef(RubySymbol.newSymbol(context.getRuntime(),
                    "icc_profile"));
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
        
        /* ImageWriteParam -> ImageTypeSpecifier -> ColorModel -> ColorSpace ->
         * ICC_Profile
         */
        if (rb_icc_profile != null && !rb_icc_profile.isNil()) {
            icc_profile = ((RubyString)rb_icc_profile).getBytes();
            cm = img.getICCColorModel(icc_profile);
            sm = cm.createCompatibleSampleModel(img.getWidth(), img.getHeight());
                    
            iwp.setDestinationType(new ImageTypeSpecifier(cm, sm));
//            System.out.println(cs);
//            iwp.setDestinationType()
        }
        
        try {
            writer.write(null, new IIOImage(img, null, null), iwp);
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
