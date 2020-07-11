def fib(n)
  if n == 0
    0
  elsif n == 1
    1
  else
    fib(n - 1) + fib(n - 2)
  end
end

num = ARGV.first
puts fib(num ? num.to_i : 25)
