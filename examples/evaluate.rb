#!bin/natalie

# Poor man's emulation of Ruby's `eval`.
# * No binding support, so unable to access local variables
# * Limited to support of bytecode methods (which is pretty much limited to printing strings)
# * Terribly slow (although most of that is to compile Natalie's compiler and parser), creating an
#   executable will make it run at a more acceptable speed).

require 'stringio'
require 'natalie/compiler'
require 'natalie/parser'

def evaluate(code)
  parser = Natalie::Parser.new(code, __FILE__)
  compiler = Natalie::Compiler.new(ast: parser.ast, encoding: parser.encoding, path: __FILE__)
  bytecode = StringIO.new(binmode: true)
  compiler.compile_to_bytecode(bytecode)
  bytecode.rewind
  loader = Natalie::Compiler::Bytecode::Loader.new(bytecode)
  im = Natalie::Compiler::InstructionManager.new(loader.instructions)
  Natalie::VM.new(im, path: __FILE__).run
end

name = ARGV.shift || 'world'
puts evaluate("puts 'hello, #{name}!'; #{name.dump}.upcase").inspect
