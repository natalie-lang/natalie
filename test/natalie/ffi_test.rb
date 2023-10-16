# skip-ruby

require_relative '../spec_helper'
require 'ffi'

SO_EXT = RbConfig::CONFIG['SOEXT']
PRISM_LIBRARY_PATH = File.expand_path("../../build/prism/build/librubyparser.#{SO_EXT}", __dir__)
STUB_LIBRARY_PATH = File.expand_path("../../build/test/support/ffi_stubs.#{SO_EXT}", __dir__)

module TestStubs
  extend FFI::Library
  ffi_lib STUB_LIBRARY_PATH
  attach_function :test_bool, [:bool], :bool
  attach_function :test_char, [:char], :char
  attach_function :test_char_pointer, [:pointer], :pointer
  attach_function :test_size_t, [:size_t], :size_t
end

module LibRubyParser
  extend FFI::Library
  ffi_lib PRISM_LIBRARY_PATH
  attach_function :pm_version, [], :pointer
  attach_function :pm_buffer_init, [:pointer], :bool
  attach_function :pm_buffer_free, [:pointer], :void
  attach_function :pm_buffer_value, [:pointer], :pointer
  attach_function :pm_buffer_length, [:pointer], :size_t
  attach_function :pm_string_sizeof, [], :size_t
end

describe 'FFI' do
  it 'raises an error if the library cannot be found' do
    lambda do
      module Foo
        extend FFI::Library
        ffi_lib "non_existent.so"
      end
    end.should raise_error(
      LoadError,
      /Could not open library.*non_existent\.so/
    )
  end

  it 'links to a shared object' do
    libs = LibRubyParser.instance_variable_get(:@ffi_libs)
    libs.size.should == 1
    lib = libs.first
    lib.should be_an_instance_of FFI::DynamicLibrary
    lib.name.should == PRISM_LIBRARY_PATH
  end

  it 'raises an error if an unknown symbol is used' do
    lambda do
      module Foo
        extend FFI::Library
        ffi_lib PRISM_LIBRARY_PATH
        attach_function :something_unknown, [], :pointer
      end
    end.should raise_error(
      FFI::NotFoundError,
      "Function 'something_unknown' not found in [#{PRISM_LIBRARY_PATH}]"
    )
  end

  it 'raises an error if an unknown type is used' do
    lambda do
      module Foo
        extend FFI::Library
        ffi_lib PRISM_LIBRARY_PATH
        attach_function :pm_version, [], :some_bad_type
      end
    end.should raise_error(
      TypeError,
      "unable to resolve type 'some_bad_type'"
    )
  end

  it 'attaches a function that can be called' do
    version = LibRubyParser.pm_version
    version.address.should be_an_instance_of(Integer)
    version.read_string.should =~ /^\d+\.\d+\.\d+$/

    sizeof = LibRubyParser.pm_string_sizeof
    sizeof.should be_an_instance_of(Integer)
    sizeof.should == 24 # could be different on another architecture I guess
  end

  it 'can allocate a pointer, pass it as an argument, and free it later' do
    pointer = FFI::MemoryPointer.new(24)

    begin
      LibRubyParser.pm_buffer_init(pointer).should == true
    ensure
      LibRubyParser.pm_buffer_free(pointer)
      pointer.free
    end
  end

  it 'can pass and return bools' do
    TestStubs.test_bool(true).should == true
    TestStubs.test_bool(false).should == false
    [1, 'foo', nil].each do |bad_arg|
      -> { TestStubs.test_bool(bad_arg) }.should raise_error(TypeError)
    end
  end

  it 'can pass and return chars' do
    TestStubs.test_char('a'.ord).should == 97
    TestStubs.test_char(0).should == 0
    TestStubs.test_char(120).should == 120
    TestStubs.test_char(512).should == 0
    -> { TestStubs.test_char(nil) }.should raise_error(TypeError)
  end

  it 'can pass and return strings' do
    s = 'foo'
    TestStubs.test_char_pointer(s).read_string.should == 'foo'
  end

  it 'can pass and return integers' do
    TestStubs.test_size_t(3).should == 3
  end
end
