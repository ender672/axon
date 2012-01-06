module Axon

  # == An Image Cropper
  #
  # Axon::Crop allows you to crop images and extract regions.
  #
  # == Example
  #
  #   image_in = Axon::Solid.new(100, 200)
  #   c = Axon::Crop.new(image_in, 50, 75, 10, 20)
  #   c.width  # => 50
  #   c.height # => 75
  #   c.gets   # => String
  #
  # == Example of Cropping Past the Boundaries of the Original Image
  #
  #   image_in = Axon::Solid.new(100, 200)
  #   c = Axon::Crop.new(image_in, 50, 75, 60, 20)
  #   c.width # => 40
  #
  class Cropper
    # The index of the next line that will be fetched by gets, starting at 0.
    attr_reader :lineno

    # :call-seq:
    #   Cropper.new(image_in, width, height, x_offset = 0, y_offset = 0)
    #
    # Crops +image_in+ to the size +width+ x +height+. Optionally, +x_offset+
    # and +y_offset+ can be used to shift the upper left corner of the cropped
    # area.
    #
    # If the cropped image extends beyond the boundaries of +image_in+ then the
    # cropped image will be truncated at the boundary.
    #
    def initialize(source, width, height, x_offset=nil, y_offset=nil)
      raise ArgumentError if width < 1 || height < 1
      raise ArgumentError if x_offset && x_offset < 1 || y_offset && y_offset < 1

      @source = source
      @width = width
      @height = height
      @x_offset = x_offset || 0
      @y_offset = y_offset || 0
      @lineno = 0
    end

    # Calculates the height of the cropped image.
    #
    def height
      if @y_offset + @height > @source.height
        [@source.height - @y_offset, 0].max
      else
        @height
      end
    end

    # Calculates the width of the cropped image.
    #
    def width
      if @x_offset + @width > @source.width
        [@source.width - @x_offset, 0].max
      else
        @width
      end
    end

    # Gets the components in the cropped image. Same as the components of the
    # source image.
    #
    def components
      @source.components
    end

    # Gets the color model of the cropped image. Same as the color model of the
    # source image.
    #
    def color_model
      @source.color_model
    end

    # Gets the next scanline from the cropped image.
    #
    def gets
      return nil if @lineno >= height || width < 1 || height < 1

      while @source.lineno < @y_offset
        break unless @source.gets
      end

      @lineno += 1

      sl_width = width * components
      sl_offset = @x_offset * components
      @source.gets[sl_offset, sl_width]
    end
  end
end
