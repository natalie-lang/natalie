require 'minitest/spec'
require 'minitest/autorun'
require 'time'

describe 'Examples' do
  Dir.chdir File.expand_path('../..', __dir__)
  Dir['examples/*.nat'].each do |path|
    next if File.read(path) =~ /# skip-test/
    describe path do
      it 'has the same output in ruby and natalie' do
        out_nat = `bin/natalie #{path} 2>&1`
        puts out_nat unless $?.to_i == 0
        expect($?.to_i).must_equal 0
        out_ruby = `ruby #{path} 2>&1`
        puts out_ruby unless $?.to_i == 0
        expect($?.to_i).must_equal 0
        expect(out_nat).must_equal(out_ruby)
      end
    end
  end
end
