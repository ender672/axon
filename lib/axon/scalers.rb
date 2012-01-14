module Axon
  # == A Nearest-neighbor Image Scaler
  #
  # Axon::NearestNeighborScaler scales images quickly using the nearest-neighbor
  # interpolation method.
  #
  # Nearest-neighbor interpolation selects the value of the nearest pixel when
  # calculating colors in the scaled image.
  #
  # == Example
  #
  #   n = Axon::NearestNeighborScaler.new(image_in, 50, 75)
  #   n.width  # => 50
  #   n.height # => 75
  #   n.gets   # => String
  #
  class NearestNeighborScaler
    # The width of the generated image.
    attr_reader :width

    # The height of the generated image.
    attr_reader :height

    # The index of the next line that will be fetched by gets, starting at 0.
    attr_reader :lineno

    # :call-seq:
    #   NearestNeighborScaler.new(image_in, width, height)
    #
    # Scales +image_in+ to the size +width+ x +height+ using the
    # nearest-neighbor interpolation method.
    #
    def initialize(source, width, height)
      raise ArgumentError if width < 1 || height < 1
      @width = width
      @height = height
      @source = source
      @lineno = 0
      @buf = nil
    end

    # Gets the components in the scaled image. Same as the components of the
    # source image.
    #
    def components
      @source.components
    end

    # Gets the next scanline from the cropped image.
    #
    def gets
      return nil if @lineno >= @height
      sample = (@lineno * @source.height / @height.to_f).floor
      @lineno += 1
      Interpolation.nearest(get_buf(sample), @width, components)
    end

    private

    def get_buf(line)
      @buf ||= @source.gets
      (line + 1 - @source.lineno).times{ @buf = @source.gets }
      @buf
    end
  end

  # == A Bilinear Image Scaler
  #
  # Axon::BilinearScaler scales images using the bilinear interpolation method.
  #
  # Bilinear interpolation calculates the color values in the resulting image by
  # looking at the four nearest pixels for each pixel in the resulting image.
  #
  # This gives a more accurate representation than nearest-neighbor
  # interpolation, at the expense of slightly blurring the resulting image.
  #
  # == Example
  #
  #   n = Axon::BilinearScaler.new(image_in, 50, 75)
  #   n.width  # => 50
  #   n.height # => 75
  #   n.gets   # => String
  #
  class BilinearScaler
    # The width of the generated image.
    attr_reader :width

    # The height of the generated image.
    attr_reader :height

    # The index of the next line that will be fetched by gets, starting at 0.
    attr_reader :lineno

    # :call-seq:
    #   BilinearScaler.new(image_in, width, height)
    #
    # Scales +image_in+ to the size +width+ x +height+ using the bilinear
    # interpolation method.
    #
    def initialize(source, width, height)
      raise ArgumentError if width < 1 || height < 1
      @width = width
      @height = height
      @source = source
      @lineno = 0
      @buf1 = nil
      @buf2 = nil
    end

    # Gets the components in the scaled image. Same as the components of the
    # source image.
    #
    def components
      @source.components
    end

    # Gets the next scanline from the cropped image.
    #
    def gets
      return nil if @lineno >= @height
      sample = @lineno * @source.height / @height.to_f
      sample_i = sample.to_i
      ty = sample - sample_i
      @lineno += 1
      get_buf(sample_i)

      Interpolation.bilinear(@buf1, @buf2, @width, ty, components)
    end

    private

    def get_buf(line)
      @buf1 = @buf2 = read_with_padding unless @buf2
      (line + 2 - @source.lineno).times do
        @buf1 = @buf2
        @buf2 = read_with_padding if @source.lineno < @source.height
      end
    end

    def read_with_padding
      cmp = @source.components
      line = @source.gets
      line + line[-cmp, cmp]
    end
  end
end
