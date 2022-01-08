def make_fiber
  Fiber.new do |i|
    Fiber.yield
    1 / 0
  end
end

def method2
  f = make_fiber
  [1, 2].each { |i| f.resume(i) }
end

def method1
  method2
end

method1
