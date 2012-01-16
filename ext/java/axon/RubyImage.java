package axon;

import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Transparency;
import java.awt.color.ColorSpace;
import java.awt.color.ICC_ColorSpace;
import java.awt.color.ICC_Profile;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.ColorModel;
import java.awt.image.ComponentColorModel;
import java.awt.image.Raster;
import java.awt.image.RenderedImage;
import java.awt.image.SampleModel;
import java.awt.image.BandedSampleModel;
import java.awt.image.WritableRaster;
import java.io.ByteArrayOutputStream;
import java.util.Vector;

import org.jruby.Ruby;
import org.jruby.RubyFixnum;
import org.jruby.RubyString;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

public class RubyImage implements RenderedImage {
    private IRubyObject rb_image;
    
    public RubyImage(IRubyObject img) {
        rb_image = img;
    }

    public WritableRaster copyData(WritableRaster raster) {
        throw new UnsupportedOperationException("copyData");
    }
    
    public ColorModel getColorModel() {
        int components, cs_type;
        boolean has_alpha;
        
        components = components();

        if (components == 1 || components == 2)
            cs_type = ColorSpace.CS_GRAY;
        else if (components == 3 || components == 4)
            cs_type = ColorSpace.CS_sRGB;
        else
            cs_type = ColorSpace.TYPE_RGB; // FIXME: Raise an exception instead
        
        has_alpha = components == 2 || components == 4;
        
        return(new ComponentColorModel(ColorSpace.getInstance(cs_type),
                has_alpha, false, Transparency.OPAQUE, DataBuffer.TYPE_BYTE));
    }
    
    public ColorModel getICCColorModel(byte[] data) {
        int components;
        ICC_ColorSpace cs;
        boolean has_alpha;
        
        components = components();

        cs = new ICC_ColorSpace(ICC_Profile.getInstance(data));
        
        has_alpha = components == 2 || components == 4;
        
        return(new ComponentColorModel(cs, has_alpha, false,
                Transparency.OPAQUE, DataBuffer.TYPE_BYTE));
    }
    
    public Raster getTile(int tileX, int tileY) {
        throw new UnsupportedOperationException("getTile");        
    }

    public Raster getData() {
        return getData(new Rectangle(getWidth(), getHeight()));
    }

    public Raster getData(Rectangle rect) {
        DataBuffer dataBuffer;
        int w, h, numbands, lineno, sl_size;
        RubyString sl;
        ByteArrayOutputStream stream;
        byte[] sl_bytes;
        int[] bandOffsets;
        Raster raster;
        
        w = getWidth();
        h = getHeight();
        numbands = components();
        lineno = lineno();
        sl_size = w * numbands;
        
        bandOffsets = new int[numbands];
        for (int i=0; i<numbands; i++)
            bandOffsets[i] = i;
        
        if (rect.height < 1 || rect.width < 1)
            throw runtime().newRuntimeError("Requested image region has invalid size.");

        stream = new ByteArrayOutputStream(rect.height * rect.width);
        
        for (int j=0; j<rect.height; j++) {
            sl = RubyString.objAsString(context(), callMethod("gets"));
            sl_bytes = sl.getBytes();
            
            if(sl_bytes.length != sl_size)
                throw runtime().newRuntimeError("Image returned a scanline with a bad size.");
            
            try {
                stream.write(sl_bytes, 0, sl_size);
            }
            catch(IndexOutOfBoundsException iob) {
                throw runtime().newRuntimeError("An index out of bounds error occurred while writing.");
            }
        }

        dataBuffer = new DataBufferByte(stream.toByteArray(), stream.size());
        try {
            raster = Raster.createInterleavedRaster(dataBuffer, w, rect.height,
                sl_size, numbands, bandOffsets, new Point(0, lineno));
        }
        catch(IllegalArgumentException iae) {
            throw runtime().newRuntimeError("An argument exception occurred while preparing image data.");
        }
        
        return(raster);
    }
    
    public Vector<RenderedImage> getSources() {
        throw new UnsupportedOperationException("getSources");
    }

    public Object getProperty(String name) {
        throw new UnsupportedOperationException("getProperty");
    }

    public String[] getPropertyNames() {
        throw new UnsupportedOperationException("getPropertyNames");
    }

    public SampleModel getSampleModel() {
        int w;
        w = getWidth();
        return new BandedSampleModel(DataBuffer.TYPE_BYTE, w, 1, components());
    }

    public int getWidth() {
        return RubyFixnum.num2int(callMethod("width"));
    }

    public int getHeight() {
        return RubyFixnum.num2int(callMethod("height"));
    }

    public int getMinX() {
        return 0;
    }

    public int getMinY() {
        return lineno();
    }

    public int getNumXTiles() {
        return 1;
    }

    public int getNumYTiles() {
        return getHeight();
    }

    public int getMinTileX() {
        return 1;
    }

    public int getMinTileY() {
        return getMinY();
    }

    public int getTileWidth() {
        throw new UnsupportedOperationException("getTileWidth");
    }

    public int getTileHeight() {
        throw new UnsupportedOperationException("getTileHeight");
    }

    public int getTileGridXOffset() {
        throw new UnsupportedOperationException("getTileGridXOffset");
    }

    public int getTileGridYOffset() {
        throw new UnsupportedOperationException("getTileGridYOffset");
    }
    
    private int lineno() {
        return RubyFixnum.num2int(callMethod("lineno"));
    }
    
    private int components() {
        return RubyFixnum.num2int(callMethod("components"));
    }

    private IRubyObject callMethod(String name) {
        return rb_image.callMethod(context(), name);
    }
    
    private ThreadContext context() {
        return runtime().getCurrentContext();
    }
    
    private Ruby runtime() {
        return rb_image.getRuntime();
    }
}
