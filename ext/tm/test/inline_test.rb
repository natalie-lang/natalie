require_relative './test_helper'
require 'ffi/clang'
require 'tempfile'

def comments_for_path(path)
  comments = []
  index = FFI::Clang::Index.new
  translation_unit = index.parse_translation_unit(path)
  cursor = translation_unit.cursor
  cursor.visit_children do |cursor, parent|
    if cursor.raw_comment_text && cursor.raw_comment_text =~ /```/
      comments << {
        name: cursor.spelling,
        text: cursor.raw_comment_text,
        line: cursor.location.line,
      }
    end
    next :recurse 
  end 
  comments
end

def blocks_for_comment(comment)
  name = comment[:name]
  lines = comment[:text].split(/\n/)
  blocks = []
  while lines.any?
    line = lines.shift
    if line =~ /```(.*)/
      type = $1
      safe_name = name.gsub(/[^a-z0-9_]/i, '')
      block = { name: "test_#{safe_name}_#{comment[:line]}_#{blocks.size}", type: type, code: [] }
      while lines.any?
        line = lines.shift
        break if line =~ /```/
        block[:code] << line.sub(/\s*\*\s*/, '')
      end
      blocks << block
    end
  end
  blocks
end

def add_block(file, block)
  code = block[:code].join("\n")
  # HACK: This means all tests share the same top-level namespace,
  # so any globals or function names will need to be unique.
  # FIXME: Append a unique id on every global and function and do
  # a find-replace in code. Yikes.
  if code =~ %r{//\s*top\-level}
    top_level_code, code = code.split(%r{// end-top-level.*})
    file.puts(top_level_code)
  end
  file.puts("void #{block[:name]}() {")
  file.puts(code)
  file.puts('}')
  file.puts
end

def compile_to_binary(bin_path, cpp_path)
  out = `c++ -g -Wall -Wextra -Werror -Wno-sign-compare -fsanitize=address -std=c++17 -I include -x c++ -o #{bin_path} #{cpp_path}`
  puts out unless $?.success?
  raise 'error compiling' unless $?.success?
end

def run_test(bin_path, block)
  out = `#{bin_path} #{block[:name]} 2>&1`
  if block[:type] =~ /should_abort/
    if $?.success?
      puts "Expected to abort, but didn't:"
      puts block[:code]
    end
    expect($?).wont_be :success?
    expect(out).must_match /abort|assertion failed/i
  else
    puts out unless $?.success?
    expect($?).must_be :success?
  end
end

describe 'inline doc tests' do
  parallelize_me!

  seen_code = {}

  Dir['include/tm/*.hpp'].each do |path|
    filename = path.sub(/include\//, '')
    describe filename do
      bin_file = Tempfile.new(filename)
      bin_file.close

      cpp_file = Tempfile.new(filename)
      cpp_file.puts(%(#include "#{filename}"))
      cpp_file.puts(%(#include "tm/tests.hpp"))
      cpp_file.puts(%(#include "tm/string.hpp"))
      cpp_file.puts(%(#include "string.h"))
      cpp_file.puts
      cpp_file.puts('using namespace TM;')
      cpp_file.puts

      fn_names = []
      fn_index = 0
      comments_for_path(path).each do |comment|
        name = comment[:name]
        describe name do
          blocks_for_comment(comment).each do |block|
            next if seen_code[block[:code]]
            seen_code[block[:code]] = true

            add_block(cpp_file, block)
            fn_names << block[:name]

            focus if block[:type] =~ /focus/
            specify do
              skip if block[:type] =~ /skip/
              run_test(bin_file.path, block)
            end
          end
        end
      end

      cpp_file.puts('int main(int argc, char **argv) {')
      cpp_file.puts('if (argc < 2) return 1;')

      fn_names.each_with_index do |fn_name, index|
        cpp_file.puts("#{index == 0 ? '' : 'else '}if (strcmp(argv[1], \"#{fn_name}\") == 0)")
        cpp_file.puts("  #{fn_name}();")
      end

      cpp_file.puts('}')
      cpp_file.close

      if fn_names.any?
        compile_to_binary(bin_file.path, cpp_file.path)
      end
    end
  end
end
