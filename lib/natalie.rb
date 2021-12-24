require_relative 'natalie/compiler'
require_relative 'natalie/parser'
require_relative 'natalie/repl/legacy'
unless RUBY_ENGINE == 'natalie'
  require_relative 'natalie/repl/main'
end
