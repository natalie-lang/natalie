module Base64
  def self.encode64(bin)
    [bin].pack("m")
  end
end
