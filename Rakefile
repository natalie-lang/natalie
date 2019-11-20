task :test do
  require_relative 'test/natalie/parser_test'
  Dir['examples/*.nat'].each do |path|
    sh "bin/natalie #{path}"
  end
end

task :cloc do
  sh "cloc --not-match-f=hashmap.* ."
end
