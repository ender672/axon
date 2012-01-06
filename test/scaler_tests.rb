module Axon
  module ScalerTests
    def test_dimensions
      [
        [7, 8],
        [71, 82],
        [@image.width, @image.height],
        [1, 1]
      ].each do |dims|
        im = Solid.new(100, 200)

        w, h = dims[0], dims[1]

        s = @scalerclass.new(im, w, h)
        assert_image_dimensions(s, w, h)
      end
    end

    def test_bad_dimensions
      [ [0, 0], [0, 1], [1, 0], [-1, -3] ].each do |dims|
        w, h = dims[0], dims[1]

        assert_raises ArgumentError do
          @scalerclass.new(@image, w, h).gets
        end
      end
    end

    def test_scalar
      [1, 2, 3, 1.1, 2.1, 0.9, 0.1].each do |scale|
        im = Solid.new(100, 200)
        width = (im.width * scale).to_i
        height = (im.height * scale).to_i

        s = @scalerclass.new(im, width, height)
        assert_image_dimensions(s, width, height)
      end
    end

    def test_bad_scalar
      [0, 0.0, -1, -0.1].each do |scale|
        assert_raises ArgumentError do
          @scalerclass.new(@image, scale).gets
        end
      end
    end

    def scale_test(*coords)
      cache = Repeater.new(Noise.new 100, 200)

      [1.0, 5.0, 0.3, 2.8].each do |scale|
        width = (cache.width * scale).to_i
        height = (cache.height * scale).to_i

        cache.rewind
        test_scaler = @scalertestclass.new(cache, width, height)
        cache.rewind
        s = @scalerclass.new(cache, width, height)
        resized = []
        s.height.times{ resized << s.gets }

        right = width - 1
        bottom = height - 1

        coords.each do |coord|
          x = coord[0]
          y = coord[1]

          x = right if x < 0
          y = bottom if y < 0

          result = resized[y][x * 3, 3].chars.map do |c|
            c.respond_to?(:ord) ? c.ord : c[0]
          end

          expected = test_scaler.calc(x, y)
          3.times do |i|
            assert_in_delta expected[i], result[i], 1
          end
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

    def test_small_scaling
      im = Solid.new(10, 20)
      sc = @scalerclass.new(im, 2, 5)
      assert_image_dimensions(sc, 2, 5)
    end
  end
end
