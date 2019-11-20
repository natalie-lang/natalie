require 'strscan'

module Natalie
  class Parser
    def initialize(code_str)
      @code_str = code_str.strip + "\n"
      @scanner = StringScanner.new(@code_str)
    end

    END_OF_EXPRESSION = /[ \t]*(;+|\n)+[ \t]*/

    def expect(expected, message)
      return if expected
      raise "expected #{message}; got: #{@scanner.inspect}"
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
      method || assignment || message || number || string
    end

    def assignment
      if @scanner.check(/#{IDENTIFIER}\s*=/)
        id = identifier
        @scanner.skip(/\s*=\s*/)
        [:assign, id, expr]
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

    def method
      if @scanner.scan(/def /)
        @scanner.skip(/\s*/)
        name = method_name
        expect(name, 'method name after def')
        @scanner.skip(/[;\n]+\s*/)
        body = []
        until @scanner.check(/\s*end[;\s]/)
          body << expr
          expect(@scanner.skip(END_OF_EXPRESSION), '; or newline')
        end
        @scanner.skip(/\s*end/)
        [:def, name, [], {}, body]
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

    def bare_word_message
      if (id = method_name)
        [:send, 'self', id, []]
      end
    end

    OPERATOR = /<<?|>>?|<=>|<=|=>|===?|\!=|=~|\!~|\||\^|&|\+|\-|\*\*?|\/|%/

    def message
      explicit_message || implicit_message
    end

    def explicit_message
      start = @scanner.pos
      if @explicit_message_recurs
        receiver = implicit_message || string || number
      else
        @explicit_message_recurs = true
        receiver = expr
      end
      @explicit_message_recurs = false
      if receiver
        if @scanner.check(/\s*\.?\s*#{OPERATOR}\s*/)
          @scanner.skip(/\s*\.?\s*/)
          message = @scanner.scan(OPERATOR)
          args = args_with_parens || args_without_parens
          expect(args, 'expression after operator')
          [:send, receiver, message, args]
        elsif @scanner.check(/\s*\.\s*/)
          @scanner.skip(/\s*\.\s*/)
          message = method_name
          expect(message, 'method call after dot')
          args = args_with_parens || args_without_parens || []
          [:send, receiver, message, args]
        else
          receiver
        end
      else
        @scanner.pos = start
        nil
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

    def implicit_message
      if (id = method_name)
        args = args_with_parens || args_without_parens || []
        [:send, 'self', id, args]
      end
    end
  end
end
