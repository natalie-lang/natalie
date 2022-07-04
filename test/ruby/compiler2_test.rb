require 'minitest/spec'
require 'minitest/autorun'

require_relative '../support/compare_rubies'

describe 'Natalie::Compiler2' do
  include CompareRubies

  parallelize_me!

  it 'compiles and runs examples/hello.rb' do
    path = File.expand_path('../../examples/hello.rb', __dir__)
    expect(run_nat_c2(path)).must_equal('hello world')
  end

  it 'compiles and runs examples/fib.rb' do
    path = File.expand_path('../../examples/fib.rb', __dir__)
    expect(run_nat_c2(path, 6)).must_equal('8')
  end

  it 'compiles and runs examples/boardslam.rb' do
    path = File.expand_path('../../examples/boardslam.rb', __dir__)
    expect(run_nat_c2(path, 3, 5, 1)).must_equal(`ruby #{path} 3 5 1`.strip)
  end

  it 'compiles and runs test/natalie/compiler2/bootstrap_test.rb' do
    path = File.expand_path('../natalie/compiler2/bootstrap_test.rb', __dir__)
    result = run_nat_c2(path)
    expect(result).must_match(/tests successful/)
    expect(result).must_equal(`ruby #{path}`.strip)
  end

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['test/natalie/*_test.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    if code =~ /# test-compiler2/
      it "compiles and runs #{path}" do
        result = run_nat_c2(path)
        expect(result).must_equal(`ruby #{path}`.strip)
      end
    end
  end
end
