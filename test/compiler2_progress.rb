require 'minitest/spec'
require 'minitest/autorun'
require 'timeout'

Dir.chdir File.expand_path('..', __dir__)

def sh(command)
  command = "systemd-run --scope -p MemoryMax=1G --user #{command}"
  Timeout.timeout(10, nil, "execution expired running: #{command}") {
    `#{command}`
  }
end

Dir['test/natalie/*_test.rb', 'spec/**/*_spec.rb'].each do |path|
  code = File.read(path, encoding: 'utf-8')
  unless code =~ /# skip-test/
    describe path do
      it 'passes' do
        sh("bin/natalie -c2 -I test/support #{path} 2>&1").strip
        expect($?).must_be :success?
      end
    end
  end
end
