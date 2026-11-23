require 'minitest/autorun'
require 'open3'
require 'shellwords'
require 'tmpdir'
require_relative '../support/nat_binary'

class AssemblyTest < Minitest::Test
  def assert_assembly(dir, *arguments)
    stdout, stderr, status = Open3.capture3({ 'TMPDIR' => dir }, NAT_BINARY, '-d', 'S', *arguments)
    assert status.success?, stderr
    assert_match(/(?:nat_llvm_eval|EVAL)/, stdout)
    paths = Dir[File.join(dir, '*.s')]
    refute_empty paths
    paths.each_with_index do |path, index|
      object = File.join(dir, "assembled-#{index}.o")
      _, errors, result =
        Open3.capture3(*Shellwords.split(ENV.fetch('CXX', 'c++')), '-c', '-x', 'assembler', path, '-o', object)
      assert result.success?, "#{path}: #{errors}"
      assert File.size?(object)
    end
    paths
  end

  def test_emits_valid_assembly_without_executing_the_program
    Dir.mktmpdir('natalie-assembly') { |dir| assert_assembly(dir, '-e', 'raise "assembly-only"') }
  end

  def test_emits_separate_units_and_native_dependencies
    Dir.mktmpdir('natalie-assembly-units') do |dir|
      File.write(File.join(dir, 'library.rb'), 'AssemblyValue = 42')
      paths =
        assert_assembly(
          dir,
          "--build-dir=#{File.join(dir, 'build')}",
          '-I',
          dir,
          '-e',
          "require 'library'; require 'tempfile'; raise AssemblyValue.to_s",
        )
      assert_operator paths.size, :>, 1
    end
  end
end
