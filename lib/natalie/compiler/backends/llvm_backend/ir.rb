require_relative 'debug_info'

module Natalie
  class Compiler
    class LlvmBackend
      # Shared LLVM emission state for instruction generate_llvm methods.
      # Each allocation happens in the entry block, including argument arrays and
      # conservative GC roots, so loops use a bounded amount of native stack.
      class IR
        # ParamKind in method.hpp; the bridge asserts this C-layout metadata ABI.
        PARAMETER_KINDS = %i[req opt rest keyreq key keyrest nokey block].freeze

        DECLARATIONS = <<~LLVM.freeze
          declare i64 @nat_llvm_integer(ptr)
          declare i64 @nat_llvm_float(double)
          declare void @nat_llvm_autoload(ptr, i64, ptr, ptr, ptr)
          declare i64 @nat_llvm_rational(ptr, i64, i64)
          declare i64 @nat_llvm_complex(i64)
          declare i64 @nat_llvm_string(ptr, ptr, i64, ptr, i32, ptr)
          declare i64 @nat_llvm_symbol(ptr, i64)
          declare i64 @nat_llvm_array(i64, ptr)
          declare i64 @nat_llvm_array_shift(i64)
          declare i64 @nat_llvm_array_pop(i64)
          declare i64 @nat_llvm_object_class()
          declare i64 @nat_llvm_const(ptr, i64, i64, ptr, i32, i32)
          declare i64 @nat_llvm_send(ptr, i64, i64, ptr, i64, ptr, i32, i32, ptr, i64, i64)
          declare void @nat_llvm_build_vars(ptr, i64)
          declare void @nat_llvm_grow_vars(ptr, i64)
          declare i64 @nat_llvm_var_get(ptr, i64, i64)
          declare void @nat_llvm_var_set(ptr, i64, i64, i64)
          declare i64 @nat_llvm_block(ptr, i64, ptr, i32, i64, i32, i64, ptr, ptr, i64)
          declare void @nat_llvm_define_method(ptr, i64, ptr, ptr, i32, ptr, i64, i64, ptr)
          declare i64 @nat_llvm_arg(ptr, i64)
          declare void @nat_llvm_check_args(ptr, ptr, i64, i64, i32, i64, ptr)
          declare i64 @nat_llvm_args(ptr, ptr, i32, i64, i64, i32, i32)
          declare i64 @nat_llvm_yield(ptr, i64, ptr, i32)
          declare i64 @nat_llvm_global_get(ptr, ptr)
          declare void @nat_llvm_global_set(ptr, ptr, i64)
          declare i64 @nat_llvm_hash(ptr, i64, ptr)
          declare i64 @nat_llvm_range(ptr, i64, i64, i32)
          declare i64 @nat_llvm_ivar_get(ptr, i64, ptr)
          declare void @nat_llvm_ivar_set(ptr, i64, ptr, i64)
          declare void @nat_llvm_const_set(ptr, i64, ptr, i64, i32)
          declare i64 @nat_llvm_define_type(ptr, i64, i64, ptr, i64, ptr, i32, i32)
          declare i64 @nat_llvm_load(ptr, ptr, ptr, i32, i64)
          declare i64 @nat_llvm_args_empty(ptr, ptr, i32)
          declare i64 @nat_llvm_args_shift(ptr, ptr, i32)
          declare i64 @nat_llvm_args_pop(ptr, ptr, i32)
          declare i64 @nat_llvm_lambda(ptr, i64, i64)
          declare i64 @nat_llvm_push_block(ptr, ptr, i32)
          declare i64 @nat_llvm_send_dynamic(ptr, i64, i64, ptr, i64, ptr, i32, i32, ptr, i64, i64, i32)
          declare void @nat_llvm_array_concat(ptr, i64, i64)
          declare void @nat_llvm_array_push(i64, i64)
          declare i64 @nat_llvm_splat(ptr, i64)
          declare i64 @nat_llvm_to_array(ptr, i64)
          declare i64 @nat_llvm_yield_array(ptr, i64, i32)
          declare i64 @nat_llvm_regexp(ptr, ptr, i64, i32, i32, ptr)
          declare i64 @nat_llvm_once(ptr, i64, ptr, ptr, ptr, ptr, ptr)
          declare i64 @nat_llvm_string_regexp(ptr, i64, i32, i32)
          declare void @nat_llvm_string_append(ptr, i64, i64)
          declare i64 @nat_llvm_case_equal(ptr, i64, i64, i32)
          declare i64 @nat_llvm_try(ptr, i64, ptr, ptr, ptr, ptr, ptr, i32, i32, i64)
          declare void @nat_llvm_jump(ptr, i64, i32, i64) noreturn
          declare i64 @nat_llvm_match_break(ptr, ptr, i64)
          declare i64 @nat_llvm_match_exception(ptr, ptr, i64)
          declare i64 @nat_llvm_rescued()
          declare i64 @nat_llvm_singleton_class(ptr, i64)
          declare i64 @nat_llvm_with_singleton(ptr, i64, ptr)
          declare void @nat_llvm_alias(ptr, i64, i64, i64)
          declare i64 @nat_llvm_super(ptr, i64, i64, ptr, i64, ptr, i32, i32)
          declare void @nat_llvm_check_keywords(ptr, ptr, i64, ptr, i64, ptr, i32)
          declare i64 @nat_llvm_keyword_get(ptr, ptr, ptr)
          declare i64 @nat_llvm_keyword_present(ptr, ptr, ptr)
          declare i64 @nat_llvm_keyword_rest(ptr, ptr, i64, ptr)
          declare i64 @nat_llvm_pop_keywords(ptr)
          declare void @nat_llvm_hash_merge(ptr, i64, i64)
          declare i32 @nat_llvm_loop_step(ptr, i64, ptr, ptr, ptr, ptr, ptr, ptr, i64, i64, i64)
          declare void @nat_llvm_redo()
          declare i64 @nat_llvm_catch_return(ptr, i64, ptr, i64)
          declare i64 @nat_llvm_cvar_get(ptr, i64, ptr, i32)
          declare void @nat_llvm_cvar_set(ptr, i64, ptr, i64)
          declare i64 @nat_llvm_alias_global(ptr, i64, i64)
          declare i64 @nat_llvm_undef_method(ptr, i64, i64)
          declare i64 @nat_llvm_is_defined(ptr, i64, ptr, ptr, ptr, ptr, ptr)
          declare void @nat_llvm_method_defined(ptr, i64, ptr, i32)
          declare void @nat_llvm_ivar_defined(ptr, i64, ptr)
          declare void @nat_llvm_global_defined(ptr, ptr)
          declare i64 @nat_llvm_last_match(ptr)
          declare i64 @nat_llvm_array_empty(i64)
          declare void @nat_llvm_hash_put(ptr, i64, i64, i64)
          declare i64 @nat_llvm_hash_delete(ptr, i64, ptr)
          declare i64 @nat_llvm_hash_has_key(ptr, i64, ptr)
          declare i64 @nat_llvm_dup_object(ptr, i64)
          declare void @nat_llvm_check_defined(i64)
          declare void @nat_llvm_set_zero(ptr, ptr)
          declare void @nat_llvm_set_location(ptr, ptr, i64)
          declare void @nat_llvm_warn(ptr, i64, ptr, i32)
        LLVM

        def initialize(
          instructions,
          source_path:,
          shared: nil,
          function_name: 'nat_llvm_eval',
          scope: nil,
          required_files: {},
          native_extensions: nil,
          module_initializer: false,
          entrypoint: 'nat_llvm_eval',
          repl: false,
          units: nil,
          module_entry: false,
          region_stack: nil,
          return_point: nil,
          loop_points: nil,
          debug_info: false,
          debug_name: nil,
          source_location: nil
        )
          @instructions = instructions
          @source_path = source_path
          @stack = []
          @shared =
            shared ||
              {
                globals: [],
                functions: [],
                serial: 0,
                module_initializer: module_initializer,
                entrypoint: entrypoint,
                repl: repl,
                units: units,
                native_wrappers: [],
                external_files: {
                },
                unit_main: function_name == 'nat_llvm_eval',
                debug: debug_info ? DebugInfo.new(source_path) : nil,
              }
          @globals = @shared[:globals]
          @function_name = function_name
          @scope = scope
          @required_files = required_files
          @native_extensions = native_extensions
          @module_entry = module_entry
          @allocas = []
          @body = []
          @scope_env = @scope || top_scope
          @region_stack = region_stack
          @return_point =
            return_point || ((@function_name == 'nat_llvm_eval' || module_entry) ? file_return_point : nil)
          @loop_points = loop_points
          @debug = @shared[:debug]
          @debug_name = debug_name || '<main>'
          @function_location =
            source_location || @instructions.to_a.find { |i| i.source_location }&.source_location || [@source_path, 1]
          @location = @function_location
          @parameters = %w[env self]
          @parameters += %w[args block] unless @function_name == 'nat_llvm_eval'
          @parameters += %w[frame exception] if @region_stack
          @subprogram = @debug&.function(@debug_name, *@function_location, @parameters)
        end

        def generate
          if @region_stack
            @stack =
              @region_stack.each_with_index.map do |operand, index|
                next operand if operand.is_a?(Integer)
                ptr = value("getelementptr i64, ptr %frame, i64 #{index}")
                value("load volatile i64, ptr #{ptr}")
              end
          end
          if @function_name == 'nat_llvm_eval' && !@shared[:module_initializer] && !@shared[:repl]
            emit "call void @nat_llvm_set_zero(ptr %env, ptr #{string(@source_path)})"
          end

          transform(@instructions)
          anonymous = @shared.fetch(:anonymous_vars, {}).fetch(@scope_env.object_id, {})
          unless @region_stack
            build_vars = @function_name == 'nat_llvm_eval' && @shared[:repl] ? 'grow_vars' : 'build_vars'
            @body.unshift(
              code_line(
                "call void @nat_llvm_#{build_vars}(ptr %env, i64 #{@scope_env.fetch(:vars).size + anonymous.size})",
                @function_location,
              ),
            )
          end
          if @function_name == 'nat_llvm_eval' && @native_extensions&.active?
            @body.unshift(
              code_line("call void @#{@native_extensions.initializer_name}(ptr %env, i64 %self)", @function_location),
            )
          end
          if @function_name == 'nat_llvm_eval' || @module_entry
            symbols =
              @shared
                .fetch(:source_symbols, {})
                .map do |name, pointer|
                  code_line("call i64 @nat_llvm_symbol(ptr #{pointer}, i64 #{name.bytesize})", @function_location)
                end
            @body.unshift(*symbols)
          end
          result = @stack.last || 4
          emit "ret i64 #{result}"
          function = <<~LLVM
            define #{@module_entry ? 'hidden' : 'internal'} i64 @#{llvm_function_name}(ptr %env, i64 %self#{@function_name == 'nat_llvm_eval' ? '' : ', ptr %args, ptr %block'}#{@region_stack ? ', ptr %frame, ptr %exception' : ''}) uwtable#{@subprogram ? " !dbg #{@subprogram}" : ''} {
            entry:
            #{@allocas.join("\n")}
            #{@debug&.parameters(@subprogram, *@function_location, @parameters)}
            #{@body.join("\n")}
            }
          LLVM
          return function unless @function_name == 'nat_llvm_eval' || @module_entry
          output = <<~LLVM
            ; Natalie experimental LLVM backend (64-bit hosts)
            ; Source: #{@source_path.inspect}
            #{@globals.join("\n")}
            #{DECLARATIONS}
            #{@debug ? 'declare void @llvm.dbg.value(metadata, metadata, metadata)' : ''}
            #{@native_extensions&.declarations(wrappers: @shared[:units] ? @shared[:native_wrappers] : nil, include_init: !@shared[:units] || @function_name == 'nat_llvm_eval')}
            #{@shared[:external_files].values.join("\n")}
            #{@shared[:functions].join("\n")}
            #{function}
          LLVM
          return output + (@debug&.generate || '') if @module_entry
          output + <<~LLVM + (@debug&.generate || '')
            define #{@shared[:module_initializer] || @shared[:repl] ? 'hidden ' : ''}i64 @#{@shared[:entrypoint]}(ptr %env, i64 %self) uwtable {
            entry:
              %result = call i64 @nat_llvm_catch_return(ptr %env, i64 %self, ptr @nat_llvm_eval_body, i64 #{@return_point})
              ret i64 %result
            }
          LLVM
        end

        def function_block
          @function_name == 'nat_llvm_eval' ? 'null' : '%block'
        end

        def top_scope
          env = @instructions.to_a.first&.env
          return { vars: {} } unless env
          env = env[:outer] while env[:outer]
          env
        end

        def llvm_function_name
          @function_name == 'nat_llvm_eval' ? 'nat_llvm_eval_body' : @function_name
        end

        def function(body, scope:, return_point: nil, debug_name: nil, source_location: @location)
          fn = name('fn')
          @shared[:functions] << self
            .class
            .new(
              body,
              source_path: @source_path,
              shared: @shared,
              function_name: fn,
              scope: scope,
              return_point: return_point,
              required_files: @required_files,
              native_extensions: @native_extensions,
              debug_name: debug_name || "#{@debug_name} (scope)",
              source_location: source_location,
            )
            .generate
          "@#{fn}"
        end

        def function_args
          @function_name == 'nat_llvm_eval' ? 'null' : '%args'
        end

        def file_return_point(path = @source_path, loaded: false)
          if (units = @shared[:units])
            return jump_token([path, :main_return]) if path == @source_path && @shared[:unit_main] && !loaded
            token = units.return_token(path)
            key = [:file_token, path]
            unless @shared[:external_files].key?(key)
              @shared[:external_files][key] = if path == @source_path && !@shared[:unit_main]
                "@#{token} = hidden global i8 0, align 1"
              else
                "@#{token} = external hidden global i8"
              end
            end
            return "ptrtoint (ptr @#{token} to i64)"
          end
          jump_token([path, :file_return])
        end

        def break_point(key)
          return file_return_point if key == -1
          jump_token([@source_path, key])
        end

        def jump_token(key)
          @shared[:break_points] ||= {}
          @shared[:break_points][key] ||= begin
            # Addresses are unique across independently compiled objects and
            # shared libraries, unlike per-compilation integer counters.
            token = "@#{name('jump_token')}"
            @globals << "#{token} = private global i8 0, align 1"
            "ptrtoint (ptr #{token} to i64)"
          end
        end

        def region(body, baseline)
          fn = name('region')
          child =
            self.class.new(
              body,
              source_path: @source_path,
              shared: @shared,
              function_name: fn,
              scope: @scope_env,
              required_files: @required_files,
              native_extensions: @native_extensions,
              region_stack: baseline,
              loop_points: @loop_points,
              return_point: @return_point,
              debug_name: "#{@debug_name} (callback)",
              source_location: @location,
            )
          @shared[:functions] << child.generate
          ["@#{fn}", [child.stack.size - 1, 0].max]
        end

        def with_loop(points)
          previous = @loop_points
          @loop_points = points
          yield
        ensure
          @loop_points = previous
        end

        def loop_jump(kind, value = '4')
          raise CompileError, "LLVM: #{kind} outside a loop" unless @loop_points
          emit "call void @nat_llvm_jump(ptr %env, i64 #{value}, i32 1, i64 #{@loop_points.fetch(kind)})"
          emit 'unreachable'
          emit "#{name('unreachable')}:"
          push('4')
        end

        def function_exception
          @region_stack ? '%exception' : 'null'
        end

        def return_value(value)
          if @region_stack
            raise CompileError, 'LLVM: return from top-level exception region is unsupported' unless @return_point
            emit "call void @nat_llvm_jump(ptr %env, i64 #{value}, i32 1, i64 #{@return_point})"
            emit 'unreachable'
          else
            emit "ret i64 #{value}"
          end
          emit "#{name('unreachable')}:"
          push('4')
        end

        def loaded_file(path)
          if (units = @shared[:units])
            units.load(path)
            fn = units.entrypoint(path)
            unless @module_entry && path == @source_path
              @shared[:external_files][[:file, path]] = "declare hidden i64 @#{fn}(ptr, i64, ptr, ptr)"
            end
            return "@#{fn}"
          end
          @shared[:loaded_files] ||= {}
          return @shared[:loaded_files][path] if @shared[:loaded_files].key?(path)
          file = @required_files.fetch(path)
          fn = name('loaded')
          @shared[:loaded_files][path] = "@#{fn}"
          @shared[:functions] << self
            .class
            .new(
              file.instructions,
              source_path: path,
              shared: @shared,
              function_name: fn,
              return_point: file_return_point(path),
              required_files: @required_files,
              native_extensions: @native_extensions,
              debug_name: "<required #{File.basename(path)}>",
            )
            .generate
          "@#{fn}"
        end

        def native_wrapper(instruction, count)
          fn = @native_extensions.wrapper(instruction, count, source_path: @shared[:units] ? @source_path : nil)
          @shared[:native_wrappers] << fn
          fn
        end

        # Pass2 already resolves each local to its owning scope. Count Ruby
        # scopes here; the bridge skips its callback environment at each hop.
        def variable_address(instruction)
          env = instruction.env
          env = env[:outer] while env[:hoist]
          depth = 0
          until env[:vars].values.any? { |var| var.equal?(instruction.meta) }
            env = env.fetch(:outer)
            env = env[:outer] while env[:hoist]
            depth += 1
          end
          [depth, instruction.meta.fetch(:index)]
        end

        def anonymous_variable(instruction, name, create: false)
          @shared[:anonymous_vars] ||= {}
          env = instruction.env
          env = env[:outer] while env[:hoist]
          depth = 0
          loop do
            vars = @shared[:anonymous_vars][env.object_id] ||= {}
            vars[name] ||= env.fetch(:vars).size + vars.size if create
            return depth, vars.fetch(name) if vars.key?(name)
            break unless env[:type] == :define_block
            env = env.fetch(:outer)
            env = env[:outer] while env[:hoist]
            depth += 1
          end
          raise CompileError, "LLVM: unbound anonymous #{name}"
        end

        attr_reader :stack, :source_path, :native_extensions

        def transform(instructions)
          previous = @current_instructions
          @current_instructions =
            instructions.is_a?(InstructionManager) ? instructions : InstructionManager.new(instructions)
          @current_instructions.walk do |instruction|
            @location = instruction.source_location || @location
            instruction.generate_llvm(self)
          end
        ensure
          @current_instructions = previous
        end

        def fetch_block_of_instructions(**options)
          @current_instructions.fetch_block(**options)
        end

        def push(operand)
          @stack << operand
        end

        def peek
          @stack.last || raise(CompileError, 'LLVM: instruction stack underflow')
        end

        def argument_array(args)
          return 'null' if args.empty?
          array = "%#{name('args')}"
          @allocas << "  #{array} = alloca [#{args.size} x i64], align 8"
          args.each_with_index do |arg, index|
            ptr = value("getelementptr [#{args.size} x i64], ptr #{array}, i64 0, i64 #{index}")
            emit "store i64 #{arg}, ptr #{ptr}"
          end
          array
        end

        def literal_cache(key)
          @shared[:literal_caches] ||= {}
          @shared[:literal_caches][key] ||= begin
            cache = "@#{name('literal_cache')}"
            @globals << "#{cache} = private global ptr null, align 8"
            cache
          end
        end

        def parameter_table(parameters)
          return 'null' if parameters.empty?
          @shared[:parameter_tables] ||= {}
          @shared[:parameter_tables][parameters] ||= begin
            entries =
              parameters.map do |kind, name|
                id = PARAMETER_KINDS.index(kind) || raise(CompileError, "LLVM: unknown parameter kind #{kind}")
                "{ i8, ptr } { i8 #{id + 1}, ptr #{name ? string(name.to_s) : 'null'} }"
              end
            entries << '{ i8, ptr } { i8 0, ptr null }'
            table = "@#{name('parameters')}"
            @globals << "#{table} = private constant [#{entries.size} x { i8, ptr }] [#{entries.join(', ')}], align 8"
            table
          end
        end

        def keyword_names(names)
          values =
            names.map do |name|
              push_call("call i64 @nat_llvm_symbol(ptr #{string(name.to_s)}, i64 #{name.to_s.bytesize})")
              pop
            end
          argument_array(values)
        end

        def truthy(operand)
          non_nil = value("icmp ne i64 #{operand}, 4")
          non_false = value("icmp ne i64 #{operand}, 0")
          value("and i1 #{non_nil}, #{non_false}")
        end

        def push_call(call)
          result = value(call)
          # Volatile stack slots keep objects visible to the conservative collector,
          # including values LLVM would otherwise keep only in registers.
          root = slot('root')
          emit "store volatile i64 #{result}, ptr #{root}"
          @stack << result
        end

        def slot(prefix)
          ptr = "%#{name(prefix)}"
          @allocas << "  #{ptr} = alloca i64, align 8"
          @allocas << "  store volatile i64 4, ptr #{ptr}"
          ptr
        end

        def symbol_name(name)
          @shared[:source_symbols] ||= {}
          @shared[:source_symbols][name] ||= string(name)
        end

        def set_location(file, line)
          return unless file && line
          @shared[:source_files] ||= {}
          filename = @shared[:source_files][file] ||= string(file)
          emit "call void @nat_llvm_set_location(ptr %env, ptr #{filename}, i64 #{line})"
        end

        def string(str)
          bytes = str.b.bytes + [0]
          global = "@#{name('str')}"
          escaped = bytes.map { |byte| '\\' + byte.to_s(16).rjust(2, '0') }.join
          @globals << "#{global} = private unnamed_addr constant [#{bytes.size} x i8] c\"#{escaped}\""
          global
        end

        def value(expression)
          var = "%#{name('v')}"
          emit "#{var} = #{expression}"
          var
        end

        def name(prefix)
          @shared[:serial] += 1
          "#{prefix}#{@shared[:serial]}"
        end

        def pop(count = nil)
          if count
            raise CompileError, 'LLVM: instruction stack underflow' if @stack.size < count
            @stack.pop(count)
          else
            @stack.pop || raise(CompileError, 'LLVM: instruction stack underflow')
          end
        end

        def code_line(line, location = @location)
          if @debug && !line.end_with?(':')
            "  #{line}, !dbg #{@debug.location(@subprogram, *location)}"
          else
            "  #{line}"
          end
        end

        def emit(line)
          @body << code_line(line)
        end

        def unsupported(instruction, reason = nil)
          raise CompileError, "LLVM backend does not support #{instruction}#{": #{reason}" if reason}"
        end
      end
    end
  end
end
