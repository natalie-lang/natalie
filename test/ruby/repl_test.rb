require 'minitest/spec'
require 'minitest/autorun'
require 'open3'
require_relative '../support/nat_binary'

class ReplWrapper
  def initialize(cmd)
    @in, @out, @wait_thr = Open3.popen2e(*cmd)
  end

  def execute(input)
    @in.puts input
  end

  def readline
    @out.readline
  end

  def quit
    @in.close
  end
end

describe 'REPL' do
  it 'can execute expressions and affect the environment' do
    @repl = ReplWrapper.new(NAT_BINARY)
    expect('x = 100').must_output('100')
    expect('x = 100').must_output('100')
    expect('y = 3 * 4').must_output('12')
    expect('@z = "z"').must_output('"z"')
    expect('x').must_output('100')
    expect('[y, @z]').must_output('[12, "z"]')
    expect('self').must_output('main')
    expect('def foo; 2; end').must_output(':foo')
    expect('def bar; 3; end').must_output(':bar')
    expect('foo = foo').must_output('nil') # does NOT call the function
    expect('foo = bar').must_output('3') # DOES call the function
    @repl.quit
  end

  def assert_repl_output(expected_output, input)
    @repl.execute(input)
    assert_equal expected_output, @repl.readline.strip
  end

  String.infect_an_assertion :assert_repl_output, :must_output
end
