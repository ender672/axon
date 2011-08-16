require "helper"

module Axon
  class TestJPEG < AxonTestCase
    def test_version
      assert JPEG_LIB_VERSION >= 62
    end
  end
end
