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

    def test_jpeg_pre_scale_one_half
      skip "JRuby's JPEG decoder doesn't pre-scale" if(RUBY_PLATFORM =~ /java/)
      io = StringIO.new
      JPEG.write(Solid.new(10, 20), io)
      io.rewind
      im = JPEG::Reader.new(io)
      width = im.width / 2
      height = im.height / 2

      r = Fit.new(im, width, height)
      assert_image_dimensions(r, width, height)

      if JPEG::LIB_VERSION >= 70
        assert_equal(4, im.scale_num)
      else
        assert_equal(2, im.scale_denom)
      end
    end

    def test_jpeg_pre_scale_two
      skip "JRuby's JPEG decoder doesn't pre-scale" if(RUBY_PLATFORM =~ /java/)
      io = StringIO.new
      JPEG.write(Solid.new(10, 20), io)
      io.rewind
      im = JPEG::Reader.new(io)
      width = im.width * 2
      height = im.height * 2

      r = Fit.new(im, width, height)
      assert_image_dimensions(r, width, height)

      if JPEG::LIB_VERSION >= 70
        assert_equal(16, im.scale_num)
      else
        assert_equal(1, im.scale_denom)
      end
    end
  end
end
