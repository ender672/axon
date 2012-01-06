require 'helper'

module Axon
  class TestFit < AxonTestCase
    def test_width_determines_downscaling
      im = Solid.new(10, 20)
      r = Fit.new(im, 5, 20)
      assert_image_dimensions(r, 5, 10)
    end

    def test_height_determines_downscaling
      im = Solid.new(10, 20)
      r = Fit.new(im, 20, 5)
      assert_image_dimensions(r, 2, 5)
    end

    def test_width_determines_upscaling
      im = Solid.new(10, 20)
      r = Fit.new(im, 100, 900)
      assert_image_dimensions(r, 100, 200)
    end

    def test_height_determines_upscaling
      im = Solid.new(10, 20)
      r = Fit.new(im, 1000, 200)
      assert_image_dimensions(r, 100, 200)
    end

    def test_dimensions_unchanged
      im = Solid.new(10, 20)
      r = Fit.new(im, 10, 20)
      assert_image_dimensions(r, 10, 20)
    end
  end
end
