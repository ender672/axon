require 'helper'
require 'writer_tests'

module Axon
  class TestJPEGWriter < AxonTestCase
    include WriterTests

    def setup
      super
      @writerclass = JPEGWriter
      @writer = @image.to_jpeg
    end

    def test_output_has_jpeg_header
      string = @writer.data

      assert_equal "\xFF\xD8\xFF\xE0", string[0..3]
      assert_equal "JFIF\x00", string[6..10]
    end

    def test_bad_bufsize
      assert_raises RuntimeError do
        @writer.write(@io_out, 0)
      end
    end
  end
end
