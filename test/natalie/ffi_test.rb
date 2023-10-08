# skip-ruby

require_relative '../spec_helper'
require 'ffi'

SO_EXT = RbConfig::CONFIG['SOEXT']
LIBRARY_PATH = File.expand_path("../../build/prism/build/librubyparser.#{SO_EXT}", __dir__)

module LibRubyParser
  extend FFI::Library
  ffi_lib LIBRARY_PATH
  attach_function :pm_version, [], :pointer
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
      /Could not open library 'non_existent\.so': non_existent\.so: cannot open shared object file: No such file or directory\./
    )
  end

  it 'links to a shared object' do
    libs = LibRubyParser.instance_variable_get(:@ffi_libs)
    libs.size.should == 1
    lib = libs.first
    lib.should be_an_instance_of FFI::DynamicLibrary
    lib.name.should == LIBRARY_PATH
  end

  it 'raises an error if an unknown symbol is used' do
    lambda do
      module Foo
        extend FFI::Library
        ffi_lib LIBRARY_PATH
        attach_function :something_unknown, [], :pointer
      end
    end.should raise_error(
      FFI::NotFoundError,
      "Function 'something_unknown' not found in [#{LIBRARY_PATH}]"
    )
  end

  it 'raises an error if an unknown type is used' do
    lambda do
      module Foo
        extend FFI::Library
        ffi_lib LIBRARY_PATH
        attach_function :pm_version, [], :some_bad_type
      end
    end.should raise_error(
      TypeError,
      "unable to resolve type 'some_bad_type'"
    )
  end

  it 'attaches a function that can be called' do
    result = LibRubyParser.pm_version
    result.address.should be_an_instance_of(Integer)
    result.read_string.should =~ /^\d+\.\d+\.\d+$/
  end
end
