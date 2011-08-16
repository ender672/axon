require 'helper'

module Axon
  class TestNearestNeighborScaler < AxonTestCase
    def test_resize_ratio
      @image.scale_nearest_neighbor(0.9).write_jpeg(@io_out)
    end

    def test_resize_dimensions
      @image.scale_nearest_neighbor(100, 200).write_jpeg(@io_out)
    end
  end
end
