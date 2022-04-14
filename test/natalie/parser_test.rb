require_relative '../spec_helper'
require_relative '../../ext/natalie_parser/lib/natalie_parser/sexp'

if RUBY_ENGINE != 'natalie'
  $LOAD_PATH << File.expand_path('../../ext/natalie_parser/lib', __dir__)
  $LOAD_PATH << File.expand_path('../../ext/natalie_parser/ext', __dir__)
  require 'natalie_parser'
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
