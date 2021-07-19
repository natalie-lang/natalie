require 'minitest/spec'
require 'minitest/autorun'
require 'time'

describe 'ruby/spec' do
  parallelize_me!

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['spec/**/*_spec.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      it 'passes all specs' do
        skip if code =~ /# skip-test/
        out_nat = `bin/natalie #{path} 2>&1`
        puts out_nat unless $?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      end
    end
  end
end
