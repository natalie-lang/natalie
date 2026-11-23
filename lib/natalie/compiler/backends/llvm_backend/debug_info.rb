module Natalie
  class Compiler
    class LlvmBackend
      # Debug metadata belongs to one LLVM module, including its nested callbacks.
      class DebugInfo
        def initialize(path)
          @entries = []
          @files = {}
          @locations = {}
          @scopes = {}
          @unit =
            metadata(
              "distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: #{file(path)}, producer: \"Natalie LLVM backend\", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)",
            )
          @word = metadata('!DIBasicType(name: "uint64_t", size: 64, encoding: DW_ATE_unsigned)')
          @pointer = metadata('!DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)')
          @types = {}
          @flags = [metadata('!{i32 2, !"Dwarf Version", i32 4}'), metadata('!{i32 2, !"Debug Info Version", i32 3}')]
        end

        def function(name, path, line, parameters)
          type =
            @types[parameters.size] ||= metadata(
              "!DISubroutineType(types: !{#{[@word, *parameters.map { |p| p == 'self' ? @word : @pointer }].join(', ')}})",
            )
          metadata(
            "distinct !DISubprogram(name: #{quote(name)}, scope: #{file(path)}, file: #{file(path)}, line: #{line}, type: #{type}, scopeLine: #{line}, spFlags: DISPFlagDefinition, unit: #{@unit})",
          )
        end

        def parameters(function, path, line, names)
          names
            .each_with_index
            .map do |name, index|
              variable =
                metadata(
                  "!DILocalVariable(name: #{quote(name)}, arg: #{index + 1}, scope: #{function}, file: #{file(path)}, line: #{line}, type: #{name == 'self' ? @word : @pointer})",
                )
              "  call void @llvm.dbg.value(metadata #{name == 'self' ? 'i64' : 'ptr'} %#{name}, metadata #{variable}, metadata !DIExpression()), !dbg #{location(function, path, line)}"
            end
            .join("\n")
        end

        def location(function, path, line)
          @locations[[function, path, line]] ||= begin
            scope =
              @scopes[[function, path]] ||= metadata(
                "!DILexicalBlockFile(scope: #{function}, file: #{file(path)}, discriminator: 0)",
              )
            metadata("!DILocation(line: #{line}, column: 1, scope: #{scope})")
          end
        end

        def generate
          [
            "!llvm.dbg.cu = !{#{@unit}}",
            "!llvm.module.flags = !{#{@flags.join(', ')}}",
            *@entries.each_with_index.map { |entry, index| "!#{index} = #{entry}" },
          ].join("\n")
        end

        private

        def metadata(entry)
          index = @entries.size
          @entries << entry
          "!#{index}"
        end

        def file(path)
          @files[path] ||= metadata(
            "!DIFile(filename: #{quote(File.basename(path))}, directory: #{quote(File.dirname(File.expand_path(path)))})",
          )
        end

        def quote(value)
          '"' + value.to_s.b.bytes.map { |byte| '\\' + byte.to_s(16).rjust(2, '0') }.join + '"'
        end
      end
    end
  end
end
