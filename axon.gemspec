require 'rake'

Gem::Specification.new do |s|
  s.files = FileList['Rakefile',
                     '*.rdoc',
                     'lib/axon.rb',
                     'lib/axon/*.rb',
                     'ext/axon/*{.c,.h}',
                     'ext/axon/extconf.rb',
                     'test/*.rb'].to_a
  s.name = 'axon'
  s.platform = Gem::Platform::RUBY
  s.require_path = 'lib'
  s.require_paths << 'ext'
  s.description = <<EOF
axon reads, manipulates, and writes images.

axon depends only on libjpeg and libpng.

axon never stores an entire image in memory. All images and operations are
streamed. This keeps memory requirements and latency low.
EOF
  s.summary = 'Axon reads, manipulates, and writes images.'
  s.version = '0.0.2'
  s.authors = ['Timothy Elliott']
  s.extensions << 'ext/axon/extconf.rb'
  s.email = 'tle@holymonkey.com'
  s.homepage = 'http://github.com/ender672/axon'
  s.has_rdoc = true
  s.extra_rdoc_files = ['README.rdoc', 'CHANGELOG.rdoc', 'TODO.rdoc']
  s.test_files = Dir.glob('test/*.rb')
end
