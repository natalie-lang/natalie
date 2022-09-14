require_relative '../spec_helper'

require 'socket'

describe 'Socket' do
  before do
    @socket = Socket.new(Socket::AF_INET, Socket::SOCK_STREAM)
  end

  it 'connects to a TCP port and can read/write' do
    @socket.connect(Socket.pack_sockaddr_in(80, '71m.us'))
    @socket.write("GET / HTTP/1.0\r\nHost: 71m.us\r\n\r\n")
    @socket.read.should =~ /this page left intentionally blank/
  end
end
