module Natalie
  class Compiler
    class ConstPrepper
      def initialize(name, pass:)
        if name.is_a?(Symbol)
          @name = name
          @namespace = PushSelfInstruction.new
        elsif name.sexp_type == :colon2
          _, namespace, @name = name
          @namespace = pass.transform_expression(namespace, used: true)
        elsif name.sexp_type == :colon3
          _, @name = name
          @namespace = PushObjectClassInstruction.new
        else
          raise "Unknown constant name type #{name.inspect} #{name.file}##{name.line}"
        end
      end

      attr_reader :name, :namespace
    end
  end
end
