require_relative '../spec_helper'

describe '$@' do
  it 'returns nil if no exception has been thrown' do
    $@.should be_nil
  end
end

describe 'Errno' do
  it 'should use a default message for unknown error numbers' do
    errno = Errno.constants.map { Errno.const_get(_1)::Errno }.max + 1
    exc = SystemCallError.new(nil, errno)
    exc.message.should == "Unknown error #{errno}"

    exc = SystemCallError.new('blah', errno)
    exc.message.should == "Unknown error #{errno} - blah"
  end

  it 'accepts negative errnos' do
    exc = SystemCallError.new(nil, -1)
    exc.message.should == 'Unknown error -1'
  end
end
