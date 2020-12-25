require 'minitest/spec'
require 'minitest/autorun'
require 'open3'

class ReplWrapper
  def initialize
    (@in, @out, @wait_thr) = Open3.popen2e(cmd)
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

  private

  def cmd
    'bin/natalie'
  end
end

describe 'REPL' do
  before do
    @repl = ReplWrapper.new
  end

  it 'can execute expressions and affect the environment' do
    @repl.execute('x = 100')
    @repl.execute('y = 3 * 4')
    @repl.execute('@z = "z"')
    @repl.execute('x')
    @repl.execute('[y, @z]')
    @repl.execute('self')
    @repl.quit
    expect(@repl.out).must_equal dedent(<<-EOF)
      100
      12
      "z"
      100
      [12, "z"]
      main
    EOF
  end

  def dedent(str)
    str.split(/\n/).map(&:strip).join("\n")
  end
end
