Dir['test/ruby/*_test.rb'].each do |path|
  load(path)
end
