require_relative '../spec_helper'

require 'socket'

describe 'Addrinfo' do
  describe '.new' do
    it 'works with packed sockaddr strings' do
      addrinfo = Addrinfo.new(Socket.pack_sockaddr_in(80, '127.0.0.1'))
      addrinfo.afamily.should == Socket::AF_INET

      addrinfo = Addrinfo.new(Socket.pack_sockaddr_in('smtp', '2001:DB8::1'))
      addrinfo.afamily.should == Socket::AF_INET6

      addrinfo = Addrinfo.new(Socket.pack_sockaddr_un('socket'))
      addrinfo.afamily.should == Socket::AF_UNIX
    end
  end
end

describe 'Socket' do
  before do
    @socket = Socket.new(Socket::AF_INET, Socket::SOCK_STREAM)
  end

  it 'connects to a TCP port and can read/write' do
    @socket.connect(Socket.pack_sockaddr_in(80, '71m.us'))
    @socket.write("GET / HTTP/1.0\r\nHost: 71m.us\r\n\r\n")
    @socket.read.should =~ /this page left intentionally blank/
  end

  it 'can respond to curl' do
    server = TCPServer.new(0)
    port = server.addr[1]
    t = Thread.new do
      conn = server.accept
      conn.gets.should == "GET / HTTP/1.1\r\n"
      line = conn.gets until line == "\r\n"
      conn.write "HTTP/1.1 200\r\n"
      conn.write "\r\n"
      conn.write "hello world\r\n"
      conn.close
    end

    out = `curl -s -v http://127.0.0.1:#{port} 2>&1`
    out.should =~ /hello world/

    t.join
  end
end
