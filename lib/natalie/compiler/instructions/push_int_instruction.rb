require_relative './base_instruction'

module Natalie
  class Compiler
    class PushIntInstruction < BaseInstruction
      def initialize(int)
        @int = int
      end

      attr_reader :int

      def to_s
        "push_int #{@int}"
      end

      def generate(transform)
        if @int > NAT_MAX_FIXNUM || @int < NAT_MIN_FIXNUM
          transform.push("IntegerObject::create(#{@int.to_s.inspect})")
        else
          transform.push("Value::integer(#{@int})")
        end
      end

      def execute(vm)
        vm.push(@int)
      end

      def serialize(_)
        # Use the algorithm of Marshal.dump with a twist: Marshal switches from type `i` to `l` for values >= (1 <<30)
        # or < -(1 << 30). If this is the case, add a first byte `5` for positive or `-5` for negative numbers, and
        # encode the absolute value using BER encoding. The bytes `5` and `-5` are impossible in regular Marshal
        # encoding.
        bytes = []
        if @int.zero?
          bytes << 0
        elsif @int > 0 && @int < 123
          bytes << (@int + 5)
        elsif @int > -124 && @int < 0
          bytes << ((@int - 5) & 255)
        elsif @int >= (1 << 30)
          bytes << 5
          bytes.concat([@int].pack('w').bytes)
        elsif @int < -(1 << 30)
          bytes << -5
          bytes.concat([@int.abs].pack('w').bytes)
        else
          int = @int
          until int.zero? || int == -1
            bytes << (int & 255)
            int >>= 8
          end
          if int.zero?
            bytes.unshift(bytes.size)
          else
            bytes.unshift(256 - bytes.size)
          end
        end
        [
          instruction_number,
          *bytes
        ].pack('C*')
      end

      def self.deserialize(io, _)
        # See {serialize} for an explanation of the algorithm.
        byte = io.read(1).unpack1('c')
        if byte.zero?
          return new(0)
        elsif byte.positive?
          if byte == 5
            int = io.read_ber_integer
            return new(int)
          elsif byte > 5 && byte < 128
            return new(byte - 5)
          end
          integer = 0
          byte.times do |i|
            integer |= io.getbyte << (8 * i)
          end
          new(integer)
        else
          if byte == -5
            int = io.read_ber_integer
            return new(-int)
          elsif byte > -129 && byte < -4
            return new(byte + 5)
          end
          byte = -byte
          integer = -1
          byte.times do |i|
            integer &= ~(255 << (8 * i))
            integer |= io.getbyte << (8 * i)
          end
          new(integer)
        end
      end
    end
  end
end
