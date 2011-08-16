require 'helper'

module Axon
  class TestICC < AxonTestCase
    def test_large_icc_roundtrip
      random_data = ""
      (2**16).times do                  # a little larger than one jpeg segment
        random_data << [rand].pack('d') # 8 bytes random data
      end

      writer = @image.to_jpeg
      writer.icc_profile = random_data
      
      image_with_icc = Axon.JPEG(writer.data)
      assert_equal random_data, image_with_icc.icc_profile
    end
  end
end
