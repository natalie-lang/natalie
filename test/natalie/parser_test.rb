require 'minitest/spec'
require 'minitest/autorun'

describe 'Natalie::Parser' do
  def build_ast(code)
    `#{bin} --ast -e #{code.inspect} 2>&1`
  end

  def bin
    File.expand_path('../../bin/natalie', __dir__)
  end

  describe '#ast' do
    it 'builds an ast' do
      out = build_ast(
        "num = 1\n" \
        "str = 'a'\n" \
        "def foo\n" \
          "2\n" \
        "end"
      )
      ast = eval(out)
      ast.must_equal [
        [:assign, 'num', [:number, '1']],
        [:assign, 'str', [:string, 'a']],
        [:def, 'foo', [], [[:number, '2']]],
      ]
    end
  end
end
