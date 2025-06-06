# usage:
# diff <(ruby test/encodings/shiftjis_debug.rb) <(bin/natalie test/encodings/shiftjis_debug.rb)

from_encoding = Encoding::SHIFT_JIS
to_encoding = Encoding::UTF_8

(0..0xFFFF).each do |codepoint|
  # test codepoint to string
  str = codepoint.chr(from_encoding)
rescue RangeError
  puts "chr: 0x#{codepoint.to_s(16)} => RangeError"
else
  bytes = str.bytes
  puts "chr: 0x#{codepoint.to_s(16)} => #{bytes.map { |b| "0x#{b.to_s(16)}" }.join(', ')}"

  # test next_char
  s2 = "#{str}a".force_encoding(from_encoding)
  puts "chars: #{s2.chars.map { |c| "0x#{c.ord.to_s(16)}" }.join(', ')}"

  # test prev_char
  s3 = "a#{str}".force_encoding(from_encoding).chop
  puts "bytes after chop: #{s3.bytes.map { |b| "0x#{b.to_s(16)}" }.join(', ')}"

  begin
    # test conversion to unicode
    unicode_string = str.encode(to_encoding)
    unicode_codepoint = unicode_string.ord
    puts "utf8: 0x#{codepoint.to_s(16)} => 0x#{unicode_codepoint.to_s(16)}"
  rescue Encoding::UndefinedConversionError
    puts "utf8: 0x#{codepoint.to_s(16)} => UndefinedConversionError"
  rescue ArgumentError
    puts "utf8: 0x#{codepoint.to_s(16)} => ArgumentError"
  else
    begin
      # test conversion from unicode
      string = unicode_string.encode(from_encoding)
    rescue Encoding::UndefinedConversionError
      puts "back: 0x#{unicode_codepoint.to_s(16)} => UndefinedConversionError"
    else
      codepoint = string.ord
      puts "back: 0x#{unicode_codepoint.to_s(16)} => 0x#{codepoint.to_s(16)}"
    end
  end
end
puts "#{RUBY_ENGINE} done"
