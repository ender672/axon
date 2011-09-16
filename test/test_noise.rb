require "helper"

module Axon
  class TestNoise < AxonTestCase
    def test_dimensions
      im = Noise.new(100, 200, :components => 1, :color_model => :GRAYSCALE)
      assert_equal 100, im.width
      assert_equal 200, im.height

      y = 0
      im.each do |sl|
        assert_equal 1 * 100, sl.size
        y += 1
      end

      assert_equal 200, y

      im.write_png(File.open("noise.png", "w"))
    end

    def test_random
      im = Noise.new(100, 200)
      last_sl = nil

      im.each do |sl|
        refute_equal sl[0], sl[1]
        refute_equal sl[0], sl[3]

        if last_sl
          refute_equal last_sl, sl
          break
        else
          last_sl = sl
        end
      end
    end
  end
end
