require 'minitest/spec'
require 'minitest/autorun'

describe '$0' do
  parallelize_me!

  it 'returns the path to the initial ruby script executed/compiled' do
    expect(`bin/natalie test/support/dollar_zero.rb`.strip).must_equal '"test/support/dollar_zero.rb"'
    `bin/natalie -c test/tmp/dollar_zero test/support/dollar_zero.rb`
    expect(`test/tmp/dollar_zero`.strip).must_equal '"test/support/dollar_zero.rb"'
  end
end

describe '$exe' do
  it 'returns the path to the compiled binary exe' do
    expect(`bin/natalie test/support/dollar_exe.rb`.strip).must_match /natalie/ # usually something like: /tmp/natalie20210119-2242842-eystnh
    `bin/natalie -c test/tmp/dollar_exe test/support/dollar_exe.rb`
    expect(`test/tmp/dollar_exe`.strip).must_equal '"test/tmp/dollar_exe"'
  end
end
