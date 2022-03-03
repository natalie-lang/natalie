require 'minitest/spec'
require 'minitest/autorun'

require_relative '../support/compare_rubies'

describe 'Natalie::VM' do
  include CompareRubies

  it 'executes examples/hello.rb' do
    path = File.expand_path('../../examples/hello.rb', __dir__)
    expect(run_nat_i(path)).must_equal('hello world')
  end

  it 'executes examples/fib.rb' do
    path = File.expand_path('../../examples/fib.rb', __dir__)
    expect(run_nat_i(path, 6)).must_equal('8')
  end

  it 'executes examples/boardslam.rb' do
    path = File.expand_path('../../examples/boardslam.rb', __dir__)
    expect(run_nat_i(path, 3, 5, 1)).must_equal(`ruby #{path} 3 5 1`.strip)
  end

  it 'executes test/natalie/compiler2/bootstrap_test.rb' do
    path = File.expand_path('../natalie/compiler2/bootstrap_test.rb', __dir__)
    result = run_nat_i(path)
    expect(result).must_match(/tests successful/)
    expect(result).must_equal(`ruby #{path}`.strip)
  end
end
