require 'tempfile'

module Natalie
  class Compiler
    BOILERPLATE = <<-END
      #include <stddef.h>
      #include <stdio.h>
      #include <stdlib.h>
      #include <string.h>

      enum NatTypeType {
        NAT_NIL_TYPE,
        NAT_TRUE_TYPE,
        NAT_FALSE_TYPE,
        NAT_KEYWORD_TYPE,
        NAT_NUMBER_TYPE,
        NAT_SYMBOL_TYPE,
        NAT_ARRAY_TYPE,
        NAT_HASHMAP_TYPE,
        NAT_STRING_TYPE,
        NAT_REGEX_TYPE,
        NAT_LAMBDA_TYPE,
        NAT_CONTINUATION_TYPE,
        NAT_ATOM_TYPE,
        NAT_BLANK_LINE_TYPE,
        NAT_ERROR_TYPE
      };

      typedef struct NatType NatType;

      struct NatType {
        enum NatTypeType type;

        union {
          long long number;
          char *symbol;
          char *keyword;
          //struct hashmap hashmap;
          NatType *atom_val;
          NatType *error_val;

          // NAT_ARRAY_TYPE
          struct {
            size_t ary_len;
            size_t ary_cap;
            NatType **ary;
          };

          // NAT_STRING_TYPE
          struct {
            size_t str_len;
            size_t str_cap;
            char *str;
          };

          // NAT_REGEX_TYPE
          struct {
            size_t regex_len;
            char *regex;
          };

          // NAT_LAMBDA_TYPE, NAT_CONTINUATION_TYPE
          //struct {
          //  NatType* (*fn)(NatEnv *env, size_t argc, NatType **args);
          //  char *function_name;
          //  NatEnv *env;
          //  size_t argc;
          //  NatType **args;
          //};
        };
      };

      NatType* nat_alloc() {
        NatType *val = malloc(sizeof(NatType));
        return val;
      }

      NatType* nat_number(long long num) {
        NatType *val = nat_alloc();
        val->type = NAT_NUMBER_TYPE;
        val->number = num;
        return val;
      }

      NatType* nat_string(char *str) {
        NatType *val = nat_alloc();
        val->type = NAT_STRING_TYPE;
        size_t len = strlen(str);
        char *copy = malloc(len + 1);
        memcpy(copy, str, len + 1);
        val->str = copy;
        val->str_len = len;
        val->str_cap = len;
        return val;
      }

      int main() {
        __MAIN__
        return 0;
      }
    END

    def initialize(ast = [])
      @ast = ast
      @var_num = 0
    end

    attr_accessor :ast

    def compile(out_path)
      write_file
      puts @c_path if ENV['DEBUG']
      puts "gcc -g -x c -o #{out_path} #{@c_path}"
      `gcc -g -x c -o #{out_path} #{@c_path}`
      File.unlink(@c_path) unless ENV['DEBUG']
    end

    def write_file
      temp_c = Tempfile.create('natalie.c')
      temp_c.write(to_c)
      temp_c.close
      @c_path = temp_c.path
    end

    def to_c
      exprs = @ast.map { |e| compile_expr(e) }.join("\n")
      BOILERPLATE.sub(/__MAIN__/, exprs)
    end

    def compile_expr(expr)
      case expr.first
      when :number
        "NatType *#{next_var_name} = nat_number(#{expr.last});"
      when :string
        "NatType *#{next_var_name} = nat_string(#{expr.last.inspect});"
      else
        raise "unknown AST node: #{expr.inspect}"
      end
    end

    private

    def next_var_name
      @var_num += 1
      "var#{@var_num}"
    end
  end
end
