module Axon
  class Noise
    include Image
    include Enumerable
    attr_reader :width, :height, :color_model, :components

    def initialize(width, height, options=nil)
      options ||= {}

      @width = width
      @height = height
      @color_model = options[:color_model] || :RGB
      @components = options[:components] || 3
      @seed = options[:seed]
    end

    def each
      prng = @seed ? Random.new(@seed) : Random.new
      @height.times do
        sl = ''
        sl.force_encoding('BINARY')
        (@width * @components).times{ sl << prng.rand(2**8) }
        yield sl
      end
    end
  end

  module Image
    # empty
  end
end
