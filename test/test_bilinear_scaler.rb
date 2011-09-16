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
      def initialize(original, scale)
        @original = original
        @scale = scale.to_f

        # pad right and bottom with one pixel
        @original_data = @original.to_a.map{ |sl| sl + sl[-3, 3] }
        @original_data << @original_data.last
      end

      def calc(x, y)
        sample_x = x / @scale
        sample_y = y / @scale

        sample_x_i = sample_x.floor
        sample_y_i = sample_y.floor

        tx = sample_x - sample_x_i
        ty = sample_y - sample_y_i

        ret = []

        cmp = @original.components

        cmp.times do |i|
          smp_x = sample_x_i * @original.components + i

          c00 = @original_data[sample_y_i][smp_x].ord
          c10 = @original_data[sample_y_i][smp_x + cmp].ord
          c01 = @original_data[sample_y_i + 1][smp_x].ord
          c11 = @original_data[sample_y_i + 1][smp_x + cmp].ord

          a = (1 - tx) * c00 + tx * c10
          b = (1 - tx) * c01 + tx * c11
          ret << ((1 - ty) * a + ty * b).floor
        end

        ret
      end
    end
  end
end
