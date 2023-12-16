require_relative './lib/natalie/compiler/flags'

task default: :build

DEFAULT_BUILD_TYPE = 'debug'.freeze
DL_EXT = RbConfig::CONFIG['DLEXT']
SO_EXT = RbConfig::CONFIG['SOEXT']
SRC_DIRECTORIES = Dir.new('src').children.select { |p| File.directory?(File.join('src', p)) }

desc 'Build Natalie'
task :build do
  type = File.exist?('.build') ? File.read('.build') : DEFAULT_BUILD_TYPE
  Rake::Task["build_#{type}"].invoke
end

desc 'Build Natalie with no optimization and all warnings (default)'
task build_debug: %i[set_build_debug libnatalie prism_c_ext ctags] do
  puts 'Build mode: debug'
end

desc 'Build Natalie with AddressSanitizer enabled'
task build_asan: %i[set_build_asan libnatalie prism_c_ext] do
  puts 'Build mode: asan'
end

desc 'Build Natalie with release optimizations enabled and warnings off'
task build_release: %i[set_build_release libnatalie prism_c_ext] do
  puts 'Build mode: release'
end

desc 'Remove temporary files created during build'
task :clean do
  SRC_DIRECTORIES.each do |subdir|
    path = File.join('build', subdir)
    rm_rf path
  end
  rm_rf 'build/build.log'
  rm_rf 'build/generated'
  rm_rf 'build/libnatalie_base.a'
  rm_rf "build/libnatalie_base.#{DL_EXT}"
  rm_rf Rake::FileList['build/*.o']
end

desc 'Remove all generated files'
task :clobber do
  rm_rf 'build'
  rm_rf '.build'
end

task distclean: :clobber

desc 'Run the test suite'
task test: %i[build build_test_support] do
  sh 'bundle exec ruby test/all.rb'
end

desc 'Run the most-recently-modified test'
task test_last_modified: :build do
  last_edited = Dir['test/**/*_test.rb', 'spec/**/*_spec.rb'].max_by { |path| File.stat(path).mtime.to_i }
  sh ['bin/natalie', '-I', 'test/support', ENV['FLAGS'], last_edited].compact.join(' ')
end

desc 'Run a folder with tests'
task :test_folder, [:folder] => :build do |task, args|
  if args[:folder].nil?
    warn("Please run with the folder as argument: `rake #{task.name}[<spec/X/Y>]")
    exit(1)
  elsif !File.directory?(args[:folder])
    warn("The folder #{args[:folder]} does not exist or is not a directory")
    exit(1)
  else
    specs = Dir["#{args[:folder]}/**/*_test.rb", "#{args[:folder]}/**/*_spec.rb"]
    sh ['bin/natalie', 'test/runner.rb', specs.to_a].join(' ')
  end
end

desc 'Run the most-recently-modified test when any source files change (requires entr binary)'
task :watch do
  files = Rake::FileList['**/*.cpp', '**/*.hpp', '**/*.rb']
  sh "ls #{files} | entr -c -s 'rake test_last_modified'"
end

desc 'Test that the self-hosted compiler builds and runs'
task test_self_hosted: %i[bootstrap build_test_support] do
  sh 'bin/nat --version'
  # The self-hosted compiler is a bit slow yet, so let's run a core subset of the tests.
  env = {
    'NAT_BINARY' => 'bin/nat',
    'GLOB'       => 'spec/language/*_spec.rb'
  }
  sh env, 'bundle exec ruby test/all.rb'
end

