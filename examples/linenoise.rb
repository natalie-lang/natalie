require 'linenoise'

Linenoise.completion_callback = lambda do |input|
  ['hello', 'hello there'].select do |command|
    command.start_with?(input) && command != input
  end
end

Linenoise.load_history('test/tmp/linenoise_history.txt')
loop do
  line = Linenoise.readline('prompt> ')
  break if line.nil?

  Linenoise.add_history(line)
  puts line
end
Linenoise.save_history('test/tmp/linenoise_history.txt')
