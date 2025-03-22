require_relative '../spec_helper'

describe 'splat operators' do
  def argument_proxy(*args, **kwargs)
    [args, kwargs]
  end

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

  it 'should work with splat operator and literal keywords' do
    args = [1, 2]
    argument_proxy(*args, foo: 'a').should == [[1, 2], { foo: 'a' }]
    argument_proxy(*[1, 2], foo: 'a').should == [[1, 2], { foo: 'a' }]
  end

  it 'should work with splat operator and double splat operator' do
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

describe 'forward args' do
  def bar(a, b:, c: nil, &block)
    [a, b, c, block&.call].compact
  end

  def foo(...)
    baz(bar(...))
  end

  def baz(a)
    a
  end

  it 'passes all arguments as-is' do
    foo(1, b: 2, c: 3).should == [1, 2, 3]
  end

  it 'passes block arguments' do
    foo(1, b: 2) { 4 }.should == [1, 2, 4]
  end
end

describe 'underscore args' do
  def underscore_arguments(_, _)
    _
  end # rubocop:disable Lint/UnderscorePrefixedVariableName

  it 'does not overwrite the first underscore' do
    underscore_arguments(1, 2).should == 1
  end
end

describe 'keyword argument following splat and optional' do
  def optional_splat_keyword_arguments(o = 1, *s, k:)
    [o, s, k]
  end

  it 'works' do
    optional_splat_keyword_arguments(:a, :b, k: :c).should == [:a, [:b], :c]
  end
end

describe 'optional and keyword argument followed by keyword rest' do
  def optional_keyword_keyword_rest_arguments(o = 1, k: 2, **kr)
    [o, k, kr]
  end

  it 'works' do
    optional_keyword_keyword_rest_arguments(:a, k: :b, c: :d).should == [:a, :b, { c: :d }]
  end
end

describe 'implicit rest arg' do
  it 'discards extra values' do
    [[1, 2, 3]].map { |x,| x }.should == [1]
  end
end

describe 'keyword argument having a previous positional argument as default' do
  def keyword_references_positional(a = 1, b: a)
    [a, b]
  end

  it 'works' do
    keyword_references_positional.should == [1, 1]
    keyword_references_positional(2).should == [2, 2]
    keyword_references_positional(2, b: 3).should == [2, 3]
  end
end

describe 'default values' do
  def blow_up
    raise 'should not be called'
  end

  def positional_arg_with_default_value_call(a = blow_up)
    a
  end

  def keyword_arg_with_default_value_call(a: blow_up)
    a
  end

  it 'does not evaluate argument default value unless needed' do
    -> { positional_arg_with_default_value_call(1) }.should_not raise_error
    -> { keyword_arg_with_default_value_call(a: 1) }.should_not raise_error
  end
end
