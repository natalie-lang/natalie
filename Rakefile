task default: :build

desc 'Build Natalie (same as build_debug)'
task build: :build_debug

desc 'Build Natalie with no optimization and all warnings (default)'
task build_debug: %i[set_build_debug libnatalie parser_c_ext] do
  puts 'Build mode: debug'
end

desc 'Build Natalie with release optimizations enabled and warnings off'
task build_release: %i[set_build_release libnatalie parser_c_ext] do
  puts 'Build mode: release'
end

desc 'Remove temporary files created during build'
task :clean do
  rm_rf 'build/build.log'
  rm_rf 'build/encoding'
  rm_rf 'build/generated'
  rm_rf 'build/libnatalie_base.a'
  rm_rf 'build/natalie_parser'
  rm_rf 'build/libnatalie_parser.a'
  rm_rf 'build/natalie_parser.so'
  rm_rf 'build/natalie_parser.bundle'
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

desc 'Run the most-recently-modified test'
task test_last_modified: :build do
  last_edited = Dir['test/**/*_test.rb', 'spec/**/*_spec.rb'].sort_by { |path| File.stat(path).mtime.to_i }.last
  sh ['bin/natalie', ENV['FLAGS'], last_edited].compact.join(' ')
end

desc 'Run the most-recently-modified test when any source files change (requires entr binary)'
task :watch do
  files = Rake::FileList['**/*.cpp', '**/*.hpp', '**/*.rb']
  sh "ls #{files} | entr -c -s 'rake test_last_modified'"
end

def num_procs
  `command -v nproc 2>&1 >/dev/null && nproc || command -v sysctl 2>&1 >/dev/null && sysctl -n hw.ncpu || echo 4`.strip
rescue SystemCallError
  '4'
end

desc 'Run the test suite using many processes in parallel'
task test_parallel: :build do
  env = {}
  env['PARALLEL'] = 'true'
  env['REPORTER'] = 'dots'
  env['NCPU'] = ENV['NCPU'] || num_procs
  sh env, 'bundle exec ruby test/all.rb'
end

desc 'Build the self-hosted version of Natalie at bin/nat'
task bootstrap: [:build, 'bin/nat']

desc 'Build MRI C Extension for the Natalie Parser'
so_ext = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'
task parser_c_ext: "build/natalie_parser.#{so_ext}"

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
  sh 'docker build -t natalie --build-arg NAT_CXX_FLAGS=-DNAT_GC_GUARD .'
end

task :docker_build_clang do
  sh 'docker build -t natalie_clang --build-arg CC=clang --build-arg CXX=clang++ --build-arg NAT_CXX_FLAGS=-DNAT_GC_GUARD .'
end

task docker_bash: :docker_build do
  sh 'docker run -it --rm --entrypoint bash natalie'
end

task :docker_build_ruby27 do
  sh 'docker build -t natalie_ruby27 --build-arg IMAGE="ruby:2.7" .'
end

task docker_test: %i[docker_test_gcc docker_test_clang docker_test_ruby27]

task docker_test_gcc: :docker_build do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie test"
end

task docker_test_clang: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang test"
end

# NOTE: this tests that Natalie can be hosted by MRI 2.7 -- not Natalie under Ruby 3 specs
task docker_test_ruby27: :docker_build_ruby27 do
  sh "docker run -e RUBYOPT=-W:no-experimental #{DOCKER_FLAGS} --rm --entrypoint rake natalie_ruby27 test"
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
PRIMARY_SOURCES = Rake::FileList['src/**/*.{c,cpp}'].exclude('src/main.cpp')
RUBY_SOURCES = Rake::FileList['src/**/*.rb'].exclude('**/extconf.rb')
SPECIAL_SOURCES = Rake::FileList['build/generated/platform.cpp', 'build/generated/bindings.cpp']
PRIMARY_OBJECT_FILES = PRIMARY_SOURCES.sub('src/', 'build/').pathmap('%p.o')
RUBY_OBJECT_FILES = RUBY_SOURCES.pathmap('build/generated/%f.o')
SPECIAL_OBJECT_FILES = SPECIAL_SOURCES.pathmap('%p.o')
OBJECT_FILES = PRIMARY_OBJECT_FILES + RUBY_OBJECT_FILES + SPECIAL_OBJECT_FILES

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
       'build/libnatalie_parser.a',
       'build/generated/numbers.rb',
       :primary_objects,
       :ruby_objects,
       :special_objects,
       'build/libnatalie.a',
       :write_compile_database,
     ]

task :build_dir do
  mkdir_p 'build/encoding' unless File.exist?('build/encoding')
  mkdir_p 'build/generated' unless File.exist?('build/generated')
end

multitask primary_objects: PRIMARY_OBJECT_FILES
multitask ruby_objects: RUBY_OBJECT_FILES
multitask special_objects: SPECIAL_OBJECT_FILES

file 'build/libnatalie.a' => %w[
  build/libnatalie_parser.a
  build/libnatalie_base.a
  build/onigmo/lib/libonigmo.a
] do |t|
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

file 'build/libnatalie_parser.a' => Rake::FileList['ext/natalie_parser/**/*.{hpp,cpp}'] do
  build_dir = File.expand_path('build/natalie_parser', __dir__)
  rm_rf build_dir
  cp_r 'ext/natalie_parser', build_dir
  sh <<-SH
    cd #{build_dir} && \
    BUILD=release rake library && \
    cp #{build_dir}/build/libnatalie_parser.a #{File.expand_path('build', __dir__)}
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

rule %r{natalie_parser/.*\.cpp\.o$} => ['src/natalie_parser/%n'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.cpp.o' => ['src/%n'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule %r{encoding/.*\.cpp\.o$} => ['src/encoding/%n'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.o' => ['.rb.cpp'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.cpp' => ['src/%n', "build/natalie_parser.#{so_ext}"] do |t|
  sh "bin/natalie --write-obj #{t.name} #{t.source}"
end

file "build/natalie_parser.#{so_ext}" => 'build/libnatalie_parser.a' do |t|
  build_dir = File.expand_path('build/natalie_parser', __dir__)
  sh <<-SH
    cd #{build_dir} && \
    rake parser_c_ext && \
    cp #{build_dir}/ext/natalie_parser/natalie_parser.#{so_ext} #{File.expand_path('build', __dir__)}
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
  unless ENV['SKIP_SUBMODULE_UPDATE']
    sh 'git submodule update --init'
  end
end

def ccache?
  return @ccache_exists if defined?(@ccache_exists)
  @ccache_exists = system('which ccache 2>&1 > /dev/null')
end

def cc
  @cc ||=
    if ENV['CC']
      ENV['CC']
    elsif ccache?
      'ccache cc'
    else
      'cc'
    end
end

def cxx
  @cxx ||=
    if ENV['CXX']
      ENV['CXX']
    elsif ccache?
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
        -Wno-unknown-warning-option
        -Wno-unused-but-set-variable
        -Wno-unused-parameter
        -Wno-unused-value
        -Wno-unused-variable
        -DNAT_GC_GUARD
      ]
    end
  user_flags = Array(ENV['NAT_CXX_FLAGS'])
  base_flags + user_flags + include_paths.map { |path| "-I #{path}" }
end

def include_paths
  [
    File.expand_path('include', __dir__),
    File.expand_path('build/onigmo/include', __dir__),
    File.expand_path('build/natalie_parser/include', __dir__)
  ]
end
