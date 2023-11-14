require_relative './instruction_manager'
require_relative './instructions'
require_relative '../vm'

class BytecodeLoader
  INSTRUCTIONS = Natalie::Compiler::INSTRUCTIONS_META

  def initialize(io)
    @io = IO.new(io)
    @instructions = load_instructions
  end

  attr_reader :instructions

  class IO
    def initialize(io)
      @io = io
    end

    def getbyte
      @io.getbyte
    end

    def read(size)
      @io.read(size)
    end

    def read_ber_integer
      result = 0
      loop do
        byte = getbyte
        result = (result << 7) + (byte & 0x7f)
        if (byte & 0x80) == 0
          break
        end
      end
      result
    end
  end

  private

  def load_instructions
    instructions = []
    loop do
      num = @io.getbyte
      break if num.nil?

      name = instruction_num_to_name(num)
      instruction_class = Natalie::Compiler.const_get("#{name}Instruction")
      instructions << instruction_class.deserialize(@io)
    end
    instructions
  end

  def instruction_num_to_name(num)
    @instruction_names ||= INSTRUCTIONS.each_with_object({}) do |(name, meta), hash|
      hash[meta.fetch(:num)] = name.to_s.capitalize.gsub(/_([a-z])/) { |m| m[1].upcase }
    end
    @instruction_names.fetch(num)
  end
end

if __FILE__ == $0
  instructions = BytecodeLoader.new(File.open(ARGV.first, 'rb')).instructions
  im = Natalie::Compiler::InstructionManager.new(instructions)
  Natalie::VM.new(im, path: 'unknown').run
end
