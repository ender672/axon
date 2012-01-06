require 'axon/scalers'

module Axon

  # == An Image Box Scaler
  #
  # Axon::Fit will scale images to fit inside given box dimensions while
  # maintaining the aspect ratio.
  #
  # == Example
  #
  #   image_in = Axon::Solid.new(10, 20)
  #   f = Axon::Fit.new(image_in, 5, 20)
  #   f.width  # => 5
  #   f.height # => 10
  #
  class Fit
    # :call-seq:
    #   Fit.new(image_in, width, height)
    #
    # Fits +image_in+ in the box dimensions given by +width+ and +height+. The
    # resulting image will not extend beyond the given +width+ or the given
    # +height+. The resulting image will maintain the aspect ratio of +image_in+
    # so the resulting image may not completely fill +width+ and +height+.
    #
    # The resulting image will match either +width+ or +height+.
    #
    def initialize(source, width, height)
      @source, @fit_width, @fit_height = source, width, height
      @scaler = nil
    end

    # Gets the components in the fitted image. Same as the components of the
    # source image.
    #
    def components
      @source.components
    end

    # Gets the color model of the fitted image. Same as the color model of the
    # source image.
    #
    def color_model
      @source.color_model
    end

    # Gets the width of the fitted image. This will be the given width or less.
    #
    def width
      @scaler ? @scaler.width : calculate_width
    end

    # Gets the height of the fitted image. This will be the given height or
    # less.
    #
    def height
      @scaler ? @scaler.height : calculate_height
    end

    # Gets the index of the next line that will be fetched by gets, starting at
    # 0.
    #
    def lineno
      @scaler ? @scaler.lineno : 0
    end

    # Gets the next scanline from the fitted image.
    #
    def gets
      @scaler ||= get_scaler
      @scaler.gets
    end

    private

    def calculate_width
      (@source.width * calc_fit_ratio).to_i
    end

    def calculate_height
      (@source.height * calc_fit_ratio).to_i
    end

    def get_scaler
      r = calc_fit_ratio

      if r > 1
        NearestNeighborScaler.new(@source, width, height)
      elsif r < 1
        if r <= 0.5 && @source.kind_of?(JPEG::Reader)
          @source.scale_denom = calc_jpeg_pre_shrink(r)
          r = calc_fit_ratio
        end
        BilinearScaler.new(@source, width, height)
      else
        @source
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
end