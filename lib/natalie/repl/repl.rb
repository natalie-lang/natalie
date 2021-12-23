#!/usr/bin/env ruby
require 'io/console'
require_relative 'highlight'
require_relative 'model'
require_relative 'suggestions'

module Natalie

class StyledString
    def initialize(str, color)
        @str = str
        @color = color
        @color_map = { green: "\u001b[32m" }
    end

    def to_s
        "#{@color_map[@color]}#{@str}\u001b[0m"
    end

    def length
        @str.length
    end
end

class GenericRepl
def initialize(vars)
    @highlighters = [
      Natalie::KEYWORD_HIGHLIGHT,
      Natalie::PASCAL_CASE_HIGLIGHT,
      Natalie::CAMEL_CASE_HIGLIGHT,
    ]
    @model = Natalie::ReplModel.new
    @suggestions = Natalie::SuggestionProvider.new(vars)
    reset 
end

def hello_natalie
    puts "\u001b[32m
@@@@@@@@@@@@@@@@@@@
@                 @
@   N         N   @     AA     TTTTTTTTT     AA     L       I    EEEEE
@   N N       N   @    A  A        T        A  A    L       I    E
@   N N N     N   @   A    A       T       A    A   L       I    EEE
@   N     N   N   @   AAAAAA       T       AAAAAA   L       I    E
@   N       N N   @   A    A       T       A    A   LLLLL   I    EEEEE
@                 @ 
@@@@@@@@@@@@@@@@@@@
       :::
@@@@@@@@@@@@@@@@@@@\u001b[0m
\n
"
end

def reset 
    @index = 0
    @in = ""
    @hist_offset = 0
end

def echo(feed)
    $stdout.print feed
end

def getch
    $stdin.getch
end

def flush
    $stdout.oflush
end

def ps1
    StyledString.new("nat> ", :green)
end

def tab
    '  '
end

def save_cursor
    echo "\u001B[s"
end

def reset_cursor
    echo "\u001B[u"
end

def highlighted_input
end

def display 
  input = @model.input
  input = Natalie::HighlightingRule.evaluate_all(@highlighters, input)
  input = @suggestions.suggest(input, @model.index)
  return "#{ps1}#{input}" unless input && input != "" 
  input.lines.each.with_index.map do |line, index|
      if index == 0
          "#{ps1}#{line}"
      else
          "\u001b[38;5;241m#{"%#{ps1.length - 1}d" % (index + 1)}\u001b[0m #{line}"
      end
  end.join
end

def get_line
    row, col = @model.cursor
    reset_cursor
    echo "\u001b[0J" # Clear
    echo display
    reset_cursor
    if row > 0
        echo "\u001b[#{row}B"
    end


    hor_shift = ps1.length + col
    echo "\u001b[#{hor_shift}C"
    getch
end

def get_command
    hello_natalie
    save_cursor
    loop do
        c = get_line
        case c.ord
        when 32..126
            @model.append(c)
        when 127 # backspace
            @model.backspace
        when 10..13
            outcome = yield @model.input
            case outcome
            when :continue
              @model.append("\n")
            when :next
              @model.save_last_in_history
              @model.reset
              puts "\n"
              save_cursor
            when :abort
              return
            end
        when 27 # maybe left or right, we need to check the next 2 chars
            first = getch
            second = getch
            if first.ord == 91
                if second.ord == 65
                  @model.go_up
                elsif second.ord == 66
                  @model.go_down
                elsif second.ord == 67
                  @model.go_right
                elsif second.ord == 68
                  @model.go_left
                end
            end
        when 9 # tab
          @model.append(tab)
        when 3 # ctrl+c
          return if @in == ""
          puts "\nPress ctrl+c again or ctrl+d anytime to quit"
          save_cursor            
          @model.reset
        when 4 # ctrl+d
            return
        else
            puts c.ord
        end
    end
end
end
end
