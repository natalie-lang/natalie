require_relative 'natalie/compiler'
require_relative 'natalie/parser'

if RUBY_ENGINE != 'natalie'
  require_relative 'natalie/repl'
end
