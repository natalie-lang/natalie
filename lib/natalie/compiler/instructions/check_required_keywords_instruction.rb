require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckRequiredKeywordsInstruction < BaseInstruction
      def initialize(keywords)
        @keywords = keywords
      end

      def to_s
        "check_required_keywords #{@keywords.join ', '}"
      end

      def generate(transform)
        hash = transform.peek
        transform.exec("env->ensure_no_missing_keywords(#{hash}, { #{@keywords.map { |kw| kw.to_s.inspect }.join ', ' } })")
      end

      def execute(vm)
        hash = vm.peek
        missing = @keywords.reject { |kw| hash.key? kw }
        if missing.size == 1
          raise ArgumentError, "missing keyword: #{missing.first.inspect}"
        elsif missing.any?
          raise ArgumentError, "missing keywords: #{missing.map(&:inspect).join ', '}"
        end
      end
    end
  end
end
