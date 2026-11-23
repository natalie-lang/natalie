# Run after `rake` and `rake llvm_runtime`:
# bin/natalie --backend=llvm examples/llvm_poc.rb

numbers = []
i = 1
while i <= 10
  numbers << i * i
  i = i + 1
end

puts 'Squares from LLVM:'
p numbers
puts numbers.sum
puts(
  if numbers.size == 10
    'ok'
  else
    'failed'
  end,
)

# Keep generated-code values alive across a runtime collection.
GC.start
puts numbers.join(', ')

# Methods, classes, default arguments, and an escaping closure.
class Counter
  def initialize(start)
    @value = start
  end

  def advance(step = 1)
    @value = @value + step
  end

  def callback
    calls = 0
    proc do |step|
      calls = calls + 1
      [calls, advance(step)]
    end
  end
end

counter = Counter.new(10)
callback = counter.callback
p callback.call(2)
GC.start
p callback.call(3)
p({ squares: numbers, window: (2...5).to_a })
