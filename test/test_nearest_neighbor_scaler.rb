require 'helper'
require 'scaler_tests'

module Axon
  class TestNearestNeighborScaler < AxonTestCase
    include ScalerTests

    def setup
      super
      @scalerclass = NearestNeighborScaler
      @scalertestclass = TestNearestNeighborScaler
    end

    class TestNearestNeighborScaler
      def initialize(original, scale)
        @original = original
        @scale = scale.to_f
        @original_data = @original.to_a
      end

      def calc(x, y)
        smp_x = (x / @scale).floor
        smp_y = (y / @scale).floor
        cmp = @original.components
        @original_data[smp_y][smp_x * cmp, cmp].chars.map{ |c| c.ord }
      end
    end
  end
end
