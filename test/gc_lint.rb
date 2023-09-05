SKIP_CLASS_MEMBERS = {
  # NOTE: our smart pointer type gets visited by everything else
  'Natalie::Value' => '*',

  # NOTE: Args cannot be heap-allocated, so pointers inside are seen on the stack
  'Natalie::Args' => '*',

  # NOTE: each key is already visited via m_hashmap
  'Natalie::HashObject' => %w[m_key_list],

  # NOTE: iterators aren't heap-allocated
  'Natalie::HashObject::iterator' => '*',
  'Natalie::HeapBlock::iterator' => '*',
  'Natalie::StringObject::iterator' => '*',
}

KNOWN_UNCOLLECTABLE_TYPES = [
  'bool',
  'char',
  'double',
  'fiber_stack_struct',
  'int',
  'long',
  'long long',
  'Natalie::Allocator',
  'Natalie::ArrayPacker::Token',
  'Natalie::Backtrace::Item',
  'Natalie::Block::BlockType',
  'Natalie::Encoding',
  'Natalie::Enumerator::ArithmeticSequenceObject::Origin',
  'Natalie::FiberObject::Status',
  'Natalie::HeapBlock',
  'Natalie::HeapBlock::FreeCellNode',
  'Natalie::Integer',
  'Natalie::LocalJumpErrorType',
  'Natalie::MethodInfo',
  'Natalie::MethodMissingReason',
  'Natalie::MethodVisibility',
  'Natalie::NativeProfilerEvent',
  'Natalie::NativeProfilerEvent::Type',
  'Natalie::ObjectType',
  'Natalie::TimeObject::Mode',
  'TM::Hashmap',
  'TM::Optional',
  'TM::String',
  'TM::Vector',
  'tm',
  'unsigned long',
  'void',
]

# I can't get libclang to give good type info for these templates
TEMPLATE_TYPES = {
  'ManagedVector<T>' => { canonical_name: 'Natalie::ManagedVector', superclass: 'Natalie::Cell' },
}

require 'bundler/inline'

gemfile do
  source 'https://rubygems.org'
  gem 'ffi-clang'
end

@classes = {}
@errors = []

def get_canonical_class_name(cursor)
  if (override = TEMPLATE_TYPES[cursor.display_name])
    return override[:canonical_name]
  end
  name = cursor.type.canonical.spelling
  if name != ''
    name
  else
    # cursor_class_template doesn't give us the canonical name, but this works :-/
    klass = cursor.display_name.sub(/<.*>/, '')
    klass = '(anon)' if klass == ''
    parent = cursor
    while (parent = parent.semantic_parent) && parent.display_name != '' && parent.display_name !~ /\/|\./
      klass = "#{parent.display_name}::#{klass}"
    end
    klass
  end
end

