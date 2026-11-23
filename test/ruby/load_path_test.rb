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
          { 'NAT_LD_FLAGS' => nil },
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

  it 'initializes native dependencies in shared libraries' do
    Dir.mktmpdir('natalie-shared-native') do |dir|
      library = File.join(dir, 'digest.so')
      cache = File.join(dir, 'cache')
      source = "require 'digest'; return Digest::SHA256.hexdigest('abc')"
      # Populate loaded-file objects without PIC, then reuse the directory for
      # shared output, which must rebuild them with the new compiler flags.
      _, err, status =
        Open3.capture3(
          NAT_BINARY,
          "--build-dir=#{cache}",
          '--build-quietly',
          '-c',
          File.join(dir, 'program'),
          '-e',
          source,
        )
      assert status.success?, err
      _, err, status =
        Open3.capture3(
          NAT_BINARY,
          "--build-dir=#{cache}",
          '--build-quietly',
          '--compilation-type=shared-object',
          '-c',
          library,
          '-e',
          source,
        )
      assert status.success?, err
      symbols, err, status = Open3.capture3('nm', '-g', library)
      assert status.success?, err
      symbol =
        symbols.lines.map { |line| line.split.last }.find { |name| name&.include?('init_') && name.include?('digest') }
      refute_nil symbol
      symbol = symbol.delete_prefix('_') if RUBY_PLATFORM.include?('darwin')
      loader = File.expand_path('../support/llvm/shared_library.cpp', __dir__)
      code = <<~RUBY
        require 'natalie/inline'
        require #{loader.inspect}
        __ld_flags__ '-ldl'
        class LibraryLoader
          __bind_static_method__ :load, :LlvmSharedLibrary_load, 3
        end
        puts LibraryLoader.load(#{library.inspect}, #{symbol.inspect}, self)
      RUBY
      out, err, status = Open3.capture3(NAT_BINARY, '-e', code)
      assert status.success?, err
      assert_equal "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n", out
    end
  end
end