desc 'Test that some representatitve code runs with the AddressSanitizer enabled'
task test_asan: :build_asan do
  sh 'bin/natalie examples/hello.rb'
  sh 'bin/natalie examples/fib.rb'
  sh 'bin/natalie examples/boardslam.rb 3 5 1'
  # These are some tests that are known to pass with AddressSanitizer enabled.
  # We'd like to have all tests passing, of course, but let's start here and
  # then hack away at other failing ones when we get time...
  %w[
    test/natalie/argument_test.rb
    test/natalie/autoload_test.rb
    test/natalie/backtrace_test.rb
    test/natalie/block_test.rb
    test/natalie/boolean_test.rb
    test/natalie/bootstrap_test.rb
    test/natalie/break_test.rb
    test/natalie/builtin_constants_test.rb
    test/natalie/call_order_test.rb
    test/natalie/caller_test.rb
    test/natalie/class_var_test.rb
    test/natalie/comparable_test.rb
    test/natalie/complex_test.rb
    test/natalie/const_defined_test.rb
    test/natalie/constant_test.rb
    test/natalie/define_method_test.rb
    test/natalie/defined_test.rb
    test/natalie/dup_test.rb
    test/natalie/enumerable_test.rb
    test/natalie/env_test.rb
    test/natalie/equality_test.rb
    test/natalie/eval_test.rb
    test/natalie/fiddle_test.rb
    test/natalie/file_test.rb
    test/natalie/fileutils_test.rb
    test/natalie/fork_test.rb
    test/natalie/freeze_test.rb
    test/natalie/global_test.rb
    test/natalie/if_test.rb
    test/natalie/implicit_conversions_test.rb
    test/natalie/instance_eval_test.rb
    test/natalie/io_test.rb
    test/natalie/ivar_test.rb
    test/natalie/kernel_integer_test.rb
    test/natalie/kernel_test.rb
    test/natalie/lambda_test.rb
    test/natalie/load_path_test.rb
    test/natalie/loop_test.rb
    test/natalie/matchdata_test.rb
    test/natalie/method_test.rb
    test/natalie/method_visibility_test.rb
    test/natalie/module_test.rb
    test/natalie/modulo_test.rb
    test/natalie/namespace_test.rb
    test/natalie/next_test.rb
    test/natalie/nil_test.rb
    test/natalie/numeric_test.rb
    test/natalie/range_test.rb
    test/natalie/rational_test.rb
    test/natalie/rbconfig_test.rb
    test/natalie/regexp_test.rb
    test/natalie/require_test.rb
    test/natalie/return_test.rb
    test/natalie/reverse_each_test.rb
    test/natalie/send_test.rb
    test/natalie/shell_test.rb
    test/natalie/singleton_class_test.rb
    test/natalie/socket_test.rb
    test/natalie/spawn_test.rb
    test/natalie/special_globals_test.rb
    test/natalie/super_test.rb
    test/natalie/symbol_test.rb
    test/natalie/tempfile_test.rb
    test/natalie/yield_test.rb
    test/natalie/zlib_test.rb
  ].each do |path|
    sh "bin/natalie #{path}"
  end
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

desc 'Build MRI C Extension for Prism'
task prism_c_ext: ["build/libprism.#{SO_EXT}", "build/prism/ext/prism/prism.#{DL_EXT}"]

desc 'Show line counts for the project'
task :cloc do
  sh 'cloc include lib src test'
end

desc 'Generate tags file for development'
task :ctags do
  if system('which ctags 2>&1 >/dev/null')
    sh "ctags #{HEADERS + SOURCES}"
  else
    puts 'Note: ctags is not available on this system'
  end
end
task tags: :ctags

desc 'Format C++ code with clang-format'
task :format do
  sh "find include src lib " \
     "-type f -name '*.?pp' " \
     "! -path src/encoding/casemap.cpp " \
     "! -path src/encoding/casefold.cpp " \
     "-exec clang-format -i --style=file {} +"
end

desc 'Show TODO and FIXME comments in the project'
task :todo do
  sh "egrep -r 'FIXME|TODO' src include lib"
end

desc 'Run clang-tidy'
task tidy: %i[build tidy_internal]

desc 'Lint GC visiting code'
task gc_lint: %i[build gc_lint_internal]

# # # # Docker Tasks (used for CI) # # # #

DOCKER_FLAGS =
  if !ENV['CI'] && $stdout.isatty
    '-i -t'
  elsif ENV['CI']
    "-e CI=#{ENV['CI']}"
  end

DEFAULT_HOST_RUBY_VERSION = 'ruby3.2'.freeze

task :docker_build_gcc do
  sh "docker build -t natalie_gcc_#{ruby_version_string} " \
     "--build-arg IMAGE='ruby:#{ruby_version_number}' " \
     '--build-arg NAT_CXX_FLAGS=-DNAT_GC_GUARD .'
end

task :docker_build_clang do
  sh "docker build -t natalie_clang_#{ruby_version_string} " \
     "--build-arg IMAGE='ruby:#{ruby_version_number}' " \
     '--build-arg NAT_CXX_FLAGS=-DNAT_GC_GUARD ' \
     '--build-arg CC=clang ' \
     '--build-arg CXX=clang++ ' \
     '--build-arg NAT_CXX_FLAGS=-DNAT_GC_GUARD ' \
     '.'
end

task docker_bash: :docker_build_clang do
  sh "docker run -it --rm --entrypoint bash natalie_clang_#{ruby_version_string}"
end

task docker_bash_lldb: :docker_build_clang do
  sh 'docker run -it --rm ' \
     '--entrypoint bash ' \
     '--cap-add=SYS_PTRACE ' \
     '--security-opt seccomp=unconfined ' \
     "natalie_clang_#{ruby_version_string}"
end

task docker_test: %i[docker_test_gcc docker_test_clang docker_test_self_hosted docker_test_asan]

task docker_test_gcc: :docker_build_gcc do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_gcc_#{ruby_version_string} test"
end

