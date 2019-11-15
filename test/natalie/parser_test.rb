require 'minitest/spec'
require 'minitest/autorun'
require_relative '../../lib/natalie'

describe Natalie::Parser do
  describe '#ast' do
    it 'builds an ast' do
      @parser = Natalie::Parser.new(
        "num = 1\n" \
        "str = 'a'\n" \
        "def foo\n" \
          "2\n" \
        "end"
      )
      @parser.ast.must_equal [
        [:assign, 'num', [:number, '1']],
        [:assign, 'str', [:string, 'a']],
        [:def, 'foo', [], [[:number, '2']]],
      ]
    end
  end
end
