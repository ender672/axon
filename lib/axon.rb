require 'axon/axon'

require 'axon/solid'
require 'axon/noise'
require 'axon/cropper'
require 'axon/scaler'
require 'axon/nearest_neighbor_scaler'
require 'axon/bilinear_scaler'
require 'axon/fit'

require 'axon/jpeg_writer'
require 'axon/png_writer'

module Axon
  VERSION = '0.0.2'

  def self.JPEG(thing, markers=nil)
    if thing.respond_to? :read
      io = thing
      rewind_after_scanlines = false
    elsif thing[0, 1] == "\xFF"
      io = StringIO.new(thing)
      rewind_after_scanlines = true
    else
      io = File.open(thing)
      rewind_after_scanlines = true
    end

    JPEGReader.new(io, markers, rewind_after_scanlines)
  end

  def self.PNG(thing)
    if thing.respond_to? :read
      io = thing
      rewind_after_scanlines = false
    elsif thing[0, 1] == "\x89"
      io = StringIO.new(thing)
      rewind_after_scanlines = true
    else
      io = File.open(thing)
      rewind_after_scanlines = true
    end

    PNGReader.new(io)
  end
end
