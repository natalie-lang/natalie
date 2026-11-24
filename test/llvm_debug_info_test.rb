require 'minitest/autorun'
require 'open3'
require 'rbconfig'
require 'tmpdir'
require_relative 'support/llvm/object_linker'
require_relative '../lib/natalie/compiler/backends/llvm_backend'

class LlvmDebugInfoTest < Minitest::Test
  include LlvmObjectLinker
  ROOT = File.expand_path('..', __dir__)
  CLI = File.join(ROOT, 'bin/natalie')
  COMPILER_COMMAND = ENV['NAT_LLVM_COMPILER'] ? [ENV.fetch('NAT_LLVM_COMPILER')] : [RbConfig.ruby, CLI]

  def test_macos_uncached_object_initializers_retain_debug_symbols
    skip 'Requires macOS debug maps' unless RUBY_PLATFORM.include?('darwin')

    Dir.mktmpdir('natalie-object-debug') do |dir|
      source = File.join(dir, 'library.rb')
      File.write(source, "def answer\n  42\nend\nanswer\n")
      object = File.join(dir, 'library.o')
      compile_debug_program(source, object, nil, compilation_type: 'object')
      executable = File.join(dir, 'program')
      _, stderr = capture_io { link_initializer_objects([object], executable, print_results: true) }
      assert_empty stderr
      stdout, stderr, status = Open3.capture3(executable)
      assert status.success?, stderr
      assert_equal "42\n", stdout
      assert_empty stderr
      stdout, stderr, status = Open3.capture3('dwarfdump', '--name=answer', "#{executable}.dSYM")
      assert status.success?, stderr
      assert_includes stdout, 'library.rb'
    end
  end

  def test_macos_uncached_executables_retain_debug_symbols
    skip 'Requires macOS debug maps' unless RUBY_PLATFORM.include?('darwin')

    %w[llvm cpp].each do |backend|
      Dir.mktmpdir('natalie-uncached-debug') do |dir|
        source = File.join(dir, 'program.rb')
        File.write(source, "def answer\n  42\nend\np answer\n")
        executable = File.join(dir, 'program')
        compile_debug_program(source, executable, nil, backend:)
        bundle = "#{executable}.dSYM"
        assert File.directory?(bundle), "Missing debugger bundle for #{backend}"
        name = backend == 'llvm' ? 'answer' : 'EVAL'
        stdout, stderr, status = Open3.capture3('dwarfdump', "--name=#{name}", bundle)
        assert status.success?, stderr
        assert_includes stdout, 'DW_TAG_subprogram'
        assert_includes stdout, 'DW_AT_decl_file'
        assert_includes stdout, 'program.rb' if backend == 'llvm'
      end
    end
  end

  def test_macos_debugger_exit_status_and_temporary_target_cleanup
    skip 'Requires macOS debug maps' unless RUBY_PLATFORM.include?('darwin')

    Dir.mktmpdir('natalie-debugger') do |dir|
      tools = File.join(dir, 'tools')
      Dir.mkdir(tools)
      debugger = File.join(tools, 'lldb')
      File.write(debugger, <<~'SH')
        #!/bin/sh
        printf '%s' "$1" > "$DEBUG_TARGET_PATH"
        test -d "$1.dSYM" || exit 94
        exit 17
      SH
      File.chmod(0o755, debugger)
      target_record = File.join(dir, 'target')
      env = { 'TMPDIR' => dir, 'PATH' => "#{tools}:#{ENV.fetch('PATH')}", 'DEBUG_TARGET_PATH' => target_record }
      stdout, stderr, status = Open3.capture3(env, *COMPILER_COMMAND, '-d', 'lldb', '-e', '42')
      assert_equal 17, status.exitstatus, stdout + stderr
      target = File.read(target_record)
      refute File.exist?(target)
      refute File.exist?("#{target}.dSYM")
    end
  end

  def test_macos_ordinary_runs_skip_debug_bundles_and_remove_the_executable
    skip 'Requires macOS debug maps' unless RUBY_PLATFORM.include?('darwin')

    Dir.mktmpdir('natalie-temporary-run') do |dir|
      tools = File.join(dir, 'tools')
      Dir.mkdir(tools)
      dsymutil = File.join(tools, 'dsymutil')
      File.write(dsymutil, "#!/bin/sh\nexit 93\n")
      File.chmod(0o755, dsymutil)
      env = { 'TMPDIR' => dir, 'PATH' => "#{tools}:#{ENV.fetch('PATH')}" }
      code = "$LOAD_PATH << #{dir.inspect}; puts 42"
      stdout, stderr, status = Open3.capture3(env, *COMPILER_COMMAND, '-e', code)
      assert status.success?, stderr
      assert_equal "42\n", stdout
      assert_empty stderr
      assert_empty Dir[File.join(dir, '*.dSYM')]
      assert_empty Dir[File.join(dir, 'natalie*')]
    end
  end

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

  def compile_debug_program(source, executable, cache, backend: 'llvm', compilation_type: 'executable')
    driver = <<~RUBY
      require #{File.join(ROOT, 'lib/natalie').inspect}
      parser = Natalie::Parser.new(File.read(#{source.inspect}), #{source.inspect})
      compiler = Natalie::Compiler.new(ast: parser.ast, path: #{source.inspect}, options: { backend: #{backend.inspect}, build: 'debug', compilation_type: #{compilation_type.inspect}, build_dir: #{cache.inspect}, build_quietly: true })
      compiler.out_path = #{executable.inspect}
      compiler.compile
    RUBY
    stdout, stderr, status = Open3.capture3(RbConfig.ruby, '-e', driver, chdir: ROOT)
    assert status.success?, stdout + stderr
    assert_empty stderr
  end
end
