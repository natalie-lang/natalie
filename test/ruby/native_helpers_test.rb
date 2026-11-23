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
end
