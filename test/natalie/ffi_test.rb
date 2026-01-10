require_relative '../spec_helper'
require 'ffi'

SO_EXT = RbConfig::CONFIG['SOEXT']
PRISM_LIBRARY_PATH = File.expand_path("../../build/prism/build/libprism.#{SO_EXT}", __dir__)
STUB_LIBRARY_PATH = File.expand_path("../../build/test/support/ffi_stubs.#{SO_EXT}", __dir__)

`rake 'build/test/support/ffi_stubs.#{SO_EXT}'` unless File.exist?(STUB_LIBRARY_PATH)

module TestStubs
  extend FFI::Library
  ffi_lib STUB_LIBRARY_PATH
  enum :test_enum, [:A, :B, :C, 10, :D, :ERR, -1]
  attach_function :get_null, [], :pointer
  attach_function :test_bool, [:bool], :bool
  attach_function :test_char, [:char], :char
  attach_function :test_char_pointer, [:pointer], :pointer
  attach_function :test_int, [:int], :int
  attach_function :test_uint, [:uint], :uint
  attach_function :test_ulong, [:ulong], :ulong
  attach_function :test_ulong_long, [:ulong_long], :ulong_long
  attach_function :test_size_t, [:size_t], :size_t
  attach_function :test_int64, [:int64], :int64
  attach_function :test_string, [], :string
  attach_function :test_string_arg, [:string], :size_t
  attach_function :test_enum_call, [:char], :test_enum
  attach_function :test_enum_argument, [:test_enum], :char
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
      Module.new do
        extend FFI::Library
        ffi_lib 'non_existent.so'
      end
    end.should raise_error(LoadError, /Could not open library.*non_existent\.so/)
  end

  it 'supports an array of strings as library and uses the first one available' do
    foo =
      Module.new do
        extend FFI::Library
        ffi_lib ['non_existent.so', STUB_LIBRARY_PATH, PRISM_LIBRARY_PATH]
      end
    libs = foo.instance_variable_get(:@ffi_libs)
    libs.size.should == 1
    lib = libs.first
    lib.should be_an_instance_of FFI::DynamicLibrary
    lib.name.should == STUB_LIBRARY_PATH
  end

  it 'raises an error if no library can be found' do
    lambda do
      Module.new do
        extend FFI::Library
        ffi_lib %w[non_existent.so neither_existent.so]
      end
    end.should raise_error(
                 LoadError,
                 /Could not open library.*non_existent\.so.*Could not open library.*neither_existent\.so/m,
               )
  end

  it 'raises an error if an empty list is provided' do
    lambda do
      Module.new do
        extend FFI::Library
        ffi_lib []
      end
    end.should raise_error(LoadError)
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
      Module.new do
        extend FFI::Library
        ffi_lib PRISM_LIBRARY_PATH
        attach_function :something_unknown, [], :pointer
      end
    end.should raise_error(FFI::NotFoundError, "Function 'something_unknown' not found in [#{PRISM_LIBRARY_PATH}]")
  end

  it 'raises an error if an unknown type is used' do
    lambda do
      Module.new do
        extend FFI::Library
        ffi_lib PRISM_LIBRARY_PATH
        attach_function :pm_version, [], :some_bad_type
      end
    end.should raise_error(TypeError, "unable to resolve type 'some_bad_type'")
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
    [1, 'foo', nil].each { |bad_arg| -> { TestStubs.test_bool(bad_arg) }.should raise_error(TypeError) }
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
    s = 'foo bar baz'
    TestStubs.test_char_pointer(s).read_string(7).should == 'foo bar'
  end

  it 'can pass and return null' do
    null = TestStubs.get_null
    null.should be_an_instance_of(FFI::Pointer)
    null.should.null?
    null.should == nil

    result = TestStubs.test_char_pointer(null)
    null.should be_an_instance_of(FFI::Pointer)
    null.should.null?
    result.should == nil

    result = TestStubs.test_char_pointer(nil)
    null.should be_an_instance_of(FFI::Pointer)
    null.should.null?
    result.should == nil
  end

  it 'can pass and return int' do
    TestStubs.test_int(3).should == 3
    TestStubs.test_int(-3).should == -3
  end

  it 'can pass and return unsigned int' do
    TestStubs.test_uint(3).should == 3
  end

  it 'can pass and return unsigned long' do
    TestStubs.test_ulong(3).should == 3
  end

  it 'can pass and return unsigned long long' do
    TestStubs.test_ulong_long(3).should == 3
  end

  it 'can pass and return size_t' do
    TestStubs.test_size_t(3).should == 3
  end

  it 'can pass and return int64_t' do
    TestStubs.test_int64(1 << 62).should == 1 << 62
  end

  it 'can return a string' do
    TestStubs.test_string.should == 'string'
  end

  it 'can use string as argument' do
    TestStubs.test_string_arg('foo').should == 3
  end

  it 'uses the underlying bytes of a string without encoding' do
    TestStubs.test_string_arg('😉').should == '😉'.bytesize
  end

  it 'converts an argument using #to_str' do
    to_str = mock('to_str')
    to_str.should_receive(:to_str).and_return('foo')
    TestStubs.test_string_arg(to_str).should == 3
  end

  it 'does not convert an argument using #to_s' do
    to_s = mock('to_s')
    to_s.should_not_receive(:to_s)
    -> { TestStubs.test_string_arg(to_s) }.should raise_error(
                 TypeError,
                 'no implicit conversion of MockObject into String',
               )
  end

  it 'does not allow for embedded nulls' do
    str = "foo\x00bar".b
    -> { TestStubs.test_string_arg(str) }.should raise_error(ArgumentError, 'string contains null byte')
  end

  it 'can return enum values' do
    TestStubs.test_enum_call('a'.ord).should == :A
    TestStubs.test_enum_call('b'.ord).should == :B
    TestStubs.test_enum_call('c'.ord).should == :C
    TestStubs.test_enum_call('d'.ord).should == :D
    TestStubs.test_enum_call('e'.ord).should == 20
    TestStubs.test_enum_call('f'.ord).should == :ERR
  end

  it 'can handle enum arguments' do
    TestStubs.test_enum_argument(:A).should == 'a'.ord
    TestStubs.test_enum_argument(:B).should == 'b'.ord
    TestStubs.test_enum_argument(:C).should == 'c'.ord
    TestStubs.test_enum_argument(:D).should == 'd'.ord
    TestStubs.test_enum_argument(:ERR).should == 'e'.ord
    -> { TestStubs.test_enum_argument(:UNLISTED) }.should raise_error(ArgumentError, 'invalid enum value, :UNLISTED')
    -> { TestStubs.test_enum_argument('UNLISTED') }.should raise_error(ArgumentError, 'invalid enum value, "UNLISTED"')
  end

  it 'accepts numeric values as enum arguments, even when not defined in the enum' do
    TestStubs.test_enum_argument(1).should == 'b'.ord
    TestStubs.test_enum_argument(123).should == 'x'.ord
  end

  it 'supports relative paths' do
    libm =
      Module.new do
        extend FFI::Library
        ffi_lib "libm.#{SO_EXT}"
        attach_function :pow, %i[double double], :double
      end
    libm.pow(2.0, 3.0).should == 8.0
  end

  it 'can omit the file extension' do
    libm =
      Module.new do
        extend FFI::Library
        ffi_lib 'libm'
        attach_function :pow, %i[double double], :double
      end
    libm.pow(2.0, 3.0).should == 8.0
  end

  it 'can omit the file extension and the "lib" part' do
    libm =
      Module.new do
        extend FFI::Library
        ffi_lib 'm'
        attach_function :pow, %i[double double], :double
      end
    libm.pow(2.0, 3.0).should == 8.0
  end

  it 'has a Library::LIBC constant' do
    libc =
      Module.new do
        extend FFI::Library
        ffi_lib FFI::Library::LIBC
        attach_function :abs, [:int], :int # Random LibC function
      end
    libc.abs(-13).should == 13
  end

  it 'supports enums as constants' do
    libtest =
      Module.new do
        extend FFI::Library
        ffi_lib STUB_LIBRARY_PATH
        TestEnum = enum(:A, :B, :C, 10, :D, :ERR, -1)
        attach_function :test_enum_call, [:char], TestEnum
        attach_function :test_enum_argument, [TestEnum], :char
      end
    libtest.test_enum_call('a'.ord).should == :A
    libtest.test_enum_call('b'.ord).should == :B
    libtest.test_enum_call('c'.ord).should == :C
    libtest.test_enum_call('d'.ord).should == :D
    libtest.test_enum_call('e'.ord).should == 20
    libtest.test_enum_call('f'.ord).should == :ERR

    libtest.test_enum_argument(:A).should == 'a'.ord
    libtest.test_enum_argument(:B).should == 'b'.ord
    libtest.test_enum_argument(:C).should == 'c'.ord
    libtest.test_enum_argument(:D).should == 'd'.ord
    libtest.test_enum_argument(:ERR).should == 'e'.ord
    -> { libtest.test_enum_argument(:UNLISTED) }.should raise_error(ArgumentError, 'invalid enum value, :UNLISTED')
    -> { libtest.test_enum_argument('UNLISTED') }.should raise_error(ArgumentError, 'invalid enum value, "UNLISTED"')
  end

  describe 'Pointer' do
    describe '#initialize' do
      it 'sets address and type_size' do
        pointer = FFI::Pointer.new(:pointer, 123)
        pointer.address.should == 123
        pointer.type_size.should == 8

        pointer = FFI::Pointer.new(123)
        pointer.address.should == 123
        pointer.type_size.should == 1
      end

      it 'sets autorelease to false' do
        pointer = FFI::Pointer.new(:pointer, 123)
        pointer.autorelease?.should == false
      end
    end

    describe '#autorelease=' do
      it 'changes the autorelease behavior' do
        pointer = FFI::Pointer.new(:pointer, 123)
        pointer.autorelease = true
        pointer.autorelease?.should == true
        pointer.autorelease = false
        pointer.autorelease?.should == false
      end
    end
  end

  describe 'MemoryPointer' do
    describe '#initialize' do
      it 'sets address and type_size' do
        pointer = FFI::MemoryPointer.new(100)
        pointer.address.should be_an_instance_of(Integer)
        pointer.type_size.should == 100
      end

      it 'sets autorelease to true' do
        pointer = FFI::MemoryPointer.new(100)
        pointer.autorelease?.should == true
      end
    end

    describe '#free' do
      it 'frees the pointer' do
        pointer = FFI::MemoryPointer.new(100)
        pointer.free
        # I guess we have to trust that this worked because
        # I don't see a way to check if a pointer has been freed.
      end
    end
  end
end
