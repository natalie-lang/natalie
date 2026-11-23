module Natalie
  class Compiler
    # Produces the [[kind, name], ...] list stored in a method's
    # ParamDescriptor table. Output is the method-style canonical form
    # (required params are :req); Proc#parameters rewrites :req -> :opt at
    # read time.
    class ParameterList
      # These names match the C++ enum in include/natalie/method.hpp.
      KIND_CPP_NAMES = {
        req: 'ParamKind::Req',
        opt: 'ParamKind::Opt',
        rest: 'ParamKind::Rest',
        keyreq: 'ParamKind::KeyReq',
        key: 'ParamKind::Key',
        keyrest: 'ParamKind::KeyRest',
        nokey: 'ParamKind::NoKey',
        block: 'ParamKind::Block',
      }.freeze

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
