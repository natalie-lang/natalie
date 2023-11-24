module Base64
  def self.encode64(str)
    [str].pack('m')
  end

  def self.decode64(str)
    str.unpack1('m')
  end

  def self.strict_encode64(str)
    [str].pack('m0')
  end

  def self.strict_decode64(str)
    str.unpack1('m0')
  end

  def self.urlsafe_encode64(bin, padding: true)
    encoded = strict_encode64(bin).tr('/+', '_-')
    encoded.tr!('=', '') unless padding
    encoded
  end

  def self.urlsafe_decode64(str)
    converted = str.tr('_-', '/+')
    strict_decode64(converted)
  end
end
