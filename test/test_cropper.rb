require 'helper'

module Axon
  class TestCropper < AxonTestCase
    def test_crop
      i = Solid.new(200, 100)
      c = Cropper.new(i, 4, 55)

      assert_equal 4, c.width
      assert_equal 55, c.height

      55.times do |i|
        assert_equal i, c.lineno
        assert_equal 4 * c.components, c.gets.size
      end

      assert_equal nil, c.gets
      assert_equal 55, c.lineno
    end

    def test_crop_too_tall
      i = Solid.new(200, 100)
      c = Cropper.new(i, 4, 133)
      assert_image_dimensions(c, 4, 100)
    end

    def test_crop_too_wide
      i = Solid.new(200, 100)
      c = Cropper.new(i, 233, 33)
      assert_image_dimensions(c, 200, 33)
    end

    def test_crop_negative_values
      i = Solid.new(200, 100)

      assert_raises(ArgumentError) do
        Cropper.new(i, -233, 33)
      end

      assert_raises(ArgumentError) do
        Cropper.new(i, 100, -33)
      end

      assert_raises(ArgumentError) do
        Cropper.new(i, 0, 23)
      end
    end

    def test_crop_with_offset
      i = Noise.new(100, 200)

      data = []
      i.height.times{ data << i.gets }
      c = Cropper.new(ArrayWrapper.new(data), 42, 51, 22, 31)

      assert_equal 42, c.width
      assert_equal 51, c.height

      51.times do |i|
        assert_equal i, c.lineno
        sl = c.gets
        assert_equal 42 * c.components, sl.size
        assert_equal data[i + 31][22 * c.components], sl[0]
      end

      assert_equal nil, c.gets
      assert_equal 51, c.lineno
    end

    def test_crop_with_offset_oob
      i = Solid.new(200, 100)
      c = Cropper.new(i, 30, 40, 220, 10)
      assert_nil c.gets
    end
  end
end
