require_relative '../lib/natalie/compiler/instructions/meta'

class VM
  INSTRUCTIONS = Natalie::Compiler::INSTRUCTIONS_META

  def initialize(io:, main:)
    @io = io
    @stack = []
    @self = main
    @ip = 0
    @instructions = load_instructions
    @frames = [{ vars: {} }]
  end

  attr_accessor :ip, :stack, :frames

  class Instruction
    def initialize(type:, args: [])
      @type = type
      @args = args
    end

    attr_reader :type, :args

    def inspect
      "#<Instruction type=#{type} args=#{args.inspect}>"
    end

    alias to_s inspect
  end

  class Block
    def initialize(ip:, vm:)
      @ip = ip
      @vm = vm
    end

    attr_reader :instructions, :vm

    def to_proc
      lambda do |*args|
        vm.frames << { args: args, vars: {} }
        vm.ip = @ip
        vm.execute
        vm.stack.pop
      ensure
        vm.frames.pop
      end
    end
  end

  def execute
    while @ip < @instructions.size
      instruction = @instructions[@ip]
      break if instruction.type == :end

      @ip += 1
      send "execute_#{instruction.type}", instruction
    end
  end

  private

  def args
    @frames.last.fetch(:args)
  end

  def vars
    @frames.last.fetch(:vars)
  end

  def execute_define_block(instruction)
    start_ip = @ip
    # TODO: recursive blocks
    until (@instructions[@ip].type == :end && @instructions[@ip].args[0] == :define_block)
      @ip += 1
    end
    @stack << Block.new(ip: start_ip, vm: self)
    @ip += 1
  end

  def execute_pop(_)
    @stack.pop
  end

  def execute_push_arg(instruction)
    index, = instruction.args[0]
    nil_default = instruction.args[1]
    # TODO: handle nil_default
    @stack << args[index]
  end

  def execute_push_argc(instruction)
    @stack << instruction.args.first
  end

  def execute_push_self(_)
    @stack << @self
  end

  def execute_push_string(instruction)
    @stack << instruction.args.first
  end

  def execute_send(instruction)
    message = instruction.args[0].to_sym
    receiver_is_self, with_block, args_array_on_stack, has_keyword_hash = instruction.args[1]
    argc = @stack.pop
    args = 1.upto(argc).map { @stack.pop }
    receiver = @stack.pop
    if with_block
      block = @stack.pop
    end
    ip_was = @ip
    result = if receiver_is_self
      receiver.send(message, *args, &block)
    else
      receiver.public_send(message, *args, &block)
    end
    @ip = ip_was
    @stack << result
  end

  def execute_variable_get(instruction)
    value = @stack.pop
    name = instruction.args[0]
    default_to_nil = instruction.args[1]
    # TODO: handle default_to_nil
    @stack << vars[name]
  end

  def execute_variable_set(instruction)
    value = @stack.pop
    name = instruction.args[0]
    local_only = instruction.args[1]
    vars[name] = value
  end

  def load_instructions
    instructions = []
    loop do
      num = @io.getbyte
      break if num.nil?

      name = instruction_num_to_name(num)
      case name
      when :define_block
        arity = read_ber_integer
        instructions << Instruction.new(type: name, args: [arity])
      when :end
        size = read_ber_integer
        matching_label = @io.read(size).to_sym
        instructions << Instruction.new(type: name, args: [matching_label])
      when :pop
        instructions << Instruction.new(type: name)
      when :push_arg
        index = read_ber_integer
        nil_default = @io.getbyte == 1
        instructions << Instruction.new(type: name, args: [index, nil_default])
      when :push_argc
        count = read_ber_integer
        instructions << Instruction.new(type: name, args: [count])
      when :push_self
        instructions << Instruction.new(type: name)
      when :push_string
        size = read_ber_integer
        string = @io.read(size)
        instructions << Instruction.new(type: name, args: [string])
      when :send
        size = read_ber_integer
        message = @io.read(size)
        flags = @io.read(4).unpack('CCCC').map { |f| f == 1 }
        instructions << Instruction.new(type: name, args: [message, flags])
      when :variable_get
        size = read_ber_integer
        var_name = @io.read(size)
        default_to_nil = @io.getbyte == 1
        instructions << Instruction.new(type: name, args: [var_name, default_to_nil])
      when :variable_set
        size = read_ber_integer
        var_name = @io.read(size)
        local_only = @io.getbyte == 1
        instructions << Instruction.new(type: name, args: [var_name, local_only])
      else
        raise "unhandled instruction: #{name}"
      end
    end
    instructions
  end

  def instruction_num_to_name(num)
    @instruction_names ||= INSTRUCTIONS.each_with_object({}) do |(name, meta), hash|
      hash[meta.fetch(:num)] = name
    end
    @instruction_names.fetch(num)
  end

  def read_ber_integer
    result = 0
    loop do
      byte = @io.getbyte
      result = (result << 7) + (byte & 0x7f)
      if (byte & 0x80) == 0
        break
      end
    end
    result
  end
end

if __FILE__ == $0
  VM.new(
    io: File.open(ARGV.first, 'rb'),
    main: self,
  ).execute
end
