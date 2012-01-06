require 'helper'
require 'scaler_tests'

module Axon
  class TestBilinearScaler < AxonTestCase
    include ScalerTests

    def setup
      super
      @scalerclass = BilinearScaler
      @scalertestclass = TestBilinearScaler
    end

    class TestBilinearScaler
      def initialize(original, width, height)
        @original = original
        @scale_x_inv = original.width / width.to_f
        @scale_y_inv = original.height / height.to_f

        # pad right and bottom with one pixel
        @data = []
        @original.height.times do
          sl = @original.gets
          @data << sl + sl[-@original.components, @original.components]
        end
        @data << @data.last
      end

      def calc(x, y)
        sample_x = x * @scale_x_inv
        sample_y = y * @scale_y_inv

        sample_x_i = sample_x.floor
        sample_y_i = sample_y.floor

        tx = sample_x - sample_x_i
        ty = sample_y - sample_y_i

        ret = []

        cmp = @original.components

        cmp.times do |i|
          smp_x = sample_x_i * @original.components + i

          c00 = @data[sample_y_i][smp_x]
          c10 = @data[sample_y_i][smp_x + cmp]
          c01 = @data[sample_y_i + 1][smp_x]
          c11 = @data[sample_y_i + 1][smp_x + cmp]

          unless RUBY_VERSION == '1.8.7'
            c00 = c00.ord
            c10 = c10.ord
            c01 = c01.ord
            c11 = c11.ord
          end

          a = (1 - tx) * c00 + tx * c10
          b = (1 - tx) * c01 + tx * c11
          ret << ((1 - ty) * a + ty * b).floor
        end

        ret
      end
    end
  end
end
