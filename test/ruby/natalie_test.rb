require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require_relative '../support/compare_rubies'
require_relative '../support/nat_binary'

describe 'Natalie tests' do
  include CompareRubies

  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  files = Dir['test/natalie/**/*_test.rb']

  if !(glob = ENV['GLOB']).to_s.empty?
    glob_files = files & Dir[*glob.split(',')]
    puts "Matched #{glob_files.size} out of #{files.size} total test files:"
    puts glob_files.to_a
    files = glob_files
  end

  files.each do |path|
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
