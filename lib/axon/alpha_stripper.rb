module Axon

  # == Strips the Alpha Channel from an Image
  #
  # Axon::AlphaStripper Removes the Alpha Channel from an Image.
  #
  # == Example
  #
  #
  class AlphaStripper

    # :call-seq:
    #   AlphaStripper.new(image_in)
    #
    # Removes the alpha channel from +image_in+.
    #
    def initialize(source)
      @source = source
    end

    # Gets the height of the image. Same as the height of the source image.
    #
    def height
      @source.height
    end

    # Gets the width of the image. Same as the width of the source image.
    #
    def width
      @source.width
    end

    # Gets the components in the image.
    #
    def components
      case @source.components
      when 2 then 1
      when 4 then 3
      else @source.components
      end
    end

    # Gets the color model of the image. Same as the color model of the source
    # image.
    #
    def color_model
      @source.color_model
    end

    # Gets the line number of the next scanline.
    #
    def lineno
      @source.lineno
    end

    # Gets the next scanline from the image.
    #
    def gets
      sl = @source.gets
      return unless sl

      # This should probably be done in C, but not seeing alpha stripping
      # performance as terribly important right now.
      case @source.components
      when 2
        (sl.size / 2).times{ |i| sl.slice!(1) }
        sl
      when 4
        (sl.size / 4).times{ |i| sl.slice!(3) }
        sl
      else
        sl
      end
    end
  end
end
