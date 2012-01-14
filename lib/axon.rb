# :main: README.rdoc

require 'axon/axon'
require 'axon/cropper'
require 'axon/fit'
require 'axon/scalers'
require 'axon/generators'
require 'axon/alpha_stripper'
require 'stringio'

module Axon
  VERSION = '0.1.1'

  # :call-seq:
  #   Axon.jpeg(thing [, markers]) -> image
  #
  # Reads a compressed JPEG image from +thing+. +thing+ can be an IO object or
  # a string of JPEG data.
  #
  # +markers+ should be an array of valid JPEG header marker symbols. Valid
  # symbols are :APP0 through :APP15 and :COM.
  #
  # If performance is important and you don't care about any markers, you can
  # avaoid reading any header markers by supplying an empty array for +markers+.
  #
  # When +markers+ is not given, all known JPEG markers will be read.
  #
  #   io_in = File.open("image.jpg", "r")
  #   image = Axon.jpeg(io_in)         # Read JPEG from a StringIO
  #
  #   jpeg_data = IO.read("image.jpg")
  #   image_2 = Axon.jpeg(jpeg_data)   # Read JPEG from image data
  #
  #   io_in = File.open("image.jpg", "r")
  #   image_3 = Axon.jpeg(io_in, [:APP2]) # Only reads the APP2 marker
  #
  def self.jpeg(thing, *args)
    thing = StringIO.new(thing) unless thing.respond_to?(:read)
    reader = JPEG::Reader.new(thing, *args)
    Image.new(reader)
  end

  # :call-seq:
  #   Axon.jpeg_file(path [, markers], &block) -> block_result
  #
  # Opens the file located at +path+ and reads it as a compressed JPEG image.
  #
  # +markers+ should be an array of valid JPEG header marker symbols. Valid
  # symbols are :APP0 through :APP15 and :COM.
  #
  # If performance is important and you don't care about any markers, you can
  # avaoid reading any header markers by supplying an empty array for +markers+.
  #
  # When +markers+ is not given, all known JPEG markers will be read.
  # 
  # This method returns the result of the block.
  #
  #   Axon.jpeg_file("image.jpg", [:APP2]) do |image|
  #     image.png_file("image.png") # saves "image.jpg" to "image.png"
  #   end
  #
  def self.jpeg_file(path, *args)
    File.open(path, 'rb') do |f|
      yield jpeg(f, *args)
    end
  end

  # :call-seq:
  #   Axon.png(thing) -> image
  #
  # Reads a compressed PNG image from +thing+. +thing+ can be an IO object,
  # the path to a PNG image, or binary PNG data.
  #
  #   io_in = File.open("image.png", "r")
  #   image = Axon.png(io_in)         # Read PNG from a StringIO
  #
  #   png_data = IO.read("image.png")
  #   image_2 = Axon.png(png_data)    # Read PNG from image data
  #
  def self.png(thing)
    thing = StringIO.new(thing) unless thing.respond_to?(:read)
    reader = PNG::Reader.new(thing)
    Image.new(reader)
  end

  # :call-seq:
  #   Axon.png_file(path, &block) -> block_result
  #
  # Opens the file located at +path+ and reads it as a compressed PNG image.
  #
  # This method returns the result of the block.
  #
  #   Axon.png_file("image.png") do |image|
  #     image.jpeg_file("image.jpg") # saves "image.png" to "image.jpeg"
  #   end
  #
  def self.png_file(path, *args)
    File.open(path, 'rb') do |f|
      yield png(f, *args)
    end
  end

  class Image
    # :call-seq:
    #   Image.new(image_in)
    #
    # Wraps +image_in+ in an easy API.
    #
    # Rather than calling Image.new directly, you may want to call Axon.jpeg
    # or Axon.png.
    #
    #   io_in = File.open("image.jpg", "r")
    #   reader = Axon::JPEG::Reader.new(io_in)
    #   image = Axon::Image.new(reader)
    #
    #   image.fit(10, 20)
    #   image.height #=> 20 or less
    #   image.width  #=> 10 or less
    #
    #   io_out = File.open("out.jpg", "w")
    #   image.jpg(io_out) # writes a compressed JPEG file
    #
    def initialize(source)
      @source = source
      self
    end

    # :call-seq:
    #   scale_bilinear(width, height)
    #
    # Scales the image using the bilinear interpolation method.
    #
    # Bilinear interpolation calculates the color values in the resulting image
    # by looking at the four nearest pixels for each pixel in the resulting
    # image.
    #
    # This gives a more accurate representation than nearest-neighbor
    # interpolation, at the expense of slightly blurring the resulting image.
    #
    # == Example
    #
    #   i = Axon::JPEG('test.jpg')
    #   i.scale_bilinear(50, 75)
    #   i.width  # => 50
    #   i.height # => 75
    #
    def scale_bilinear(*args)
      @source = BilinearScaler.new(@source, *args)
      self
    end

    # :call-seq:
    #   scale_nearest(width, height)
    #
    # Scales the image using the nearest-neighbor interpolation method.
    #
    # Nearest-neighbor interpolation selects the value of the nearest pixel when
    # calculating colors in the scaled image.
    #
    # == Example
    #
    #   i = Axon::JPEG('test.jpg')
    #   i.scale_nearest(50, 75)
    #   i.width  # => 50
    #   i.height # => 75
    #
    def scale_nearest(*args)
      @source = NearestNeighborScaler.new(@source, *args)
      self
    end

    # :call-seq:
    #   fit(width, height)
    #
    # Scales the image to fit inside given box dimensions while maintaining the
    # aspect ratio.
    #
    # == Example
    #
    #   i = Axon::JPEG('test.jpg')
    #   i.fit(5, 20)
    #   i.width  # => 5
    #   i.height # => 10
    #
    def fit(*args)
      @source = Fit.new(@source, *args)
      self
    end

    # :call-seq:
    #   crop(width, height, x_offset = 0, y_offset = 0)
    #
    # Crops the image and extracts regions.
    #
    # If the region extends beyond the boundaries of the image then the cropped
    # image will be truncated at the boundaries.
    #
    # == Example
    #
    #   i = Axon::JPEG('test.jpg')
    #   i.crop(50, 75, 10, 20)
    #   c.width  # => 50
    #   c.height # => 75
    #
    # == Example of Cropping Past the Boundaries
    #
    #   i = Axon::JPEG('test.jpg')
    #   i.crop(50, 75, 60, 20)
    #   i.width # => 40 # note that this is not 50
    #
    def crop(*args)
      @source = Cropper.new(@source, *args)
      self
    end

    # :call-seq:
    #   jpeg(io_out [, options])
    #
    # Writes the image to +io_out+ as compressed JPEG data. Returns the number
    # of bytes written.
    #
    # If the image has an alpha channel it will be stripped.
    #
    # +options+ may contain the following symbols:
    #
    # * :bufsize     -- the maximum size in bytes of the writes that will be
    #   made to +io_out+
    # * :quality     -- JPEG quality on a 0..100 scale.
    # * :exif        -- Raw exif string that will be saved in the header.
    # * :icc_profile -- Raw ICC profile string that will be saved in the header.
    #
    # == Example
    #
    #   i = Axon::JPEG('input.jpg')
    #   io_out = File.open('output.jpg', 'w')
    #   i.jpeg(io_out, :quality => 88) # writes the image to output.jpg
    #
    def jpeg(*args)
      case @source.components
      when 2,4 then @source = AlphaStripper.new(@source)
      end
      JPEG.write(@source, *args)
    end

    # :call-seq:
    #   jpeg_file(path [, options])
    #
    # Writes the image to a new file at +path+ as compressed JPEG data.
    # Returns the number of bytes written.
    #
    # If the image has an alpha channel it will be stripped.
    # 
    # See Axon#jpeg for a description of +options+.
    #
    # == Example
    #
    #   Axon.png_file("image.png") do |image|
    #     image.jpeg_file("image.jpg") # saves the image to "image.jpeg"
    #   end
    #
    def jpeg_file(path, *args)
      File.open(path, 'wb') do |f|
        jpeg(f, *args)
      end
    end

    # :call-seq:
    #   png(io_out)
    #
    # Writes the image to +io_out+ as compressed PNG data. Returns the number
    # of bytes written.
    #
    # == Example
    #
    #   i = Axon::JPEG('input.png')
    #   io_out = File.open('output.png', 'w')
    #   i.png(io_out) # writes the image to output.png
    #
    def png(*args)
      PNG.write(@source, *args)
    end

    # :call-seq:
    #   png_file(path)
    #
    # Writes the image to a new file at +path+ as compressed PNG data. Returns
    # the number of bytes written.
    #
    # == Example
    #
    #   Axon.jpeg_file("image.jpg") do |image|
    #     image.png_file("image.png") # saves the image to "image.jpeg"
    #   end
    #
    def png_file(path, *args)
      File.open(path, 'wb') do |f|
        png(f, *args)
      end
    end

    # Gets the components in the image.
    #
    def components
      @source.components
    end

    # Gets the color model of the image.
    #
    def color_model
      @source.color_model
    end

    # Gets the width of the image.
    #
    def width
      @source.width
    end

    # Gets the height of the image.
    #
    def height
      @source.height
    end

    # Gets the index of the next line that will be fetched by gets, starting at
    # 0.
    def lineno
      @source.lineno
    end

    # Gets the next scanline from the image.
    #
    def gets
      @source.gets
    end
  end
end
