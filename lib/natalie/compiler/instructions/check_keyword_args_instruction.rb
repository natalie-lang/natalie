require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckKeywordArgsInstruction < BaseInstruction
      def initialize(required_keywords:, optional_keywords:, keyword_rest:)
        @required_keywords = required_keywords
        @optional_keywords = optional_keywords
        @keyword_rest = keyword_rest
        unless %i[none present forbidden].include?(@keyword_rest)
          raise 'keyword_rest must be :none, :present, or :forbidden'
        end
      end

      def to_s
        'check_keyword_args ' \
          "required: #{@required_keywords.join(', ')}; " \
          "optional: #{@optional_keywords.join(', ')}; " \
          "keyword_rest: #{@keyword_rest}"
      end

      def generate(transform)
        transform.exec(
          'args.check_keyword_args(env, ' \
            "{ #{@required_keywords.map { |kw| transform.intern(kw) }.join(', ')} }, " \
            "{ #{@optional_keywords.map { |kw| transform.intern(kw) }.join(', ')} }, " \
            "Args::KeywordRestType::#{@keyword_rest.capitalize})",
        )
      end

      def execute(vm)
        raise 'todo'
        # missing = @keywords.reject { |kw| hash.key? kw }
        # if missing.size == 1
        #   raise ArgumentError, "missing keyword: #{missing.first.inspect}"
        # elsif missing.any?
        #   raise ArgumentError, "missing keywords: #{missing.map(&:inspect).join ', '}"
        # end
      end

      def serialize(rodata)
        rest_type = { none: 0, present: 1, forbidden: 2 }.fetch(@keyword_rest)
        bytecode = [instruction_number, @required_keywords.size, @optional_keywords.size, rest_type].pack('CwwC')
        [@required_keywords, @optional_keywords].each do |keywords|
          keywords.each do |keyword|
            position = rodata.add(keyword.to_s)
            bytecode << [position].pack('w')
          end
        end
        bytecode
      end

      def self.deserialize(io, rodata)
        required_keywords = []
        optional_keywords = []
        [required_keywords, optional_keywords].each do |keywords|
          io.read_ber_integer.times do
            position = io.read_ber_integer
            keywords << rodata.get(position, convert: :to_sym)
          end
        end
        rest_type = io.read(1).unpack1('C')
        keyword_rest = { 0 => :none, 1 => :present, 2 => :forbidden }.fetch(rest_type)
        new(required_keywords:, optional_keywords:, keyword_rest:)
      end
    end
  end
end
