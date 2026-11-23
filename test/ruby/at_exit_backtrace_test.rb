require 'minitest/autorun'
require_relative '../support/nat_binary'
require_relative '../support/command'

class AtExitBacktraceTest < Minitest::Test
  def test_script_frame_is_present_while_the_script_runs
    output = Command.new(NAT_BINARY, '-e', 'p caller(0)').run
    assert_equal "[\"-e:1:in '<main>'\"]\n", output
  end

  def test_finished_script_frame_is_absent_inside_at_exit
    output = Command.new(NAT_BINARY, '-e', 'at_exit { p caller(0) }').run
    assert_equal "[\"-e:1:in 'block in <main>'\"]\n", output
  end
end
