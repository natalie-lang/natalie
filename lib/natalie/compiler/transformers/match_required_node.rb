# frozen_string_literal: true

module Natalie
  class Compiler
    module Transformers
      class MatchRequiredNode
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
          raise SyntaxError, 'Targets other then local variables not yet supported' unless node.requireds.all? { |n| n.type == :local_variable_target_node }
          raise SyntaxError, 'Targets other then local variables not yet supported' unless node.posts.all? { |n| n.type == :local_variable_target_node }

          # Transform `expr => [a, b] into `a, b = ->(expr) { expr.deconstruct }.call(expr)`
          targets = node.requireds.map(&:name)
          expected_size = node.requireds.size + node.posts.size
          expected_size_str = expected_size.to_s
          if node.rest
            targets << :"*#{node.rest.expression&.name}"
            expected_size = "(#{expected_size}..)"
            expected_size_str << '+'
          end
          targets.concat(node.posts.map(&:name))
          <<~RUBY
            #{targets.join(', ')} = lambda do |result|
              values = result.deconstruct
              unless #{expected_size} === values.size
                raise ::NoMatchingPatternError, "\#{result}: \#{values} length mismatch (given \#{values.size}, expected #{expected_size_str})"
              end
              values
            rescue NoMethodError
              raise ::NoMatchingPatternError, "\#{result}: \#{result} does not respond to #deconstruct"
            end.call(#{value.location.slice})
          RUBY
        end

        def transform_eqeqeq_check(node, value)
          # Transform `expr => var` into `->(res, var) { res === var }.call(expr, var)`
          <<~RUBY
            lambda do |result, expect|
              unless expect === result
                raise ::NoMatchingPatternError, "\#{result}: \#{expect} === \#{result} does not return true"
              end
            end.call(#{value.location.slice}, #{node.location.slice})
          RUBY
        end

        def transform_local_variable_target_node(node, value)
          # Transform `expr => var` into `var = ->(res) { res }.call(expr)`
          <<~RUBY
            #{node.name} = lambda do |result|
              result
            end.call(#{value.location.slice})
          RUBY
        end
      end
    end
  end
end
