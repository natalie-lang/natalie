require_relative '../spec_helper'
require_relative '../fixtures/classes'
require_relative '../shared/pack_sockaddr'

describe "Socket.pack_sockaddr_in" do
  it_behaves_like :socket_pack_sockaddr_in, :pack_sockaddr_in
end
