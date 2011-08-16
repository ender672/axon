require 'stringio'

module Axon
  class PNGWriter
    include PNGNativeWriter

    attr_reader :image

    def initialize(image)
      @image = image
    end

    def write(io)
      png_native_write(io)
    end

    def data
      s = StringIO.new
      s.set_encoding 'BINARY' if s.respond_to?(:set_encoding)

      write(s)
      s.string
    end
  end

  module Image
    def to_png
      PNGWriter.new self
    end

    def write_png(io)
      PNGWriter.new(self).write(io)
    end
  end
end
