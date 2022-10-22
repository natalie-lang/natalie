module Base64
  def self.encode64(str)
    [str].pack("m")
  end

  def self.strict_encode64(str)
    [str].pack("m0")
  end

  def self.urlsafe_encode64(bin, padding: true)
    encoded = strict_encode64(bin).tr('/+', '_-')
    encoded.tr!('=', '') unless padding
    encoded
  end
end
