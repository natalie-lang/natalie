require 'minitest/spec'
require 'minitest/autorun'

describe 'ARGV' do
  parallelize_me!

  it 'is an empty array if there are no args' do
    expect(%x(bin/natalie -e "p ARGV").strip).must_equal '[]'
  end

  it 'is an array containing args passed to the process' do
    expect(%x(bin/natalie -e "p ARGV" x y).strip).must_equal '["x", "y"]'
  end

  it 'contains args after the double hyphen --' do
    expect(%x(bin/natalie -e "p ARGV" -h).strip).must_match /Usage:/
    expect(%x(bin/natalie -e "p ARGV" -- -h).strip).must_equal '["-h"]'
  end
end
