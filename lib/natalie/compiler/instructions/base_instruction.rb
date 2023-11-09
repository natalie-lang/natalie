module Natalie
  class Compiler
    class BaseInstruction
      # methods, ifs, loops, etc. have a body
      def has_body?
        false
      end

      def self.label
        @label ||=
          self.underscore(self.name.sub(/Instruction$/, '')).to_sym
      end

      def matching_label
        nil
      end

      attr_accessor :env

      class << self
        attr_accessor :instruction_number

        def inherited(subclass)
          @number ||= 0
          subclass.instruction_number = @number += 1
        end
      end

      def instruction_number
        self.class.instruction_number
      end

      private

      def self.underscore(name)
        name.split('::').last.gsub(/[A-Z]/) { |c| '_' + c.downcase }.sub(/^_/, '')
      end
    end
  end
end
