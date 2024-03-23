require_relative '../spec_helper'
require_relative '../fixtures/classes'
require_relative '../shared/socketpair'

# NATFIXME: Timeout, disable for now
xdescribe "Socket.socketpair" do
  it_behaves_like :socket_socketpair, :socketpair
end
