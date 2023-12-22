require 'linenoise'

Linenoise.completion_callback = lambda do |input|
  ['help', 'history', 'delete', 'multi_line'].select do |command|
    command.start_with?(input) && command != input
  end
end

Linenoise.load_history('test/tmp/linenoise_history.txt')

def help
  puts 'commands:'
  puts 'help             print this help'
  puts 'history          print the history list'
  puts 'delete <index>   delete the history item at the given index'
  puts 'multi_line       toggle multi-line editing mode (aka word-wrap)'
end
help

loop do
  line = Linenoise.readline('prompt> ')
  break if line.nil?

  Linenoise.add_history(line)

  case line.strip
  when ''
    # noop
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
  when 'multi_line'
    Linenoise.multi_line ^= true
    puts "multi_line (aka word-wrap) is now #{Linenoise.multi_line ? 'on' : 'off'}"
  else
    puts "unknown command: #{line}"
  end
end
Linenoise.save_history('test/tmp/linenoise_history.txt')
