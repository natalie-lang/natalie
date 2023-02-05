require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_static_method__ :random_bytes :OpenSSL_Random_random_bytes
  end
end
