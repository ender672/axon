module Axon
  class NearestNeighborScaler < Scaler
    include NearestNeighborScaling
    include Image
    include Enumerable

    def each
      dest_y = 0
      sample_y = 0

      @image.each_with_index do |scanline, orig_y|
        while sample_y == orig_y
          yield interpolate_scanline(scanline)
          dest_y += 1
          sample_y = (dest_y / @height_ratio).to_i
        end
      end
    end
    
    private
    
    def _interpolate_scanline(scanline)
      dest_sl = ''

      scanline.size.times do |dest_x|
        sample_x = (dest_x / @width_ratio).to_i
        dest_sl << scanline[sample_x * components, components]
      end

      return dest_sl      
    end
  end

  module Image
    def scale_nearest_neighbor(*args)
      NearestNeighborScaler.new(self, *args)
    end
  end
end
