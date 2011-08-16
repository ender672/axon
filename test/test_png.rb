require "helper"

module Axon
  class TestPNG < AxonTestCase
    def test_version
      assert PNG_LIB_VERSION >= 10200
    end
  end
end
