SKIP_CLASS_MEMBERS = {
  # our smart pointer type gets visited by everything else
  'Natalie::Value' => '*',

  # Args cannot be heap-allocated, so pointers inside are seen on the stack
  'Natalie::Args' => '*',

  # each key is already visited via m_hashmap
  'Natalie::HashObject' => %w[m_key_list],

  # iterators aren't heap-allocated
  'Natalie::HashObject::iterator' => '*',
  'Natalie::HeapBlock::iterator' => '*',
  'Natalie::StringObject::iterator' => '*',

  # not heap allocated
  'Natalie::EncodingObject::EncodeOptions' => '*',

  # members visited from GlobalEnv::visit_children()
  'Natalie::InstanceEvalContext' => '*',

  # these pointers are visited from the stack and from other objects
  'coroutine_user_data' => '*',
}

GARBAGE_COLLECTED_BASE_CLASSES = %w[
  Natalie::Cell
  Natalie::Integer
  Natalie::Object
  Natalie::Value
]

PATH_PATTERN = %r{^include/natalie}

require 'bundler/inline'

gemfile do
  source 'https://rubygems.org'
  gem 'nokogiri', '~> 1.18.0'
end

@locations = {}
@classes = {}
@classes_by_name = {}
@errors = []
@types = {}

def get_type_info(id)
  return nil if id.nil?

  @types[id] ||= build_type_info(id)
end

def build_type_info(id)
  type = @doc.at("##{id}")
  case type.name
  when 'PointerType', 'ReferenceType'
    {
      kind: type.name,
      id: type['id'],
      type: get_type_info(type['type'])
    }
  when 'ElaboratedType'
    # type['qualifier'] = "TM::" (unnecessary?)
    get_type_info(type['type'])
  when 'Class', 'Struct'
    namespace = get_type_info(type['context'])
    raise "I didn't expect #{namespace.inspect}" unless ['Namespace', 'Class', 'Method'].include?(namespace[:kind])
    {
      kind: type.name,
      id: type['id'],
      name: [
        namespace.fetch(:name),
        type['name'],
      ].compact.join('::'),
    }
  when 'FundamentalType'
    { kind: type.name, id: type['id'], name: type['name'] }
  when 'Typedef'
    # <Typedef id="_2" name="__int128_t" type="_3368" context="_1" location="f0:0" file="f0" line="0"/>
    # We might need the type info?
    { kind: type.name, id: type['id'], name: type['name'] }
  when 'Enumeration'
    { kind: type.name, id: type['id'], name: type['name'] }
  when 'CvQualifiedType'
    # we don't care about const-ness
    get_type_info(type['type'])
  when 'Namespace'
    name = type['name']
    name = nil if name == '::'
    { kind: type.name, id: type['id'], name: }
  when 'ArrayType'
    { kind: type.name, id: type['id'], type: get_type_info(type['type']) }
  when 'Method'
    # A Class inside a Method is really a lambda. :-)
    # <Method id="_33366" name="srand" returns="_8106" context="_8151" access="public" location="f253:45" file="f253" line="45" static="1" inline="1" mangled="_ZN7Natalie12RandomObject5srandEPNS_3EnvEN2TM8OptionalINS_5ValueEEE">
    { kind: type.name, id: type['id'], name: type['name'] }
  when 'FunctionType'
    { kind: type.name, id: type['id'], name: type['name'] }
  else
    raise "Unknown type for id #{id}: #{type.inspect}"
  end
end

def header_path_to_cpp_path(path)
  path.sub(%r{^include/natalie/}, 'src/').sub(/\.hpp$/, '.cpp')
end

def find_visit_method_code(path)
  lines = []
  in_method = false
  braces = 0
  File.read(path).each_line(chomp: true) do |line|
    in_method = true if line =~ /void [A-Za-z:]*visit_children\(.*Visitor &visitor\)/
    if in_method
      braces += (line.count('{') - line.count('}'))
      lines << line
      if braces.zero?
        in_method = false
        break
      end
    end
  end
  lines
end

def garbage_collected?(type)
  # { kind: "PointerType", id: "_3371", type: { kind: "FundamentalType", id: "_8095", name: "char" } }
  case type[:kind]
  when 'PointerType', 'ReferenceType', 'ArrayType'
    garbage_collected?(type[:type])
  when 'Class', 'Struct'
    klass = @classes[type[:id]]
    GARBAGE_COLLECTED_BASE_CLASSES.include?(type[:name]) || superclass_garbage_collected?(klass) || members_garbage_collected?(klass)
  when 'Enumeration', 'FundamentalType', 'Typedef', 'FunctionType'
    false
  else
    raise "Unhandled type: #{type.inspect}"
  end
