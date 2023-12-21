require 'linenoise'

Linenoise.load_history('test/tmp/linenoise_history.txt')
loop do
  line = Linenoise.readline('prompt> ')
  break if line.nil?

  Linenoise.add_history(line)
  puts line
end
Linenoise.save_history('test/tmp/linenoise_history.txt')
