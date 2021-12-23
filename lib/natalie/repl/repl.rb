#!/usr/bin/env ruby
require 'io/console'
require_relative 'highlight'

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
def initialize
    @history = []
    @highlighters = [
      Natalie::KEYWORD_HIGHLIGHT,
      Natalie::PASCAL_CASE_HIGLIGHT,
      Natalie::CAMEL_CASE_HIGLIGHT,
    ]
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

def history_back
    @hist_offset = [@hist_offset + 1, @history.length].min
    @in = @history[@history.length - @hist_offset] || ""
end

def history_forward
    @hist_offset = [@hist_offset - 1, 0].max
    @in = @history[@history.length - @hist_offset] || ""
end

def go_left
    @index = [@index - 1, 0].max
end

def go_right
    @index = [@index + 1, @in.length].min
end

def set_index_by_row_and_col(row, col)
    @index = if row == 0
        col
    else
        @in.lines[0..row].each.map {|line| line.length}.sum
    end
end

def go_up
    row, col = cursor
    if row == 0
        history_back
        @index = 0
        return
    end
    lines = @in.lines
    diff = [col - lines[row - 1].length + 1, 0].max
    @index = [@index - lines[row - 1].length - diff, 0].max
end

def go_down
    row, col = cursor
    if row >= (number_of_rows - 1)
        history_forward
        @index = 0 # Might make sense to move at the end to speed up scrolling through history
        return 
    end
    lines = @in.lines
    diff = [col - (lines[row + 1] || "").length + 1, 0].max
    @index = [@in.lines[row].length + @index - diff, @in.length].min
end

def cursor
    return [0, 0] if @in == ""
    row = 0
    col = 0
    (0..(@index - 1)).each do |i|
        if @in[i] == "\n"
            row += 1
            col = 0
        else
            col += 1
        end
    end
    return [row, col]
end

def number_of_rows 
    @in.count("\n") + 1
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
  Natalie::HighlightingRule.evaluate_all(@highlighters, @in)
end

def formatted_input 
    input = highlighted_input
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
    row, col = cursor
    reset_cursor
    echo "\u001b[0J" # Clear
    echo formatted_input
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
            @in.insert(@index, c)
            @index += 1
        when 127 # backspace
            @in.slice!(@index - 1)
            go_left
        when 10..13
            outcome = yield @in
            case outcome
            when :continue
                @in.insert(@index, "\n")
                @index += 1
            when :next
                @history.append(@in)
                reset
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
                    go_up
                elsif second.ord == 66
                    go_down
                elsif second.ord == 67
                    go_right
                elsif second.ord == 68
                    go_left
                end
            end
        when 9 # tab
            @in.insert(@index, tab)
            @index += tab.length
        when 3 # ctrl+c
            return if @in == ""
            puts "\nPress ctrl+c again or ctrl+d anytime to quit"
            save_cursor            
            reset
        when 4 # ctrl+d
            return
        else
            puts c.ord
        end
    end
end
end
end
