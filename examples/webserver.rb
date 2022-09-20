require 'socket'

server = TCPServer.new 3000

loop do
  client = server.accept
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
  client.close
end
