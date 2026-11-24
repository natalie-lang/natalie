require 'minitest/autorun'
require 'open3'
require 'rbconfig'
require 'tmpdir'

class LlvmDebugInfoTest < Minitest::Test
  ROOT = File.expand_path('..', __dir__)

  def test_debug_objects_map_methods_blocks_and_ensure_callbacks_to_ruby_lines
    Dir.mktmpdir('natalie-llvm-debug') do |dir|
      library = File.join(dir, 'dependency.rb')
      File.write(library, <<~RUBY)
        def answer
          captured = 40
          begin
            -> { captured + 2 }.call
          ensure
            puts :ensured
          end
        end
      RUBY
      source = File.join(dir, 'program.rb')
      File.write(source, "before = :before\n\nrequire #{library.inspect}\np answer")
      executable = File.join(dir, 'program')
      cache = File.join(dir, 'cache')
      compile_debug_program(source, executable, cache)
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "ensured\n42\n", stdout
      assert_empty stderr
      objects = Dir[File.join(cache, '**', '*.o')]
      refute_empty objects
      command =
        if RUBY_PLATFORM.include?('darwin')
          %w[dwarfdump --debug-info --debug-line]
        else
          %w[readelf --wide --debug-dump=info --debug-dump=decodedline]
        end
      stdout, stderr, status = Open3.capture3(*command, *objects)
      assert status.success?, stderr
      assert_includes stdout, 'dependency.rb'
      assert_match(/DW_AT_name[^\n]*answer/, stdout)
      assert_includes stdout, '<block>'
      assert_includes stdout, 'answer (callback)'
      assert_includes stdout, 'DW_TAG_formal_parameter'
      assert_match(/DW_AT_name[^\n]*env/, stdout)
      if RUBY_PLATFORM.include?('darwin')
        assert_match(/0x[0-9a-f]+\s+4\s+1\s+\d+/, stdout)
      else
        assert_match(/dependency\.rb\s+4\s+0x/, stdout)
      end
      main_object = objects.find { |path| File.basename(path) == 'main.o' }
      stdout, stderr, status = Open3.capture3(*command, main_object)
      assert status.success?, stderr
      if RUBY_PLATFORM.include?('darwin')
        assert_match(/0x[0-9a-f]+\s+3\s+1\s+\d+/, stdout)
      else
        assert_match(/program\.rb\s+3\s+0x/, stdout)
      end
    end
  end

  def test_macos_debug_bundles_retain_runtime_ruby_methods
    skip 'Requires macOS debug maps' unless RUBY_PLATFORM.include?('darwin')
    skip 'Requires a debug runtime' if File.read(File.join(ROOT, '.build')).strip == 'release'

    Dir.mktmpdir('natalie-runtime-debug') do |dir|
      source = File.join(dir, 'program.rb')
      File.write(source, 'p [1, 2, 3].sum')
      executable = File.join(dir, 'program')
      compile_debug_program(source, executable, File.join(dir, 'cache'))
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "6\n", stdout
      assert_empty stderr
      stdout, stderr, status = Open3.capture3('dsymutil', executable, chdir: dir)
      assert status.success?, stdout + stderr
      assert_empty stderr
      stdout, stderr, status = Open3.capture3('dwarfdump', '--name=sum', "#{executable}.dSYM")
      assert status.success?, stderr
      assert_includes stdout, 'enumerable.rb'
      assert_match(/DW_AT_name\s+\("sum"\)/, stdout)
    end
  end

  private

  def compile_debug_program(source, executable, cache)
    driver = <<~RUBY
      require #{File.join(ROOT, 'lib/natalie').inspect}
      parser = Natalie::Parser.new(File.read(#{source.inspect}), #{source.inspect})
      compiler = Natalie::Compiler.new(ast: parser.ast, path: #{source.inspect}, options: { backend: 'llvm', build: 'debug', build_dir: #{cache.inspect}, build_quietly: true })
      compiler.out_path = #{executable.inspect}
      compiler.compile
    RUBY
    stdout, stderr, status = Open3.capture3(RbConfig.ruby, '-e', driver, chdir: ROOT)
    assert status.success?, stdout + stderr
  end
end
