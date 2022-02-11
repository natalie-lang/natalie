module Natalie
  class Compiler2
    class BaseInstruction
      # methods, ifs, loops, etc. have a body
      def has_body?
        false
      end

      def label
        underscore(self.class.name.sub(/Instruction$/, '')).to_sym
      end

      def matching_label
        nil
      end

      attr_accessor :env

      private

      def underscore(name)
        name.split('::').last.gsub(/[A-Z]/) { |c| '_' + c.downcase }.sub(/^_/, '')
      end
    end
  end
end
