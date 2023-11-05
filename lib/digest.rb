require 'digest/sha1'
require 'digest/sha2'

module Digest
  def self.hexencode(str)
    str = str.to_str if !str.is_a?(String) && str.respond_to?(:to_str)
    raise TypeError, "no implicit conversion of #{str.nil? ? 'nil' : str.class} into String" unless str.is_a?(String)
    str.unpack1('H*')
  end
end
