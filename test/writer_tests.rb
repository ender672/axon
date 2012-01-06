module Axon
  module WriterTests
    def test_writes_something
      @mod.write(@image, @io_out)
      refute @io_out.string.empty?
    end

    def test_returns_bytes_written
      ret = @mod.write(@image, @io_out)
      assert_equal @io_out.size, ret
    end

    def test_invalid_numeric_height
      [0, -100, 0.0001].each do |h|
        assert_raises RuntimeError do
          @mod.write(CustomHeightImage.new(h), @io_out)
        end
      end
    end

    def test_invalid_height_type
      [nil, :foo].each do |h|
        assert_raises TypeError, "should throw a TypeError when given a #{h.class} for height." do
          @mod.write(CustomHeightImage.new(h), @io_out)
        end
      end
    end

    def test_height_raises_exception
      im = CustomHeightImage.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(im, @io_out)
      end
    end

    def test_invalid_numeric_width
      [0, -100, 0.0001].each do |w|
        assert_raises RuntimeError do
          @mod.write(CustomWidthImage.new(w), @io_out)
        end
      end
    end

    def test_invalid_width_type
      [nil, :foo].each do |w|
        assert_raises TypeError, "should throw a TypeError when given a #{w.class} for width." do
          @mod.write(CustomWidthImage.new(w), @io_out)
        end
      end
    end

    def test_width_raises_exception
      im = CustomWidthImage.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(im, @io_out)
      end
    end

    def test_nil_image_color_model
      assert_raises TypeError do
        @mod.write(CustomColorModelImage.new(nil), @io_out)
      end
    end

    def test_invalid_color_model
      assert_raises RuntimeError do
        @mod.write(CustomColorModelImage.new(:foo), @io_out)
      end
    end

    def test_color_model_raises_exception
      im = CustomColorModelImage.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(im, @io_out)
      end
    end

    def test_invalid_image_components
      [0, -100, 0.0001, 15].each do |w|
        assert_raises RuntimeError do
          @mod.write(CustomComponentsImage.new(w), @io_out)
        end
      end
    end

    def test_invalid_components_type
      [nil, :foo].each do |w|
        assert_raises TypeError do
          @mod.write(CustomComponentsImage.new(w), @io_out)
        end
      end
    end

    def test_invalid_components_type
      [nil, :foo].each do |w|
        assert_raises TypeError, "should throw a TypeError when given a #{w.class} for components." do
          @mod.write(CustomComponentsImage.new(w), @io_out)
        end
      end
    end

    def test_components_raises_exception
      im = CustomComponentsImage.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(im, @io_out)
      end
    end

    def test_invalid_lineno_type
      [-1, nil, :foo].each do |l|
        # we don't know if the writer will care but test anyways to detect
        # interpreter crashes and mem leaks.
        @mod.write(CustomLinenoImage.new(l), @io_out)
      end
    end

    def test_odd_gets_type
      [nil, :foo, 1234].each do |l|
        assert_raises RuntimeError do
          @mod.write(CustomGetsImage.new(l), @io_out)
        end
      end
    end

    def test_gets_raises_exception_immediately
      im = CustomGetsImage.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(im, @io_out)
      end
    end
    
    def test_gets_raises_exception_later
      proc = Proc.new{ |im| im.lineno > 3 ? raise(CustomError) : im.gets }
      i = CustomGetsImage.new(proc)
      assert_raises CustomError do
        @mod.write(i, @io_out)
      end
      assert_equal 4, i.lineno
    end

    def test_gets_return_is_too_short_immediately
      proc = Proc.new{ |im| im.gets[2, -1] }
      assert_raises RuntimeError do
        @mod.write(CustomGetsImage.new(proc), @io_out)
      end
    end

    def test_gets_return_is_too_short_later
      proc = Proc.new{ |im| im.lineno > 3 ? im.gets[2, -1] : im.gets }
      i = CustomGetsImage.new(proc)
      assert_raises RuntimeError do
        @mod.write(i, @io_out)
      end
      assert_equal 5, i.lineno
    end
    
    def test_gets_return_is_too_long_immediately
      proc = Proc.new{ |im| im.gets * 2 }
      assert_raises RuntimeError do
        @mod.write(CustomGetsImage.new(proc), @io_out)
      end
    end

    def test_gets_return_is_too_long_later
      proc = Proc.new{ |im| im.lineno > 3 ? im.gets * 2 : im.gets }
      i = CustomGetsImage.new(proc)
      assert_raises RuntimeError do
        @mod.write(i, @io_out)
      end
      assert_equal 5, i.lineno
    end

    def test_io_raises_exception_immediately
      io = CustomIO.new(Proc.new{ raise CustomError })
      assert_raises CustomError do
        @mod.write(@image, io)
      end
    end

    def test_io_returns_invalid_type
      [nil, :foo, "bar"].each do |r|
        im = Solid.new(200, 100)
        assert_raises TypeError, "should get a TypeError when IO#write returns a #{r.class}." do
          @mod.write(im, CustomIO.new(r))
        end
      end
    end

    def test_io_returns_invalid_length
      [0, -1, -100, 2000, 1].each do |r|
        assert_raises RuntimeError do
          @mod.write(@image, CustomIO.new(r))
        end
      end
    end
  end
end
