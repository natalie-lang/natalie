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

Linenoise.highlight_callback = lambda do |input|
  tokens = Natalie::Parser.new(input, '(repl)').tokenize.value.map(&:first)
  highlighted = ''
  offset = 0

  tokens.each do |token|
    until offset >= token.location.start_offset
      highlighted << ' '
      offset += 1
    end

    case token.type
    when :INTEGER
      highlighted << "\e[31m#{token.value}\e[0m"
    when /^KEYWORD_/
      highlighted << "\e[32m#{token.value}\e[0m"
    when :FLOAT
      highlighted << "\e[33m#{token.value}\e[0m"
    when :STRING_BEGIN, :STRING_CONTENT, :STRING_END
      highlighted << "\e[34m#{token.value}\e[0m"
    when :PARENTHESIS_LEFT, :PARENTHESIS_RIGHT,
         :BRACE_LEFT, :BRACE_RIGHT,
         :BRACKET_LEFT_ARRAY, :BRACKET_RIGHT_ARRAY,
         :PLUS, :MINUS, :STAR, :SLASH, :PERCENT, :CARET, :TILDE, :BANG,
         :COMMA, :SEMICOLON, :DOT, :COLON, :DOUBLE_COLON, :QUESTION_MARK,
         :AMPERSAND, :AMPERSAND_AMPERSAND, :PIPE, :PIPE_PIPE
      highlighted << "\e[35m#{token.value}\e[0m"
    when :IDENTIFIER
      highlighted << "\e[36m#{token.value}\e[0m"
    else
      highlighted << token.value
    end

    offset = token.location.end_offset
  end

  highlighted
end

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
