require_relative '../spec_helper'
require 'sexp_processor'

if RUBY_ENGINE != 'natalie'
  require 'ruby_parser'
  class Parser
    def self.parse(code, path = '(string)')
      node = RubyParser.new.parse(code, path)
      if node.nil?
        s(:block)
      elsif node.first == :block
        node
      else
        s(:block, node)
      end
    rescue Racc::ParseError, RubyParser::SyntaxError => e
      raise SyntaxError, e.message
    end
  end
end

describe 'Parser' do
  describe '#parse' do
    it 'parses examples/fib.rb' do
      fib = File.read(File.expand_path('../../examples/fib.rb', __dir__))
      Parser.parse(fib).should ==
        s(
          :block,
          s(
            :defn,
            :fib,
            s(:args, :n),
            s(
              :if,
              s(:call, s(:lvar, :n), :==, s(:lit, 0)),
              s(:lit, 0),
              s(
                :if,
                s(:call, s(:lvar, :n), :==, s(:lit, 1)),
                s(:lit, 1),
                s(
                  :call,
                  s(:call, nil, :fib, s(:call, s(:lvar, :n), :-, s(:lit, 1))),
                  :+,
                  s(:call, nil, :fib, s(:call, s(:lvar, :n), :-, s(:lit, 2))),
                ),
              ),
            ),
          ),
          s(:lasgn, :num, s(:call, s(:const, :ARGV), :first)),
          s(
            :call,
            nil,
            :puts,
            s(:call, nil, :fib, s(:if, s(:lvar, :num), s(:call, s(:lvar, :num), :to_i), s(:lit, 25))),
          ),
        )
    end
  end
end
