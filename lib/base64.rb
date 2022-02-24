module Base64
  def self.encode64(str)
    [str].pack("m")
  end

  def self.strict_encode64(str)
    [str].pack("m0")
  end
end
