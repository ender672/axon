require 'mkmf'

$CFLAGS += " -g -O0" if ENV['GCC_DEBUG']

unless find_header('jpeglib.h')
  abort "libjpeg headers are missing. Please install libjpeg headers."
end

unless have_library('jpeg', nil)
  abort "libjpeg is missing. Please install libjpeg."
end

unless find_header('png.h')
  abort "libpng headers are missing. Please install libpng headers."
end

unless have_library('png', nil)
  abort "libpng is missing. Please install libpng."
end

create_makefile('jpeg/axon')
