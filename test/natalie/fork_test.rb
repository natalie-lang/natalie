require_relative '../spec_helper'

TMP_DIR = File.expand_path('../tmp', __dir__)

describe :process_fork, shared: true do
  before do
    @path = File.join(TMP_DIR, 'child.txt')
    `rm -f #{@path}`
  end

  context 'with a block' do
    it 'creates a child process with given block and returns the pid' do
      pid = @object.fork do
        `echo 'hello from child with block' > #{@path}`
      end
      pid.should be_kind_of(Integer)
      pid.should > 0
      Process.wait(pid)
      File.read(@path).strip.should == 'hello from child with block'
    end
  end

  context 'without a block' do
    it 'creates a child process and returns pid' do
      pid = @object.fork
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

describe 'Kernel.fork' do
  it "is a private method" do
    Kernel.should have_private_instance_method(:fork)
  end

  # Skip these tests, it's a burden to get this with a private method, and the tests don't add that much
end

describe 'Kernel#fork' do
  it_behaves_like :process_fork, :fork, Kernel
end

describe "Process.fork" do
  it_behaves_like :process_fork, :fork, Process
end
