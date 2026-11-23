#!/usr/bin/env ruby
# Import MRI's Onigmo using ext/onigmo/upstream.json and ext/onigmo.patch.
# Usage: ruby tools/vendor_onigmo.rb [path/to/ruby] [revision]
# With no revision, reproduce the recorded snapshot. An explicit revision
# updates the pin. Read Git objects so local MRI edits cannot enter the import.

require 'fileutils'
require 'json'
require 'open3'
require 'tmpdir'

abort "Usage: #{$PROGRAM_NAME} [path/to/ruby] [revision]" if ARGV.length > 2

root = File.expand_path('..', __dir__)
destination = File.join(root, 'ext/onigmo')
source = File.expand_path(ARGV[0] || '../ruby', root)
manifest_path = File.join(destination, 'upstream.json')
manifest = JSON.parse(File.read(manifest_path))

def git_read(source, *arguments)
  output, status = Open3.capture2('git', '-C', source, *arguments)
  abort "git #{arguments.first} failed" unless status.success?
  output
end

revision = git_read(source, 'rev-parse', '--verify', "#{ARGV[1] || manifest.fetch('revision')}^{commit}").strip

# Resolve all files and apply the adaptation patch before changing the tree.
Dir.mktmpdir('natalie-onigmo-') do |staging|
  manifest
    .fetch('files')
    .each do |target, upstream|
      output = File.join(staging, target)
      FileUtils.mkdir_p(File.dirname(output))
      File.binwrite(output, git_read(source, 'show', "#{revision}:#{upstream}"))
    end
  success = system('patch', '--batch', '--fuzz=0', '-p1', '-d', staging, '-i', File.join(root, 'ext/onigmo.patch'))
  abort 'Onigmo adaptation patch failed; vendored files were not changed' unless success

  manifest
    .fetch('files')
    .each_key do |target|
      output = File.join(destination, target)
      FileUtils.mkdir_p(File.dirname(output))
      FileUtils.cp(File.join(staging, target), output)
    end
end

manifest['revision'] = revision
File.write(manifest_path, JSON.pretty_generate(manifest) + "\n")
puts "Imported #{manifest.fetch('files').length} MRI files at #{revision}"
