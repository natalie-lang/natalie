require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineMethodInstruction < BaseInstruction
      def initialize(name:, arity:, file:, line:, break_point: nil)
        @name = name
        @arity = arity
        @break_point = break_point

        # source location info
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      attr_reader :name, :arity, :file, :line
      attr_accessor :break_point

      def to_s
        s = "define_method #{@name}"
        s << " (break point: #{break_point})" if break_point
        s
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        klass = transform.pop
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        fn = transform.temp("defn_#{@name}")
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args &&args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(fn, body)
        end
        transform.exec(
          "Object::define_method(env, #{klass}, #{transform.intern(@name)}, #{fn}, #{@arity}, #{@break_point.to_i})",
        )
      end

      def execute(vm)
        klass = vm.pop
        klass = klass.class unless klass.respond_to?(:define_method)
        start_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :define_method)
        name = @name
        klass.define_method(name) do |*args, **kwargs, &block|
          scope = { vars: {} }
          args << kwargs if kwargs.any?
          vm.push_call(name:, return_ip: vm.ip, args:, kwargs:, scope:, block: block)
          vm.ip = start_ip
          vm.with_self(self) do
            begin
              vm.run
            ensure
              vm.ip = vm.pop_call[:return_ip]
            end
            vm.pop # result must be returned to SendInstruction
          end
        end
        case vm.method_visibility
        when :private
          klass.send(:private, @name)
        when :protected
          klass.send(:protected, @name)
        end
      end

      def serialize(rodata)
        raise NotImplementedError, 'Methods with more than 127 arguments are not supported' if @arity > 127

        position = rodata.add(@name.to_s)
        [instruction_number, position, @arity, @break_point.to_i].pack('CwcC')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        arity = io.read(1).unpack1('c')
        break_point = io.read_ber_integer
        break_point = nil if break_point.zero?
        new(
          name:,
          arity:,
          file: '', # FIXME
          line: 0, # FIXME
          break_point:,
        )
      end
    end
  end
end
