require_relative '../spec_helper'

TMP_DIR = File.expand_path('../tmp', __dir__)

describe 'Kernel#fork' do
  before do
    @path = File.join(TMP_DIR, 'child.txt')
    `rm -f #{@path}`
  end

  context 'with a block' do
    it 'creates a child process with given block and returns the pid' do
      pid = fork do
        `echo 'hello from child with block' > #{@path}`
      end
      pid.should be_kind_of(Integer)
      pid.should > 0
      Process.wait(pid)
      File.read(@path).strip.should == 'hello from child with block'
    end
  end

  context 'without a block' do
    # NATFIXME: This now prints an extra '.' in Natalie
    xit 'creates a child process and returns pid' do
      pid = fork
      if pid.nil?
        # child
        `echo 'hello from child without block' > #{@path}`
        exit!
      else
        # parent
        pid.should be_kind_of(Integer)
        pid.should > 0
        Process.wait(pid)
      end
      File.read(@path).strip.should == 'hello from child without block'
    end
  end
end
