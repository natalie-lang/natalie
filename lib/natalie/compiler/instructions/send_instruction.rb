require_relative './base_instruction'

module Natalie
  class Compiler
    # use:
    # push(block) if block
    # push(receiver)
    # push(*args)
    # push(argc)
    # send(message)
    class SendInstruction < BaseInstruction
      def initialize(message, receiver_is_self:, with_block:, file:, line:, args_array_on_stack: false, has_keyword_hash: false, forward_args: false)
        # the message to send
        @message = message.to_sym

        # implied self, no explicit receiver, e.g. foo vs something.foo
        @receiver_is_self = receiver_is_self

        # the args array is already on the stack and ready;
        # no need to collect individual args
        @args_array_on_stack = args_array_on_stack

        # a block is on the stack
        @with_block = with_block

        # a keyword hash is last in the args
        @has_keyword_hash = has_keyword_hash

        # true when special ... syntax was used
        @forward_args = forward_args

        # source location info
        @file = file
        @line = line
      end

      attr_reader :message,
                  :receiver_is_self,
                  :with_block,
                  :args_array_on_stack,
                  :has_keyword_hash,
                  :file,
                  :line

      def with_block?
        !!@with_block
      end

      def to_s
        s = "send #{@message.inspect}"
        s << ' to self' if @receiver_is_self
        s << ' with block' if @with_block
        s << ' (args array on stack)' if @args_array_on_stack
        s << ' (has keyword hash)' if @has_keyword_hash
        s
      end

      def generate(transform)
        if @args_array_on_stack
          if @forward_args
            args_list = transform.pop
          else
            args = "#{transform.pop}->as_array()"
            args_list = "Args(#{args}, #{@has_keyword_hash ? 'true' : 'false'})"
          end
        else
          arg_count = transform.pop
          raise "bad argc #{arg_count.inspect} for SendInstruction #{@message.inspect}" unless arg_count.is_a?(Integer)
          args = []
          arg_count.times { args.unshift transform.pop }
          args_list = "Args({ #{args.join(', ')} }, #{@has_keyword_hash ? 'true' : 'false'})"
        end

        transform.set_file(@file)
        transform.set_line(@line)

        receiver = transform.pop
        unless receiver.is_a?(String)
          puts "Something went wrong: bad receiver #{receiver.inspect} for SendInstruction #{@message.inspect}"
          puts "Got arg_count = #{arg_count.inspect}"
          puts "Got args = #{args.inspect}"
          exit 1
        end

        block = @with_block ? "to_block(env, #{transform.pop})" : 'nullptr'

        call = "#{receiver}.#{method}(env, #{transform.intern(@message)}, #{args_list}, #{block}, self)"

        if message =~ /\w=$|\[\]=$/
          transform.exec(call)
          # Setters always return the value that was set.
          # In the case of foo=, there is only one argument.
          # In the case of []=, there can be multiple arguments.
          # The last argument is the value we need to return.
          if @args_array_on_stack
            transform.push("#{args}->last()")
          else
            transform.push(args.last)
          end
        else
          transform.exec_and_push("send_#{@message}", call)
        end
      end

      def execute(vm)
        if @args_array_on_stack
          args = vm.pop
        else
          arg_count = vm.pop
          args = []
          arg_count.times { args.unshift vm.pop }
        end

        receiver = vm.pop

        if args.empty? && %i[public private protected].include?(@message)
          vm.method_visibility = @message
          vm.push(receiver)
          return
        end

        # TODO: put these special intercepts somewhere else?
        if %i[__dir__].include?(@message)
          vm.push(vm.main.__dir__)
          return
        end

        old_last_match = vm.global_variables[:$~]
        vm.with_self(receiver) do
          block = vm.pop if @with_block
          result = receiver.send(method, @message, *args, &block)
          vm.push result
        end
        vm.global_variables[:$~] = $~ if $~ != old_last_match
      end

      def to_method_defined_instruction
        MethodDefinedInstruction.new(
          message,
          receiver_is_self: @receiver_is_self,
          with_block: @with_block,
          args_array_on_stack: @args_array_on_stack,
          has_keyword_hash: @has_keyword_hash,
        )
      end

      def serialize(rodata)
        position = rodata.add(@message.to_s)
        flags = 0
        [receiver_is_self, with_block, args_array_on_stack, has_keyword_hash].each_with_index do |flag, index|
          flags |= (1 << index) if flag
        end
        [
          instruction_number,
          position,
          flags,
        ].pack("CwC")
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        message = rodata.get(position, convert: :to_sym)
        flags = io.getbyte
        receiver_is_self = flags[0] == 1
        with_block = flags[1] == 1
        args_array_on_stack = flags[2] == 1
        has_keyword_hash = flags[3] == 1
        new(
          message,
          receiver_is_self: receiver_is_self,
          with_block: with_block,
          args_array_on_stack: args_array_on_stack,
          has_keyword_hash: has_keyword_hash,
          file: '', # FIXME
          line: 0 # FIXME
        )
      end

      private

      def method
        if @receiver_is_self
          :send
        else
          :public_send
        end
      end
    end
  end
end
