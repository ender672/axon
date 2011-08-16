module Axon
  class Solid
    include Image
    include Enumerable
    attr_reader :width, :height, :color_model, :components

    def initialize(width, height, color=nil, color_model=nil)
      @width, @height = width, height
      @color = color || "\x00\x00\x00"
      @color_model = color_model || :RGB
      @components = @color.size
    end

    def each
      sl = @color * width
      height.times { yield sl }
    end
  end

  module Image
    # empty
  end
end
