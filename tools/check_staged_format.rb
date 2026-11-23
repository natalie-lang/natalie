require 'fileutils'
require 'open3'
require 'tmpdir'

def git(*args)
  output, error, status = Open3.capture3('git', *args)
  abort error unless status.success?
  output
end

Dir.chdir(git('rev-parse', '--show-toplevel').strip)
root = Dir.pwd

# Read modes from the index too, so symlinks and submodules are never followed.
entries =
  git('ls-files', '--stage', '-z')
    .split("\0")
    .to_h do |entry|
      metadata, path = entry.split("\t", 2)
      [path, metadata.split.first]
    end
paths = git('diff', '--cached', '--name-only', '--diff-filter=ACMR', '-z').split("\0")
paths.select! { |path| %w[100644 100755].include?(entries[path]) }
paths.reject! { |path| path.match?(%r{\A(?:build|ext)/}) }

ruby_files =
  paths.select { |path| path.match?(/\.(?:rb|rake|gemspec)\z/) || %w[Gemfile Rakefile bin/natalie].include?(path) }
cpp_files = paths.select { |path| path.match?(/\.(?:c|cc|cpp|cxx|h|hh|hpp|hxx)\z/) }
cpp_files -= %w[src/encoding/casemap.cpp src/encoding/casefold.cpp]
exit 0 if ruby_files.empty? && cpp_files.empty?

configs =
  entries.keys.select do |path|
    %w[.streerc .clang-format _clang-format .clang-format-ignore].include?(File.basename(path)) &&
      %w[100644 100755].include?(entries[path])
  end

failed = false
Dir.mktmpdir('natalie-staged-format-') do |snapshot|
  # Preserve relative paths and staged formatter settings, without checking out or
  # stashing the working tree. Partially staged files are checked as committed.
  (ruby_files + cpp_files + configs).uniq.each do |path|
    destination = File.join(snapshot, path)
    FileUtils.mkdir_p(File.dirname(destination))
    File.binwrite(destination, git('show', ":#{path}"))
  end

  checks = [
    [%w[bundle exec stree check --], ruby_files],
    [%w[clang-format --dry-run --Werror --style=file --], cpp_files],
  ]
  checks.each do |command, files|
    files.each_slice(100) do |batch|
      output, error, status =
        Open3.capture3({ 'BUNDLE_GEMFILE' => File.join(root, 'Gemfile') }, *command, *batch, chdir: snapshot)
      $stdout.write(output)
      $stderr.write(error)
      failed = true unless status.success?
    rescue Errno::ENOENT => e
      warn "Formatting check unavailable: #{e.message}"
      failed = true
    end
  end
end

if failed
  warn 'Commit rejected: staged files must pass Syntax Tree and clang-format.'
  warn 'Format the listed files with bundle exec stree write or clang-format -i, then stage them again.'
  exit 1
end
