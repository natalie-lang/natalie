require 'socket'

server = TCPServer.new 3000

pids =
  (1..10).map do
    fork do
      loop do
        client = server.accept
        # sleep 1 # for more-easily testing concurrency :-)
        method, request_target, _http_version = client.gets.strip.split
        headers = {}
        until (line = client.gets) =~ /^\r?\n$/
          name, value = line.strip.split(': ')
          headers[name] = value
        end
        p([method, request_target])
        if method.upcase == 'GET'
          case request_target
          when '/'
            client.write "HTTP/1.1 200\r\n"
            client.write "\r\n"
            client.write "hello world\r\n"
          else
            client.write "HTTP/1.1 404\r\n"
            client.write "\r\n"
            client.write "resource not found\r\n"
          end
        else
          client.write "HTTP/1.1 405\r\n"
          client.write "\r\n"
          client.write "method not allowed\r\n"
        end
      rescue StandardError
        puts 'client misbehaved or connection dropped or something'
      ensure
        client.close
      end
    end
  end

pids.map { |pid| Process.wait(pid) }
