require 'minitest/spec'
require 'minitest/autorun'
require 'time'

describe 'ruby/spec' do
  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['spec/**/*_spec.{rb,nat}'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    next if code =~ /# skip-test/
    describe path do
      it 'passes all specs' do
        out_nat = `bin/natalie #{path} 2>&1`.force_encoding("US-ASCII").encode("utf-8", replace: nil)
        puts out_nat unless $?.success?
        expect($?).must_be :success?
      end
    end
  end
end
