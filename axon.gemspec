require 'rake'

Gem::Specification.new do |s|
  s.files = FileList['Rakefile',
                     '*.rdoc',
                     'lib/axon.rb',
                     'lib/axon/*.rb',
                     'lib/axon/axon.jar',
                     'ext/axon/*{.c,.h}',
                     'ext/axon/extconf.rb',
                     'ext/java/axon/*.java',
                     'test/*.rb',
                     '.gemtest'
  ].to_a
  s.name = 'axon'
  s.platform = Gem::Platform::RUBY
  s.require_path = 'lib'
  s.description = <<EOF
Read, manipulate, and write images with an emphasis on speed and a low memory
profile.
EOF
  s.summary = 'Read, write and resize images.'
  s.version = '0.2.0'
  s.authors = ['Timothy Elliott']
  s.extensions << 'ext/axon/extconf.rb'
  s.email = 'tle@holymonkey.com'
  s.homepage = 'http://github.com/ender672/axon'
  s.has_rdoc = true
  s.extra_rdoc_files = ['README.rdoc', 'CHANGELOG.rdoc', 'TODO.rdoc']
  s.test_files = Dir.glob('test/*.rb')
end
