require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineMethodInstruction < BaseInstruction
      def initialize(name:, arity:, file:, line:)
        @name = name
        @arity = arity

        # source location info
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      attr_reader :name, :arity, :file, :line

      def to_s
        "define_method #{@name}"
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

        klass = transform.pop
        body = transform.fetch_block_of_instructions(expected_label: :define_method)
        fn = transform.temp("defn_#{@name}")
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self, Args args, Block *block) {"
          body << t.transform('return')
          body << '}'
          transform.top(body)
        end
        transform.exec("#{klass}->define_method(env, #{transform.intern(@name)}, #{fn}, #{@arity})")
      end

      def execute(vm)
        klass = vm.pop
        klass = klass.class unless klass.respond_to?(:define_method)
        start_ip = vm.ip
        vm.skip_block_of_instructions(expected_label: :define_method)
        name = @name
        klass.define_method(name) do |*args, &block|
          scope = { vars: {} }
          vm.push_call(name: name, return_ip: vm.ip, args: args, scope: scope, block: block)
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

      def serialize
        raise NotImplementedError, 'Methods with more than 127 arguments are not supported' if @arity > 127

        name_string = @name.to_s
        [
          instruction_number,
          name_string.bytesize,
          name_string,
          @arity
        ].pack("Cwa#{name_string.bytesize}c")
      end

      def self.deserialize(io)
        size = io.read_ber_integer
        name = io.read(size)
        arity = io.read(1).unpack1('c')
        new(
          name:,
          arity:,
          file: '', # FIXME
          line: 0 # FIXME
        )
      end
    end
  end
end
