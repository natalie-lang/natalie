require 'minitest/spec'
require 'minitest/autorun'

describe 'friendly backtrace' do
  parallelize_me!

  it 'works with -e' do
    out = `bin/natalie -e "def bar; xxx; end; def foo; bar; end; foo" 2>&1`
    expect(out).must_equal <<-EOF
Traceback (most recent call last):
        2: from -e:1:in `<main>'
        1: from -e:1:in `foo'
-e:1:in `bar': undefined method `xxx' for main (NoMethodError)
    EOF
  end

  it 'works with a file' do
    out = `bin/natalie test/ruby/fixtures/error.rb 2>&1`
    expect(out).must_equal <<-EOF
Traceback (most recent call last):
        5: from test/ruby/fixtures/error.rb:15:in `<main>'
        4: from test/ruby/fixtures/error.rb:12:in `method1'
        3: from test/ruby/fixtures/error.rb:6:in `method2'
        2: from test/ruby/fixtures/error.rb:6:in `each'
        1: from test/ruby/fixtures/error.rb:7:in `block in method2'
test/ruby/fixtures/error.rb:2:in `method_with_error': undefined method `something_non_existent' for main (NoMethodError)
    EOF
  end
end
