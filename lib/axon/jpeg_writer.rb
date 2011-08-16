require 'stringio'

module Axon
  class JPEGWriter
    include JPEGNativeWriter

    attr_accessor :quality, :icc_profile, :exif
    attr_reader :io, :bufsize, :image

    def initialize(image)
      @image = image
      @quality = nil
      @icc_profile = nil
      @exif = nil
    end
    
    def write(io, bufsize = nil)
      @bufsize = bufsize || 512
      @io = io
      jpeg_native_write
    end

    def data
      s = StringIO.new
      s.set_encoding 'BINARY' if s.respond_to?(:set_encoding)

      write(s)
      s.string
    end
  end

  module Image
    def to_jpeg(*args)
      JPEGWriter.new self, *args
    end

    def write_jpeg(io)
      JPEGWriter.new(self).write(io)
    end
  end
end
