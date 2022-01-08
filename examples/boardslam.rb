#!/usr/bin/env ruby

# This script brute-force computes solutions for a 6x6 grid of numbers (1-36) using
# 3 given digits (chosen by dice roll). See https://p.widencdn.net/gxkq6m/Howtoplayguide
#
# pass 3 numbers as arguments:
#
# bin/natalie examples/boardslam.rb 3 5 1
#
# 3   - 5^0 - 1   = 1
# 3   - 5^0 * 1   = 2
# 3   + 5^0 - 1   = 3
# 3   + 5^0 * 1   = 4
# 3   + 5^0 + 1   = 5
# 3^0 + 5   * 1   = 6
# 3   + 5   - 1   = 7
# 3   + 5   * 1   = 8
# 3   + 5   + 1   = 9
# 3^2 + 5^0 * 1   = 10
# 3^2 + 5^0 + 1   = 11
# 5   - 1   * 3   = 12
# 3^2 + 5   - 1   = 13
# 3   * 5   - 1   = 14
# 3   * 5   * 1   = 15
# 3   * 5   + 1   = 16
# 5^2 - 3^2 + 1   = 17
# 5   + 1   * 3   = 18
# 3   + 1   * 5   = 20
# 3^3 - 5   - 1   = 21
# 3^3 - 5   * 1   = 22
# 3^3 - 5   + 1   = 23
# 3^0 * 5^2 - 1   = 24
# 3^0 + 5^2 - 1   = 25
# 3^0 + 5^2 * 1   = 26
# 3   + 5^2 - 1   = 27
# 3   + 5^2 * 1   = 28
# 3   + 5^2 + 1   = 29
# 3^3 + 5   - 1   = 31
# 3^3 + 5   * 1   = 32
# 3^2 + 5^2 - 1   = 33
# 3^2 + 5^2 * 1   = 34
# 3^2 + 5^2 + 1   = 35
# 5   - 1   * 3^2 = 36
#
# missing answers: 19, 30

class BoardSlam
  BOARD = 1..36
  OPERATIONS = %w[+ - * /]

  def initialize(x, y, z)
    @numbers = [x, y, z]
    @answers = {}
  end

  attr_reader :numbers, :answers

  def variants(num)
    [num, "#{num}^0", "#{num}^2", "#{num}^3"]
  end

  def expand(num)
    return num if num.is_a?(Integer)
    num, exp = num.split('^').map(&:to_i)
    num**exp
  end

  def results
    each_order do |x_base, y_base, z_base|
      each_variant(x_base, y_base, z_base) do |(x_pretty, x), (y_pretty, y), (z_pretty, z)|
        each_operation_pair do |op1, op2|
          result1 = x.send(op1, y)
          next if op1 == '/' && x % y != 0
          result2 = result1.send(op2, z)
          next if op2 == '/' && result1 % z != 0
          if BOARD.include?(result2) and !answers.key?(result2)
            answers[result2] =
              "#{x_pretty.to_s.ljust(3)} #{op1} #{y_pretty.to_s.ljust(3)} #{op2} #{z_pretty.to_s.ljust(3)}"
          end
        end
      end
    end
    answers
  end

  def each_order
    @numbers.permutation.each { |(x, y, z)| yield x, y, z }
  end

  def each_variant(x_base, y_base, z_base)
    variants(x_base).each do |x|
      x_expanded = expand(x)
      variants(y_base).each do |y|
        y_expanded = expand(y)
        variants(z_base).each do |z|
          z_expanded = expand(z)
          yield([x, x_expanded], [y, y_expanded], [z, z_expanded])
        end
      end
    end
  end

  def each_operation_pair
    OPERATIONS.each do |op1|
      OPERATIONS.each do |op2|
        yield(op1, op2)
        yield(op2, op1)
      end
    end
  end

  def missing
    BOARD.to_a - results.keys
  end
end

if $0 == __FILE__
  if ARGV.size != 3
    puts 'usage: bin/natalie examples/boardslam.rb 3 5 1'
    exit
  end
  board = BoardSlam.new(*ARGV.map(&:to_i))
  board.results.sort.each { |result, equation| puts equation.ljust(15) + ' = ' + result.to_s }
  puts
  if board.missing.any?
    puts "missing answers: #{board.missing.join(', ')}"
  else
    puts 'all answers possible!'
  end
end
