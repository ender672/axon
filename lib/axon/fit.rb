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
      @aspect_ratio = width / height.to_f
      @scaler = nil
    end

    # Gets the components in the fitted image. Same as the components of the
    # source image.
    #
    def components
      @source.components
    end

    # Gets the width of the fitted image. This will be the given width or less.
    #
    def width
      @scaler ? @scaler.width : (@source.width * calc_fit_ratio).to_i
    end

    # Gets the height of the fitted image. This will be the given height or
    # less.
    #
    def height
      @scaler ? @scaler.height : (@source.height * calc_fit_ratio).to_i
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

    def get_scaler
      r = calc_fit_ratio
      return @source if r == 1
      
      final_width = (r * @source.width).to_i
      final_height = (r * @source.height).to_i

      if @source.kind_of?(JPEG::Reader)
        jpeg_scale_dct(r)
        r = calc_fit_ratio
        return @source if r == 1
      end

      if r > 1
        NearestNeighborScaler.new(@source, final_width, final_height)
      elsif r < 1
        BilinearScaler.new(@source, final_width, final_height)
      end
    end

    def source_aspect_ratio
      @source.width / @source.height.to_f
    end

    def calc_fit_ratio
      if source_aspect_ratio > @aspect_ratio
        @fit_width / @source.width.to_f
      else
        @fit_height / @source.height.to_f
      end
    end

    # Some versions of libjpeg can perform DCT scaling during the jpeg decoding
    # phase. This is fast and accurate scaling, so we want to take advantage of
    # it if at all possible.
    #
    # Since this form of scaling only happens in increments, we probably won't
    # be able to scale to the exact desired size, so our strategy is to scale to
    # as close to the desired size as possible without scaling too much.
    #
    # This depends on our version of libjpeg:
    #   * libjpeg version 7 and greater can scale N/8 with all N from 1 to 16.
    #   * libjpeg version 6 and below can scale 1/N with all N from 1 to 8.
    def jpeg_scale_dct(r)
      if JPEG::LIB_VERSION >= 70
        # when shrinking, we want scale_num to be the next highest integer
        if r < 1
          @source.scale_num = (r * 8).ceil
        # when growing, we want scale_num to be the next lowest integer
        else
          @source.scale_num = (r * 8).to_i
        end
      else
        if r <= 0.5
          @source.scale_denom = case (1/r).to_i
          when 2,3     then 2
          when 4,5,6,7 then 4
          else              8
          end
        end
      end
    end
  end
end