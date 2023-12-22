require 'linenoise'

HISTORY_PATH = 'linenoise_history.txt'

Linenoise.completion_callback = lambda do |input|
  ['help', 'history', 'delete', 'multi_line'].select do |command|
    command.start_with?(input) && command != input
  end
end

Linenoise.hints_callback = lambda do |input|
  if input =~ /^delete\s*$/
    [' <index> or all', 35, false]
  end
end

Linenoise.highlight_callback = lambda do |input|
  input.split(' ').map do |word|
    case word
    when 'help'
      "\e[31m#{word}\e[0m"
    when 'history', 'delete'
      "\e[33m#{word}\e[0m"
    when 'all'
      "\e[34m#{word}\e[0m"
    when 'clear'
      "\e[32m#{word}\e[0m"
    when 'multi_line'
      "\e[35m#{word}\e[0m"
    when /^\d+$/
      "\e[36m#{word}\e[0m"
    else
      word
    end
  end.join(' ')
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
