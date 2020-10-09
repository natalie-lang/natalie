require 'minitest/spec'
require 'minitest/autorun'
require 'time'

def run_nat(path, *args)
  out_nat = `bin/natalie -I test/support #{path} #{args.join(' ')} 2>&1`
  puts out_nat unless $?.success?
  expect($?).must_be :success?
  out_nat
end

def run_ruby(path, *args)
  out_ruby = `ruby -I test/support #{path} #{args.join(' ')} 2>&1`
  puts out_ruby unless $?.to_i == 0
  expect($?).must_be :success?
  out_ruby
end

def run_both_and_compare(path, *args)
  out_nat = run_nat(path, *args)
  out_ruby = run_ruby(path, *args)
  expect(out_nat).must_equal(out_ruby)
end

describe 'Natalie tests' do
  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['test/natalie/*_test.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      if code =~ /# skip-ruby/
        it 'it passes' do
          skip if code =~ /# skip-test/
          run_nat(path)
        end
      else
        it 'has the same output in ruby and natalie' do
          skip if code =~ /# skip-test/
          run_both_and_compare(path)
        end
      end
    end
  end

  describe 'examples/fib.rb' do
    it 'computes the Nth fibonacci number' do
      run_both_and_compare('examples/fib.rb', 5)
    end
  end

  describe 'examples/boardslam.rb' do
    it 'prints solutions to a 6x6 boardslam game card' do
      run_both_and_compare('examples/boardslam.rb', 1, 2, 3)
    end
  end
end
