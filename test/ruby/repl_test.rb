require 'minitest/spec'
require 'minitest/autorun'
require 'open3'
require 'timeout'
require_relative '../support/nat_binary'

class ReplWrapper
  def initialize(cmd)
    @in, @out, @wait_thr = Open3.popen2e(*cmd)
  end

  def execute(input)
    @in.puts input
  end

  def readline
    Timeout.timeout(120) { @out.readline }
  end

  def quit
    @in.close
    unless @wait_thr.join(30)
      Process.kill('KILL', @wait_thr.pid)
      @wait_thr.join
    end
    @out.close
  end
end

describe 'REPL' do
  before do
    arguments = [NAT_BINARY]
    arguments << "--backend=#{ENV.fetch('NAT_REPL_TEST_BACKEND')}" if ENV['NAT_REPL_TEST_BACKEND']
    @repl = ReplWrapper.new(arguments)
  end

  after { @repl.quit }

  it 'can execute expressions and affect the environment' do
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
    expect('[1,2,3].to_enum.each {}').must_output('[1, 2, 3]')
  end

  it 'preserves closures while adding and updating local variables' do
    expect('x = 10; reader = -> { x }; writer = ->(value) { x = value }; nil').must_output('nil')
    expect('extra = 20; x = 12').must_output('12')
    expect('reader.call').must_output('12')
    expect('writer.call(42)').must_output('42')
    expect('[x, extra, reader.call]').must_output('[42, 20, 42]')
    expect('future = nil; capture = -> { future }; future = 7').must_output('7')
    expect('another = 9; future = 8').must_output('8')
    expect('capture.call').must_output('8')
  end

  it 'accepts multiline definitions and recovers after runtime and syntax errors' do
    expect("def double(value)\nvalue * 2\nend").must_output(':double')
    expect('double(21)').must_output('42')
    @repl.execute('saved = 123; begin; raise "repl failure"; ensure; puts "ensured"; end')
    @repl.execute(':recovered')
    output = []
    loop do
      line = @repl.readline.strip
      break if line == ':recovered'
      output << line
    end
    assert_includes output, 'ensured'
    assert_includes output.join("\n"), 'repl failure (RuntimeError)'
    expect('saved').must_output('123')
    @repl.execute('1 + * 2')
    @repl.execute(':syntax_recovered')
    output = []
    loop do
      line = @repl.readline.strip
      break if line == ':syntax_recovered'
      output << line
    end
    assert_includes output.join("\n"), 'syntax error'
    expect('double(saved)').must_output('246')
  end

  it 'initializes native dependencies and keeps their methods callable across inputs' do
    fixture = File.expand_path('../support/llvm/fixture.cpp', __dir__)
    expect(
      "require 'natalie/inline'; require #{fixture.inspect}; " \
        'class ReplNative; __bind_static_method__ :subtract, :LlvmFixture_subtract; end; ' \
        '$llvm_extension_initialized',
    ).must_output('1')
    expect('ReplNative.subtract(50, 8)').must_output('42')
  end

  def assert_repl_output(expected_output, input)
    @repl.execute(input)
    assert_equal expected_output, @repl.readline.strip
  end

  String.infect_an_assertion :assert_repl_output, :must_output
end
