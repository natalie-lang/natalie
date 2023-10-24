require_relative '../spec_helper'

describe 'Kernel#spawn' do
  it 'accepts an env hash' do
    temp = Tempfile.create('spawn_output')
    temp.close
    ENV['PARENT'] = 'parent'
    pid = spawn({ 'FOO' => 'BAR' }, "bash", "-c", "echo \"$FOO $PARENT\" > #{temp.path}")
    Process.wait(pid)
    File.read(temp.path).should == "BAR parent\n"
  end
end
