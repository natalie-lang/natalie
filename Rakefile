task :test do
  Dir['test/natalie/*_test.rb'].each do |path|
    load(path)
  end
end

task :cloc do
  sh "cloc --not-match-f=hashmap.* ."
end
