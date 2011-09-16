module Axon
  class Scaler
    attr_reader :width, :height

    def initialize(image, *args)
      @image = image

      if args.size == 1
        init_ratio(args[0])
      elsif args.size == 2
        init_dims(args[0], args[1])
      else
        raise ArgumentError, "Must give one or two arguments"
      end

      if @height_ratio <= 0 || @width_ratio <= 0
        raise ArgumentError, "Can't scale to zero or below"
      end
    end

    def components
      @image.components
    end

    def color_model
      @image.color_model
    end

    private

    def init_ratio(ratio)
      @width_ratio = @height_ratio = ratio.to_f
      @width = (@image.width * ratio).floor
      @height = (@image.height * ratio).floor
    end

    def init_dims(width, height)
      @width_ratio = width.to_f / @image.width
      @width = width

      @height_ratio = height.to_f / @image.height
      @height = height
    end
  end
end
