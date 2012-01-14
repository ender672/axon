require 'helper'

module Axon
  class TestAlphaStripper < AxonTestCase
    def test_strip_grayscale_alpha
      im = Solid.new(20, 30, "\x00\x00", :GRAYSCALE)
      noalpha = AlphaStripper.new(im)
      assert_equal 1, noalpha.components
      assert_image_dimensions noalpha, 20, 30
    end

    def test_strip_rgb_alpha
      im = Solid.new(20, 30, "\x00\x00\x00\x00", :RGB)
      noalpha = AlphaStripper.new(im)
      assert_equal 3, noalpha.components
      assert_image_dimensions noalpha, 20, 30
    end

    def test_leaves_grayscale_untouched
      im = Solid.new(20, 30, "\x00", :GRAYSCALE)
      noalpha = AlphaStripper.new(im)
      assert_equal 1, noalpha.components
      assert_image_dimensions noalpha, 20, 30
    end

    def test_leaves_rgb_untouched
      im = Solid.new(20, 30, "\x00\x00\x00", :RGB)
      noalpha = AlphaStripper.new(im)
      assert_equal 3, noalpha.components
      assert_image_dimensions noalpha, 20, 30
    end
  end
end
