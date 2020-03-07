require 'minitest/spec'
require 'minitest/autorun'
require 'time'

describe 'Natalie tests' do
  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['test/natalie/*_test.nat'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    next if code =~ /# skip-test/
    describe path do
      it 'has the same output in ruby and natalie' do
        out_nat = `bin/natalie -I test/support #{path} 2>&1`
        puts out_nat unless $?.to_i == 0
        expect($?.to_i).must_equal 0
        unless code =~ /# skip-ruby-test/
          out_ruby = `ruby -r./test/support/ruby_require_patch -I test/support #{path} 2>&1`
          puts out_ruby unless $?.to_i == 0
          expect($?.to_i).must_equal 0
          expect(out_nat).must_equal(out_ruby)
        end
      end
    end
  end
end
