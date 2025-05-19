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
    exc.message.should =~ /Unknown error:? #{errno}/

    exc = SystemCallError.new('blah', errno)
    exc.message.should =~ /Unknown error:? #{errno} - blah/
  end

  it 'accepts negative errnos' do
    exc = SystemCallError.new(nil, -1)
    exc.message.should =~ /Unknown error:? -1/
  end

  it 'converts objects with #to_int' do
    errno = mock('errno')
    errno.should_receive(:to_int).and_return(1)
    exc = SystemCallError.new(nil, errno)
    exc.message.should == SystemCallError.new(nil, 1).message
  end

  it 'raises a TypeError if the errno cannot be converted with #to_int' do
    -> { SystemCallError.new(nil, :errno) }.should raise_error(
                 TypeError,
                 'no implicit conversion of Symbol into Integer',
               )
  end

  it 'raises a TypeError if the conversion with #to_int does not result in an Integer' do
    errno = mock('errno')
    errno.should_receive(:to_int).and_return(:not_an_int)
    -> { SystemCallError.new(nil, errno) }.should raise_error(
                 TypeError,
                 "can't convert MockObject to Integer (MockObject#to_int gives Symbol)",
               )
  end
end
