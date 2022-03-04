require_relative './base_instruction'

module Natalie
  class Compiler2
    class ConstFindInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "const_find #{@name}"
      end

      def generate(transform)
        namespace = transform.pop
        transform.push("#{namespace}->const_find(env, #{name.to_s.inspect}_s, Object::ConstLookupSearchMode::NotStrict)")
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
    end
  end
end
