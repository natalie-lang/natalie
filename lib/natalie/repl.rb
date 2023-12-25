require_relative '../natalie'
require 'fiddle'
require 'fileutils'
require 'tempfile'
require 'linenoise'
require 'natalie/inline'

state_dir = File.join(Dir.home, '.local/state')
if File.directory?(state_dir)
  FileUtils.mkdir_p(File.join(state_dir, 'natalie'))
  HISTORY_PATH = File.join(state_dir, 'natalie/history.txt')
else
  HISTORY_PATH = File.join(Dir.home, '.natalie_history.txt')
end

Linenoise.load_history(HISTORY_PATH)

vars = {}
repl_num = 0

@env = nil

loop do
  line = Linenoise.readline('nat> ')
  break if line.nil?

  next if line.strip.empty?

  Linenoise.add_history(line)

  source_path = '(repl)'
  parser = Natalie::Parser.new(line.strip, source_path, locals: vars.keys)
  ast = begin
    parser.ast
  rescue Natalie::Parser::ParseError => e
    puts e.message
    puts e.backtrace
    next
  end

  compiler = Natalie::Compiler.new(ast: ast, path: source_path, encoding: parser.encoding)
  compiler.repl = true
  compiler.repl_num = (repl_num += 1)
  compiler.vars = vars

  temp = Tempfile.create("natalie-repl.#{RbConfig::CONFIG['SOEXT']}")
  compiler.out_path = temp.path

  compiler.compile

  vars = compiler.context[:vars]

  library = Module.new do
    extend FFI::Library
    ffi_lib temp.path
    attach_function :EVAL, [:pointer], :pointer
  end

  ffi_ptr_class = FFI::Pointer
  result = nil
  error = false
  __inline__ <<~END
    auto env_ptr = self->ivar_get(env, "@env"_s);
    if (env_ptr->is_nil()) {
        auto e = new Env { env };
        env_ptr = ffi_ptr_class_var.send(env, "new"_s, { "pointer"_s, Value::integer((uintptr_t)e) });
        self->ivar_set(env, "@env"_s, env_ptr);
    }
    auto result = library_var.public_send(env, "EVAL"_s, { env_ptr });
    if (result.public_send(env, "null?"_s)->is_truthy()) {
        result_var = NilObject::the();
        error_var = TrueObject::the();
    } else {
        result_var = (Object *)result.send(env, "address"_s)->as_integer()->to_nat_int_t();
        error_var = FalseObject::the();
    }
  END

  p result unless error

  File.unlink(temp.path)
end

Linenoise.save_history(HISTORY_PATH)
