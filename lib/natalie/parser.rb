require 'strscan'

module Natalie
  class Parser
    def initialize(code_str)
      @code_str = code_str.strip + "\n"
      @scanner = StringScanner.new(@code_str)
      @lr_wrap = {}
    end

    COMMENT = /#[^\n]*/
    SEMI_END_OF_EXPR = /[ \t]*;+[ \t]*/
    NEWLINE_END_OF_EXPR = /[ \t]*(#{COMMENT})?\n+[ \t]*/
    END_OF_EXPRESSION = /#{SEMI_END_OF_EXPR}|#{NEWLINE_END_OF_EXPR}/

    def expect(expected, msg)
      return if expected
      raise "expected #{msg}; got: #{@scanner.inspect}"
    end

    def ast
      ast = []
      while !@scanner.eos? && (e = expr)
        ast << e
        expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
      end
      ast
    end

    def expr
      while @scanner.skip(/\s*#{COMMENT}\s*/); end
      klass || mod || method || if_expr || postfix_if || explicit_message || assignment || implicit_message || array || integer || string || symbol
    end

    def message_receiver_expr
      klass || mod || method || explicit_message || no_arg_message || array || integer || string || symbol
    end

    def assignment
      if @scanner.check(/#{IDENTIFIER}\s*=/)
        id = identifier
        @scanner.skip(/\s*=\s*/)
        [:assign, id, expr]
      end
    end

    def array
      if @scanner.scan(/\[/)
        @scanner.skip(/\s*/)
        ary = []
        unless @scanner.skip(/\]/)
          ary << expr
          until @scanner.scan(/\s*\]/)
            @scanner.skip(/\s*,\s*/)
            ary << expr
          end
        end
        [:array, ary]
      end
    end

    def integer
      if (n = @scanner.scan(/\-?\d+/))
        [:integer, n]
      end
    end

    def string
      if (s = @scanner.scan(/'[^']*'|"[^"]*"/))
        [:string, s[1...-1]]
      end
    end

    def symbol
      if (s = @scanner.scan(/:#{METHOD_NAME_OR_BARE_SYMBOL}/))
        [:symbol, s[1..-1]]
      elsif @scanner.check(/:["']/)
        @scanner.skip(/:/)
        [:symbol, string.last]
      end
    end

    def klass
      if (s = @scanner.scan(/class /))
        @scanner.skip(/\s*/)
        name = constant
        expect(name, 'class name after class keyword')
        superclass = constant if @scanner.skip(/\s*<\s*/)
        @scanner.skip(END_OF_EXPRESSION)
        body = []
        until @scanner.check(/\s*end[;\s]/)
          body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        @scanner.skip(/\s*end/)
        [:class, name, superclass, body]
      end
    end

    def mod
      if (s = @scanner.scan(/module /))
        @scanner.skip(/\s*/)
        name = constant
        expect(name, 'module name after module keyword')
        @scanner.skip(END_OF_EXPRESSION)
        body = []
        until @scanner.check(/\s*end[;\s]/)
          body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        @scanner.skip(/\s*end/)
        [:module, name, body]
      end
    end

    def if_expr
      if (s = @scanner.scan(/if /))
        @scanner.skip(/\s*/)
        condition = expr
        expect(condition, 'condition after if keyword')
        @scanner.skip(END_OF_EXPRESSION)
        true_body = []
        until @scanner.check(/\s*(elsif|else|end)[;\s]+/)
          true_body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        node = [:if, condition, true_body]
        while @scanner.skip(/\s*elsif /)
          @scanner.skip(/\s*/)
          condition = expr
          expect(condition, 'condition after elsif keyword')
          @scanner.skip(END_OF_EXPRESSION)
          elsif_body = []
          until @scanner.check(/\s*(elsif|else|end)[;\s]+/)
            elsif_body << expr
            expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
          end
          node << condition
          node << elsif_body
        end
        if @scanner.skip(/\s*else[;\s]+/)
          else_body = []
          until @scanner.check(/\s*end[;\s]+/)
            else_body << expr
            expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
          end
          node << :else
          node << else_body
        end
        @scanner.skip(/\s*end/)
        node
      end
    end

    def method
      if @scanner.scan(/def /)
        @scanner.skip(/\s*/)
        name = method_name
        expect(name, 'method name after def keyword')
        args = []
        unless @scanner.skip(END_OF_EXPRESSION)
          args = method_args_with_parens || method_args_without_parens
          expect(args, 'arguments after method name')
          @scanner.skip(/[;\s]+/)
        end
        body = []
        until @scanner.check(/\s*end[;\s]/)
          body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        @scanner.skip(/\s*end/)
        [:def, name, args, {}, body]
      end
    end

    IDENTIFIER = /[a-z][a-z0-9_]*/i

    def identifier
      @scanner.scan(IDENTIFIER)
    end

    METHOD_NAME_OR_BARE_SYMBOL = /#{IDENTIFIER}[\!\?=]?|==|\!=/

    def method_name
      @scanner.scan(METHOD_NAME_OR_BARE_SYMBOL)
    end

    def constant
      @scanner.scan(/[A-Z][a-z0-9_]*/)
    end

    OPERATOR = /<<?|>>?|<=>|<=|=>|===?|\!=|=~|\!~|\||\^|&|\+|\-|\*\*?|\/|%/

    def self.lr_wrap(name, terminal)
      define_method(name) do
        start = @scanner.pos
        if @lr_wrap[start]
          @lr_wrap[start] = false
          return
        end
        @lr_wrap[start] = true
        node = send(terminal)
        if node
          node2 = send("#{name}_inner", node)
          if node2
            node = node2
            loop do
              node2 = send("#{name}_inner", node)
              break if node2.nil?
              node = node2
            end
            node
          else
            @scanner.pos = start
            nil
          end
        else
          @scanner.pos = start
          nil
        end
      end
    end

    lr_wrap(:postfix_if, :expr)

    def postfix_if_inner(result_if_true)
      if @scanner.skip(/[ ]+if /)
        condition = expr
        expect(condition, 'condition after if')
        [:if, condition, [result_if_true], :else, [[:send, nil, 'nil', []]]]
      end
    end

    lr_wrap(:explicit_message, :message_receiver_expr)

    def explicit_message_inner(receiver)
      if @scanner.check(/\s*\.?\s*#{OPERATOR}\s*/)
        @scanner.skip(/\s*\.?\s*/)
        message = @scanner.scan(OPERATOR)
        args = args_with_parens || args_without_parens || numeric_arg
        expect(args, 'expression after operator')
        [:send, receiver, message, args]
      elsif @scanner.check(/\[/)
        subscript = array
        expect(subscript, 'subscript with terminating ]')
        [:send, receiver, '[]', subscript.last]
      elsif @scanner.skip(/\s*\.\s*/)
        message = method_name
        expect(message, 'method call after dot')
        args = args_with_parens || args_without_parens || []
        [:send, receiver, message, args.compact]
      end
    end

    def args_with_parens
      if @scanner.check(/[ \t]*\(\s*/)
        @scanner.skip(/[ \t]*\(\s*/)
        args = [expr]
        while @scanner.skip(/[ \t]*,\s*/)
          args << expr
        end
        expect(@scanner.skip(/\s*\)/), ')')
        args
      end
    end

    def args_without_parens
      if @scanner.check(/[ \t]+/)
        @scanner.skip(/[ \t]+/)
        args = [expr]
        while @scanner.skip(/[ \t]*,\s*/)
          args << expr
        end
        args
      end
    end

    def numeric_arg
      if (i = integer)
        [i]
      end
    end

    def method_args_with_parens
      if @scanner.check(/[ \t]*\(\s*/)
        @scanner.skip(/[ \t]*\(\s*/)
        args = [identifier]
        while @scanner.skip(/[ \t]*,\s*/)
          args << identifier
        end
        expect(@scanner.skip(/\s*\)/), ')')
        args
      end
    end

    def method_args_without_parens
      if @scanner.check(/[ \t]+/)
        @scanner.skip(/[ \t]+/)
        args = [identifier]
        while @scanner.skip(/[ \t]*,\s*/)
          args << identifier
        end
        args
      end
    end

    def implicit_message
      if (id = method_name)
        args = args_with_parens || args_without_parens || []
        [:send, nil, id, args.compact] # FIXME: compact here is a hack to do with operator precedence :-(
      end
    end

    def no_arg_message
      if (id = method_name)
        [:send, nil, id, []]
      end
    end
  end
end
