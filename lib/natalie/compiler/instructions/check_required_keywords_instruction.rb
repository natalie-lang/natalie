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

      def serialize
        bytecode = [instruction_number, @keywords.size].pack('Cw')
        @keywords.each do |keyword|
          keyword_string = keyword.to_s
          bytecode << [keyword_string.bytesize, keyword_string].pack("wa#{keyword_string.bytesize}")
        end
        bytecode
      end

      def self.deserialize(io)
        keywords = []
        io.read_ber_integer.times do
          size = io.read_ber_integer
          keywords << io.read(size).to_sym
        end
        new(keywords)
      end
    end
  end
end
