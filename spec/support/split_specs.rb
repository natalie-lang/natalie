# This is used by our GitHub build workflow to split spec files into groups.
#
# Usage:
#
#     ruby spec/support/split_specs.rb <num_groups> groupN
#
# Example:
#
#     # Split specs into 3 groups and print group 1
#     ruby spec/support/split_specs.rb 3 group1

groups, num = ARGV

raise 'expected "groupN"' unless num =~ /group(\d+)/
num = $1.to_i - 1

list = Dir['spec/**/*_spec.rb']
slice_size = (list.size / groups.to_f).ceil
groups = []
list.each_slice(slice_size).each do |group|
  groups << group
end

puts groups[num].join(',')
