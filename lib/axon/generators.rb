module Axon
  # == A Noise Image Generator
  #
  # Axon::Noise will generate images with random pixel color values.
  #
  # == Example
  #
  #   Axon::Noise.new(100, 200, :components => 1)
  #
  class Noise
    # The width of the generated image.
    attr_reader :width

    # The height of the generated image.
    attr_reader :height

    # The components in the generated image.
    attr_reader :components

    # The index of the next line that will be fetched by gets, starting at 0.
    attr_reader :lineno

    # :call-seq:
    #   Noise.new(width, height, options = {})
    #
    # Creates a new noise image object with dimensions +width+ x +height+.
    #
    # +options+ may contain the following optional hash key values:
    #
    # * :components -- The number of components in the generated image.
    #
    def initialize(width, height, options=nil)
      options ||= {}

      @width = width
      @height = height
      @components = options[:components] || 3
      @lineno = 0
      @empty_string = String.new
      if @empty_string.respond_to? :force_encoding
        @empty_string.force_encoding('BINARY')
      end
    end

    # Gets the next scanline from the generated image.
    #
    def gets
      return nil if @lineno >= @height
      sl = @empty_string.dup
      (@width * @components).times{ sl << rand(2**8) }
      @lineno += 1
      sl
    end
  end

  # == A Solid Color Image Generator
  #
  # Axon::Solid will generate images with a solid color value.
  #
  # == Example
  #
  #   Axon::Solid.new(100, 200, "\x0A\x14\x69")
  #
  class Solid
    # The width of the generated image.
    attr_reader :width

    # The height of the generated image.
    attr_reader :height

    # The components in the generated image.
    attr_reader :components

    # The index of the next line that will be fetched by gets, starting at 0.
    attr_reader :lineno

    # :call-seq:
    #   Solid.new(width, height, color = "\x00\x00\x00", color_model = :RGB)
    #
    # Creates a new solid color image object with dimensions +width+ x +height+.
    #
    # The optional argument +color+ is the binary value that will be assigned
    # to each pixel.
    #
    def initialize(width, height, color=nil)
      @width, @height = width, height
      @color = color || "\x00\x00\x00"
      @components = @color.size
      @lineno = 0
    end

    # Gets the next scanline from the generated image.
    #
    def gets
      return nil if @lineno >= @height
      @lineno += 1
      @color * width
    end
  end
end
