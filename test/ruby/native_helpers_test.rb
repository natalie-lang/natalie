require 'minitest/autorun'
require_relative '../support/nat_binary'
require_relative '../support/command'

class NativeHelpersTest < Minitest::Test
  def test_explicit_native_calls_and_method_arguments
    output = Command.new(NAT_BINARY, 'test/support/native_helpers.rb').run
    assert_equal <<~OUTPUT, output.lines.reject { |line| line.strip.empty? }.join
      nil
      [42, "record"]
      [0, 2]
      [[], [[:req], [:req]]]
      [7, 7]
      wrong arity
      wrong arity
    OUTPUT
  end

  def test_scoped_inline_is_retired
    [
      'def f; __inline__("return self;"); end; f',
      '1.times { __inline__("return self;") }',
      'class C; __inline__("return self;"); end',
    ].each do |body|
      error = assert_raises(Command::Error) { Command.new(NAT_BINARY, '-e', "require 'natalie/inline'; #{body}").run }
      assert_includes error.to_s, 'scoped __inline__ has been retired'
    end
  end
end
