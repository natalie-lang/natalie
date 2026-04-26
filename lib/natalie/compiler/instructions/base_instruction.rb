require_relative '../parameter_list'

module Natalie
  class Compiler
    class BaseInstruction
      # methods, ifs, loops, etc. have a body
      def has_body?
        false
      end

      def serialize_parameters(rodata, parameters)
        buf = [parameters.size].pack('w')
        parameters.each do |kind, name|
          code = ParameterList::KIND_CODES.fetch(kind)
          pos = name ? rodata.add(name.to_s) : 0
          buf << [code, pos].pack('Cw')
        end
        buf
      end

      def self.deserialize_parameters(io, rodata)
        count = io.read_ber_integer
        Array.new(count) do
          code = io.read(1).unpack1('C')
          pos = io.read_ber_integer
          name = pos.zero? ? nil : rodata.get(pos, convert: :to_sym)
          [ParameterList::CODE_KINDS.fetch(code), name]
        end
      end

      def self.label
        @label ||= self.underscore(self.name.sub(/Instruction$/, '')).to_sym
      end

      def label
        self.class.label
      end

      def matching_label
        nil
      end

      attr_accessor :env

      class << self
        def instruction_number
          @instruction_number ||= INSTRUCTIONS.index(self) or raise('Instruction not found!')
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
