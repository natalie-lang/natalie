# Regenerates the Big5 <-> Unicode table in src/encoding/big5_encoding_object.cpp.
#
# The table is derived directly from MRI Ruby's Big5 encoder/decoder so that
# Natalie produces byte-for-byte identical output.
#
# To regenerate:
#
#     ruby test/encodings/big5_tables.rb > /tmp/big5_table.cpp
#
# Then splice the output over the existing `const long Big5Index_max = ...`
# through the closing `};` near the top of big5_encoding_object.cpp.

TRAILS_PER_LEAD = 157
MAX_POINTER = 19_781

def pointer_for(lead, trail)
  offset = trail < 0x7F ? 0x40 : 0x62
  (lead - 0x81) * TRAILS_PER_LEAD + (trail - offset)
end

entries = {}
(0x81..0xFE).each do |lead|
  (0x40..0xFE).each do |trail|
    next if trail == 0x7F
    next if trail > 0x7E && trail < 0xA1
    bytes = [lead, trail].pack('C*').force_encoding('Big5')
    next unless bytes.valid_encoding?
    begin
      entries[pointer_for(lead, trail)] = bytes.encode('UTF-8').codepoints.first
    rescue Encoding::UndefinedConversionError
    end
  end
end

puts "const long Big5Index_max = #{MAX_POINTER};"
puts
puts '// Big5 <-> Unicode table'
puts '// clang-format off'
puts 'const long Big5Index[] = {'
row = []
(0..MAX_POINTER).each do |p|
  row << (entries[p] || -1).to_s
  if row.size == 10
    puts "    #{row.join(', ')},"
    row.clear
  end
end
puts "    #{row.join(', ')}," unless row.empty?
puts '};'
puts '// clang-format on'
