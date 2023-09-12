require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require_relative '../support/compare_rubies'
require_relative '../support/nat_binary'

describe 'Natalie tests' do
  include CompareRubies

  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['test/natalie/*_test.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      if code =~ /# skip-ruby/
        it 'it passes' do
          run_nat(path)
        end
      else
        it 'has the same output in ruby and natalie' do
          run_both_and_compare(path)
        end
      end
    end
  end
end
