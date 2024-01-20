require_relative '../spec_helper'
require_relative '../fixtures/classes'

# NATFIXME: Timeout, disable for now
xdescribe "Socket.for_fd" do
  before :each do
    @server = TCPServer.new("127.0.0.1", 0)
    @port = @server.addr[1]
    @client = TCPSocket.open("127.0.0.1", @port)
  end

  after :each do
    @socket.close
    NATFIXME 'Fix IO#autoclose', exception: Errno::EBADF do
      @client.close
    end
    @host.close
    @server.close
  end

  it "creates a new Socket that aliases the existing Socket's file descriptor" do
    @socket = Socket.for_fd(@client.fileno)
    @socket.autoclose = false
    @socket.fileno.should == @client.fileno

    @socket.send("foo", 0)
    @client.send("bar", 0)

    @host = @server.accept
    @host.read(3).should == "foo"
    @host.read(3).should == "bar"
  end
end
