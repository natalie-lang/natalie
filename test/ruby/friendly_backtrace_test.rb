require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'friendly backtrace' do
  parallelize_me!

  it 'works with -e' do
    out = `#{NAT_BINARY} -e "def bar; xxx; end; def foo; bar; end; foo" 2>&1`
    expect(out).must_equal <<-EOF
Traceback (most recent call last):
        2: from -e:1:in `<main>'
        1: from -e:1:in `foo'
-e:1:in `bar': undefined method `xxx' for main (NoMethodError)
    EOF
  end

  it 'works with a file' do
    out = `#{NAT_BINARY} test/ruby/fixtures/error.rb 2>&1`
    expect(out).must_equal <<-EOF
Traceback (most recent call last):
        5: from test/ruby/fixtures/error.rb:13:in `<main>'
        4: from test/ruby/fixtures/error.rb:10:in `method1'
        3: from test/ruby/fixtures/error.rb:6:in `method2'
        2: from test/ruby/fixtures/error.rb:6:in `each'
        1: from test/ruby/fixtures/error.rb:6:in `block in method2'
test/ruby/fixtures/error.rb:2:in `method_with_error': undefined method `something_non_existent' for main (NoMethodError)
    EOF
  end

  it 'works with a fiber' do
    out = `#{NAT_BINARY} test/ruby/fixtures/fiber_error.rb 2>&1`
    expect(out).must_equal <<-EOF
Traceback (most recent call last):
        1: from test/ruby/fixtures/fiber_error.rb:4:in `block in make_fiber'
test/ruby/fixtures/fiber_error.rb:4:in `/': divided by 0 (ZeroDivisionError)
    EOF
  end
end
