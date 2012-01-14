require 'rubygems'
require 'minitest/autorun'
require 'axon'
require 'stringio'

module Axon
  class AxonTestCase < MiniTest::Unit::TestCase
    # Generate a solid velvet image
    def setup
      super
      @image = Solid.new 10, 15, "\x0A\x14\x69"
      @io_out = StringIO.new
      @io_out.set_encoding 'ASCII-8BIT' if @io_out.respond_to?(:set_encoding)
    end

    def assert_image_dimensions(image, width, height)
      cmp = image.components

      assert_equal width, image.width
      assert_equal height, image.height

      height.times do |i|
        assert_equal i, image.lineno
        assert_equal width * cmp, image.gets.size, 'image.gets should return scanlines with length width'
      end

      assert_equal height, image.lineno
      assert_nil image.gets, 'image.gets should be nil after reading height lines'
      assert_equal height, image.lineno
    end

    def skip_symbol_fixnums
      skip("ruby 1.8.7 treats symbols as fixnums") unless RUBY_VERSION >= "1.9"
    end
  end

  class CustomError < RuntimeError; end

  module CallOrReturnValue
    def call_or_return(*args)
      @custom.respond_to?(:call) ? @custom.call(@parent, *args) : @custom
    end
  end

  class CustomImage
    include CallOrReturnValue

    def initialize(custom)
      @parent = Solid.new 100, 200
      @custom = custom
    end

    def height; @parent.height; end
    def width; @parent.width; end
    def color_model; @parent.color_model; end
    def components; @parent.components; end
    def gets; @parent.gets; end
    def lineno; @parent.lineno; end
  end

  class CustomHeightImage < CustomImage
    def height; call_or_return; end
  end

  class CustomWidthImage < CustomImage
    def width; call_or_return; end
  end

  class CustomColorModelImage < CustomImage
    def color_model; call_or_return; end
  end

  class CustomComponentsImage < CustomImage
    def components; call_or_return; end
  end

  class CustomLinenoImage < CustomImage
    def lineno; call_or_return; end
  end

  class CustomGetsImage < CustomImage
    def gets; call_or_return; end
  end
  
  class CustomIO
    include CallOrReturnValue

    attr_accessor :custom

    def initialize(custom, *args)
      @parent = StringIO.new(*args)
      @custom = custom
    end

    def write(*args); call_or_return(*args); end
    def read(*args); call_or_return(*args); end
    def string; @parent.string; end
  end

  class ArrayWrapper
    attr_reader :lineno, :components, :color_model

    def initialize(ary, components=nil, color_model=nil)
      @ary = ary
      @components = components || 3
      @color_model = color_model || :RGB
      @lineno = 0
    end

    def width
      @ary[0].size
    end

    def height
      @ary.size
    end

    def gets
      return nil if @lineno > @ary.size
      res = @ary[@lineno]
      @lineno += 1
      res
    end
  end

  class Repeater
    attr_reader :lineno

    def initialize(source)
      @source = source
      @data = []
      @lineno = 0
    end

    def components
      @source.components
    end

    def color_model
      @source.color_model
    end

    def height
      @source.height
    end

    def width
      @source.width
    end

    def gets
      if @data.size <= @lineno
        res = @source.gets.dup
        return nil unless res
        @data << res
      end

      @lineno += 1
      @data[@lineno - 1].dup
    end

    def rewind
      @lineno = 0
    end
  end
end
