require 'helper'
require 'writer_tests'

module Axon
  class TestJPEGWriter < AxonTestCase
    include WriterTests

    def setup
      super
      @mod = JPEG
    end

    def test_output_has_jpeg_header
      JPEG.write(@image, @io_out)
      assert_equal "\xFF\xD8\xFF\xE0", @io_out.string[0..3]
      assert_equal "JFIF\x00", @io_out.string[6..10]
    end

    def test_invalid_bufsize
      assert_raises RuntimeError do
        JPEG.write(@image, @io_out, :bufsize => 0)
      end

      assert_raises RuntimeError do
        JPEG.write(@image, @io_out, :bufsize => -5)
      end
    end

    def test_symbol_bufsize
      assert_raises TypeError do
        JPEG.write(@image, @io_out, :bufsize => :foo)
      end
    end

    def test_quality_filesize
      file_sizes = {}
      [-10, 0, 1, 50, 100, 130].each do |q|
        im = Solid.new(100, 200)
        io = StringIO.new
        JPEG.write(im, io, :quality => q)
        file_sizes[q] = io.size
      end

      assert file_sizes[-10] > 1

      # libjpeg treats values under 1 as 1
      assert_equal file_sizes[-10], file_sizes[0]
      assert_equal file_sizes[0], file_sizes[1]

      assert file_sizes[50] > file_sizes[1]
      assert file_sizes[100] > file_sizes[50]

      # libjpeg treats values over 100 as 100
      assert_equal file_sizes[130], file_sizes[100]
    end

    def test_symbol_quality
      assert_raises TypeError do
        JPEG.write(@image, @io_out, :quality => :foo)
      end
    end

    def test_exif_roundtrip
      random_data = ""
      (100).times do
        random_data << [rand].pack('d') # 800 bytes random data
      end

      io_out = StringIO.new
      JPEG.write(@image, io_out, :exif => random_data)
      io_out.rewind

      image_with_exif = JPEG::Reader.new(io_out)
      assert_equal random_data, image_with_exif.exif
    end

    def test_exif_with_icc_roundtrip
      random_icc_data = ""
      (2**16).times do                  # a little larger than one jpeg segment
        random_icc_data << [rand].pack('d') # 8 bytes random data
      end

      random_exif_data = ""
      (100).times do
        random_exif_data << [rand].pack('d') # 800 bytes random data
      end

      io_out = StringIO.new
      JPEG.write(@image, io_out, :icc_profile => random_icc_data,
                 :exif => random_exif_data)
      io_out.rewind

      image_with_exif_and_icc = JPEG::Reader.new(io_out)

      assert_equal random_exif_data, image_with_exif_and_icc.exif
      assert_equal random_icc_data, image_with_exif_and_icc.icc_profile
    end

    def test_large_icc_roundtrip
      random_data = ""
      (2**16).times do                  # a little larger than one jpeg segment
        random_data << [rand].pack('d') # 8 bytes random data
      end

      io_out = StringIO.new
      JPEG.write(@image, io_out, :icc_profile => random_data)
      io_out.rewind

      image_with_icc = JPEG::Reader.new(io_out)
      assert_equal random_data, image_with_icc.icc_profile
    end
  end
end
