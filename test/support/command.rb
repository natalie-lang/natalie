require 'timeout'
require 'open3'

class Command
  def initialize(command, *args, timeout: 600)
    @command = command
    @args = args
    @timeout = timeout
  end

  class Error < StandardError
    def initialize(message, out:)
      @message = message
      @out = out
    end

    attr_reader :out

    def to_s
      "#{@message}\noutput: #{@out.size == 0 ? '(none)' : @out}"
    end
  end

  class TimeoutError < Error
  end

  def run
    out = []
    status = -1
    begin
      Timeout.timeout(@timeout) do
        status =
          Open3.popen2e(@command, *@args) do |_i, o, t|
            begin
              out << o.gets until o.eof?
              t.value.to_i
            ensure
              begin
                Process.kill(9, t.pid)
              rescue StandardError
                nil
              end
            end
          end
      end
    rescue Timeout::Error
      raise TimeoutError.new("execution expired running: #{full_command}", out: out.join)
    end
    raise Error.new("command failed with status #{status} running: #{full_command}", out: out.join) if status != 0
    out.join("\n")
  end

  private

  def full_command
    [@command, *@args].join(' ')
  end
end
