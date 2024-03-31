require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableGetInstruction < BaseInstruction
      def initialize(name, default_to_nil: false)
        @name = name.to_sym
        @default_to_nil = default_to_nil
      end

      attr_reader :name, :default_to_nil

      attr_accessor :meta

      def to_s
        s = "variable_get #{@name}"
        s << ' (default_to_nil)' if @default_to_nil
        s
      end

      def generate(transform)
        (depth, var) = transform.find_var(@name)

        raise "unknown variable #{@name}" if var.nil?

        env = 'env'
        depth.times { env << '->outer()' }
        index = var[:index]

        if !var.fetch(:declared) && @default_to_nil
          @meta[:declared] = true
          if @meta.fetch(:captured)
             "#{env}->var_set(#{@name.to_s.inspect}, #{index}, NilObject::the())"
          else
            transform.exec("Value #{@meta.fetch(:name)} = NilObject::the()")
          end
        end

        code = if @meta.fetch(:captured)
                 "#{env}->var_get(#{@name.to_s.inspect}, #{index})"
               else
                 @meta.fetch(:name)
               end

        if var.fetch(:declared)
          transform.push(code)
        else
          raise "variable not defined #{@name}"
        end
      end

      def execute(vm)
        if (var = vm.find_var(@name))
          vm.push(var.fetch(:value))
        elsif @default_to_nil
          vm.push(nil)
        else
          raise "unknown variable #{@name}"
        end
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)
        [
          instruction_number,
          position,
          @default_to_nil ? 1 : 0,
        ].pack('CwC')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        default_to_nil = io.getbool
        new(name, default_to_nil: default_to_nil)
      end
    end
  end
end
