require "helper"

module Axon
  class TestGenerators < AxonTestCase
    def test_solid_color
      g = Solid.new 10, 15, "\x0A\x14\x69"
      sl = g.gets
      assert_equal "\x0A\x14\x69", sl[9, 3]
    end

    def test_solid_dimensions
      g = Solid.new(23, 33)
      assert_image_dimensions(g, 23, 33)
    end

    def test_noise_dimensions
      im = Noise.new(100, 200, :components => 1, :color_model => :GRAYSCALE)
      assert_image_dimensions(im, 100, 200)
    end
  end
end
