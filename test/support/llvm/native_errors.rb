# Run as a compiled Natalie program to exercise the libffi adapter itself.
module Natalie
  class Compiler
    class CompileError < StandardError
    end
  end
end
require_relative '../../../lib/natalie/compiler/backends/llvm_backend/native'
require 'tmpdir'

native = Natalie::Compiler::LlvmBackend::Native.new(build: 'release')
invalid_ssa = <<~LLVM
  define i64 @bad() {
  entry:
    %x = add i64 1, %x
    ret i64 %x
  }
LLVM
valid = 'define i64 @good() { ret i64 42 }'

Dir.mktmpdir do |dir|
  [
    ['invalid llvm ir', File.join(dir, 'bad.o'), 'expected top-level entity'],
    [invalid_ssa, File.join(dir, 'bad.o'), 'Only PHI nodes may reference their own value'],
    [valid, File.join(dir, 'missing', 'bad.o'), 'No such file or directory'],
  ].each do |ir, path, expected|
    begin
      native.emit_object(ir, path)
      raise 'Expected LLVM compilation to fail'
    rescue Natalie::Compiler::CompileError => error
      raise error unless error.message.include?(expected)
    end
    GC.start
  end

  path = File.join(dir, 'good.o')
  native.emit_object(valid, path)
  raise 'LLVM did not recover after errors' unless File.size?(path)
end

puts 'LLVM native error recovery passed'
