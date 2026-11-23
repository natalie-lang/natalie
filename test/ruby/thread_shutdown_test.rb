require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'
require_relative '../support/command'

describe 'thread shutdown' do
  parallelize_me!

  ['sleep', 'while true; end'].each do |body|
    it "terminates a background thread running #{body.inspect} before runtime teardown" do
      code = <<~RUBY
        ready = Queue.new
        Thread.new do
          begin
            ready << true
            #{body}
          ensure
            puts 'worker stopped'
          end
        end
        ready.pop
        puts 'main finished'
      RUBY
      out = Command.new(NAT_BINARY, '-e', code, timeout: 120).run
      expect(out.lines.map(&:strip).reject(&:empty?)).must_equal ['main finished', 'worker stopped']
    end
  end
end
