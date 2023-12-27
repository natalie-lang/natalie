require_relative 'natalie/compiler'
require_relative 'natalie/parser'
require_relative 'natalie/vm'

unless RUBY_ENGINE == 'natalie'
  # FIXME
  require_relative 'natalie/repl/main'
end

if RUBY_ENGINE == 'natalie'
  require_relative 'natalie/api'
end
