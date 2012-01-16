require 'helper'
require 'reader_tests'

module Axon
  module JPEG
    class TestJPEGReader < AxonTestCase
      include ReaderTests

      def setup
        super
        io = StringIO.new
        JPEG.write(@image, io)
        @data = io.string
        @readerclass = Reader
        @reader = Reader.new(StringIO.new(@data))
      end

      def test_in_color_model
        skip unless @reader.respond_to?(:in_color_model)
        assert_equal :YCbCr, @reader.in_color_model
      end

      def test_set_in_color_model
        skip unless @reader.respond_to?(:in_color_model=)        
        @reader.in_color_model = :RGB
        assert_equal :RGB, @reader.in_color_model

        assert_raises(RuntimeError){ @reader.in_color_model = "foobar" }
      end

      def test_set_out_color_model
        skip unless @reader.respond_to?(:color_model=)
        @reader.color_model = :YCbCr
        assert_equal :YCbCr, @reader.color_model

        assert_raises(RuntimeError){ @reader.color_model = "foobar" }
      end

      def test_scale_num
        skip unless @reader.respond_to?(:scale_num)
        assert @reader.scale_num > 0
      end

      def test_set_scale_num
        skip unless @reader.respond_to?(:scale_num=)
        @reader.scale_num = 5
        assert_equal 5, @reader.scale_num
      end

      def test_scale_denom
        skip unless @reader.respond_to?(:scale_denom)
        assert @reader.scale_denom > 0
      end

      def test_set_scale_denom
        skip unless @reader.respond_to?(:scale_denom=)
        @reader.scale_denom = 8
        assert_equal 8, @reader.scale_denom
      end

      def test_scale_denom_affects_image_size
        skip unless @reader.respond_to?(:scale_denom=)
        pre_width = @reader.width
        pre_height = @reader.height

        @reader.scale_denom = 2

        if JPEG::LIB_VERSION >= 70
          # I can't really explain why this doubles our dimensions. The
          # jpeg decompressor reports 8 / 2 for scale_num / scale_denom.
          assert_equal pre_width * 2, @reader.width
          assert_equal pre_height * 2, @reader.height
        else
          assert_equal pre_width / 2, @reader.width
          assert_equal pre_height / 2, @reader.height
        end
      end

      def test_scale_denom_affects_written_image
        skip unless @reader.respond_to?(:scale_denom=)
        pre_width = @reader.width
        pre_height = @reader.height

        @reader.scale_denom = 2

        io_out = StringIO.new
        JPEG.write(@reader, io_out)
        io_out.rewind

        new_velvet_reader = Reader.new(io_out)

        if JPEG::LIB_VERSION >= 70
          assert_image_dimensions(new_velvet_reader, pre_width * 2, pre_height * 2)
        else
          assert_image_dimensions(new_velvet_reader, pre_width / 2, pre_height / 2)
        end
      end

      def test_dct_method
        skip unless @reader.respond_to?(:dct_method)
        assert_equal Reader::DEFAULT_DCT, @reader.dct_method
      end

      def test_set_dct_method
        skip unless @reader.respond_to?(:dct_method=)
        @reader.dct_method = :IFAST
        assert_equal :IFAST, @reader.dct_method
      end

      def test_markers_read_by_default
        skip "JRuby ImageIO does not give access to headers" if(RUBY_PLATFORM =~ /java/)        
        refute_empty @reader[:APP0]
      end

      def test_empty_marker_prevents_reads
        skip "JRuby ImageIO does not give access to headers" if(RUBY_PLATFORM =~ /java/)        
        r = Reader.new StringIO.new(@data), []
        assert_empty r[:APP0]
      end

      def test_marker_content
        skip "JRuby ImageIO does not give access to headers" if(RUBY_PLATFORM =~ /java/)        
        assert_match(/^JFIF/, @reader[:APP0].first)
      end

      def test_no_configuration_after_initiated
        skip unless @reader.respond_to?(:dct_method)        
        @reader.gets
        assert_raises(RuntimeError) { @reader.dct_method = :IFAST }
        assert_raises(RuntimeError) { @reader.scale_denom = 4 }
      end
    end
  end
end
