require_relative '../spec_helper'

require 'socket'
require_relative '../../spec/library/socket/fixtures/classes'

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

    # This test uses a binary dump of struct sockaddr, which may differ between platforms.
    platform_is :linux do
      it 'works with the result of BasicSocket#getsockname (which is often smaller than sizeof(struct sockaddr))' do
        addrinfo = Addrinfo.new("\x01\x00/tmp/sock\x00".b)
        addrinfo.afamily.should == Socket::AF_UNIX
      end
    end
  end
end

describe 'BasicSocket' do
  describe 'BasicSocket#send' do
    before :each do
      @client = Socket.new(:INET, :DGRAM)
      @server = Socket.new(:INET, :DGRAM)

      @server.bind(Socket.sockaddr_in(0, '127.0.0.1'))
    end

    after :each do
      @client.close
      @server.close
    end

    it 'Using explicit nil argument as dest_sockaddr' do
      -> {
        @client.send('hello', 0, nil)
      }.should raise_error(SystemCallError, /Destination address required/)
    end

    it 'tries to dest_sockaddr into String using #to_str' do
      -> {
        @client.send('hello', 0, Object.new)
      }.should raise_error(TypeError, 'no implicit conversion of Object into String')

      dest_sockaddr = mock('dest_sockaddr')
      dest_sockaddr.should_receive(:to_str).and_return(@server.getsockname.to_s)
      @client.send('hello', 0, dest_sockaddr).should == 5
    end

    it 'does not support an empty string as dest_sockaddr' do
      -> {
        @client.send('hello', 0, '')
      }.should raise_error(Errno::EINVAL)
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

  it 'reads from buffer even if other end of socket has stopped writing' do
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

    client = TCPSocket.new('127.0.0.1', port)
    client.write "GET / HTTP/1.1\r\n" \
                 "Host: localhost:#{port}\r\n" \
                 "User-Agent: ruby\r\n" \
                 "\r\n"

    t.join
  end

  describe 'Socket.unpack_sockaddr_un' do
    before :each do
      @path = SocketSpecs.socket_path
      @server = UNIXServer.new(@path)
    end

    after :each do
      @server.close if @server
      rm_r @path
    end

    it 'can unpack the result of BasicSocket#getsockname (which is often smaller than sizeof(struct sockaddr_un))' do
      packed = @server.getsockname
      Socket.unpack_sockaddr_un(packed).should == @path
    end
  end
end

describe 'UNIXServer' do
  before :each do
    @path = SocketSpecs.socket_path
    @server = UNIXServer.new(@path)
  end

  after :each do
    @server.close if @server
    rm_r @path
  end

  describe '#inspect' do
    it 'includes the path' do
      @server.inspect.should.include?(@path)
    end
  end
end

describe "Socket::BasicSocket#getsockname" do
  before :each do
    @path = SocketSpecs.socket_path
    @server = UNIXServer.new(@path)
  end

  after :each do
    @server.close if @server
    rm_r @path
  end

  it 'returns the sockaddr associated with the socket' do
    sockaddr = Socket.unpack_sockaddr_un(@server.getsockname)
    sockaddr.should == @path
  end
end
