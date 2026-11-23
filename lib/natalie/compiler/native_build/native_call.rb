require_relative './code_writer'
require_relative '../instructions/inline_cpp_instruction'

module Natalie
  class Compiler
    module NativeBuild
      # This renderer accepts explicit native macros, never Ruby instructions.
      class NativeCall < CodeWriter
        def initialize(instruction, compiler_context:, transform_data:, stack: [])
          raise ArgumentError, 'Expected a native macro instruction' unless instruction.is_a?(InlineCppInstruction)

          super(compiler_context:, transform_data:, stack:)
          @instruction = instruction
          @env = instruction.env
        end

        def generate
          @instruction.generate(self)
          @code << pop
          stringify_code(@code, 'return')
        end
      end
    end
  end
end
