#!/usr/bin/env ruby

require 'etc'
require 'fileutils'
require 'json'
require 'optparse'
require 'rbconfig'
require 'shellwords'
require 'tmpdir'

# Run unchanged specs through both compiler backends, including ruby_exe children.
# Reports distinguish matching passes from empty runs, timeouts, and failures.
class LlvmSpecRunner
  ROOT = File.expand_path('..', __dir__)
  CLI = File.join(ROOT, 'bin/natalie')

  def initialize(argv)
    @jobs = [Etc.nprocessors, 4].min
    @timeout = 120
    @backends = %w[cpp llvm]
    @report = File.join(ROOT, 'build/llvm-spec-results.json')
    parser =
      OptionParser.new do |opts|
        opts.banner = 'Usage: bundle exec ruby tools/llvm_spec_runner.rb [options] SPEC_OR_GLOB ...'
        opts.on('--jobs=N', Integer) { |n| @jobs = n }
        opts.on('--timeout=SECONDS', Float) { |n| @timeout = n }
        opts.on('--backend=BACKEND', %w[cpp llvm both]) { |b| @backends = b == 'both' ? %w[cpp llvm] : [b] }
        opts.on('--report=PATH') { |path| @report = File.expand_path(path) }
        opts.on('-h', '--help') do
          puts opts
          exit
        end
      end
    parser.parse!(argv)
    raise ArgumentError, 'jobs and timeout must be positive' unless @jobs.positive? && @timeout.positive?
    @files =
      Dir.chdir(ROOT) { argv.flat_map { |pattern| Dir.glob(pattern) }.select { |path| File.file?(path) }.uniq.sort }
    raise ArgumentError, parser.to_s if @files.empty?
  end

  def driver(path, backend)
    command = [RbConfig.ruby, CLI, "--backend=#{backend}"].shelljoin
    File.write(path, "#!/bin/sh\nexec #{command} \"$@\"\n")
    File.chmod(0o755, path)
  end

  def run_spec(file, backend, driver_path, directory, index)
    stdout_path = File.join(directory, "#{index}-#{backend}.out")
    stderr_path = File.join(directory, "#{index}-#{backend}.err")
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    pid =
      Process.spawn(
        { 'NAT_BINARY' => driver_path },
        RbConfig.ruby,
        CLI,
        "--backend=#{backend}",
        file,
        chdir: ROOT,
        out: stdout_path,
        err: stderr_path,
        pgroup: true,
      )
    timed_out = false
    until (wait = Process.wait2(pid, Process::WNOHANG))
      if Process.clock_gettime(Process::CLOCK_MONOTONIC) - started > @timeout
        timed_out = true
        begin
          Process.kill('KILL', -pid)
        rescue Errno::ESRCH
        end
        wait = Process.wait2(pid)
        break
      end
      sleep 0.05
    end
    status = wait.last
    stdout = File.binread(stdout_path).force_encoding(Encoding::UTF_8).scrub
    stderr = File.binread(stderr_path).force_encoding(Encoding::UTF_8).scrub
    counts = %w[passed failed errored skipped].to_h { |name| [name, 0] }
    stdout.scan(/(\d+) spec\(s\) (passed|failed|errored|skipped)\./) { |count, name| counts[name] += count.to_i }
    {
      success: status.success? && !timed_out,
      exit_status: status.exitstatus,
      signal: status.termsig,
      timed_out: timed_out,
      seconds: (Process.clock_gettime(Process::CLOCK_MONOTONIC) - started).round(3),
      counts: counts,
      stdout: stdout,
      stderr: stderr,
    }
  end

  def classify(results)
    if results.values.all? { |result| result[:success] }
      return 'empty' if results.values.any? { |result| result[:counts]['passed'].zero? }
      return results.values.map { |result| result[:counts] }.uniq.size == 1 ? 'passed' : 'different_counts'
    end
    return 'failed' if results.size == 1
    return 'llvm_failed' if results.fetch('cpp')[:success]
    return 'cpp_failed' if results.fetch('llvm')[:success]
    'both_failed'
  end

  def run
    results = []
    lock = Mutex.new
    queue = Queue.new
    @files.each_with_index { |file, index| queue << [file, index] }
    Dir.mktmpdir('natalie-spec-drivers') do |directory|
      drivers =
        @backends.to_h do |backend|
          path = File.join(directory, backend)
          driver(path, backend)
          [backend, path]
        end
      workers =
        [@jobs, @files.size].min.times.map do
          Thread.new do
            loop do
              begin
                file, index = queue.pop(true)
              rescue ThreadError
                break
              end
              runs = @backends.to_h { |b| [b, run_spec(file, b, drivers.fetch(b), directory, index)] }
              outcome = { file: file, outcome: classify(runs), runs: runs }
              lock.synchronize do
                results << outcome
                warn "#{results.size}/#{@files.size} #{outcome[:outcome]} #{file}"
              end
            end
          end
        end
      workers.each(&:value)
    end
    summary = results.group_by { |r| r[:outcome] }.transform_values(&:size)
    examples = results.select { |r| r[:outcome] == 'passed' }.sum { |r| r[:runs].values.first[:counts]['passed'] }
    FileUtils.mkdir_p(File.dirname(@report))
    File.write(
      @report,
      JSON.pretty_generate(
        {
          backends: @backends,
          jobs: @jobs,
          timeout: @timeout,
          summary: summary,
          matching_passed_examples: examples,
          results: results.sort_by { |r| r[:file] },
        },
      ) + "\n",
    )
    puts "#{summary.inspect}; #{examples} passed examples in matching successful files"
    puts "Report: #{@report}"
    summary.keys.all? { |key| %w[passed empty].include?(key) }
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    exit(LlvmSpecRunner.new(ARGV).run ? 0 : 1)
  rescue ArgumentError, OptionParser::ParseError => error
    warn error.message
    exit 2
  end
end
