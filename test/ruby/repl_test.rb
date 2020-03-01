require 'minitest/spec'
require 'minitest/autorun'
require 'open3'

class ReplWrapper
  def initialize
    (@in, @out, @wait_thr) = Open3.popen2e(cmd)
  end

  def execute(input)
    @in.puts(input + "\n")
  end

  def out
    @out.read.strip
  end

  def quit
    @in.puts("\x04")
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
      nat> x = 100
      100
      nat> y = 3 * 4
      12
      nat> @z = "z"
      "z"
      nat> x
      100
      nat> [y, @z]
      [12, "z"]
      nat> self
      main
      nat> 
    EOF
  end

  def dedent(str)
    str.split(/\n/).map(&:strip).join("\n")
  end
end
