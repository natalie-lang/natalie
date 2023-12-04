# ruby test/encodings/eucjp_debug.rb > rb.txt
# bin/natalie test/encodings/eucjp_debug.rb > nat.txt
# diff rb.txt nat.txt

from_encoding = Encoding::EUC_JP
to_encoding = Encoding::UTF_8

(0..0x8ffefe).each do |codepoint|
  $stderr.print("0x#{codepoint.to_s(16)}    \r") if codepoint % 0x10000 == 0

  if (str = codepoint.chr(from_encoding) rescue nil)
    bytes = str.bytes
    puts "chr: 0x#{codepoint.to_s(16)} => #{bytes.map { |b| "0x#{b.to_s(16)}" }.join(', ')}"
    begin
      unicode_string = str.encode(to_encoding)
      unicode_codepoint = unicode_string.ord
      puts "utf8: 0x#{codepoint.to_s(16)} => 0x#{unicode_codepoint.to_s(16)}"
    rescue Encoding::UndefinedConversionError
    else
      string = unicode_string.encode(from_encoding)
      codepoint = string.ord
      puts "back: 0x#{unicode_codepoint.to_s(16)} => 0x#{codepoint.to_s(16)}"
    end
  end
end
