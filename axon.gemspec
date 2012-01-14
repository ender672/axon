require 'rake'

Gem::Specification.new do |s|
  s.files = FileList['Rakefile',
                     '*.rdoc',
                     'lib/axon.rb',
                     'lib/axon/*.rb',
                     'ext/axon/*{.c,.h}',
                     'ext/axon/extconf.rb',
                     'ext/java/axon/*.java',
                     'test/*.rb',
                     '.gemtest'
  ].to_a
  s.name = 'axon'
  s.platform = Gem::Platform::RUBY
  s.require_path = 'lib'
  s.require_paths << 'ext'
  s.description = <<EOF
Reads, writes and resizes images quickly and with minimal memory use. Runs on
MRI, Rubinius, and JRuby.
EOF
  s.summary = 'Reads, writes and resizes images quickly.'
  s.version = '0.1.1'
  s.authors = ['Timothy Elliott']
  s.extensions << 'ext/axon/extconf.rb'
  s.email = 'tle@holymonkey.com'
  s.homepage = 'http://github.com/ender672/axon'
  s.has_rdoc = true
  s.extra_rdoc_files = ['README.rdoc', 'CHANGELOG.rdoc', 'TODO.rdoc']
  s.test_files = Dir.glob('test/*.rb')
end
