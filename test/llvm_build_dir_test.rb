require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'fileutils'
require 'rbconfig'
require_relative '../lib/natalie'
require_relative '../lib/natalie/compiler/backends/llvm_backend'

class LlvmBuildDirTest < Minitest::Test
  ROOT = File.expand_path('..', __dir__)
  CLI = File.join(ROOT, 'bin/natalie')
  COMPILER_COMMAND = ENV['NAT_LLVM_COMPILER'] ? [ENV.fetch('NAT_LLVM_COMPILER')] : [RbConfig.ruby, CLI]

  def build(cache, *arguments, env: {}, chdir: ROOT)
    Open3.capture3(
      env,
      *COMPILER_COMMAND,
      '--backend=llvm',
      "--build-dir=#{cache}",
      '--build-quietly',
      *arguments,
      chdir: ROOT,
    )
  end

  def run_ok(cache, *arguments, env: {})
    stdout, stderr, status = build(cache, *arguments, env: env)
    assert status.success?, stderr
    assert_empty stderr
    stdout
  end

  def age_objects(cache)
    paths = Dir[File.join(cache, 'llvm', '**', '*.o')]
    refute_empty paths
    paths.each { |path| File.utime(Time.at(1), Time.at(1), path) }
    paths
  end

  def object_for(cache, source)
    Dir[File.join(cache, 'llvm', '**', "#{Natalie::Compiler::LlvmBackend::Units.id(source)}.o")].fetch(0)
  end

  def test_cached_entrypoint_options_coexist_with_native_dependencies
    Dir.mktmpdir('natalie-llvm-entrypoint-cache') do |dir|
      cache = File.join(dir, 'build')
      code = "require 'tempfile'; Tempfile.create('llvm-entrypoint') { puts 'cached native entrypoint' }; GC.start"
      2.times do |attempt|
        stdout, stderr, status = build(cache, '--profile-app', '--print-objects', '-e', code, chdir: dir)
        assert status.success?, stderr
        assert_match(/Total allocations: \d+/, stdout + stderr)
        path = stdout[/Profile path: (.+)/, 1]
        refute_nil path, stdout
        assert File.size?(path)
        File.unlink(path)
        if attempt.zero?
          objects = age_objects(cache)
          assert objects.any? { |object| File.basename(object) == 'native.o' }
          assert objects.any? { |object| File.basename(object) == 'entrypoint.o' }
        else
          assert Dir[File.join(cache, 'llvm', '**', '*.o')].all? { |object| File.mtime(object) == Time.at(1) }
        end
      end
      stdout = run_ok(cache, '-e', code)
      assert_equal "cached native entrypoint\n", stdout
    end
  end

  def test_reuses_separate_ruby_objects_and_rebuilds_only_the_changed_file
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'build with spaces')
      main = File.join(dir, 'main.rb')
      first = File.join(dir, 'first.rb')
      second = File.join(dir, 'second.rb')
      File.write(first, "value = 'a'; First = value")
      File.write(second, "Second = 'b'")
      File.write(main, "require #{first.inspect}; require #{second.inspect}; puts First + Second")
      assert_equal "ab\n", run_ok(cache, main)
      objects = age_objects(cache)
      assert_equal 3, objects.size
      assert_equal "ab\n", run_ok(cache, main)
      assert objects.all? { |path| File.mtime(path) == Time.at(1) }
      File.write(first, "value = 'changed'; extra = 1; First = value")
      assert_equal "changedb\n", run_ok(cache, main)
      refute_equal Time.at(1), File.mtime(object_for(cache, first))
      assert_equal Time.at(1), File.mtime(object_for(cache, second))
      assert_equal Time.at(1), File.mtime(objects.find { |path| File.basename(path) == 'main.o' })
    end
  end

  def test_native_cache_tracks_headers_flags_and_recovers_from_failed_compilation
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'build with spaces')
      extension_dir = File.join(dir, 'native with spaces')
      FileUtils.mkdir_p(extension_dir)
      header = File.join(extension_dir, 'number.hpp')
      extension = File.join(extension_dir, 'cached_fixture.cpp')
      File.write(header, '#define CACHED_NUMBER 11')
      File.write(extension, <<~CPP)
        #include "natalie.hpp"
        #include "number.hpp"
        #ifndef CACHED_OFFSET
        #define CACHED_OFFSET 0
        #endif
        using namespace Natalie;
        Value init_cached_fixture(Env *env, Value self) { return Value::nil(); }
        Value Cached_number(Env *env, Value self, Args &&args, Block *block) {
            return Value::integer(CACHED_NUMBER + CACHED_OFFSET);
        }
      CPP
      code = <<~RUBY
        require 'natalie/inline'
        require #{extension.inspect}
        class CachedNative
          __bind_static_method__ :number, :Cached_number, 0
        end
        puts CachedNative.number
      RUBY
      assert_equal "11\n", run_ok(cache, '-e', code)
      objects = age_objects(cache)
      native = objects.find { |path| File.basename(path) == 'native.o' }
      assert_equal "11\n", run_ok(cache, '-e', code)
      assert objects.all? { |path| File.mtime(path) == Time.at(1) }
      original_time = File.mtime(header)
      File.write(header, '#define CACHED_NUMBER 12')
      File.utime(original_time, original_time, header)
      assert_equal "12\n", run_ok(cache, '-e', code)
      refute_equal Time.at(1), File.mtime(native)
      ruby_object = objects.find { |path| File.basename(path) == 'main.o' }
      assert_equal Time.at(1), File.mtime(ruby_object)
      File.utime(Time.at(1), Time.at(1), native)
      assert_equal "15\n", run_ok(cache, '-e', code, env: { 'NAT_CXX_FLAGS' => '-DCACHED_OFFSET=3' })
      refute_equal Time.at(1), File.mtime(native)
      File.write(header, '#error intentional_cache_failure')
      _, stderr, status = build(cache, '-e', code)
      refute status.success?
      assert_includes stderr, 'intentional_cache_failure'
      File.write(header, '#define CACHED_NUMBER 12')
      assert_equal "12\n", run_ok(cache, '-e', code)
    end
  end

  def test_cached_required_files_preserve_returns_cycles_and_autoload
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'build')
      first = File.join(dir, 'first.rb')
      second = File.join(dir, 'second.rb')
      autoloaded = File.join(dir, 'autoloaded.rb')
      File.write(first, <<~RUBY)
        require #{second.inspect}
        begin
          return
        ensure
          $loaded_ensure = true
        end
      RUBY
      File.write(second, "require #{first.inspect}; $loaded_second = true")
      File.write(autoloaded, 'CachedAutoload = 42')
      code = <<~RUBY
        require #{first.inspect}
        autoload :CachedAutoload, #{autoloaded.inspect}
        p [$loaded_ensure, $loaded_second, CachedAutoload]
      RUBY
      2.times { assert_equal "[true, true, 42]\n", run_ok(cache, '-e', code) }
      script = File.join(dir, 'self_require.rb')
      File.write(script, <<~RUBY)
        $self_require_count = ($self_require_count || 0) + 1
        require #{script.inspect} if $self_require_count == 1
        p $self_require_count
      RUBY
      assert_equal "2\n2\n", run_ok(cache, script)
    end
  end

  def test_native_wrappers_in_required_units_have_stable_names
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'build')
      first = File.join(dir, 'native_first.rb')
      second = File.join(dir, 'native_second.rb')
      main = File.join(dir, 'main.rb')
      File.write(first, <<~RUBY)
        require 'natalie/inline'
        require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
        __function__ 'llvm_fixture_scale', ['double', 'int'], 'double'
        p $llvm_extension_initialized
        p __call__('llvm_fixture_scale', 2.5, 3)
      RUBY
      File.write(second, <<~RUBY)
        require 'natalie/inline'
        class CachedSecond
          __define_method__ :number, [], 'return Value::integer(2);'
          def self.scaled
            __call__('llvm_fixture_scale', 4.0, 2)
          end
        end
      RUBY
      File.write(
        main,
        "require #{first.inspect}; require #{second.inspect}; p [CachedSecond.scaled, CachedSecond.new.number]",
      )
      expected = "1\n7.5\n[8.0, 2]\n"
      assert_equal expected, run_ok(cache, main)
      objects = age_objects(cache)
      File.open(first, 'a') { |file| file.puts "__cxx_flags__ '-DCACHED_UNUSED_FLAG=1'" }
      assert_equal expected, run_ok(cache, main)
      assert_equal Time.at(1), File.mtime(object_for(cache, second))
      assert_equal Time.at(1), File.mtime(objects.find { |path| File.basename(path) == 'main.o' })
    end
  end

  def test_shared_library_output_reuses_cached_units_and_loads_into_a_host
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'cache')
      dependency = File.join(dir, 'dependency.rb')
      File.write(dependency, '$cached_shared_value = 42')
      library = File.join(dir, 'cache_library.so')
      code = "require #{dependency.inspect}; def answer; $cached_shared_value; end"
      arguments = ['--compilation-type=shared-object', '-c', library, '-e', code]
      assert_equal '', run_ok(cache, *arguments)
      objects = age_objects(cache)
      assert_equal '', run_ok(cache, *arguments)
      assert objects.all? { |path| File.mtime(path) == Time.at(1) }
      names, stderr, status = Open3.capture3('nm', '-g', library)
      assert status.success?, stderr
      symbol =
        names
          .lines
          .map { |line| line.split.last }
          .find { |name| name&.include?('init_') && name.include?('cache_library') }
      refute_nil symbol, names
      symbol = symbol.delete_prefix('_') if RUBY_PLATFORM.include?('darwin')
      loader = <<~RUBY
        require 'natalie/inline'
        require #{File.join(ROOT, 'test/support/llvm/shared_library.cpp').inspect}
        __ld_flags__ '-ldl'
        class CachedLoader
          __bind_static_method__ :load, :LlvmSharedLibrary_load, 3
        end
        target = Class.new
        p CachedLoader.load(#{library.inspect}, #{symbol.inspect}, target)
        p target.new.answer
      RUBY
      assert_equal ":answer\n42\n", run_ok(cache, '-e', loader)
    end
  end

  def test_cached_object_output_links_ruby_and_native_units
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      cache = File.join(dir, 'build')
      dependency = File.join(dir, 'dependency.rb')
      File.write(dependency, 'CachedObjectValue = 42')
      object = File.join(dir, 'combined.o')
      code = <<~RUBY
        require #{dependency.inspect}
        require 'natalie/inline'
        require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
        p CachedObjectValue
        p $llvm_extension_initialized
      RUBY
      2.times { assert_equal '', run_ok(cache, '--compilation-type=object', '-c', object, '-e', code) }
      executable = File.join(dir, 'program')
      compiler = Natalie::Compiler.new(ast: nil, path: '-e')
      compiler.out_path = executable
      Natalie::Compiler::NativeBuild::Linker.new(
        in_paths: [object, *Natalie::Compiler::LlvmBackend::RUNTIME_OBJECTS],
        out_path: executable,
        compiler: compiler,
        compiler_context: {
          compile_ld_flags: [],
        },
        keep_inputs: true,
      ).link
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "42\n1\n", stdout
    end
  end

  def test_multiple_compilers_can_share_the_build_directory
    Dir.mktmpdir('natalie-llvm-cache') do |dir|
      threads = Array.new(3) { Thread.new { build(dir, '-e', 'puts 42') } }
      threads.each do |thread|
        stdout, stderr, status = thread.value
        assert status.success?, stderr
        assert_equal "42\n", stdout
        assert_empty stderr
      end
    end
  end
end
