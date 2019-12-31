require 'minitest/spec'
require 'minitest/autorun'
require 'time'

describe 'ruby/spec' do
  Dir.chdir File.expand_path('../..', __dir__)
  Dir['spec/**/*_spec.nat'].each do |path|
    code = File.read(path)
    describe path do
      it 'passes all specs' do
        out_nat = `bin/natalie #{path} 2>&1`
        puts out_nat unless $?.to_i == 0
        expect($?.to_i).must_equal 0
      end
    end
  end
end
