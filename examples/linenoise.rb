require 'linenoise'

Linenoise.completion_callback = lambda do |input|
  ['help', 'history', 'delete'].select do |command|
    command.start_with?(input) && command != input
  end
end

Linenoise.load_history('test/tmp/linenoise_history.txt')

def help
  puts 'commands: help, history, delete <index>'
end
help

loop do
  line = Linenoise.readline('prompt> ')
  break if line.nil?

  Linenoise.add_history(line)

  case line.strip
  when 'help'
    help
  when 'history'
    p Linenoise.history
  when /^delete($| )/
    unless line =~ /^delete (\d+)$/
      puts 'You must specify an index, e.g. delete 3'
      next
    end
    index = $1.to_i
    history = Linenoise.history
    history.delete_at(index)
    Linenoise.history = history
    p Linenoise.history
  else
    puts "unknown command: #{line}"
  end
end
Linenoise.save_history('test/tmp/linenoise_history.txt')
