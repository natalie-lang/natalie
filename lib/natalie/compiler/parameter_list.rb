module Natalie
  class Compiler
    # Produces the [[kind, name], ...] list stored in a method's
    # ParamDescriptor table. Output is the method-style canonical form
    # (required params are :req); Proc#parameters rewrites :req -> :opt at
    # read time.
    class ParameterList
      # Single source of truth for the eight parameter kinds. Each row carries
      # the wire code used by bytecode serialization and the matching C++
      # ParamKind enum name used by codegen. Adding a kind requires touching
      # this table and the C++ enum in include/natalie/method.hpp.
      KINDS = [
        [:req, 1, 'Req'],
        [:opt, 2, 'Opt'],
        [:rest, 3, 'Rest'],
        [:keyreq, 4, 'KeyReq'],
        [:key, 5, 'Key'],
        [:keyrest, 6, 'KeyRest'],
        [:nokey, 7, 'NoKey'],
        [:block, 8, 'Block'],
      ].freeze
      KIND_CODES = KINDS.to_h { |sym, code, _| [sym, code] }.freeze
      CODE_KINDS = KINDS.to_h { |sym, code, _| [code, sym] }.freeze
      KIND_CPP_NAMES = KINDS.to_h { |sym, _, cpp| [sym, "ParamKind::#{cpp}"] }.freeze

      def initialize(node)
        node = node.parameters if node.is_a?(Prism::BlockParametersNode)
        @node = node
      end

      def to_a
        case @node
        when ::Prism::ParametersNode
          @node.signature
        when ::Prism::NumberedParametersNode
          @node.maximum.times.map { |i| [:req, :"_#{i + 1}"] }
        when ::Prism::ItParametersNode
          [[:req, nil]]
        else
          []
        end
      end
    end
  end
end
