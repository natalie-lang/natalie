task default: :build

desc 'Build Natalie (same as build_debug)'
task build: :build_debug

desc 'Build Natalie with no optimization and all warnings (default)'
task build_debug: %i[set_build_debug libnatalie]

desc 'Build Natalie with release optimizations enabled and warnings off'
task build_release: %i[set_build_release libnatalie]

desc 'Remove temporary files created during build'
task :clean do
  rm_rf 'build/build.log'
  rm_rf 'build/generated'
  rm_rf 'build/libnatalie_base.a'
  rm_rf Rake::FileList['build/*.o']
end

desc 'Remove all generated files'
task :clobber do
  rm_rf 'build'
end

task distclean: :clobber

desc 'Run the test suite'
task test: :build do
  sh 'bundle exec ruby test/all.rb'
end

desc 'Build the self-hosted version of Natalie at bin/nat'
task bootstrap: [:build, 'bin/nat']

desc 'Show line counts for the project'
task :cloc do
  sh 'cloc include lib src test'
end

desc 'Generate tags file for development'
task :ctags do
  sh 'ctags -R --exclude=.cquery_cache --exclude=ext --exclude=build --append=no .'
end
task tags: :ctags

desc 'Format C++ code with clang-format'
task :format do
  sh "find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +"
  sh "find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +"
  sh 'bundle exec rbprettier --write .'
end

desc 'Show TODO and FIXME comments in the project'
task :todo do
  sh "egrep -r 'FIXME|TODO' src include lib"
end

desc 'Run clang-tidy'
task tidy: %i[build tidy_internal]

# # # # Docker Tasks (used for CI) # # # #

DOCKER_FLAGS =
  if !ENV['CI'] && STDOUT.isatty
    '-i -t'
  elsif ENV['CI']
    "-e CI=#{ENV['CI']}"
  end

task :docker_build do
  sh 'docker build -t natalie .'
end

task docker_bash: :docker_build do
  sh 'docker run -it --rm --entrypoint bash natalie'
end

task :docker_build_clang do
  sh 'docker build -t natalie_clang --build-arg CC=clang --build-arg CXX=clang++ .'
end

task :docker_build_ruby27 do
  sh 'docker build -t natalie_ruby27 --build-arg IMAGE="ruby:2.7" .'
end

task docker_test: %i[docker_test_gcc docker_test_clang docker_test_release docker_test_ruby27]

task docker_test_gcc: :docker_build do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie test"
end

task docker_test_clang: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang test"
end

task docker_test_release: :docker_build do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie clean build_release test"
end

# NOTE: this tests that Natalie can be hosted by MRI 3.0 -- not Natalie under Ruby 3 specs
task docker_test_ruby27: :docker_build_ruby27 do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_ruby27 test"
end

# # # # Build Compile Database # # # #

if system('which compiledb 2>&1 >/dev/null')
  $compiledb_out = []

  def $stderr.puts(str)
    write(str + "\n")
    $compiledb_out << str
  end

  task :write_compile_database do
    if $compiledb_out.any?
      File.write('build/build.log', $compiledb_out.join("\n"))
      sh 'compiledb < build/build.log'
    end
  end
else
  task :write_compile_database do
    # noop
  end
end

# # # # Internal Tasks and Rules # # # #

STANDARD = 'c++17'
HEADERS = Rake::FileList['include/**/{*.h,*.hpp}']
PRIMARY_SOURCES = Rake::FileList['src/**/*.{c,cpp}'].exclude('src/main.cpp', 'src/parser_c_ext/*')
RUBY_SOURCES = Rake::FileList['src/**/*.rb'].exclude('**/extconf.rb')
SPECIAL_SOURCES = Rake::FileList['build/generated/platform.cpp', 'build/generated/bindings.cpp']
OBJECT_FILES =
  PRIMARY_SOURCES.pathmap('build/%f.o') + RUBY_SOURCES.pathmap('build/generated/%f.o') + SPECIAL_SOURCES.pathmap('%p.o')

require 'tempfile'

task(:set_build_debug) do
  ENV['BUILD'] = 'debug'
  File.write('.build', 'debug')
end
task(:set_build_release) do
  ENV['BUILD'] = 'release'
  File.write('.build', 'release')
end

task libnatalie: [
       :update_submodules,
       :bundle_install,
       :build_dir,
       'build/onigmo/lib/libonigmo.a',
       'build/generated/numbers.rb',
       :primary_sources,
       :ruby_sources,
       :special_sources,
       'build/libnatalie.a',
       :write_compile_database,
     ]

task :build_dir do
  mkdir_p 'build/generated' unless File.exist?('build/generated')
end

multitask primary_sources: PRIMARY_SOURCES.pathmap('build/%f.o')
multitask ruby_sources: RUBY_SOURCES.pathmap('build/generated/%f.o')
multitask special_sources: SPECIAL_SOURCES.pathmap('build/generated/%f.o')

