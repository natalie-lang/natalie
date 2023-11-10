require_relative '../lib/natalie/compiler/instructions/meta'

class VM
  INSTRUCTIONS = Natalie::Compiler::INSTRUCTIONS_META

  def initialize(io)
    @io = io
  end

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

  def execute
    instructions = load_instructions
    puts instructions
    # TODO: something
  end

  private

  def load_instructions
    instructions = []
    loop do
      num = @io.getbyte
      break if num.nil?

      name = instruction_num_to_name(num)
      case name
      when :pop
        instructions << Instruction.new(type: name)
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
  VM.new(File.open(ARGV.first, 'rb')).execute
end
