# The `errno` program comes in the `moreutils` package.

`errno -l`.each_line do |line|
  name, number, description = line.strip.split(/\s+/, 3)
  puts "      #{name}: [#{number}, #{description.inspect}],"
end

# I run this on macos with:
#
#     ruby lib/natalie/gen_errno.rb
#
# ...and on Linux (from my Mac) with:
#
#     docker run -i -t --rm -v$(pwd)/lib/natalie/gen_errno.rb:/gen_errno.rb ruby:3.1 bash
#     apt update && apt install -y -q moreutils
#     ruby gen_errno.rb
