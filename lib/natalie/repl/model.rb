module Natalie
  class ReplModel
    def initialize(input = '', index = 0)
      @in = input
      @index = index
      @hist_offset = 0
      @history = []
    end

    def input
      @in
    end

    def index
      @index
    end

    def reset
      @in = ''
      @index = 0
      @hist_offset = 0
    end

    def backspace
      @in.slice!(@index - 1)
      go_left
    end

    def save_last_in_history
      @history.append(@in)
    end

    def history_back
      @hist_offset = [@hist_offset + 1, @history.length].min
      @in = @history[@history.length - @hist_offset] || ''
    end

    def history_forward
      @hist_offset = [@hist_offset - 1, 0].max
      @in = @history[@history.length - @hist_offset] || ''
    end

    def go_left
      @index = [@index - 1, 0].max
    end

    def go_right
      @index = [@index + 1, @in.length].min
    end

    def set_index_by_row_and_col(row, col)
      @index = row == 0 ? col : @in.lines[0..row].each.map { |line| line.length }.sum
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
      diff = [col - (lines[row + 1] || '').length + 1, 0].max
      @index = [@in.lines[row].length + @index - diff, @in.length].min
    end

    def cursor
      return 0, 0 if @in == ''
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
      return row, col
    end

    def number_of_rows
      @in.count("\n") + 1
    end

    def append(c)
      @in.insert(@index, c)
      @index += c.length
    end
  end
end
