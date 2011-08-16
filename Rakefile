require 'rubygems'
require 'hoe'

HOE = Hoe.spec 'axon' do
  developer('Timothy Elliott', 'tle@holymonkey.com')
  self.readme_file   = 'README.rdoc'
  self.history_file  = 'CHANGELOG.rdoc'
  self.extra_dev_deps << ['rake-compiler', '>= 0']
  self.spec_extras = { :extensions => ["ext/axon/extconf.rb"] }
end

require "rake/extensiontask"

Rake::ExtensionTask.new('axon', HOE.spec) do |ext|
  ext.lib_dir = File.join('lib', 'axon')
end

Rake::Task[:test].prerequisites << :compile

desc 'Run a test in looped mode so that you can look for memory leaks.'
task 'test_loop' do
  code = %Q[require '#{$*[1]}'; loop{ MiniTest::Unit.new.run }]
  cmd = %Q[ruby -Ilib -Itest -e "#{ code }"]
  system cmd
end

desc 'Watch Memory use of a looping test'
task 'test_loop_mem' do
  system 'watch "ps aux | grep Itest"'
end
