# usage:
# diff <(ruby test/encodings/shiftjis_debug.rb) <(bin/natalie test/encodings/shiftjis_debug.rb)

from_encoding = Encoding::SHIFT_JIS
to_encoding = Encoding::UTF_8

(0..0xEAA4).each do |codepoint|
  begin
    str = codepoint.chr(from_encoding)
  rescue RangeError
    puts "chr: 0x#{codepoint.to_s(16)} => RangeError"
  else
    bytes = str.bytes
    puts "chr: 0x#{codepoint.to_s(16)} => #{bytes.map { |b| "0x#{b.to_s(16)}" }.join(', ')}"
    begin
      unicode_string = str.encode(to_encoding)
      unicode_codepoint = unicode_string.ord
      puts "utf8: 0x#{codepoint.to_s(16)} => 0x#{unicode_codepoint.to_s(16)}"
    rescue Encoding::UndefinedConversionError
      puts "utf8: 0x#{codepoint.to_s(16)} => UndefinedConversionError"
    rescue ArgumentError
      puts "utf8: 0x#{codepoint.to_s(16)} => ArgumentError"
    else
      # string = unicode_string.encode(from_encoding)
      # codepoint = string.ord
      # puts "back: 0x#{unicode_codepoint.to_s(16)} => 0x#{codepoint.to_s(16)}"
    end
  end
end
puts "#{RUBY_ENGINE} done"
