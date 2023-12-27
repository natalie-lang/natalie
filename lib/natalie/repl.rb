require 'ffi'
require 'fileutils'
require 'linenoise'
require 'natalie/inline'
require 'tempfile'

module LibNat
  extend FFI::Library
  ffi_lib "build/libnat.#{RbConfig::CONFIG['SOEXT']}"

  attach_function :init_libnat2, %i[pointer pointer], :pointer
  attach_function :new_parser, %i[pointer pointer pointer], :pointer
  attach_function :new_compiler, %i[pointer pointer pointer], :pointer

  def self.init
    env = FFI::Pointer.from_env
    self_ptr = to_ptr
    init_libnat2(env, self_ptr)
  end
end

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

GC.disable

LibNat.init

loop do
  line = Linenoise.readline('nat> ')
  break if line.nil?

  next if line.strip.empty?

  Linenoise.add_history(line)

  source_path = '(repl)'
  locals = vars.keys
  parser = LibNat.new_parser(line.strip.to_ptr, source_path.to_ptr, locals.to_ptr).to_obj
  ast = begin
    parser.ast
  rescue Natalie::Parser::ParseError => e
    puts e.message
    puts e.backtrace
    next
  end

  compiler = LibNat.new_compiler(ast.to_ptr, source_path.to_ptr, parser.encoding.to_ptr).to_obj
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

  env = FFI::Pointer.from_env
  result_ptr = library.EVAL(env)

  unless result_ptr.null?
    p result_ptr.to_obj
  end

  File.unlink(temp.path)
end

Linenoise.save_history(HISTORY_PATH)
