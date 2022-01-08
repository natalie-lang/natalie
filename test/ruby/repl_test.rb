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

  def out
    @out.read.strip
  end

  def quit
    @in.close
  end
end

describe 'REPL' do
  describe 'MRI-hosted' do
    it 'can execute expressions and affect the environment' do
      execute(NAT_BINARY)
    end
  end

  def execute(cmd)
    @repl = ReplWrapper.new(cmd)
    @repl.execute('x = 100')
    @repl.execute('y = 3 * 4')
    @repl.execute('@z = "z"')
    @repl.execute('x')
    @repl.execute('[y, @z]')
    @repl.execute('self')
    @repl.execute('def foo; 2; end')
    @repl.execute('def bar; 3; end')
    @repl.execute('foo = foo') # does NOT call the function
    @repl.execute('foo = bar') # DOES call the function
    @repl.quit
    expect(@repl.out).must_equal dedent(<<-EOF)
      nat> 100
      nat> 12
      nat> "z"
      nat> 100
      nat> [12, "z"]
      nat> main
      nat> :foo
      nat> :bar
      nat> nil
      nat> 3
      nat>
    EOF
  end

  def dedent(str)
    str.split(/\n/).map(&:strip).join("\n")
  end
end
