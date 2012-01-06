require 'helper'
require 'reader_tests'

module Axon
  module PNG
    class TestPNGReader < AxonTestCase
      include ReaderTests

      def setup
        super
        io = StringIO.new
        PNG.write(@image, io)
        @data = io.string
        @readerclass = Reader
        @reader = Reader.new(StringIO.new(@data))
      end
    end
  end
end
