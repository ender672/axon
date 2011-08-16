require 'helper'

module Axon
  class TestCropper < AxonTestCase
    def test_crop
      @image.scale_nearest_neighbor(40).crop(320, 123).write_jpeg(@io_out)
    end
  end
end
