require 'strscan'

module Natalie
  class Parser
    def initialize(code_str)
      @code_str = code_str.strip + "\n"
      @scanner = StringScanner.new(@code_str)
      @lr_wrap = {}
    end

    END_OF_EXPRESSION = /[ \t]*(;+|\n)+[ \t]*/

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
      klass || method || assignment || explicit_message || implicit_message || array || number || string
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

    def number
      if (n = @scanner.scan(/\d+/))
        [:number, n]
      end
    end

    def string
      if (s = @scanner.scan(/'[^']*'|"[^"]*"/))
        [:string, s[1...-1]]
      end
    end

    def klass
      if (s = @scanner.scan(/class /))
        @scanner.skip(/\s*/)
        name = class_name
        expect(name, 'method name after class keyword')
        superclass = class_name if @scanner.skip(/\s*<\s*/)
        @scanner.skip(/\s*[;\n]+\s*/)
        body = []
        until @scanner.check(/\s*end[;\s]/)
          body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        @scanner.skip(/\s*end/)
        [:class, name, superclass, body]
      end
    end

    def method
      if @scanner.scan(/def /)
        @scanner.skip(/\s*/)
        name = method_name
        expect(name, 'method name after def keyword')
        args = []
        unless @scanner.skip(/\s*[;\n]+\s*/)
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

    METHOD_NAME = /#{IDENTIFIER}[\!\?=]?/

    def method_name
      @scanner.scan(METHOD_NAME)
    end

    def class_name
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
      end
    end

    lr_wrap(:explicit_message, :expr)

    def explicit_message_inner(receiver)
      if @scanner.check(/\s*\.?\s*#{OPERATOR}\s*/)
        @scanner.skip(/\s*\.?\s*/)
        message = @scanner.scan(OPERATOR)
        args = args_with_parens || args_without_parens
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
        [:send, 'self', id, args.compact] # FIXME: compact here is a hack to do with operator precedence :-(
      end
    end
  end
end
