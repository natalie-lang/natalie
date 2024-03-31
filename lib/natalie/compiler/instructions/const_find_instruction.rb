require_relative './base_instruction'

module Natalie
  class Compiler
    class ConstFindInstruction < BaseInstruction
      def initialize(name, strict:, file: nil, line: nil)
        @name = name.to_sym
        @strict = strict

        # source location info
        @file = file
        @line = line
      end

      attr_reader :name, :strict, :file, :line

      def to_s
        s = "const_find #{@name}"
        s << ' (strict)' if @strict
        s
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        namespace = transform.pop
        search_mode = @strict ? 'Strict' : 'NotStrict'
        transform.exec_and_push(:const, "#{namespace}->const_find_with_autoload(env, self, #{transform.intern(name)}, Object::ConstLookupSearchMode::#{search_mode})")
      end

      def execute(vm)
        owner = vm.pop
        owner = owner.class unless owner.respond_to?(:const_get)
        owner = find_object_with_constant(owner) || owner
        vm.push(owner.const_get(@name))
      end

      def parent(mod)
        return if mod == Object
        parent = mod.name.split('::')[0...-1].join('::')
        if parent == ''
          Object
        else
          Object.const_get(parent)
        end
      end

      def find_object_with_constant(obj)
        begin
          return obj if obj.constants.include?(@name)
        end while (obj = parent(obj))
      end

      def serialize(rodata)
        position = rodata.add(@name.to_s)

        [
          instruction_number,
          position,
          @strict ? 1 : 0,
        ].pack("CwC")
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        strict = io.getbool
        new(
          name,
          strict:,
          file: '', # FIXME
          line: 0 # FIXME
        )
      end
    end
  end
end
