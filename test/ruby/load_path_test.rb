require 'minitest/spec'
require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require_relative '../support/nat_binary'

describe 'Compiled load path updates' do
  it 'prepends paths ahead of directories supplied with -I' do
    Dir.mktmpdir('natalie-load-path') do |dir|
      first = File.join(dir, 'first')
      second = File.join(dir, 'second')
      Dir.mkdir(first)
      Dir.mkdir(second)
      File.write(File.join(first, 'load_path_choice.rb'), "puts 'first'")
      File.write(File.join(second, 'load_path_choice.rb'), "puts 'second'")
      code = "$LOAD_PATH.unshift(#{second.inspect}); require 'load_path_choice'"
      out, err, status = Open3.capture3(NAT_BINARY, '-I', first, '-e', code)
      assert status.success?, err
      assert_equal "second\n", out
    end
  end

  it 'keeps expression output separate from paths containing spaces and newlines' do
    Dir.mktmpdir('natalie-load-path') do |dir|
      library = File.join(dir, "with spaces\nand a newline")
      Dir.mkdir(library)
      File.write(File.join(library, 'load_path_output.rb'), "puts 'loaded'")
      code = "$LOAD_PATH << (puts 'evaluating'; #{library.inspect}); require 'load_path_output'"
      out, err, status = Open3.capture3(NAT_BINARY, '-e', code)
      assert status.success?, err
      assert_equal "evaluating\nloaded\n", out
    end
  end

  it 'rejects a path that is not a directory' do
    Dir.mktmpdir('natalie-load-path') do |dir|
      code = "$LOAD_PATH << #{File.join(dir, 'missing').inspect}"
      _, err, status = Open3.capture3(NAT_BINARY, '-e', code)
      refute status.success?
      assert_match(/is not a directory/, err)
    end
  end

  it 'evaluates paths when the parent compiler is building a shared library' do
    Dir.mktmpdir('natalie-load-path') do |dir|
      library = File.join(dir, 'library')
      _, err, status =
        Open3.capture3(
          { 'NAT_LD_FLAGS' => '-shared' },
          NAT_BINARY,
          '--compilation-type=shared-object',
          '--build-dir=build',
          '--build-quietly',
          '-c',
          'library',
          '-e',
          "$LOAD_PATH << #{dir.inspect}",
          chdir: dir,
        )
      assert status.success?, err
      assert File.size(library).positive?
    end
  end

  it 'reports a failed helper instead of accepting an empty result' do
    _, err, status = Open3.capture3(NAT_BINARY, '-e', '$LOAD_PATH << 123')
    refute status.success?
    assert_match(/load path must be a String/, err)
    assert_match(/Could not evaluate load path/, err)
  end

  it 'builds a shared library without a build directory and with punctuation in its path' do
    Dir.mktmpdir('natalie-shared-library') do |dir|
      library = File.join(dir, 'library-with.dots.so')
      _, err, status =
        Open3.capture3(
          { 'NAT_LD_FLAGS' => '-shared' },
          NAT_BINARY,
          '--compilation-type=shared-object',
          '-c',
          library,
          '-e',
          'nil',
        )
      assert status.success?, err
      assert File.size(library).positive?
    end
  end
end
