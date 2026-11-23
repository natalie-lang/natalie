require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'rbconfig'
require_relative '../support/nat_binary'
require_relative '../../lib/natalie/compiler/native_build'

class NativeInitializerTest < Minitest::Test
  def test_top_level_return_with_ensure_returns_to_the_initializer_caller
    Dir.mktmpdir('natalie-native-initializer') do |dir|
      library = File.join(dir, "returning.#{RbConfig::CONFIG.fetch('SOEXT')}")
      source = 'begin; return 42; ensure; $initializer_ensure = true; end'
      out, err, status = Open3.capture3(NAT_BINARY, '--compilation-type=shared-object', '-c', library, '-e', source)
      assert status.success?, out + err
      symbols, err, status = Open3.capture3('nm', '-g', library)
      assert status.success?, err
      initializer = "init_#{Natalie::Compiler::NativeBuild.object_name(library)}"
      symbol = symbols.lines.map { |line| line.split.last }.find { |name| name&.include?(initializer) }
      refute_nil symbol, symbols
      symbol = symbol.delete_prefix('_') if RUBY_PLATFORM.include?('darwin')
      fixture = File.expand_path('../support/llvm/shared_library.cpp', __dir__)
      code = <<~RUBY
        require 'natalie/inline'
        require #{fixture.inspect}
        __ld_flags__ '-ldl'
        class InitializerLoader
          __bind_static_method__ :load, :LlvmSharedLibrary_load, 3
        end
        p InitializerLoader.load(#{library.inspect}, #{symbol.inspect}, Module.new)
        p $initializer_ensure
        p :after_initializer
      RUBY
      out, err, status = Open3.capture3(NAT_BINARY, '-e', code)
      assert status.success?, err
      assert_equal "42\ntrue\n:after_initializer\n", out
      assert_empty err
    end
  end
end
