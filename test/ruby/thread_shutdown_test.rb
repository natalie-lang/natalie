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
            puts [true.class, false.class, nil.class].join(', ')
            puts 'worker stopped'
          end
        end
        ready.pop
        puts 'main finished'
      RUBY
      out = Command.new(NAT_BINARY, '-e', code, timeout: 120).run
      expect(out.lines.map(&:strip).reject(&:empty?)).must_equal [
                     'main finished',
                     'TrueClass, FalseClass, NilClass',
                     'worker stopped',
                   ]
    end
  end

  it 'does not wait for inherited parent threads in a forked child' do
    code = <<~RUBY
      worker = Thread.new { sleep }
      Thread.pass until worker.status == 'sleep'
      child = fork { exit 0 }
      _, status = Process.wait2(child)
      puts status.exitstatus
    RUBY
    out = Command.new(NAT_BINARY, '-e', code, timeout: 120).run
    expect(out.strip).must_equal '0'
  end
end
