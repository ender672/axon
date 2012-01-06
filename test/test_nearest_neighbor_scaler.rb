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
      def initialize(original, width, height)
        @original = original
        @scale_x_inv = original.width / width.to_f
        @scale_y_inv = original.height / height.to_f
        @data = []
        @original.height.times do
          sl = @original.gets
          @data << sl + sl[-@original.components, @original.components]
        end
        @data << @data.last
      end

      def calc(x, y)
        smp_x = (x * @scale_x_inv).floor
        smp_y = (y * @scale_y_inv).floor
        cmp = @original.components
        @data[smp_y][smp_x * cmp, cmp].chars.map do |c|
          c.respond_to?(:ord) ? c.ord : c[0]
        end
      end
    end
  end
end
