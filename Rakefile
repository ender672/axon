require 'rake/clean'
require 'rake/testtask'

# Java compiling
if(RUBY_PLATFORM =~ /java/)

  def quote_file_list(l)
    list = FileList.new(l)
    (list.to_a.map { |f| "'#{f}'" }).join(' ')
  end

  file 'ext/axon/java/axon.jar' => FileList.new('ext/axon/java/axon/*.java') do
    cd 'ext/java' do
      sh "javac -g -cp #{Config::CONFIG['prefix']}/lib/jruby.jar #{FileList['axon/*.java']}"
      sh "jar cf axon/axon.jar #{quote_file_list('axon/*.class')}"
    end
  end

  desc 'Compile axon jar extension'
  task :compile => 'ext/axon/java/axon.jar'
# gcc compiling
else
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

  desc 'Compile axon C extension'
  task :compile => 'ext/axon/axon.so'
end

desc 'Clean up Rubinius .rbc files.'
namespace :clean do
  task :rbc do
    system "find . -name *.rbc -delete"
  end
end

Rake::TestTask.new do |t|
  t.libs << 'test'
  t.libs << (RUBY_PLATFORM =~ /java/ ? 'ext/java' : 'ext')
  t.test_files = FileList['test/test*.rb']
  t.verbose = true
end

CLEAN.add('ext/axon/*{.o,.so,.log}', 'ext/axon/Makefile')
CLEAN.add('ext/java/axon/*.class', 'ext/java/axon/axon.jar')
CLOBBER.add('*.gem')

task :test => :compile
task :default => :test
