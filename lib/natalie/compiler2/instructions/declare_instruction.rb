require_relative './base_instruction'

module Natalie
  class Compiler2
    class DeclareVarInstruction < BaseInstruction
      def initialize(name)
        @name = name
      end

      attr_reader :name

      def to_s
        "declare #{@name}"
      end

      def generate(transform)
        name = transform.temp(@name)
        if (transform.vars[@name])
          throw "Redeclaration of #{@name}"
        else
          transform.vars[@name] = { name: name, captured: false}
        end

        transform.exec("Value #{name}")
      end

      def execute(vm)
        # do nothing
      end
    end
  end
end
