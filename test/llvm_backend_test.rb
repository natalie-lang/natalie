require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'rbconfig'
require_relative '../lib/natalie'
require_relative '../lib/natalie/compiler/backends/llvm_backend'

class LlvmBackendTest < Minitest::Test
  ROOT = File.expand_path('..', __dir__)
  CLI = File.join(ROOT, 'bin/natalie')

  def run_cli(*args, env: {})
    Open3.capture3(env, RbConfig.ruby, CLI, *args, chdir: ROOT)
  end

  def assert_program(code, expected)
    stdout, stderr, status = run_cli('--backend=llvm', '-e', code)
    assert status.success?, stderr
    assert_equal expected, stdout
    assert_empty stderr
  end

  def test_literals_and_dynamic_dispatch
    assert_program(
      'p [nil, false, true, :hello, -42, 1.5]; p 123456789012345678901234567890 + 1; puts "héllo\u0000world".bytesize',
      "[nil, false, true, :hello, -42, 1.5]\n123456789012345678901234567891\n12\n",
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
      _, stderr, status =
        run_cli('--backend=llvm', '--compilation-type=object', '-c', path, '-e', 'puts 40 + 2', env: env)
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

  def test_unsupported_construct_is_rejected_without_fallback
    _, stderr, status = run_cli('--backend=llvm', '-e', 'p(/#{1}/o)')
    refute status.success?
    assert_includes stderr, 'once-only regexp interpolation'
  end

  def test_short_circuit_branches_merge_the_entire_stack
    assert_program(
      'x = nil; p [x || 3, false && 4, 5 && 6]; p(10 + (x || 2)); p(true || false)',
      "[3, false, 6]\n12\ntrue\n",
    )
  end

  def test_spec_runner_reports_remaining_ruby_instruction_boundary
    _, stderr, status = run_cli('--backend=llvm', 'spec/core/string/strip_spec.rb')
    refute status.success?
    assert_includes stderr, 'LLVM backend does not support hash_merge'
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
    _, stderr, status =
      run_cli('--backend=llvm', '-e', 'require "natalie/inline"; def f; __inline__("return self;"); end; f')
    refute status.success?
    assert_includes stderr, 'scoped inline C++'
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
      compiler = Natalie::Compiler.new(ast: nil, path: '-e')
      compiler.out_path = linked
      Natalie::Compiler::CppBackend::Linker.new(
        in_paths: [object, *Natalie::Compiler::LlvmBackend::RUNTIME_OBJECTS],
        out_path: linked,
        compiler: compiler,
        compiler_context: {
          compile_ld_flags: [],
        },
        keep_inputs: true,
      ).link
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
