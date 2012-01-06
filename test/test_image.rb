require 'helper'
require 'tempfile'

module Axon
  class TestImage < AxonTestCase
    def setup
      super
      io = StringIO.new
      JPEG.write(Solid.new(10, 20), io)
      @jpeg_data = io.string

      io = StringIO.new
      PNG.write(Solid.new(10, 20), io)
      @png_data = io.string
    end

    def test_jpeg_helper_io
      image = Axon.jpeg(StringIO.new(@jpeg_data))
      assert_image_dimensions(image, 10, 20)
    end

    def test_jpeg_helper_stringdata
      image = Axon.jpeg(@jpeg_data)
      assert_image_dimensions(image, 10, 20)
    end

    def test_png_helper_io
      image = Axon.png(StringIO.new(@png_data))
      assert_image_dimensions(image, 10, 20)
    end

    def test_jpeg_helper_stringdata
      image = Axon.png(@png_data)
      assert_image_dimensions(image, 10, 20)
    end

    def test_bilinear
      image = Axon.jpeg(@jpeg_data)
      image.scale_bilinear(50, 75)
      assert_image_dimensions(image, 50, 75)
    end

    def test_nearest
      image = Axon.jpeg(@jpeg_data)
      image.scale_nearest(50, 75)
      assert_image_dimensions(image, 50, 75)
    end

    def test_fit
      image = Axon.jpeg(@jpeg_data)
      image.fit(50, 80)
      assert_image_dimensions(image, 40, 80)
    end

    def test_crop
      image = Axon.jpeg(@jpeg_data)
      image.crop(5, 10, 6, 2)
      assert_image_dimensions(image, 4, 10)
    end
  end
end
