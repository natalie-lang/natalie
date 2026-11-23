require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'rbconfig'
require_relative '../lib/natalie'
require_relative '../lib/natalie/compiler/backends/llvm_backend'
require_relative 'support/llvm/object_linker'

class LlvmBackendTest < Minitest::Test
  include LlvmObjectLinker
  ROOT = File.expand_path('..', __dir__)
  CLI = File.join(ROOT, 'bin/natalie')
  COMPILER_COMMAND = ENV['NAT_LLVM_COMPILER'] ? [ENV.fetch('NAT_LLVM_COMPILER')] : [RbConfig.ruby, CLI]

  def run_cli(*args, env: {}, chdir: ROOT)
    Open3.capture3(env, *COMPILER_COMMAND, *args, chdir:)
  end

  def assert_program(code, expected)
    stdout, stderr, status = run_cli('--backend=llvm', '-e', code)
    assert status.success?, stderr
    assert_equal expected, stdout
    assert_empty stderr
  end

  def test_native_helpers_compile_without_loading_the_cpp_backend
    Dir.mktmpdir('natalie-independent-llvm') do |dir|
      output = File.join(dir, 'program')
      code = <<~RUBY
        $LOAD_PATH.unshift(File.expand_path(#{dir.inspect}))
        require 'natalie/inline'
        class IndependentNative
          __define_method__ :answer, [], 'return Value::integer(42);'
        end
        puts IndependentNative.new.answer
      RUBY
      driver = <<~RUBY
        require #{File.join(ROOT, 'lib/natalie').inspect}
        parser = Natalie::Parser.new(#{code.inspect}, '-e')
        compiler = Natalie::Compiler.new(ast: parser.ast, path: '-e', options: { backend: 'llvm' })
        compiler.out_path = #{output.inspect}
        compiler.compile
        abort 'Loaded the C++ Ruby backend' if defined?(Natalie::Compiler::CppBackend)
      RUBY
      stdout, stderr, status = Open3.capture3(RbConfig.ruby, '-e', driver, chdir: ROOT)
      assert status.success?, stdout + stderr
      stdout, stderr, status = Open3.capture3(output)
      assert status.success?, stderr
      assert_equal "42\n", stdout
      assert_empty stderr
    end
  end

  def test_literals_and_dynamic_dispatch
    assert_program(
      'p [nil, false, true, :hello, -42, 1.5]; p 123456789012345678901234567890 + 1; puts "héllo\u0000world".bytesize',
      "[nil, false, true, :hello, -42, 1.5]\n123456789012345678901234567891\n12\n",
    )
  end

  def test_release_optimization_preserves_gc_roots_and_ensured_returns
    Dir.mktmpdir('natalie-llvm-release') do |dir|
      dependency = File.join(dir, 'dependency.rb')
      File.write(dependency, <<~RUBY)
        require 'natalie/inline'
        class OptimizedNative
          __define_method__ :answer, [], 'return Value::integer(42);'
        end
      RUBY
      code = <<~RUBY
        require #{dependency.inspect}
        def build_reader
          captured = Array.new(100) { |i| "value-\#{i}" }
          begin
            return -> { [captured.first, captured.last, OptimizedNative.new.answer] }
          ensure
            GC.start
          end
        end
        reader = build_reader
        10.times do
          Array.new(100) { 'garbage' * 100 }
          GC.start
        end
        p reader.call
        begin
          raise 'expected'
        rescue => e
          puts e.message
        ensure
          puts :ensured
        end
      RUBY
      source = File.join(dir, 'program.rb')
      File.write(source, code)
      executable = File.join(dir, 'program')
      [nil, File.join(dir, 'cache')].each do |cache|
        driver = <<~RUBY
          require #{File.join(ROOT, 'lib/natalie').inspect}
          parser = Natalie::Parser.new(File.read(#{source.inspect}), #{source.inspect})
          options = { backend: 'llvm', build: 'release', build_dir: #{cache.inspect}, build_quietly: true }
          compiler = Natalie::Compiler.new(ast: parser.ast, path: #{source.inspect}, options: options)
          compiler.out_path = #{executable.inspect}
          compiler.compile
        RUBY
        stdout, stderr, status = Open3.capture3(RbConfig.ruby, '-e', driver, chdir: ROOT)
        assert status.success?, stdout + stderr
        stdout, stderr, status = Open3.capture3(executable)
        assert status.success?, stderr
        assert_equal "[\"value-0\", \"value-99\", 42]\nexpected\nensured\n", stdout
        assert_empty stderr
      end
    end
  end

  def test_missing_llvm_library_reports_a_compile_error
    stdout, stderr, status = run_cli('--backend=llvm', '-e', 'puts 42', env: { 'NAT_LLVM_LIBRARY' => '/missing/LLVM' })
    assert_equal 1, status.exitstatus
    assert_empty stdout
    assert_includes stderr, 'Cannot load LLVM:'
    assert_includes stderr, 'NAT_LLVM_LIBRARY'
    refute_includes stderr, 'terminate called'
  end

  def test_module_function_preserves_closures_and_parameter_metadata
    assert_program(<<~'RUBY', "[42, 43]\n[[:req, :value], [:key, :increment]]\n")
      module Example
        offset = 40
        define_method(:calculate) { |value, increment: 1| offset + value + increment }
        module_function :calculate
        class << self
          alias result calculate
        end
      end
      GC.start
      p [Example.calculate(1), Example.result(1, increment: 2)]
      p Example.method(:calculate).parameters
    RUBY
  end

  def test_discovers_llvm_through_llvm_config
    require_relative '../lib/natalie/compiler/backends/llvm_backend/native'
    library = Natalie::Compiler::LlvmBackend::Native.new.send(:library)
    skip 'A library file is needed for llvm-config discovery' unless File.file?(library)
    Dir.mktmpdir('natalie-llvm-config') do |dir|
      File.symlink(
        File.realpath(library),
        File.join(dir, RUBY_PLATFORM.include?('darwin') ? 'libLLVM.dylib' : 'libLLVM.so'),
      )
      config = File.join(dir, 'llvm-config')
      File.write(config, "#!/bin/sh\nprintf '%s\\n' #{dir.shellescape}\n")
      File.chmod(0755, config)
      stdout, stderr, status =
        run_cli(
          '--backend=llvm',
          '-e',
          'puts 42',
          env: {
            'NAT_LLVM_LIBRARY' => nil,
            'PATH' => "#{dir}:#{ENV.fetch('PATH')}",
          },
        )
      assert status.success?, stderr
      assert_equal "42\n", stdout
      assert_empty stderr
    end
  end

  def test_rational_and_complex_literals
    assert_program(
      'p [1r, 0.25r, 2i, 0.5ri, 123456789012345678901234567890r]',
      "[(1/1), (1/4), (0+2i), (0+(1/2)*i), (123456789012345678901234567890/1)]\n",
    )
  end

  def test_conditionals_and_local_initialization
    assert_program(
      'if false; x = 1; end; p x; x = if 0; 2; else; 3; end; p x; p(if nil; 4; else; 5; end); p !false; p !0',
      "nil\n2\n5\ntrue\nfalse\n",
    )
  end

  def test_nested_loops_and_post_condition_loop
    assert_program(
      'sum = 0; i = 0; while i < 3; j = 0; while j < 2; sum = sum + i; j = j + 1; end; i = i + 1; end; begin; sum = sum + 1; end while false; p sum',
      "7\n",
    )
  end

  def test_setter_returns_assigned_value
    assert_program('a = [1, 2]; p(a[0] = 9); p a', "9\n[9, 2]\n")
  end

  def test_gc_roots_survive_repeated_collections
    assert_program(
      'a = ["persistent" * 20]; i = 0; while i < 100; temporary = "x" * 1000; GC.start; i = i + 1; end; puts a[0].size; puts temporary.size',
      "200\n1000\n",
    )
  end

  def test_frozen_strings
    stdout, stderr, status = run_cli('--backend=llvm', '--enable-frozen-string-literal', '-e', 'p "hello".frozen?')
    assert status.success?, stderr
    assert_equal "true\n", stdout
  end

  def test_encoded_strings_and_frozen_literal_identity
    assert_program(<<~'RUBY', "true\n[97, 0, 255]\nASCII-8BIT\nASCII-8BIT\n")
      # encoding: ASCII-8BIT
      # frozen_string_literal: true
      def literal; "a\x00\xff"; end
      first = literal
      5.times { GC.start }
      p first.equal?(literal) && literal.equal?("a\x00\xff")
      p literal.bytes
      puts literal.encoding.name
      puts "ascii".encoding.name
    RUBY
    assert_program(<<~'RUBY', "[233]\nISO-8859-1\n")
      # encoding: ISO-8859-1
      value = "\xe9"
      p value.bytes
      puts value.encoding.name
    RUBY
  end

  def test_constant_lookup_errors_preserve_class_and_module_locations
    code = <<~RUBY
      module LocationModule
        class LocationClass
          MissingLocationConstant
        end
      end
    RUBY
    _, stderr, status = run_cli('--backend=llvm', '-e', code)
    assert_equal 1, status.exitstatus
    assert_includes stderr, "-e:3:in '<class:LocationClass>'"
    assert_includes stderr, "-e:2:in '<module:LocationModule>'"
    assert_includes stderr, "-e:1:in '<main>'"
  end

  def test_runtime_exception_unwinds_through_llvm_frame
    stdout, stderr, status = run_cli('--backend=llvm', '-e', 'puts "before"; raise "llvm exception"; puts "after"')
    assert_equal 1, status.exitstatus
    assert_equal "before\n", stdout
    assert_includes stderr, 'llvm exception (RuntimeError)'
    refute_includes stderr, 'terminate called'
  end

  def test_executable_and_argv
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      path = File.join(dir, 'program')
      _, stderr, status = run_cli('--backend=llvm', '-c', path, '-e', 'p ARGV')
      assert status.success?, stderr
      stdout, stderr, status = Open3.capture3(path, 'one', 'two')
      assert status.success?, stderr
      assert_equal "[\"one\", \"two\"]\n", stdout
    end
  end

  def test_ir_and_object_emission_need_no_cpp_compiler
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      env = { 'CXX' => File.join(dir, 'no-cpp-compiler') }
      stdout, stderr, status = run_cli('--emit-llvm', '-e', 'puts 40 + 2', env: env)
      assert status.success?, stderr
      assert_includes stdout, 'define i64 @nat_llvm_eval'
      refute_includes stdout, '#include'
      path = File.join(dir, 'program.o')
      ir_path = File.join(dir, 'program.ll')
      File.write(ir_path, stdout)
      emit = <<~RUBY
        require #{File.join(ROOT, 'lib/natalie').inspect}
        require #{File.join(ROOT, 'lib/natalie/compiler/backends/llvm_backend/native').inspect}
        Natalie::Compiler::LlvmBackend::Native.new.emit_object(File.read(#{ir_path.inspect}), #{path.inspect})
      RUBY
      _, stderr, status = Open3.capture3(env, RbConfig.ruby, '-e', emit)
      assert status.success?, stderr
      assert_operator File.size(path), :>, 0
    end
  end

  def test_cpp_driver_only_links_precompiled_objects
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      driver = File.join(dir, 'link-only')
      File.write(driver, <<~SH)
        #!/bin/sh
        for arg do
          case "$arg" in
            -c|-x|*.cpp|*.cc) echo 'unexpected C++ compilation' >&2; exit 99 ;;
          esac
        done
        exec "$NAT_LLVM_REAL_CXX" "$@"
      SH
      File.chmod(0o755, driver)
      env = { 'CXX' => driver, 'NAT_LLVM_REAL_CXX' => ENV.fetch('CXX', 'c++') }
      stdout, stderr, status = run_cli('--backend=llvm', '-e', 'puts 42', env: env)
      assert status.success?, stderr
      assert_equal "42\n", stdout
      assert Natalie::Compiler::LlvmBackend::RUNTIME_OBJECTS.all? { |path| File.file?(path) }
    end
  end

  def test_keep_source_retains_usable_llvm_ir
    stdout, stderr, status = run_cli('--backend=llvm', '--keep-source', '-e', 'puts 42')
    assert status.success?, stderr
    assert_includes stdout, "42\n"
    path = stdout[/source file path is: (.+\.ll)/, 1]
    refute_nil path, stdout
    assert_includes File.read(path), '@nat_llvm_eval('
    Dir.mktmpdir('natalie-retained-ir') do |dir|
      require_relative '../lib/natalie/compiler/backends/llvm_backend/native'
      object = File.join(dir, 'retained.o')
      Natalie::Compiler::LlvmBackend::Native.new.emit_object(File.read(path), object)
      assert File.size?(object)
    end
  ensure
    File.unlink(path) if path && File.file?(path)
  end

  def test_print_objects_keeps_allocations_and_dumps_them_at_exit
    stdout, stderr, status = run_cli('--backend=llvm', '--print-objects', '-e', 'puts "llvm object dump"')
    assert status.success?, stderr
    assert_includes stdout, "llvm object dump\n"
    assert_match(/<StringObject 0x[0-9a-f]+ string="llvm object dump">/, stdout + stderr)
    assert_match(/Total allocations: \d+/, stdout + stderr)
  end

  def test_profile_app_records_runtime_events
    Dir.mktmpdir('natalie-llvm-profile') do |dir|
      stdout, stderr, status =
        run_cli('--backend=llvm', '--profile-app', '-e', 'puts "llvm profile"; GC.start', chdir: dir)
      assert status.success?, stderr
      assert_includes stdout, "llvm profile\n"
      path = stdout[/Profile path: (.+)/, 1]
      refute_nil path, stdout
      require 'json'
      events = JSON.parse(File.read(path))
      assert events.any? { |event| event['name'].include?('puts') }, events.inspect
      assert events.any? { |event| event['name'] == 'GC_Collect' }, events.inspect
    end
  end

  def compile_shared_library(dir, name, code, backend: 'llvm')
    path = File.join(dir, 'build', "#{name}.#{RbConfig::CONFIG.fetch('SOEXT')}")
    FileUtils.mkdir_p(File.dirname(path))
    _, stderr, status = run_cli("--backend=#{backend}", '--compilation-type=shared-object', '-c', path, '-e', code)
    assert status.success?, stderr
    symbols, stderr, status = Open3.capture3('nm', '-g', path)
    assert status.success?, stderr
    initializer = Natalie::Compiler::NativeBuild.object_name(path)
    symbol = symbols.lines.map { |line| line.split.last }.find { |name| name&.include?("init_#{initializer}") }
    refute_nil symbol, symbols
    symbol = symbol.delete_prefix('_') if RUBY_PLATFORM.include?('darwin')
    [path, symbol]
  end

  def shared_library_loader
    <<~RUBY
      require 'natalie/inline'
      require #{File.join(ROOT, 'test/support/llvm/shared_library.cpp').inspect}
      __ld_flags__ '-ldl'
      class SharedLibraryLoader
        __bind_static_method__ :load, :LlvmSharedLibrary_load, 3
      end
    RUBY
  end

  def test_shared_libraries_load_into_cpp_and_llvm_hosts_without_symbol_collisions
    Dir.mktmpdir('natalie-shared-test') do |dir|
      first = compile_shared_library(File.join(dir, 'first'), 'library-with-dashes', <<~RUBY)
        require 'natalie/inline'
        __define_method__ :native_answer, [], 'return Value::integer(1);'
        def across(peer)
          peer.receive { return :first }
          :wrong
        end
        def answer
          GC.start
          ['first', native_answer]
        end
      RUBY
      second = compile_shared_library(File.join(dir, 'second'), 'library-with-dashes', <<~RUBY)
        require 'natalie/inline'
        __define_method__ :native_answer, [], 'return Value::integer(9);'
        require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
        __bind_method__ :subtract, :LlvmFixture_subtract, 2
        __bind_static_method__ :apply, :LlvmFixture_yield, 1
        def receive
          yield
          :wrong
        end
        def answer
          GC.start
          ['second', native_answer, subtract(20, 3), self.class.apply(6) { |n| n * 7 }]
        end
      RUBY
      code = shared_library_loader + <<~RUBY
        first = Class.new
        second = Class.new
        original_name = $0
        p SharedLibraryLoader.load(#{first[0].inspect}, #{first[1].inspect}, first)
        p SharedLibraryLoader.load(#{second[0].inspect}, #{second[1].inspect}, second)
        p [first.new.answer, second.new.answer, first.new.answer]
        p $llvm_extension_initialized
        p $0 == original_name
        p first.new.across(second.new)
      RUBY
      %w[cpp llvm].each do |backend|
        stdout, stderr, status = run_cli("--backend=#{backend}", '-e', code)
        assert status.success?, stderr
        assert_equal ":answer\n:answer\n[[\"first\", 1], [\"second\", 9, 17, 42], [\"first\", 1]]\n1\ntrue\n:first\n",
                     stdout
        assert_empty stderr
      end
    end
  end

  def test_shared_library_returns_and_exceptions_cross_the_cpp_abi
    %w[cpp llvm].each do |library_backend|
      Dir.mktmpdir('natalie-shared-test') do |dir|
        plain = compile_shared_library(dir, 'plain', '6 * 7', backend: library_backend)
        dependency = File.join(dir, 'dependency.rb')
        File.write(dependency, <<~RUBY)
          begin
            $shared_dependency = 7
            return
          ensure
            $shared_dependency += 1
          end
        RUBY
        returning = compile_shared_library(dir, 'returning', <<~RUBY, backend: library_backend)
          require #{dependency.inspect}
          begin
            return 42
          ensure
            $shared_ensure = true
          end
        RUBY
        raising = compile_shared_library(dir, 'raising', <<~RUBY, backend: library_backend)
          begin
            raise 'shared failure'
          ensure
            $shared_ensure = :exception
          end
        RUBY
        code = shared_library_loader + <<~RUBY
          p SharedLibraryLoader.load(#{plain[0].inspect}, #{plain[1].inspect}, Module.new)
          p SharedLibraryLoader.load(#{returning[0].inspect}, #{returning[1].inspect}, Module.new)
          p $shared_ensure
          p $shared_dependency
          begin
            SharedLibraryLoader.load(#{raising[0].inspect}, #{raising[1].inspect}, Module.new)
          rescue => e
            p [e.message, $shared_ensure]
          end
        RUBY
        %w[cpp llvm].each do |backend|
          stdout, stderr, status = run_cli("--backend=#{backend}", '-e', code)
          assert status.success?, stderr
          assert_equal "42\n42\ntrue\n8\n[\"shared failure\", :exception]\n", stdout
          assert_empty stderr
        end
      end
    end
  end

  def test_short_circuit_branches_merge_the_entire_stack
    assert_program(
      'x = nil; p [x || 3, false && 4, 5 && 6]; p(10 + (x || 2)); p(true || false)',
      "[3, false, 6]\n12\ntrue\n",
    )
  end

  def test_real_nil_spec_uses_the_normal_runner
    stdout, stderr, status = run_cli('--backend=llvm', 'spec/core/nil/nil_spec.rb')
    assert status.success?, stderr
    assert_includes stdout, '1 spec(s) passed.'
    assert_empty stderr
  end

  def test_real_spec_runner_reports_assertion_failures
    Dir.mktmpdir('natalie-llvm-spec') do |dir|
      file = File.join(dir, 'failure_spec.rb')
      File.write(file, <<~RUBY)
        require #{File.join(ROOT, 'spec/spec_helper').inspect}
        describe 'LLVM runner' do
          it('reports a failed assertion') { nil.should == true }
        end
      RUBY
      stdout, stderr, status = run_cli('--backend=llvm', file)
      assert_equal 1, status.exitstatus, stderr
      assert_includes stdout, '1 spec(s) failed.'
      assert_includes stdout, '0 spec(s) errored.'
      assert_empty stderr
    end
  end

  def native_extension_program
    <<~RUBY
      require 'natalie/inline'
      require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
      class NativeFixture
        __bind_method__ :subtract, :LlvmFixture_subtract, 2
        __bind_static_method__ :apply, :LlvmFixture_yield, 1
      end
      p $llvm_extension_initialized
      p NativeFixture.new.subtract(20, 3)
      p NativeFixture.apply(6) { |n| n * 7 }
      __function__ 'llvm_fixture_scale', ['double', 'int'], 'double'
      p __call__('llvm_fixture_scale', 2.5, 3)
    RUBY
  end

  def test_native_extension_initialization_bindings_and_callbacks
    assert_program(native_extension_program, "1\n17\n42\n7.5\n")
  end

  def test_inline_cpp_method_definition
    assert_program(
      'require "natalie/inline"; class NativeAnswer; __define_method__(:answer, [:n], "return n.send(env, \"+\"_s, { Value::integer(1) });"); end; puts NativeAnswer.new.answer(41)',
      "42\n",
    )
  end

  def test_explicit_native_calls_and_method_arguments
    assert_program(
      "require #{File.join(ROOT, 'test/support/native_helpers.rb').inspect}",
      "nil\n[42, \"record\"]\n[0, 2]\n[[], [[:req], [:req]]]\n[7, 7]\nwrong arity\nwrong arity\n",
    )
  end

  def test_native_extension_allocation_constants_and_compile_flags
    code = <<~RUBY
      require 'natalie/inline'
      require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
      __cxx_flags__ '-DLLVM_NATIVE_TEST_CONSTANT=23'
      class NativeAllocation
        __alloc_func__ :LlvmFixture_allocate
        __constant__ 'ANSWER', 'int', 'LLVM_NATIVE_TEST_CONSTANT'
      end
      NativeAllocation.new
      p $llvm_allocated
      p NativeAllocation::ANSWER
    RUBY
    assert_program(code, "true\n23\n")
  end

  def test_native_exception_unwinds_through_llvm
    code = <<~RUBY
      require 'natalie/inline'
      require #{File.join(ROOT, 'test/support/llvm/fixture.cpp').inspect}
      class NativeFailure
        __bind_static_method__ :fail, :LlvmFixture_fail
      end
      NativeFailure.fail
    RUBY
    _, stderr, status = run_cli('--backend=llvm', '-e', code)
    assert_equal 1, status.exitstatus
    assert_includes stderr, 'native extension failure (RuntimeError)'
    refute_includes stderr, 'terminate called'
  end

  def test_native_extension_in_required_ruby_file
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      file = File.join(dir, 'library.rb')
      File.write(file, native_extension_program)
      assert_program("require #{file.inspect}; p NativeFixture.new.subtract(10, 4)", "1\n17\n42\n7.5\n6\n")
    end
  end

  def test_scoped_inline_cpp_is_rejected_explicitly
    [
      'def f; __inline__("return self;"); end; f',
      '1.times { __inline__("return self;") }',
      'class C; __inline__("return self;"); end',
    ].each do |body|
      _, stderr, status = run_cli('--backend=llvm', '-e', "require 'natalie/inline'; #{body}")
      refute status.success?
      assert_includes stderr, 'scoped __inline__ has been retired'
    end
  end

  def test_native_extension_ir_emission_needs_no_cpp_compiler
    stdout, stderr, status =
      run_cli('--emit-llvm', '-e', native_extension_program, env: { 'CXX' => '/no-cpp-compiler' })
    assert status.success?, stderr
    assert_includes stdout, 'call void @nat_llvm_init_extensions'
    assert_includes stdout, 'declare i64 @nat_llvm_ext_'
    refute_includes stdout, '#include'
  end

  def test_builtin_tempfile_cpp_extension
    assert_program(
      'require "natalie/inline"; require "tempfile.cpp"; class NativeTempfile; __bind_method__ :initialize, :Tempfile_initialize; def file; @tmpfile; end; end; file = NativeTempfile.new("llvm", "/tmp").file; file.write("native works"); file.rewind; p file.read; path = file.path; file.close; File.unlink(path)',
      "\"native works\"\n",
    )
  end

  def test_multiple_object_initializers_link_with_native_macros_and_shared_ruby_dependencies
    [false, true].each do |cached|
      Dir.mktmpdir('natalie-object-initializers') do |dir|
        dependency = File.join(dir, 'dependency.rb')
        File.write(dependency, '$object_dependency_loads = ($object_dependency_loads || 0) + 1')
        first = File.join(dir, 'first.rb')
        second = File.join(dir, 'second.rb')
        File.write(first, <<~RUBY)
          $object_zero = $0
          require #{dependency.inspect}
          require 'natalie/inline'
          class FirstInitializer
            __define_method__ :answer, [], 'return Value::integer(40);'
          end
          $object_reader = -> { FirstInitializer.new.answer }
        RUBY
        File.write(second, <<~RUBY)
          require #{dependency.inspect}
          require 'natalie/inline'
          class SecondInitializer
            __define_method__ :answer, [], 'return Value::integer(2);'
          end
          GC.start
          p [$object_reader.call, SecondInitializer.new.answer, $object_dependency_loads, $0 == $object_zero]
        RUBY
        objects =
          [first, second].map do |source|
            object = "#{source}.o"
            options = ['--backend=llvm', '--compilation-type=object', '-c', object]
            options += ["--build-dir=#{File.join(dir, 'cache')}", '--build-quietly'] if cached
            _, stderr, status = run_cli(*options, source)
            assert status.success?, stderr
            object
          end
        executable = File.join(dir, 'program')
        link_initializer_objects(objects, executable)
        stdout, stderr, status = Open3.capture3(executable)
        assert status.success?, stderr
        assert_equal "[40, 2, 1, true]\n", stdout
        assert_empty stderr
      end
    end
  end

  def test_object_initializers_preserve_return_values_and_ensure
    Dir.mktmpdir('natalie-object-return') do |dir|
      plain = File.join(dir, 'plain.o')
      returning = File.join(dir, 'returning.o')
      [[plain, '40 + 2'], [returning, 'begin; return 43; ensure; puts :ensured; end']].each do |object, code|
        _, stderr, status = run_cli('--backend=llvm', '--compilation-type=object', '-c', object, '-e', code)
        assert status.success?, stderr
      end
      executable = File.join(dir, 'program')
      link_initializer_objects([plain, returning], executable, print_results: true)
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "42\nensured\n43\n", stdout
      assert_empty stderr
    end
  end

  def test_native_extension_executable_and_combined_object
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      executable = File.join(dir, 'program')
      object = File.join(dir, 'program.o')
      _, stderr, status = run_cli('--backend=llvm', '-c', executable, '-e', native_extension_program)
      assert status.success?, stderr
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "1\n17\n42\n7.5\n", stdout
      _, stderr, status =
        run_cli('--backend=llvm', '--compilation-type=object', '-c', object, '-e', native_extension_program)
      assert status.success?, stderr
      linked = File.join(dir, 'from-object')
      link_initializer_objects([object], linked)
      stdout, stderr, status = Open3.capture3(linked)
      assert status.success?, stderr
      assert_equal "1\n17\n42\n7.5\n", stdout
    end
  end

  def test_splat_arguments_yield_and_setter_results
    assert_program(
      'def sum(*a); a.sum; end; p sum(1, *[2,3], 4); def y(*a); yield(*a); end; p y(4,5) { |a,b| a+b }; a = [0]; p(a[*[0]] = 9); p [1,*[2,3],4]',
      "10\n9\n9\n[1, 2, 3, 4]\n",
    )
  end

  def test_anonymous_arguments_and_blocks_survive_closure_capture
    assert_program(
      'def capture(*); proc { GC.start; [*] }; end; p capture(1,2).call; def relay(&); [1,2].map(&); end; p relay { |x| x * 3 }',
      "[1, 2]\n[3, 6]\n",
    )
  end

  def test_full_argument_forwarding_preserves_block
    assert_program(
      'def relay(...); [1,2].map(...); end; p relay { |x| x * 3 }; def print_all(...); p(...); end; print_all(1,2)',
      "[3, 6]\n1\n2\n",
    )
  end

  def test_interpolation_regexps_and_case_matching
    assert_program(
      'x=3; p "hello #{x}!"; pattern="a+"; p /#{pattern}/i.match?("AAA"); p(/x/.frozen?); p case "42"; when /^\\d+$/; :digits; else; :other; end; p case 2; when *[1,2,3]; :member; end',
      "\"hello 3!\"\ntrue\ntrue\n:digits\n:member\n",
    )
  end

  def test_binary_regexp_literals
    assert_program(<<~'RUBY', "ASCII-8BIT\ntrue\nnil\nUTF-8\n[195, 169]\n")
      # encoding: binary
      pattern = /\xAA/
      puts pattern.encoding.name
      p pattern.match?("\xAA")
      p pattern.match("\nA")
      puts /./u.encoding.name
      p /./u.match("\u{e9}")[0].bytes
    RUBY
  end

  def test_case_matching_keeps_other_expression_operands
    assert_program(
      'p [7, case 2; when 1; 10; when 2; 20; else; 30; end, 8]; a,b = [4,5]; p [a,b]',
      "[7, 20, 8]\n[4, 5]\n",
    )
  end

  def test_rescue_results_and_nested_exception_state
    assert_program(
      'p [10, (begin; raise "outer"; rescue => e; p $!.message; begin; raise "inner"; rescue; p $!.message; end; p $!.message; 20; end), 30]; p $!',
      "\"outer\"\n\"inner\"\n\"outer\"\n[10, 20, 30]\nnil\n",
    )
  end

  def test_ensure_runs_for_returns_and_block_breaks
    assert_program(
      'def f; begin; return 2; ensure; puts :method; end; end; p f; p(1.times { begin; break 4; ensure; puts :block; end }); def g; [1].each { return 8 }; 9; end; p g',
      "method\n2\nblock\n4\n8\n",
    )
  end

  def test_lambda_return_and_block_next_run_ensure
    assert_program(
      'f = -> { begin; return 9; ensure; p :lambda; end }; p f.call; p [1,2].map { |x| begin; next x+10; ensure; p x; end }',
      ":lambda\n9\n1\n2\n[11, 12]\n",
    )
  end

  def test_return_from_rescue_and_exception_from_ensure
    assert_program(
      'def f; begin; raise "bad"; rescue; return 7; end; end; p f; begin; begin; raise "first"; ensure; raise "second"; end; rescue => e; p e.message; end; p $!',
      "7\n\"second\"\nnil\n",
    )
  end

  def test_exception_regions_share_locals_and_gc_roots
    assert_program(
      'x = "persistent"; begin; x = [x * 10]; raise "bad"; rescue; GC.start; p x[0].size; x = 4; ensure; p x; end; p x',
      "100\n4\n4\n",
    )
  end

  def test_singleton_methods_class_bodies_and_aliases
    assert_program(
      'class A; def self.name; :a; end; class << self; alias other_name name; end; end; p A.name; p A.other_name; o = Object.new; def o.answer; 42; end; p o.answer',
      ":a\n:a\n42\n",
    )
  end

  def test_super_with_explicit_arguments_and_block
    assert_program(
      'class A; def f(x); yield(x+1); end; end; class B < A; def f(x); super(x*2) { |n| n+3 }; end; alias g f; end; p B.new.g(4)',
      "12\n",
    )
  end

  def test_super_forwards_original_arguments_and_current_block
    assert_program(
      'class A; def f(a,b); yield(a+b); end; end; class B < A; def f(a,b); super; end; end; p B.new.f(3,4) { |n| n*2 }; class C < A; def f(a,b); super(*[a+1,b+1]); end; end; p C.new.f(3,4) { |n| n*2 }',
      "14\n18\n",
    )
  end

  def test_keyword_defaults_rest_and_forwarding
    assert_program(
      'def f(x, a:, b: 2, **rest); [x,a,b,rest]; end; def relay(...); f(...); end; p relay(9,a:1,c:3); def anon(*,**,&); f(*,**,&); end; p anon(8,a:nil,b:false,d:6); h={a:1,z:4}; p f(7,**h); p h',
      "[9, 1, 2, {c: 3}]\n[8, nil, false, {d: 6}]\n[7, 1, 2, {z: 4}]\n{a: 1, z: 4}\n",
    )
  end

  def test_keyword_validation_uses_runtime_diagnostics
    assert_program(
      'def f(a:,b:); end; begin; f; rescue => e; p e.message; end; def g(x,a:); end; begin; g; rescue => e; p e.message; end; def h(a:1); end; begin; h(z:2,y:3); rescue => e; p e.message; end; def none(**nil); :ok; end; p none; begin; none(x:1); rescue => e; p e.message; end',
      "\"missing keywords: :a, :b\"\n\"wrong number of arguments (given 0, expected 1; required keyword: a)\"\n\"unknown keywords: :z, :y\"\n:ok\n\"no keywords accepted\"\n",
    )
  end

  def test_empty_keywords_and_methods_without_keyword_parameters
    assert_program('def f; :ok; end; p f(**{}); def g(*a); a; end; p g(**{}); p g(x:1)', ":ok\n[]\n[{x: 1}]\n")
  end

  def test_keyword_ir_needs_no_generated_cpp
    code = 'def f(a:, b: 2, **rest); [a,b,rest]; end; p f(a:1,x:3)'
    stdout, stderr, status = run_cli('--emit-llvm', '-e', code, env: { 'CXX' => '/no-cpp-compiler' })
    assert status.success?, stderr
    assert_includes stdout, 'nat_llvm_check_keywords'
    refute_includes stdout, 'nat_llvm_init_extensions'
  end

  def test_tempfile_ruby_library_with_native_extension
    assert_program(
      'require "tempfile"; Tempfile.create(["llvm", ".tmp"]) { |f| f.write("real stdlib"); f.rewind; p f.read }',
      "\"real stdlib\"\n",
    )
  end

  def test_loop_jumps_run_ensure_and_return_break_value
    assert_program(
      'i=0; p(while i<5; begin; i+=1; next if i==2; break i if i==4; ensure; p i; end; end)',
      "1\n2\n3\n4\n4\n",
    )
  end

  def test_redo_skips_loop_condition_and_runs_block_ensure
    assert_program(
      'i=0; j=0; while (i+=1)<3; j+=1; redo if j<3; end; p [i,j]; values=[]; [1].each { |x| begin; values << x; redo if values.count(x)<2; ensure; values << :ensure; end }; p values',
      "[3, 4]\n[1, :ensure, 1, :ensure]\n",
    )
  end

  def test_retry_restarts_rescue_body_and_restores_exception
    assert_program(
      'n=0; begin; n+=1; raise "again" if n<3; rescue; retry; ensure; puts :ensure; end; p n; p $!',
      "ensure\n3\nnil\n",
    )
  end

  def test_top_level_return_crosses_loop_and_ensure_regions
    assert_program('i=0; begin; while true; i+=1; return if i==2; end; ensure; p i; end; puts :unreachable', "2\n")
    assert_program('begin; [1].each { return }; ensure; puts :done; end; puts :unreachable', "done\n")
  end

  def test_return_in_required_file_only_exits_that_file
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      file = File.join(dir, 'early_return.rb')
      File.write(file, 'begin; puts :before; return; ensure; puts :ensured; end; puts :unreachable')
      assert_program("p require(#{file.inspect}); puts :loaded", "before\nensured\ntrue\nloaded\n")
    end
  end

  def test_real_redo_spec_runs_ensure_and_cancels_pending_redo
    stdout, stderr, status = run_cli('--backend=llvm', 'spec/language/redo_spec.rb')
    assert status.success?, stderr
    assert_includes stdout, '8 spec(s) passed.'
    assert_empty stderr
  end

  def test_class_variables_follow_runtime_inheritance_and_defaulting
    assert_program(
      'class A; @@n ||= 3; def self.bump; @@n += 1; end; def self.n; @@n; end; end; class B < A; end; p B.bump; p A.n; p B.n',
      "4\n4\n4\n",
    )
  end

  def test_undef_hides_inherited_methods_and_global_aliases_share_storage
    assert_program(
      'class A; def f; 1; end; end; class B < A; undef f; end; p B.new.respond_to?(:f); $first=1; alias $second $first; $second=3; p $first',
      "false\n3\n",
    )
  end

  def test_defined_probes_preserve_nil_valued_variables_and_do_not_call_methods
    assert_program(
      '$g=nil; @i=nil; p [defined?($g),defined?($missing_g),defined?(@i),defined?(@missing_i),defined?(MissingConstant)]; def f; raise "not called"; end; p defined?(f); p defined?(does_not_exist); p defined?(puts); p defined?(f).frozen?',
      "[\"global-variable\", nil, \"instance-variable\", nil, nil]\n\"method\"\nnil\n\"method\"\ntrue\n",
    )
  end

  def test_regexp_capture_access_and_definition_checks
    assert_program(
      '/(a)/ =~ "a"; p $1; p defined?($1); /z/ =~ "a"; p $1; p defined?($1)',
      "\"a\"\n\"global-variable\"\nnil\nnil\n",
    )
  end

  def test_block_destructuring_defaults_and_keyword_hash_copy
    assert_program(
      'f=proc { |(a,b),c=3| [a,b,c] }; p f.call([[1,2]]); p proc { |a=3,b=4| [a,b] }.call; h={a:1,x:2}; p proc { |a:3, **rest| [a,rest] }.call(**h); p h',
      "[1, 2, 3]\n[3, 4]\n[1, {x: 2}]\n{a: 1, x: 2}\n",
    )
  end

  def test_method_proc_and_lambda_parameter_metadata
    assert_program(
      'def f(a,b=2,*rest,c:,d:3,**opts,&block); end; p method(:f).parameters; p proc { |x,y=1,*r,a:,**k,&b| }.parameters; p ->(x,*r,**nil) {}.parameters',
      "[[:req, :a], [:opt, :b], [:rest, :rest], [:keyreq, :c], [:key, :d], [:keyrest, :opts], [:block, :block]]\n[[:opt, :x], [:opt, :y], [:rest, :r], [:keyreq, :a], [:keyrest, :k], [:block, :b]]\n[[:req, :x], [:rest, :r], [:nokey]]\n",
    )
  end

  def test_next_and_break_specs_unwind_ensure_handlers
    %w[next break].each do |name|
      stdout, stderr, status = run_cli('--backend=llvm', "spec/language/#{name}_spec.rb")
      assert status.success?, stderr
      assert_includes stdout, '35 spec(s) passed.'
      assert_empty stderr
    end
  end

  def test_methods_defaults_rest_and_recursion
    assert_program(
      'def sum(n, extra = 1); if n == 0; extra; else; n + sum(n - 1, extra); end; end; def rest(a, *xs); [a, xs]; end; p sum(4); p sum(2, 10); p rest(1, 2, 3)',
      "11\n13\n[1, [2, 3]]\n",
    )
  end

  def test_classes_modules_constants_and_instance_variables
    assert_program(
      'module M; C = 3; class A; def initialize(x); @x = x; end; def value(y = 2); @x + y; end; end; end; p M::C; p M::A.new(4).value; p M::A.new(4).value(5)',
      "3\n6\n9\n",
    )
  end

  def test_nested_and_escaping_closures_share_locals
    assert_program(
      'def counter(n); proc { |step| n = n + step; GC.start; n }; end; c = counter(10); p c.call(2); p c.call(3); x = 0; 2.times { 3.times { x = x + 1 } }; p x',
      "12\n15\n6\n",
    )
  end

  def test_yield_and_explicit_block_parameter
    assert_program(
      'def twice(x); yield x; yield x; end; total = 0; twice(3) { |n| total = total + n }; p total; def apply(&b); b.call(4); end; p apply { |x| x * 2 }',
      "6\n8\n",
    )
  end

  def test_lambda_and_block_next
    assert_program('f = ->(x) { x + 1 }; p f.call(2); p [1, 2, 3].map { |x| next 9 if x == 2; x }', "3\n[1, 9, 3]\n")
  end

  def test_hashes_ranges_and_globals
    assert_program('$answer = {a: 42}; p $answer[:a]; p((2...5).to_a)', "42\n[2, 3, 4]\n")
  end

  def test_wrong_method_arity_and_exception_in_callback
    stdout, stderr, status = run_cli('--backend=llvm', '-e', 'def f(x); x; end; f')
    assert_equal 1, status.exitstatus
    assert_includes stderr, 'wrong number of arguments'
    stdout, stderr, status = run_cli('--backend=llvm', '-e', '1.times { raise "callback error" }')
    assert_equal 1, status.exitstatus
    assert_includes stderr, 'callback error (RuntimeError)'
    refute_includes stderr, 'terminate called'
  end

  def test_require_load_and_separate_file_locals
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      file = File.join(dir, 'required.rb')
      File.write(file, 'x = 99; $loads = ($loads || 0) + 1; def loaded_value; 42; end')
      assert_program(
        "x = 7; p require(#{file.inspect}); p require(#{file.inspect}); p load(#{file.inspect}); p $loads; p loaded_value; p x",
        "true\nfalse\ntrue\n2\n42\n7\n",
      )
    end
  end

  def test_autoload_registration_survives_gc_and_is_deferred
    Dir.mktmpdir('natalie-llvm-autoload') do |dir|
      file = File.join(dir, 'lazy.rb')
      File.write(file, '$autoload_runs = ($autoload_runs || 0) + 1; module LazySpace; Answer = 42; end')
      assert_program(<<~RUBY, "nil\ntrue\n42\n42\n1\nnil\n")
        module LazySpace; end
        def register
          LazySpace.module_eval { autoload :Answer, #{file.inspect} }
        end
        register
        5.times { GC.start }
        p $autoload_runs
        p LazySpace.autoload?(:Answer) == #{file.inspect}
        p LazySpace::Answer
        p LazySpace::Answer
        p $autoload_runs
        p LazySpace.autoload?(:Answer)
      RUBY
    end
  end

  def test_autoload_shares_require_state_and_preserves_caller_locals
    Dir.mktmpdir('natalie-llvm-autoload') do |dir|
      file = File.join(dir, 'shared.rb')
      File.write(
        file,
        '$autoload_runs = ($autoload_runs || 0) + 1; local = :loaded; LazyFirst = 1; LazySecond = 2; return',
      )
      assert_program(<<~RUBY, "[2, 1]\nfalse\n1\n:caller\n")
        local = :caller
        autoload :LazyFirst, #{file.inspect}
        autoload :LazySecond, #{file.inspect}
        p [LazySecond, LazyFirst]
        p require(#{file.inspect})
        p $autoload_runs
        p local
      RUBY
    end
  end

  def test_recursive_require
    Dir.mktmpdir('natalie-llvm-test') do |dir|
      a, b = %w[a.rb b.rb].map { |name| File.join(dir, name) }
      File.write(a, "require_relative 'b'; puts 'a'")
      File.write(b, "require_relative 'a'; puts 'b'")
      assert_program("require #{a.inspect}", "b\na\n")
    end
  end

  def test_invalid_ir_reports_an_error_without_aborting
    require_relative '../lib/natalie/compiler/backends/llvm_backend'
    require_relative '../lib/natalie/compiler/backends/llvm_backend/native'
    error =
      assert_raises(Natalie::Compiler::CompileError) do
        Natalie::Compiler::LlvmBackend::Native.new.emit_object('invalid llvm ir', '/unused.o')
      end
    assert_includes error.message, 'expected top-level entity'
  end

  def test_verifier_rejects_invalid_ssa_without_aborting
    require_relative '../lib/natalie/compiler/backends/llvm_backend/native'
    ir = <<~LLVM
      define i64 @bad() {
      entry:
        br label %other
      other:
        %x = add i64 1, %x
        ret i64 %x
      }
    LLVM
    error =
      assert_raises(Natalie::Compiler::CompileError) do
        Natalie::Compiler::LlvmBackend::Native.new.emit_object(ir, '/unused.o')
      end
    assert_includes error.message, 'Only PHI nodes may reference their own value'
  end
end
