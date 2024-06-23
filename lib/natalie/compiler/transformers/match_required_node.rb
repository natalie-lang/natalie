# frozen_string_literal: true

module Natalie
  class Compiler
    module Transformers
      class MatchRequiredNode
        attr_reader :compiler

        def initialize(compiler)
          @compiler = compiler
        end

        def call(node)
          case node.pattern.type
          when :array_pattern_node
            transform_array_pattern_node(node.pattern, node.value)
          when :local_variable_target_node
            transform_local_variable_target_node(node.pattern, node.value)
          else
            transform_eqeqeq_check(node.pattern, node.value)
          end
        end

        private

        def transform_array_pattern_node(node, value)
          raise SyntaxError, 'Rest argument in array pattern not yet supported' unless node.rest.nil?
          raise SyntaxError, 'Post arguments in array pattern not yet supported' unless node.posts.empty?
          raise SyntaxError, 'Targets other then local variables not yet supported' unless node.requireds.all? { |n| n.type == :local_variable_target_node }

          # Transform `expr => [a, b] into `a, b = ->(expr) { expr.deconstruct }.call(expr)`
          target = node.requireds.map(&:name).join(', ')
          code_str = <<~RUBY
            #{target} = lambda do |result|
              values = result.deconstruct
              if values.size != #{node.requireds.size}
                raise ::NoMatchingPatternError, "\#{result}: \#{result} length mismatch (given \#{values.size}, expected #{node.requireds.size})"
              end
              values
            rescue NoMethodError
              raise ::NoMatchingPatternError, "\#{result}: \#{result} does not respond to #deconstruct"
            end.call(#{value.location.slice})
          RUBY
          parser = Natalie::Parser.new(code_str, compiler.file.path, locals: compiler.current_locals)
          compiler.transform_expression(parser.ast.statements, used: false)
        end

        def transform_eqeqeq_check(node, value)
          # Transform `expr => var` into `->(res, var) { res === var }.call(expr, var)`
          code_str = <<~RUBY
            lambda do |result, expect|
              unless expect === result
                raise ::NoMatchingPatternError, "\#{result}: \#{expect} === \#{result} does not return true"
              end
            end.call(#{value.location.slice}, #{node.location.slice})
          RUBY
          parser = Natalie::Parser.new(code_str, compiler.file.path, locals: compiler.current_locals)
          compiler.transform_expression(parser.ast.statements, used: false)
        end

        def transform_local_variable_target_node(node, value)
          # Transform `expr => var` into `var = ->(res) { res }.call(expr)`
          code_str = <<~RUBY
            #{node.name} = lambda do |result|
              result
            end.call(#{value.location.slice})
          RUBY
          parser = Natalie::Parser.new(code_str, compiler.file.path, locals: [node.name])
          compiler.transform_expression(parser.ast.statements, used: false)
        end
      end
    end
  end
end