task docker_test_clang: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang_#{ruby_version_string} test"
end

task docker_test_self_hosted: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang_#{ruby_version_string} test_self_hosted"
end

task docker_test_asan: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang_#{ruby_version_string} test_asan"
end

task docker_tidy: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang_#{ruby_version_string} tidy"
end

task docker_gc_lint: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie_clang_#{ruby_version_string} gc_lint"
end

def ruby_version_string
  string = ENV['RUBY'] || DEFAULT_HOST_RUBY_VERSION
  raise 'must be in the format rubyX.Y' unless string =~ /^ruby\d\.\d$/
  string
end

def ruby_version_number
  ruby_version_string.sub('ruby', '')
end

# # # # Build Compile Database # # # #

if system('which compiledb 2>&1 >/dev/null')
  $compiledb_out = [] # rubocop:disable Style/GlobalVars

  def $stderr.puts(str)
    write(str + "\n")
    $compiledb_out << str # rubocop:disable Style/GlobalVars
  end

  task :write_compile_database do
    if $compiledb_out.any? # rubocop:disable Style/GlobalVars
      File.write('build/build.log', $compiledb_out.join("\n")) # rubocop:disable Style/GlobalVars
      sh 'compiledb < build/build.log'
    end
  end
else
  task :write_compile_database do
    # noop
  end
end

# # # # Internal Tasks and Rules # # # #

STANDARD = 'c++17'.freeze
HEADERS = Rake::FileList['include/**/{*.h,*.hpp}']

PRIMARY_SOURCES = Rake::FileList['src/**/*.{c,cpp}'].exclude('src/main.cpp')
RUBY_SOURCES = Rake::FileList['src/**/*.rb'].exclude('**/extconf.rb')
SPECIAL_SOURCES = Rake::FileList['build/generated/platform.cpp', 'build/generated/bindings.cpp']
SOURCES = PRIMARY_SOURCES + RUBY_SOURCES + SPECIAL_SOURCES

PRIMARY_OBJECT_FILES = PRIMARY_SOURCES.sub('src/', 'build/').pathmap('%p.o')
RUBY_OBJECT_FILES = RUBY_SOURCES.pathmap('build/generated/%{^src/,}p.o')
SPECIAL_OBJECT_FILES = SPECIAL_SOURCES.pathmap('%p.o')
OBJECT_FILES = PRIMARY_OBJECT_FILES + RUBY_OBJECT_FILES + SPECIAL_OBJECT_FILES

require 'tempfile'

task(:set_build_debug) do
  ENV['BUILD'] = 'debug'
  File.write('.build', 'debug')
end

task(:set_build_asan) do
  ENV['BUILD'] = 'asan'
  File.write('.build', 'asan')
end

task(:set_build_release) do
  ENV['BUILD'] = 'release'
  File.write('.build', 'release')
end

task libnatalie: [
  :update_submodules,
  :bundle_install,
  :build_dir,
  'build/zlib/libz.a',
  'build/onigmo/lib/libonigmo.a',
  'build/libprism.a',
  "build/libprism.#{SO_EXT}",
  'build/generated/numbers.rb',
  :primary_objects,
  :ruby_objects,
  :special_objects,
  'build/libnatalie.a',
  "build/libnatalie_base.#{DL_EXT}",
  :write_compile_database,
]

task :build_dir do
  mkdir_p 'build/generated' unless File.exist?('build/generated')
end

task build_test_support: "build/test/support/ffi_stubs.#{SO_EXT}"

multitask primary_objects: PRIMARY_OBJECT_FILES
multitask ruby_objects: RUBY_OBJECT_FILES
multitask special_objects: SPECIAL_OBJECT_FILES