file 'build/libnatalie.a' => %w[build/libnatalie_base.a build/onigmo/lib/libonigmo.a] do |t|
  if RUBY_PLATFORM =~ /darwin/
    sh "libtool -static -o #{t.name} #{t.sources.join(' ')}"
  else
    ar_script = ["create #{t.name}"]
    t.sources.each { |source| ar_script << "addlib #{source}" }
    ar_script << 'save'
    ENV['AR_SCRIPT'] = ar_script.join("\n")
    sh 'echo "$AR_SCRIPT" | ar -M'
  end
end

file 'build/libnatalie_base.a' => OBJECT_FILES + HEADERS do |t|
  sh "ar rcs #{t.name} #{OBJECT_FILES}"
end

file 'build/onigmo/lib/libonigmo.a' do
  build_dir = File.expand_path('build/onigmo', __dir__)
  rm_rf build_dir
  cp_r 'ext/onigmo', build_dir
  sh <<-SH
    cd #{build_dir} && \
    sh autogen.sh && \
    ./configure --with-pic --prefix #{build_dir} && \
    make -j 4 && \
    make install
  SH
end

file 'build/generated/numbers.rb' do |t|
  f1 = Tempfile.new(%w[numbers .cpp])
  f2 = Tempfile.create('numbers')
  f2.close
  begin
    f1.puts '#include <stdio.h>'
    f1.puts "#include \"natalie/constants.hpp\""
    f1.puts 'int main() {'
    f1.puts "  printf(\"NAT_MAX_FIXNUM = %lli\\n\", NAT_MAX_FIXNUM);"
    f1.puts "  printf(\"NAT_MIN_FIXNUM = %lli\\n\", NAT_MIN_FIXNUM);"
    f1.puts '}'
    f1.close
    sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -o #{f2.path} #{f1.path}"
    sh "#{f2.path} > #{t.name}"
  ensure
    File.unlink(f1.path)
    File.unlink(f2.path)
  end
end

file 'build/generated/platform.cpp' => OBJECT_FILES - ['build/generated/platform.cpp.o'] do |t|
  git_revision = `git show --pretty=%H --quiet`.chomp
  File.write(t.name, <<~END)
    #include "natalie.hpp"
    const char *Natalie::ruby_platform = #{RUBY_PLATFORM.inspect};
    const char *Natalie::ruby_release_date = "#{Time.now.strftime('%Y-%m-%d')}";
    const char *Natalie::ruby_revision = "#{git_revision}";
  END
end

file 'build/generated/platform.cpp.o' => 'build/generated/platform.cpp' do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.name.pathmap('%d/%n')}"
end

file 'build/generated/bindings.cpp.o' => ['lib/natalie/compiler/binding_gen.rb'] + HEADERS do |t|
  sh "ruby lib/natalie/compiler/binding_gen.rb > #{t.name.pathmap('%d/%n')}"
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.name.pathmap('%d/%n')}"
end

file 'bin/nat' => OBJECT_FILES + ['bin/natalie'] do
  sh 'bin/natalie -c bin/nat bin/natalie'
end

rule '.c.o' => 'src/%n' do |t|
  sh "#{cc} -g -fPIC -c -o #{t.name} #{t.source}"
end

rule '.cpp.o' => ['src/%n'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.o' => ['.rb.cpp'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.cpp' => 'src/%n' do |t|
  sh "bin/natalie --write-obj #{t.name} #{t.source}"
end

file 'build/parser_c_ext.so' => :libnatalie do
  build_dir = File.expand_path('build/parser_c_ext', __dir__)
  rm_rf build_dir
  cp_r 'src/parser_c_ext', build_dir
  sh <<-SH
    cd #{build_dir} && \
    ruby extconf.rb && \
    make && \
    cp parser_c_ext.so ..
  SH
end

task :tidy_internal do
  # FIXME: excluding big_int.cpp for now since clang-tidy thinks it has memory leaks (need to check that).
  sh "clang-tidy --fix #{HEADERS} #{PRIMARY_SOURCES.exclude('src/dtoa.c', 'src/big_int.cpp')}"
end

task :bundle_install do
  sh 'bundle check || bundle install'
end

task :update_submodules do
  sh 'git submodule update --init'
end

def cc
  @cc ||=
    if ENV['CC']
      ENV['CC']
    elsif system('which ccache 2>&1 > /dev/null')
      'ccache cc'
    else
      'cc'
    end
end

def cxx
  @cxx ||=
    if ENV['CXX']
      ENV['CXX']
    elsif system('which ccache 2>&1 > /dev/null')
      'ccache c++'
    else
      'c++'
    end
end

def cxx_flags
  base_flags =
    case ENV['BUILD']
    when 'release'
      %w[-pthread -fPIC -g -O2]
    else
      %w[
        -pthread
        -fPIC
        -g
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unknown-warning-option
      ]
    end
  base_flags + include_paths.map { |path| "-I #{path}" }
end

def include_paths
  [File.expand_path('include', __dir__), File.expand_path('build/onigmo/include', __dir__)]
end
