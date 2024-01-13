task default: :test

desc 'Remove all generated files'
task :clobber do
  rm_rf 'build'
end

task clean: :clobber
task distclean: :clobber

desc 'Run the test suite'
task :test do
  sh 'bundle exec ruby test/all.rb'
end

HEADERS = Rake::FileList['**/*.hpp']
STANDARD = 'c++17'.freeze

desc 'Build compile_commands.json (requires compiledb)'
task :compiledb do
	mkdir_p 'build'
  File.open('build/dummy.cpp', 'w') do |file|
    HEADERS.each do |header|
      file.puts "#include \"#{header.sub('include/', '')}\""
    end
    file.puts 'int main() { return 0; }'
  end
  compiledb_sh "#{cxx} -std=#{STANDARD} -I include -o build/dummy build/dummy.cpp"
end

desc 'Run tests when files change (requires entr binary in path)'
task :watch do
  sh "ls #{HEADERS} | entr -c -s 'rake test'"
end

desc 'Show line counts for the project'
task :cloc do
  sh 'cloc include'
end

desc 'Generate tags file for development'
task :ctags do
  sh 'ctags -R --exclude=.cquery_cache --exclude=build --append=no .'
end
task tags: :ctags

desc 'Format C++ code with clang-format'
task :format do
  sh "find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +"
end

desc 'Show TODO and FIXME comments in the project'
task :todo do
  sh "egrep -r 'FIXME|TODO' src include"
end

# # # # Docker Tasks (used for CI) # # # #

DOCKER_FLAGS =
  if !ENV['CI'] && STDOUT.isatty
    '-i -t'
  elsif ENV['CI']
    "-e CI=#{ENV['CI']}"
  end

task :docker_build do
  sh 'docker build -t tm .'
end

task docker_bash: :docker_build do
  sh 'docker run -it --rm --entrypoint bash tm'
end

task docker_test: :docker_build do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake tm test"
end

# # # # Internal Tasks and Rules # # # #

def ccache_exists?
  return @ccache_exists if defined?(@ccache_exists)
  @ccache_exists = system('which ccache 2>&1 > /dev/null')
end

def cxx
  @cxx ||=
    if ENV['CXX']
      ENV['CXX']
    elsif ccache_exists?
      'ccache c++'
    else
      'c++'
    end
end

def compiledb_sh(command)
  sh command
  sh "echo #{command.inspect} | compiledb"
end
