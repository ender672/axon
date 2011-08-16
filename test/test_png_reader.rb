require 'helper'

module Axon
  class TestPNGReader < AxonTestCase
    include ReaderTests

    def setup
      super
      @readerclass = PNGReader
      @data = @image.to_png.data
      @io_in = StringIO.new @data
      @reader = PNGReader.new(@io_in)
    end
  end
end
