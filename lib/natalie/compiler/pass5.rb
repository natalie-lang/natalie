module Natalie
  class Compiler
    # Remove unused variables from generated C code
    class Pass5
      def initialize(compiler_context, code)
        @compiler_context = compiler_context
        @code = code
      end

      def go
        return @code if ENV['DISABLE_VARIABLE_ELIMINATION']
        loop do
          uiv = unused_if_vars
          uv = unused_vars
          break if uiv.empty? && uv.empty?
          uiv.each do |var|
            @code.gsub!(/NatObject \*#{var};\n/, '')
            @code.gsub!(/#{var} = .+\n/, '')
          end
          uv.each do |var|
            @code.gsub!(/NatObject \*#{var} = /, '')
          end
        end
        @code
      end

      private

      def unused_vars
        all_vars - used_vars
      end

      def unused_if_vars
        declarations = @code.scan(/NatObject \*((nat_[a-z]+_)?if\d+);/).map(&:first)
        sets = @code.scan(/((nat_[a-z]+_)?if\d+) = /).map(&:first)
        all = @code.scan(/((nat_[a-z]+_)?if\d+)/).map(&:first)
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
