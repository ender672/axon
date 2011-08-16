module Axon
  class Fit
    include Image
    include Enumerable

    def initialize(source, width, height)
      @source, @fit_width, @fit_height = source, width, height
    end

    def components
      @source.components
    end

    def color_model
      @source.color_model
    end

    def width
      @source.width * calc_fit_ratio
    end

    def height
      @source.height * calc_fit_ratio
    end

    def each
      r = calc_fit_ratio

      if r > 1
        scaler = NearestNeighborScaler.new(@source, r)
        scaler.each{ |*a| yield(*a) }
      elsif r < 1
        if r <= 0.5 && @source.kind_of?(JPEGReader)
          @source.scale_denom = calc_jpeg_pre_shrink(r)
          r = calc_fit_ratio
        end
        scaler = BilinearScaler.new(@source, r)
        scaler.each{ |*a| yield(*a) }
      else
        @source.each{ |*a| yield(*a) }
      end
    end

    private

    def calc_fit_ratio
      width_ratio = @fit_width.to_f / @source.width
      height_ratio = @fit_height.to_f / @source.height
      [width_ratio, height_ratio].min
    end

    def calc_jpeg_pre_shrink(r)
      case (1/r)
      when (0...2) then nil
      when (2...4) then 2
      when (4...8) then 4
      else 8
      end
    end
  end

  module Image
    def fit(*args)
      Fit.new(self, *args)
    end
  end
end