def get_class_details_for_path(path)
  code = File.read(path)
  code_lines = code.split(/\n/)
  index = FFI::Clang::Index.new
  translation_unit = index.parse_translation_unit(path, ['-I', 'include', '-std=c++17'])
  cursor = translation_unit.cursor

  cursor.visit_children do |cursor, parent|
    if cursor.location&.file == path
      #p([cursor.kind, cursor.location&.line, cursor&.display_name])
      if %i[cursor_class_decl cursor_class_template cursor_struct cursor_union].include?(cursor.kind) && cursor.definition?
        line = code_lines[cursor.location.line - 1]
        if line =~ /:/
          base = cursor.type.declaration.filter { |c| c.kind == :cursor_cxx_base_specifier }.first&.spelling
          if base =~ /^class (.+)/
            superclass = $1
          elsif cursor.kind == :cursor_class_template
            superclass = TEMPLATE_TYPES[cursor.display_name][:superclass]
          else
            raise 'could not get base class'
          end
        else
          superclass = nil
        end
        klass = get_canonical_class_name(cursor)
        @classes[klass] = {
          kind: cursor.kind,
          name: klass,
          path: path,
          location: cursor.location,
          superclass: superclass,
          members: [],
          visit_children_method: nil,
        }
      elsif cursor.kind == :cursor_cxx_method && cursor.display_name =~ /^visit_children\(/
        lines = code[cursor.extent.start.offset..cursor.extent.end.offset].split("\n")
        klass = get_canonical_class_name(cursor.semantic_parent)
        @classes[klass][:visit_children_method] = lines[1...-1]
      elsif cursor.kind == :cursor_field_decl
        klass = get_canonical_class_name(cursor.semantic_parent)
        @classes[klass][:members] << {
          type: cursor.type.canonical.spelling,
          name: cursor.display_name,
        }
      end
    end
    :recurse
  end
end

def garbage_collected?(type)
  type = lookup_type(type)
  if %w[Natalie::Cell Natalie::Object Natalie::Value].include?(type)
    true
  elsif (klass = @classes[type])
    if %w[Natalie::Cell Natalie::Object Natalie::Value].include?(klass[:superclass])
      true
    elsif (superclass = @classes[klass[:superclass]])
      garbage_collected?(superclass[:name])
    else
      raise 'unlikely' if klass[:name].strip =~ /.*Object$/
      false
    end
  else
    false
  end
end

def lookup_type(type)
  type
    .sub(/const /, '')
    .sub(/\s*\*?\s*(\[\d*\])?$/, '') # * and []
    .strip
end

def verify_visited(klass, type, name)
  type = type.sub(/const /, '').strip
  details = @classes.fetch(klass)
  if garbage_collected?(type)
    if (method_lines = details[:visit_children_method])
      visits_directly = method_lines.grep(/^\s*visitor\.visit\(&?#{name}/).any?
      loops_over = method_lines.grep(/for\s*\(.*:\s*\*?#{name}\)/).any?
      unless visits_directly || loops_over
        @errors << "#{klass}'s visit_children() method does not visit #{name} (type #{type})!"
      end
    else
      location = details[:location]
      @errors << "#{klass} (#{location.file}##{location.line}) does not have a visit_children() method!"
    end
  elsif !KNOWN_UNCOLLECTABLE_TYPES.include?(lookup_type(type))
    puts "CHECK - not a Natalie class? : #{type} (found in #{klass})"
  end
end

def verify_visits_superclass(klass)
  details = @classes.fetch(klass)
  if details[:superclass] && details[:visit_children_method]
    superclass = details[:superclass].split('::').last
    if klass != 'Natalie::Object' && superclass != 'Cell' && details[:visit_children_method].grep(/#{superclass}::visit_children\(visitor\)/).empty?
      @errors << "#{klass}'s visit_children() method does not call #{superclass}::visit_children()!"
    end
  end
end

paths = (Dir['include/natalie/**/*.hpp'] + Dir['src/**/*.cpp']).to_a
paths.each_with_index do |path, index|
  puts "processing file #{index + 1} of #{paths.size} : #{path}"
  next if path =~ /natalie_parser/
  get_class_details_for_path(path)
end

@classes.each do |klass, details|
  next if SKIP_CLASS_MEMBERS[klass] == '*'

  next if %i[cursor_struct cursor_union].include?(details[:kind])

  details[:members].each do |member|
    next if SKIP_CLASS_MEMBERS.fetch(klass, []).include?(member[:name])

    if member[:type] =~ /^[:\w\*]+ \(\*\)\(.*\)$/
      # looks like a function pointer type
      next
    end

    types = []
    if member[:type] =~ /<(.*)>/
      types << member[:type].split('<').first
      types += $1.split(/\s*,\s*/)
    else
      types = [member[:type]]
    end

    types.each do |t|
      verify_visited(klass, t, member[:name])
    end
  end

  verify_visits_superclass(klass)
end

if @errors.any?
  puts 'Errors:'
  puts @errors
  exit 1
end
