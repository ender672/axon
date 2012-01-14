module Axon
  module ReaderTests
    def test_header_dimensions
      assert_equal @image.width, @reader.width
      assert_equal @image.height, @reader.height
    end
    
    def test_header_components
      assert_equal @image.components, @reader.components
    end
    
    def test_io_returns_too_much_data
      io = CustomIO.new(Proc.new{ |io, *args| io.read(*args)[0..20] * 100 }, @data)
      assert_raises(RuntimeError) { @readerclass.new io }
    end
    
    def test_io_returns_nil
      assert_raises(RuntimeError) { @readerclass.new(CustomIO.new(nil)) }
    end

    def test_io_returns_one_byte_at_a_time
      io = CustomIO.new(Proc.new{ |io, len| io.read(len || 1) }, @data)
      r = @readerclass.new(io)
      r.gets
    end

    def test_io_raises_exception
      io = CustomIO.new(Proc.new{ raise CustomError }, @data)
      assert_raises(CustomError) { @readerclass.new io }
    end

    def test_empty_string_io
      assert_raises(RuntimeError) { @readerclass.new(CustomIO.new("")) }
    end
    
    def test_not_a_string_io
      assert_raises(TypeError) { @readerclass.new(CustomIO.new(:foo)) }
    end

    def test_lineno
      assert_equal 0, @reader.lineno
      @reader.height.times do |i|
        assert_equal i, @reader.lineno
        @reader.gets
      end
      assert_equal nil, @reader.gets
      assert_equal @reader.height, @reader.lineno
    end
    
    def test_gets
      size = @reader.width * @reader.components
      @reader.height.times do
        assert_equal size, @reader.gets.size
      end
      assert_equal nil, @reader.gets
    end
    
    def test_recovers_from_initial_io_exception
      ex_io = CustomIO.new(Proc.new{ raise CustomError }, @data)
      r = @readerclass.allocate
      assert_raises(CustomError) { r.send(:initialize, ex_io) }
      r.send(:initialize, StringIO.new(@data))
      r.gets
    end
  end
end
