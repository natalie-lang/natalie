module Natalie
  class Compiler
    # Remove unused variables from generated C code
    class Pass4
      def initialize(compiler_context, code)
        @compiler_context = compiler_context
        @code = code
      end

      def go
        return @code if ENV['DISABLE_PASS3']
        unused_if_vars.each do |var|
          @code.gsub!(/NatObject \*#{var};\n/, '')
          @code.gsub!(/#{var} = .+\n/, '')
        end
        unused_vars.each do |var|
          @code.gsub!(/NatObject \*#{var} = /, '')
        end
        @code
      end

      private

      def unused_vars
        all_vars - used_vars
      end

      def unused_if_vars
        declarations = @code.scan(/NatObject \*(if\d+);/).map(&:first)
        sets = @code.scan(/(if\d+) = /).map(&:first)
        all = @code.scan(/if\d+/)
        all.uniq.select do |var|
          all.count(var) - declarations.count(var) - sets.count(var) == 0
        end
      end

      def used_vars
        counted = @code.scan(/[\w_]+/).each_with_object({}) { |v, h| h[v] ||= 0; h[v] += 1 }
        counted.select { |v, c| c > 1 }.keys
      end

      def all_vars
        @code.scan(/NatObject \*([\w_]+) =/).map(&:first)
      end
    end
  end
end
