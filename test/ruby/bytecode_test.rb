require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require_relative '../support/compare_rubies'

describe 'Bytecode VM tests' do
  include CompareRubies
  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['test/bytecode/**/*_test.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      it 'it passes' do
        run_nat(path)
      end
    end
  end
end
