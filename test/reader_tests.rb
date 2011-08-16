module Axon
  module ReaderTests
    def test_read_image_from_io
      assert_equal 10, @reader.width
      assert_equal 15, @reader.height
    end
    
    def test_read_image_from_string
      assert_equal 10, @reader.width
      assert_equal 15, @reader.height
    end
    
    def test_num_components
      assert_equal 3, @reader.components
    end
    
    def test_color_model
      assert_equal :RGB, @reader.color_model
    end
    
    class DoubleIO < StringIO
      def read(*args)
        s = super
        s[0..20] * 100
      end
    end
    
    def test_io_returns_too_much
      f = DoubleIO.new @data
      assert_raises(RuntimeError) { @readerclass.new f }
    end
    
    class NilIO
      def read(*args); end
    end
    
    def test_io_returns_nil
      assert_raises(RuntimeError) { @readerclass.new NilIO.new }
    end
    
    class RaiseIO
      def read(*args)
        raise 'heck'
      end
    end
    
    def test_io_raises_exception
      assert_raises(RuntimeError) { @readerclass.new RaiseIO.new }
    end

    class EmptyStringIO
      def read(*args)
        ""
      end
    end

    def test_empty_string_io
      assert_raises(RuntimeError) { @readerclass.new EmptyStringIO.new }
    end
    
    class ThatsNotAStringIO
      def read(*args)
        :this_should_be_a_string
      end
    end
    
    def test_not_a_string_io
      assert_raises(TypeError) { @readerclass.new ThatsNotAStringIO.new }
    end
    
    def test_each
      size = @reader.width * @reader.components

      @reader.each do |scan_line|
        assert_equal size, scan_line.size
      end
    end
    
    def test_no_shenanigans_during_each
      @reader.each do |sl|
        assert_raises(RuntimeError) { @reader.each{} }
        break
      end
    end
    
    class OneExceptionIO < StringIO
      def initialize(*args)
        @raised_exception = false
        super
      end
      
      def read(*args)
        unless @raised_exception
          @raised_exception = true
          raise 'heck'
        end
        super
      end
    end
    
    def test_recovers_from_initial_io_exception
      io = OneExceptionIO.new @data
      r = @readerclass.allocate
      assert_raises(RuntimeError) { r.send(:initialize, io) }
      r.send(:initialize, io)
      r.each{ }
    end

    def test_multiple_each_calls
      @reader.each{ }
      @io_in.rewind
      @reader.each{ }
    end
  end
end
