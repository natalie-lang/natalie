require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require_relative '../support/nat_binary'
require_relative '../../lib/natalie/compiler/native_build'

class UnitLocalsTest < Minitest::Test
  def test_object_initializers_keep_captured_locals_separate_from_the_caller
    Dir.mktmpdir('natalie-object-locals') do |dir|
      object = File.join(dir, 'library.o')
      source = 'value = :object; $object_reader = -> { value }; :result'
      stdout, stderr, status = Open3.capture3(NAT_BINARY, '--compilation-type=object', '-c', object, '-e', source)
      assert status.success?, stdout + stderr
      initializer = "init_#{Natalie::Compiler::NativeBuild.object_name(object)}"
      code = <<~RUBY
        require 'natalie/inline'
        __inline__ #{"Value #{initializer}(Env *, Value);".inspect}
        __function__ #{initializer.inspect}, ['Env *', 'Value'], 'Value'
        __ld_flags__ #{object.inspect}
        value = :caller
        reader = -> { value }
        result = __call__(#{initializer.inspect}, self)
        GC.start
        p [reader.call, $object_reader.call, result]
      RUBY
      stdout, stderr, status = Open3.capture3(NAT_BINARY, '-e', code)
      assert status.success?, stderr
      assert_equal "[:caller, :object, :result]\n", stdout
      assert_empty stderr
    end
  end

  def test_loaded_files_keep_captured_locals_separate_from_the_caller
    fixture = File.expand_path('../support/unit_locals.rb', __dir__)
    code = <<~RUBY
      value = :main
      reader = -> { value }
      require #{fixture.inspect}
      first = $unit_reader
      p [reader.call, first.call]
      def import
        value = :method
        reader = -> { value }
        load #{fixture.inspect}
        [reader.call, $unit_reader.call]
      end
      p import
      p [reader.call, first.call]
    RUBY
    [false, true].each do |cached|
      Dir.mktmpdir('natalie-unit-locals') do |dir|
        arguments = []
        arguments << "--build-dir=#{File.join(dir, 'cache')}" << '--build-quietly' if cached
        stdout, stderr, status = Open3.capture3(NAT_BINARY, *arguments, '-e', code)
        assert status.success?, stderr
        assert_equal "[:main, :loaded]\n[:method, :loaded]\n[:main, :loaded]\n", stdout
        assert_empty stderr
      end
    end
  end

  def test_top_level_flip_flop_starts_with_fresh_local_storage
    code = '$hits = []; 10.times { |i| $hits << i if (i == 4)..(i == 4) }; p $hits'
    stdout, stderr, status = Open3.capture3(NAT_BINARY, '-e', code)
    assert status.success?, stderr
    assert_equal "[4]\n", stdout
    assert_empty stderr
  end
end
