require_relative '../../../lib/natalie'
require_relative '../../../lib/natalie/compiler/native_build/source_file'

module LlvmObjectLinker
  def link_initializer_objects(objects, executable, print_results: false)
    compiler = Natalie::Compiler.new(ast: nil, path: '(object host)')
    compiler.out_path = executable
    context = { required_cpp_files: {}, source_path: '(object host)', compile_cxx_flags: [], compile_ld_flags: [] }
    backend = Struct.new(:compiled_files, :compiler_context).new({}, context)
    data = Natalie::Compiler::NativeBuild::TransformData.new
    names = objects.map { |path| "init_#{Natalie::Compiler::NativeBuild.object_name(path)}" }
    data.top[:initializers] = names.map { |name| "Value #{name}(Env *, Value);" }.join("\n")
    calls =
      names.map do |name|
        call = "#{name}(env, self)"
        print_results ? "self.send(env, \"p\"_s, { #{call} });" : "#{call};"
      end
    source =
      Natalie::Compiler::NativeBuild::SourceFile.new(
        type: :main,
        body: calls.join("\n") + "\nreturn Value::nil();",
        transform_data: data,
        ruby_path: '(object host)',
        compiler: compiler,
        backend: backend,
      )
    source.out_path = File.join(File.dirname(executable), 'object-host.o')
    host = source.compile_object_file
    Natalie::Compiler::NativeBuild::Linker.new(
      in_paths: [host, *objects, Natalie::Compiler::LlvmBackend::RUNTIME_BRIDGE],
      out_path: executable,
      compiler: compiler,
      compiler_context: context,
      keep_inputs: true,
    ).link
  end
end
