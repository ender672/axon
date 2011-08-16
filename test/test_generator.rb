require "helper"

module Axon
  class TestGenerator < AxonTestCase
    def test_new_solid
      g = Solid.new 10, 15, "\x0A\x14\x69"
      g.write_jpeg(@io_out)
    end
  end
end
