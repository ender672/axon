require 'helper'

module Axon
  class TestJPEGReader < AxonTestCase
    include ReaderTests

    def setup
      super
      @readerclass = JPEGReader
      @data = @image.to_jpeg.data
      @io_in = StringIO.new @data
      @reader = @readerclass.new(@io_in)
    end

    def test_in_color_model
      assert_equal :YCbCr, @reader.in_color_model
    end
    
    def test_set_in_color_model
      @reader.in_color_model = :RGB
      assert_equal :RGB, @reader.in_color_model
      
      assert_raises(RuntimeError){ @reader.in_color_model = "foobar" }
    end
    
    def test_set_out_color_model
      @reader.color_model = :YCbCr
      assert_equal :YCbCr, @reader.color_model
      
      assert_raises(RuntimeError){ @reader.color_model = "foobar" }
    end
    
    def test_scale_num
      assert @reader.scale_num > 0
    end
    
    def test_set_scale_num
      @reader.scale_num = 5
      assert_equal 5, @reader.scale_num
    end
    
    def test_scale_denom
      assert @reader.scale_denom > 0
    end
    
    def test_set_scale_denom
      @reader.scale_denom = 8
      
      assert_equal 8, @reader.scale_denom
    end
    
    def test_scale_denom_affects_image_size
      pre_width = @reader.width
      pre_height = @reader.height
      
      @reader.scale_denom = 2
      
      assert @reader.width < pre_width
      assert @reader.height < pre_height
    end

    def test_scale_denom_affects_written_image
      pre_width = @reader.width
      pre_height = @reader.height

      writer = JPEGWriter.new(@reader)
      @reader.scale_denom = 2

      new_velvet_io = StringIO.new(writer.data)

      new_velvet_reader = JPEGReader.new(new_velvet_io)

      assert new_velvet_reader.width < pre_width
      assert new_velvet_reader.height < pre_height
    end
    
    def test_dct_method
      assert_equal JPEGReader::DEFAULT_DCT, @reader.dct_method
    end
    
    def test_set_dct_method
      @reader.dct_method = :IFAST
      
      assert_equal :IFAST, @reader.dct_method
    end
    
    def test_markers_read_by_default
      refute_empty @reader[:APP0]
    end
    
    def test_empty_marker_prevents_reads
      @io_in.rewind
      r = JPEGReader.new @io_in, []
      assert_empty r[:APP0]
    end
    
    def test_marker_read
      assert_match(/^JFIF/, @reader[:APP0].first)
    end
    
    def test_no_configuration_after_initiated
      @reader.each do |sl|
        assert_raises(RuntimeError) { @reader.dct_method = :IFAST }
        assert_raises(RuntimeError) { @reader.scale_denom = 4 }
        break
      end
    end
  end
end
