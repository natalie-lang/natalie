require_relative '../spec_helper'

def argument_proxy(*args, **kwargs)
  [args, kwargs]
end

describe 'splat operators' do
  it 'should work with literal arguments' do
    argument_proxy(1).should == [[1], {}]
    argument_proxy(1, 2).should == [[1, 2], {}]
    argument_proxy(1, 2, {}).should == [[1, 2, {}], {}]
  end

  it 'should work with literal keyword arguments' do
    argument_proxy(foo: 'a').should == [[], { foo: 'a' }]
    argument_proxy(foo: 'a', bar: 'b').should == [[], { foo: 'a', bar: 'b' }]
  end

  it 'should work with literal positional and keyword arguments' do
    argument_proxy(1, foo: 'a').should == [[1], { foo: 'a' }]
    argument_proxy(1, 2, foo: 'a', bar: 'b').should == [[1, 2], { foo: 'a', bar: 'b' }]
  end

  it 'should work with splat operators' do
    args = [1, 2]
    argument_proxy(*args).should == [[1, 2], {}]
    argument_proxy(*[1, 2]).should == [[1, 2], {}]
    args << {}
    argument_proxy(*args).should == [[1, 2, {}], {}]
    argument_proxy(*[1, 2, {}]).should == [[1, 2, {}], {}]
  end

  # NATFIXME: splat operator and literal keywords
  xit 'should work with splat operator and literal keywords' do
    args = [1, 2]
    argument_proxy(*args, foo: 'a').should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], foo: 'a').should == [[1, 2], { foo: 'a' }]
  end

  # NATFIXME: splat operator and double splat operator
  xit 'should work with splat operator and double splat operator' do
    args = [1, 2]
    kwargs = { foo: 'a' }
    argument_proxy(*args, **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*args, **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
  end

  it 'should work with literal positional arguments and double splat operator' do
    kwargs = { foo: 'a' }
    argument_proxy(1, 2, **kwargs).should == [[1, 2], { foo: 'a' }]
    argument_proxy(1, 2, **{ foo: 'a' }).should == [[1, 2], { foo: 'a' }]
  end
end
