require 'helper'
require 'writer_tests'

module Axon
  module PNG
    class TestPNGWriter < AxonTestCase
      include WriterTests

      def setup
        super
        @mod = PNG
      end
    end
  end
end
