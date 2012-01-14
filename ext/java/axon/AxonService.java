package axon;

import org.jruby.Ruby;
import org.jruby.runtime.load.BasicLibraryService;

public class AxonService implements BasicLibraryService {
    public boolean basicLoad(Ruby runtime) {
        PNG.initPNG(runtime);
        PNGReader.initPNGReader(runtime);
        JPEG.initJPEG(runtime);
        JPEGReader.initJPEGReader(runtime);
        Interpolation.initInterpolation(runtime);
        return true;
    }
}
