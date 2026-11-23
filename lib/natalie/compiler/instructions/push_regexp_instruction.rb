require_relative './base_instruction'
require_relative '../regexp_encoding'

module Natalie
  class Compiler
    class PushRegexpInstruction < BaseInstruction
      include StringToCpp
      include RegexpEncoding

      def initialize(regexp, encoding: nil)
        @regexp = regexp
        @encoding = encoding
      end

      def to_s
        str = "push_regexp #{@regexp.inspect}"
        str += " (#{@encoding.name})" if @encoding
        str
      end

      def generate_llvm(transform)
        source = @regexp.source
        encoding_flag = @encoding == Encoding::EUC_JP ? 1 : (source.encoding == Encoding::ASCII_8BIT ? 2 : 0)
        transform.push_call(
          "call i64 @nat_llvm_regexp(ptr %env, ptr #{transform.string(source)}, i64 #{source.bytesize}, i32 #{@regexp.options}, i32 #{encoding_flag}, ptr #{transform.literal_cache([:regexp, self])})",
        )
      end

      def generate(transform)
        transform.exec_and_push(:regexp, <<~CPP)
            [&]() -> Value {
              static OnceCache *cache = nullptr;
              OnceCache::Guard guard(env, &cache);
              if (auto cached = guard.value()) return cached.value();
              auto pattern = StringObject::create(String(#{string_to_cpp(@regexp.source)}, #{@regexp.source.bytesize}), EncodingObject::get(Encoding::#{@regexp.source.encoding == Encoding::ASCII_8BIT ? 'ASCII_8BIT' : 'UTF_8'}));
              auto regexp = RegexpObject::create(env, pattern, #{@regexp.options}, #{encoding});
              regexp->freeze();
              Value value(regexp);
              guard.set(value);
              return value;
            }();
          CPP
      end
    end
  end
end
