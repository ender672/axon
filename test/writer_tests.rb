module Axon
  module WriterTests
    def test_writes_something
      @writer.write(@io_out)
      refute @io_out.string.empty?
    end

    class ZeroHeightImage < Solid
      def height; 0; end
    end

    class NegativeHeightImage < Solid
      def height; -1; end
    end

    class NilHeightImage < Solid
      def height; nil; end
    end

    def test_bad_dimension
      i = ZeroHeightImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
      
      i = NegativeHeightImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end

      i = NilHeightImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises TypeError do
        writer.write(@io_out)
      end
    end

    class NilColorModelImage < Solid
      def color_model; nil; end
    end

    def test_bad_color_model
      i = NilColorModelImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end

    class TooManyComponentsImage < Solid
      def components; 15; end
    end

    class ZeroComponentsImage < Solid
      def components; 0; end
    end

    class NilComponentsImage < Solid
      def components; nil; end
    end

    def test_bad_components
      i = NilComponentsImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises TypeError do
        writer.write(@io_out)
      end

      i = TooManyComponentsImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end

      i = ZeroComponentsImage.new 10, 15, @velvet
      writer = @writerclass.new(i)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end
    
    class RaisingIO
      def write(data)
        raise 'hell'
      end
    end
    
    def test_io_raises_exception
      io = RaisingIO.new
      
      assert_raises RuntimeError do
        @writer.write(io)
      end
    end
    
    class NilImage < Solid
      def each
        10.times{ yield nil }
      end
    end
    
    def test_image_scanlines_yield_nil
      image = NilImage.new(10, 15, @velvet)
      writer = @writerclass.new(image)
      
      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end

    class TooManyScanLinesImage < Solid
      def each
        super
        yield(@color * width)
      end
    end

    def test_image_yields_too_many_scanlines
      image = TooManyScanLinesImage.new 10, 15, @velvet
      writer = @writerclass.new(image)
      writer.write(@io_out)
    end

    class TooFewScanLinesImage < Solid
      def each
        yield(@color * width)
      end
    end
    
    def test_image_yields_too_few_scanlines
      image = TooFewScanLinesImage.new 10, 15, @velvet
      writer = @writerclass.new(image)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end

    class RaisingImage < Solid
      def each
        raise 'chickens'
      end
    end
    
    def test_image_scanlines_raises_exception
      image = RaisingImage.new(10, 15, @velvet)
      writer = @writerclass.new(image)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end
    
    class BigWidthImage < Solid
      def each
        sl = @color * width * 10
        height.times { yield sl }
      end
    end
    
    def test_image_scanlines_returns_too_much
      image = BigWidthImage.new(10, 15, @velvet)
      writer = @writerclass.new(image)

      assert_raises RuntimeError do
        writer.write(@io_out)
      end
    end
  end
end
