module Axon
  class NearestNeighborScaler < Scaler
    include NearestNeighborScaling
    include Image
    include Enumerable

    def each
      dest_y = 0
      sample_y = 0

      @image.each_with_index do |scanline, source_y|
        while sample_y == source_y
          yield interpolate_scanline(scanline)
          dest_y += 1
          break if dest_y >= @height
          sample_y = (dest_y / @height_ratio).floor
        end
      end
    end
  end

  module Image
    def scale_nearest_neighbor(*args)
      NearestNeighborScaler.new(self, *args)
    end
  end
end