end

def superclass_garbage_collected?(klass)
  return false unless klass
  return false unless (superclass = klass[:superclass])

  garbage_collected?(superclass)
end

def members_garbage_collected?(klass)
  return false unless klass
  return false unless klass[:members]

  klass[:members].any? do |member|
    # prevent infinite recursion with self-referential classes
    return false if @seen_members.key?(member)
    @seen_members[member] = true

    garbage_collected?(member[:type])
  end
end

def verify_visited(id, member)
  klass = @classes.fetch(id)
  type = member.fetch(:type)
  @seen_members = {}
  if garbage_collected?(type)
    if (method_lines = klass[:visit_children_method])
      name = member.fetch(:name)
      visits_directly = method_lines.grep(/^\s*visitor\.visit\(&?#{name}/).any?
      loops_over = method_lines.grep(/for\s*\(.*:\s*\*?#{name}\)/).any?
      unless visits_directly || loops_over
        @errors << "#{klass[:name]}'s visit_children() method does not visit #{name}\n" \
                   "type: #{type.inspect}\n" \
                   "method code:\n#{method_lines.any? ? method_lines.join("\n") : '(empty)'}"
      end
    else
      @errors << "#{klass[:name]} (#{klass[:path]}##{klass[:line]}) does not have a visit_children() method!"
    end
  else
    if type[:kind] == 'Class' && type[:name] =~ /Object/ && type[:name] !~ /std::function/
      raise "Sanity check: #{member.inspect} is probably garbage collected :-)"
    end
  end
end

def verify_visits_superclass(id)
  klass = @classes.fetch(id)
  class_name = klass[:name]
  if (superclass_id = klass.dig(:superclass, :id)) && klass[:visit_children_method]
    superclass = @classes[superclass_id]
    superclass_name_without_namespace = superclass[:name].split('::').last
    if class_name != 'Natalie::Object' && superclass[:name] != 'Natalie::Cell' && klass[:visit_children_method].grep(/#{superclass_name_without_namespace}::visit_children\(visitor\)/).empty?
      @errors << "#{class_name}'s visit_children() method does not call #{superclass[:name]}::visit_children()!"
    end
  end
end

xml = `castxml --castxml-output=1 -o - -I include -I ext/tm/include -I ext/onigmo include/natalie.hpp`
exit 1 unless $?.success?

@doc = Nokogiri::XML(xml)

puts 'preloading locations'
@doc.css('File').each do |file|
  next unless Pathname.new(file['name']).relative?

  @locations[file['id']] = file['name']
end

puts 'preloading classes'
@doc.css('Class', 'Struct').each do |klass|
  # <Class id="_6241" name="ArrayObject" context="_1038" location="f178:18" file="f178" line="18" ...>

  next unless (path = @locations[klass['file']])

  type = get_type_info(klass['id'])
  name = type.fetch(:name)
  info = {
    name:,
    type:,
    path:,
    line: klass['line'],
    superclass: get_type_info(klass['bases']),
    members: [],
    visit_children_method: nil,
  }
  @classes[klass['id']] = info
  @classes_by_name[name] = info
end

puts 'preloading fields'
@doc.css('Field').each do |field|
  # <Field id="_22589" name="m_vector" type="_5999" init="{}" context="_6241" access="private" location="f178:225" file="f178" line="225" offset="448"/>

  next unless (klass = @classes[field['context']])

  klass[:members] << {
    type: get_type_info(field['type']),
    name: field['name'],
  }
end

puts 'preloading methods'
@doc.css('Method').each do |method|
  # <Method id="_22586" name="visit_children" returns="_5568" context="_6241" access="public" location="f178:210" file="f178" line="210"

  next unless method['name'] == 'visit_children'
  next unless (klass = @classes[method['context']])

  path = @locations[method['file']]
  if method['inline'] == '1'
    klass[:visit_children_method] = find_visit_method_code(path)
  else
    path = header_path_to_cpp_path(path)
    klass[:visit_children_method] = find_visit_method_code(path)
  end
end

puts 'processing data'
@classes.each do |id, details|
  class_name = details[:name]

  next if SKIP_CLASS_MEMBERS[class_name] == '*'

  next unless details[:path] =~ PATH_PATTERN

  details[:members].each do |member|
    next if SKIP_CLASS_MEMBERS.fetch(class_name, []).include?(member[:name])

    verify_visited(id, member)
  end

  verify_visits_superclass(id)
end

if @errors.any?
  puts
  puts 'Errors:'
  @errors.each do |error|
    puts error
    puts
  end
  exit 1
end

puts 'done'
