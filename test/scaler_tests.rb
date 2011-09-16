module Axon
  module ScalerTests
    def test_dimensions
      [
        [7, 8],
        [71, 82],
        [@image.width, @image.height],
        [1, 1]
      ].each do |dims|
        w, h = dims[0], dims[1]

        y = 0
        @scalerclass.new(@image, w, h).each do |sl|
          assert_equal w * @image.components, sl.size
          y += 1
        end

        assert_equal h, y
      end
    end

    def test_bad_dimensions
      [ [0, 0], [0, 1], [1, 0], [-1, -3] ].each do |dims|
        w, h = dims[0], dims[1]

        assert_raises ArgumentError do
          @scalerclass.new(@image, w, h).each{ |sl| }
        end
      end
    end

    def test_scalar
      [1, 2, 3, 1.1, 2.1, 0.9, 0.1].each do |scale|
        y = 0
        expected_width = @image.components * (@image.width * scale).to_i

        @scalerclass.new(@image, scale).each do |sl|
          assert_equal expected_width, sl.size
          y += 1
        end
        assert_equal((@image.height * scale).floor, y)
      end
    end

    def test_bad_scalar
      [0, 0.0, -1, -0.1].each do |scale|
        assert_raises ArgumentError do
          @scalerclass.new(@image, scale).each{ |sl| }
        end
      end
    end

    def scale_test(*coords)
      noise = Noise.new 100, 200, :seed => 1

      [1.0, 5.0, 0.3, 2.8].each do |scale|
        test_scaler = @scalertestclass.new(noise, scale)
        resized = @scalerclass.new(noise, scale).to_a

        right = (noise.width * scale).floor - 1
        bottom = (noise.height * scale).floor - 1

        coords.each do |coord|
          x = coord[0]
          y = coord[1]

          x = right if x < 0
          y = bottom if y < 0

          result = resized[y][x * 3, 3].chars.map{ |c| c.ord }

          assert_equal test_scaler.calc(x, y), result
        end
      end
    end

    def test_pixel_values
      scale_test [10, 10], [29, 20], [9, 15]
    end

    def test_edge_pixel_values
      scale_test [0, 10], [29, 0], [-1, 15], [17, -1]
    end

    def test_corner_pixel_values
      scale_test [0, 0], [-1, 0], [0, -1], [-1, -1]
    end
  end
end
