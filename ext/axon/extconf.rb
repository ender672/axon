require 'mkmf'

$CFLAGS << " -g -O0" if ENV['GCC_DEBUG']
png_header_path = []
png_lib_path = []

if RUBY_PLATFORM =~ /darwin/
  $CFLAGS << " -I/usr/X11/include"
  $LDFLAGS << " -L/usr/X11/lib"
  png_lib_path << '/usr/X11/lib'
  png_header_path << '/usr/X11/include'
end

unless find_header('jpeglib.h')
  abort "libjpeg headers are missing. Please install libjpeg headers."
end

unless have_library('jpeg')
  abort "libjpeg is missing. Please install libjpeg."
end

unless find_header('png.h', png_header_path.join(File::PATH_SEPARATOR))
  abort "libpng headers are missing. Please install libpng headers."
end

unless find_library('png', nil, png_lib_path.join(File::PATH_SEPARATOR))
  abort "libpng is missing. Please install libpng."
end

create_makefile('axon/axon')
