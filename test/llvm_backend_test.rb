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
    _, stderr, status = run_cli('--backend=llvm', '-e', 'def foo; 1; end; foo')
    refute status.success?
    assert_includes stderr, 'LLVM backend does not support define_method'
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
