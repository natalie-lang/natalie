require 'minitest/autorun'
require 'open3'
require 'shellwords'
require 'tmpdir'
require_relative '../lib/natalie'
require_relative '../lib/natalie/compiler/backends/llvm_backend/native'
require_relative 'support/command'

class LlvmSanitizerTest < Minitest::Test
  def compile_and_run(index)
    ir = <<~LLVM
      define i32 @main() {
      entry:
        %buffer = alloca [1 x i64], align 8
        %element = getelementptr [1 x i64], ptr %buffer, i64 0, i64 #{index}
        store volatile i64 42, ptr %element, align 8
        ret i32 0
      }
    LLVM
    Dir.mktmpdir('natalie-llvm-asan') do |dir|
      object = File.join(dir, 'program.o')
      executable = File.join(dir, 'program')
      Natalie::Compiler::LlvmBackend::Native.new(build: 'sanitized').emit_object(ir, object)
      stdout, stderr, status =
        Open3.capture3(*Shellwords.split(ENV.fetch('CXX', 'c++')), '-fsanitize=address', object, '-o', executable)
      assert status.success?, stdout + stderr
      begin
        [Command.new('env', 'ASAN_OPTIONS=detect_leaks=0:abort_on_error=0', executable, timeout: 20).run, true]
      rescue Command::TimeoutError
        raise
      rescue Command::Error => error
        [error.out, false]
      end
    end
  end

  def test_generated_llvm_code_detects_stack_buffer_overflow
    output, success = compile_and_run(2)
    refute success, output
    assert_match(/AddressSanitizer: stack-buffer-overflow/, output)
  end

  def test_generated_llvm_code_accepts_valid_stack_access
    output, success = compile_and_run(0)
    assert success, output
    assert_empty output
  end
end
