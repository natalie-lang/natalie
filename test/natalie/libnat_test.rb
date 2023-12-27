# skip-ruby

require_relative '../spec_helper'

require 'ffi'
require 'tempfile'
require 'natalie/inline'

module LibNat
  extend FFI::Library
  ffi_lib "build/libnat.#{RbConfig::CONFIG['SOEXT']}"

  attach_function :init_libnat2, %i[pointer pointer], :pointer
  attach_function :new_parser, %i[pointer pointer pointer], :pointer
  attach_function :new_compiler, %i[pointer pointer pointer], :pointer

  def self.init
    env = FFI::Pointer.from_env
    init_libnat2(env, FFI::Pointer.new(:pointer, self.object_id))
  end

  def self.parse(code, path)
    locals = []
    parser = LibNat.new_parser(code.to_ptr, path.to_ptr, locals.to_ptr).to_obj
    parser.ast
  end

  def self.compile(ast, path, encoding)
    compiler = LibNat.new_compiler(ast.to_ptr, path.to_ptr, encoding.to_ptr).to_obj
    temp = Tempfile.create("natalie.#{RbConfig::CONFIG['SOEXT']}")
    compiler.repl = true # actually this should be called "shared"
    compiler.out_path = temp.path
    compiler.compile
    temp.path
  end
end

describe 'libnat.so' do
  before :all do
    GC.disable
    LibNat.init
  end

  it 'can parse code' do
    ast = LibNat.parse('1 + 2', 'bar.rb')
    ast.should be_an_instance_of(Prism::ProgramNode)
  end

  it 'can compile code' do
    ast = LibNat.parse('1 + 2', 'foo.rb')
    temp_path = LibNat.compile(ast, 'foo.rb', Encoding::UTF_8)

    library = Module.new do
      extend FFI::Library
      ffi_lib(temp_path)
      attach_function :EVAL, [:pointer], :pointer
    end

    env = FFI::Pointer.from_env
    result = library.EVAL(env).to_obj
    result.should == 3
  ensure
    File.unlink(temp_path)
  end
end
