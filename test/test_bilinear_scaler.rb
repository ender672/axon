require 'helper'

module Axon
  class TestBilinearScaler < AxonTestCase
    def test_scaling
      @image.scale_bilinear(0.7).write_jpeg(@io_out)
    end
  end
end