file 'build/libnatalie.a' => %w[
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

file "build/libnatalie_base.#{DL_EXT}" => OBJECT_FILES + HEADERS do |t|
  sh "#{cxx} -shared -fPIC -rdynamic -Wl,-undefined,dynamic_lookup -o #{t.name} #{OBJECT_FILES}"
end

file 'build/onigmo/lib/libonigmo.a' do
  build_dir = File.expand_path('build/onigmo', __dir__)
  patch_path = File.expand_path('ext/onigmo.patch', __dir__)
  rm_rf build_dir
  cp_r 'ext/onigmo', build_dir
  sh <<-SH
    cd #{build_dir} && \
    sh autogen.sh && \
    ./configure --with-pic --prefix #{build_dir} && \
    git apply #{patch_path} && \
    make -j 4 && \
    make install
  SH
end

file 'build/zlib/libz.a' do
  build_dir = File.expand_path('build/zlib', __dir__)
  rm_rf build_dir
  cp_r 'ext/zlib', build_dir
  sh <<-SH
    cd #{build_dir} && \
    ./configure && \
    make -j 4
  SH
end

file 'build/generated/numbers.rb' do |t|
  f1 = Tempfile.new(%w[numbers .cpp])
  f2 = Tempfile.create('numbers')
  f2.close
  begin
    f1.puts '#include <stdio.h>'
    f1.puts '#include "natalie/constants.hpp"'
    f1.puts 'int main() {'
    f1.puts '  printf("NAT_MAX_FIXNUM = %lli\n", NAT_MAX_FIXNUM);'
    f1.puts '  printf("NAT_MIN_FIXNUM = %lli\n", NAT_MIN_FIXNUM);'
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

rule '.cpp.o' => ['src/%{build/,}X'] + HEADERS do |t|
  subdir = File.split(t.name).first
  mkdir_p subdir unless File.exist?(subdir)
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.o' => ['.rb.cpp'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

rule '.rb.cpp' => ['src/%{build\/generated/,}X'] do |t|
  subdir = File.split(t.name).first
  mkdir_p subdir unless File.exist?(subdir)
  sh "bin/natalie --write-obj #{t.name} #{t.source}"
end

file "build/libprism.#{SO_EXT}" => ['build/libprism.a']

file 'build/libprism.a' => ["build/prism/ext/prism/prism.#{DL_EXT}"] do
  build_dir = File.expand_path('build/prism', __dir__)
  cp "#{build_dir}/build/libprism.a", File.expand_path('build', __dir__)
  cp "#{build_dir}/build/libprism.#{SO_EXT}", File.expand_path('build', __dir__)
end

file "build/prism/ext/prism/prism.#{DL_EXT}" => Rake::FileList['ext/prism/**/*.{h,c,rb}'] do
  build_dir = File.expand_path('build/prism', __dir__)
  patch_dir = File.expand_path('ext/prism-patches', __dir__)

  rm_rf build_dir
  cp_r 'ext/prism', build_dir

  sh <<-SH
    cd #{build_dir} && \
    git apply #{File.join(patch_dir, 'Rakefile.patch')} && \
    PRISM_FFI_BACKEND=true rake templates
  SH

  # patch some files in Prism
  Dir.glob(File.expand_path('ext/prism-patches/*.patch', __dir__)).each do |patch|
    next if patch.end_with?('Rakefile.patch') # already applied
    sh "cd #{build_dir} && git apply #{patch}"
  end

  sh <<-SH
    cd #{build_dir} && \
    make && \
    cd ext/prism && \
    ruby extconf.rb && \
    make -j 4
  SH
end

file "build/test/support/ffi_stubs.#{SO_EXT}" => 'test/support/ffi_stubs.c' do |t|
  mkdir_p 'build/test/support'
  sh "#{cc} -shared -fPIC -rdynamic -Wl,-undefined,dynamic_lookup -o #{t.name} #{t.source}"
end

task :tidy_internal do
  sh "clang-tidy --warnings-as-errors='*' #{PRIMARY_SOURCES.exclude('src/dtoa.c')}"
end

task :gc_lint_internal do
  sh "ruby test/gc_lint.rb"
end

task :bundle_install do
  sh 'bundle check || bundle install'
end

task :update_submodules do
  unless ENV['SKIP_SUBMODULE_UPDATE']
    sh 'git submodule update --init --recursive'
  end
end

def ccache_exists?
  return @ccache_exists if defined?(@ccache_exists)
  @ccache_exists = system('which ccache 2>&1 > /dev/null')
end

def cc
  @cc ||=
    if ENV['CC']
      ENV['CC']
    elsif ccache_exists?
      'ccache cc'
    else
      'cc'
    end
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

def cxx_flags
  base_flags =
    case ENV['BUILD']
    when 'release'
      Natalie::Compiler::Flags::RELEASE_FLAGS
    when 'asan'
      Natalie::Compiler::Flags::ASAN_FLAGS
    else
      Natalie::Compiler::Flags::DEBUG_FLAGS
    end
  base_flags += ['-fPIC'] # needed for repl
  base_flags += ['-D_DARWIN_C_SOURCE'] if RUBY_PLATFORM =~ /darwin/ # needed for Process.groups to return more than 16 groups on macOS
  user_flags = Array(ENV['NAT_CXX_FLAGS'])
  base_flags + user_flags + include_paths.map { |path| "-I #{path}" }
end

def include_paths
  [
    File.expand_path('include', __dir__),
    File.expand_path('ext/tm/include', __dir__),
    File.expand_path('ext/minicoro', __dir__),
    File.expand_path('build', __dir__),
    File.expand_path('build/onigmo/include', __dir__),
    File.expand_path('build/prism/include', __dir__),
  ]
end
