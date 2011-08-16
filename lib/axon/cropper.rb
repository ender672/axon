module Axon
  class Cropper
    include Enumerable
    include Image

    attr_reader :image, :width, :height, :components, :color_model

    def initialize(image, width, height, x_offset=nil, y_offset=nil)
      @image = image
      @width = width
      @height = height
      @x_offset = x_offset || 0
      @y_offset = y_offset || 0
      @components = image.components
      @color_model = image.color_model
    end

    def each
      sl_width = @width * @components
      sl_offset = @x_offset * @components

      @image.each_with_index do |orig_sl, i|
        next if i < @y_offset
        yield orig_sl[sl_offset, sl_width]
        break if i == @height - 1
      end
    end
  end

  module Image
    def crop(*args)
      Cropper.new(self, *args)
    end
  end
end
