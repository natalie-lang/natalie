require_relative '../spec_helper'
require 'sexp_processor'

if RUBY_ENGINE != 'natalie'
  require 'natalie_parser'
  class NatalieParser
    def self.parse(code, path = '(string)')
      NatalieParser.parse(code, path)
    end
  end
end

describe 'NatalieParser' do
  describe '#parse' do
    it 'parses examples/fib.rb' do
      fib = File.read(File.expand_path('../../examples/fib.rb', __dir__))
      NatalieParser.parse(fib).should ==
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
