module Natalie
  class Compiler
    module StringToCpp
      private

      def string_to_cpp(string)
        chars =
          string.to_s.bytes.map do |byte|
            case byte
            when 9
              "\\t"
            when 10
              "\\n"
            when 13
              "\\r"
            when 27
              "\\e"
            when 34
              "\\\""
            when 92
              "\\134"
            when 32..126
              byte.chr(Encoding::ASCII_8BIT)
            else
              "\\%03o" % byte
            end
          end
        '"' + chars.join + '"'
      end
    end
  end
end
