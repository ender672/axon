require 'helper'

module Axon
  class TestReader < AxonTestCase
    def setup
      data = Solid.new(100, 200, "\x0A\x14\x69").to_jpeg.data
      @io_in = StringIO.new(data)
      @io_out = StringIO.new
    end

    def test_short_chained_example

      # Short, chained example. Reads a JPEG from io_in and writes scaled png to
      # io_out.
      Axon.JPEG(@io_in).fit(100, 100).write_png(@io_out)
    end

    def test_longer_example
      # Even longer example, reads the JPEG header, looks at properties and header
      # values, sets decompression options, scales the image, sets compression
      # options, and writes a JPEG.
      image = Axon.JPEG(@io_in, [:APP2])

      puts image.width
      puts image.height
      puts image[:APP2]
      image.scale_denom = 4

      jpeg = image.fit(100, 100).to_jpeg
      jpeg.quality = 88
      jpeg.write(@io_out)
    end
  end
end
