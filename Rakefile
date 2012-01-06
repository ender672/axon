require 'rake/clean'
require 'rake/testtask'

file 'ext/axon/Makefile' do
  cd 'ext/axon' do
    ruby "extconf.rb #{ENV['EXTOPTS']}"
  end
end

file 'ext/axon/axon.so' => FileList.new('ext/axon/Makefile', 'ext/axon/*{.c,.h}') do
  cd 'ext/axon' do
    sh 'make'
  end
end

CLEAN.add('ext/axon/*{.o,.so,.log}', 'ext/axon/Makefile')
CLOBBER.add('*.gem')

desc 'Clean up Rubinius .rbc files.'
namespace :clean do
  task :rbc do
    system "find . -name *.rbc -delete"
  end
end

Rake::TestTask.new do |t|
  t.libs += ['test', 'ext']
  t.test_files = FileList['test/test*.rb']
  t.verbose = true
end

desc 'Compile axon'
task :compile => 'ext/axon/axon.so'
task :test => :compile
task :default => :test
