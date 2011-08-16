require 'helper'

module Axon
  class TestExif < AxonTestCase
    def test_exif_roundtrip
      random_data = ""
      (100).times do
        random_data << [rand].pack('d') # 800 bytes random data
      end

      writer = @image.to_jpeg
      writer.exif = random_data
      
      image_with_exif = Axon.JPEG(writer.data)
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
      
      writer = @image.to_jpeg
      writer.icc_profile = random_icc_data
      writer.exif = random_exif_data

      image_with_exif_and_icc = Axon.JPEG(writer.data)

      assert_equal random_exif_data, image_with_exif_and_icc.exif
      assert_equal random_icc_data, image_with_exif_and_icc.icc_profile
    end
  end
end
