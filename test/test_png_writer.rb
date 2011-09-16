require 'helper'
require 'writer_tests'

module Axon
  class TestPNGWriter < AxonTestCase
    include WriterTests

    def setup
      super
      @writerclass = PNGWriter
      @writer = @image.to_png
    end
  end
end
