require 'rake/clean'
require 'rake/testtask'

# Java compiling
def quote_file_list(l)
  list = FileList.new(l)
  (list.to_a.map { |f| "'#{f}'" }).join(' ')
end

file 'lib/axon/axon.jar' => FileList.new('ext/java/axon/*.java') do
  cd 'ext/java' do
    sh "javac -g -cp #{Config::CONFIG['prefix']}/lib/jruby.jar #{FileList['axon/*.java']}"
    sh "jar cf axon/axon.jar #{quote_file_list('axon/*.class')}"
  end

  FileUtils.mv 'ext/java/axon/axon.jar', 'lib/axon/axon.jar'
end

# gcc compiling
file 'ext/axon/Makefile' do
  cd 'ext/axon' do
    ruby "extconf.rb #{ENV['EXTOPTS']}"
  end
end

file 'lib/axon/axon.so' => FileList.new('ext/axon/Makefile', 'ext/axon/*{.c,.h}') do
  cd 'ext/axon' do
    sh 'make'
  end
  
  FileUtils.mv 'ext/axon/axon.so', 'lib/axon/axon.so'
end

desc 'Clean up Rubinius .rbc files.'
namespace :clean do
  task :rbc do
    system "find . -name *.rbc -delete"
  end
end

Rake::TestTask.new do |t|
  t.libs << 'test'
  t.test_files = FileList['test/test*.rb']
  t.verbose = true
end

CLEAN.add('ext/axon/*{.o,.so,.log}', 'ext/axon/Makefile')
CLEAN.add('ext/java/axon/*.class', 'ext/java/axon/axon.jar')
CLEAN.add('lib/axon/axon{.so,.jar}')
CLOBBER.add('*.gem')

desc 'Build the gem'
task :gem => "lib/axon/axon.jar" do
  system "gem build axon.gemspec"
end

desc 'Compile the extension'
task :compile => "lib/axon/axon.#{RUBY_PLATFORM =~ /java/ ? 'jar' : 'so'}"

task :test => :compile
task :default => :test
