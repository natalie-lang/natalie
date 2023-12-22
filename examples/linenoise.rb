require 'linenoise'

HISTORY_PATH = 'linenoise_history.txt'

Linenoise.completion_callback = lambda do |input|
  ['help', 'history', 'delete', 'multi_line'].select do |command|
    command.start_with?(input) && command != input
  end
end

Linenoise.load_history(HISTORY_PATH)

def help
  puts 'commands:'
  puts 'help             print this help'
  puts 'history          print the history list'
  puts 'delete <index>   delete the history item at the given index'
  puts 'delete all       delete the entire history'
  puts 'clear            clear the screen'
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
  when /^delete\s+(\d+)$/
    index = $1.to_i
    history = Linenoise.history
    history.delete_at(index)
    Linenoise.history = history
    p Linenoise.history
  when /^delete\s+all$/
    Linenoise.history = []
    p Linenoise.history
  when /^delete/
    puts 'You must specify an index or "all"'
  when 'clear'
    Linenoise.clear_screen
  when 'multi_line'
    Linenoise.multi_line ^= true
    puts "multi_line (aka word-wrap) is now #{Linenoise.multi_line ? 'on' : 'off'}"
  else
    puts "unknown command: #{line}"
  end
end
Linenoise.save_history(HISTORY_PATH)
