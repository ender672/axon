module Axon
  class BilinearScaler < Scaler
    include BilinearScaling
    include Image
    include Enumerable

    def each
      source_y = 0
      dest_y = 0
      sample_y = 0

      each_scanline_with_next do |sl1, sl2|
        while sample_y.to_i == source_y
          yield interpolate_scanline(sl1, sl2, sample_y)
          dest_y += 1
          break if dest_y >= @height
          sample_y = dest_y / @height_ratio
        end
        
        source_y += 1
      end
    end

    private

    # Yields each scanline with the following scanline. The final scanline is
    # yielded with itself.

    def each_scanline_with_next
      last_sl = nil

      each_scanline_with_padding do |scanline|
        yield last_sl, scanline if last_sl
        last_sl = scanline
      end
    end

    # Adds a one pixel padding to the right of the image and one pixel of
    # padding to the bottom by duplicating the last pixels.

    def each_scanline_with_padding
      last = nil
      components = @image.components

      @image.each do |scanline|
        scanline << scanline[-components, components] # bonus pixel
        yield scanline
        last = scanline
      end

      yield last # bonus scanline
    end
  end
  
  module Image
    def scale_bilinear(*args)
      BilinearScaler.new(self, *args)
    end
  end
end
