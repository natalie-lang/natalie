index = []
Dir.glob('**/*.rb', base: '.coverage').each do |filename|
  coverage_filename = File.join('.coverage', filename)
  covered_lines = File.read(coverage_filename).split("\n").map(&:to_i)
  code = File.read(filename)
  html_filename = File.join('.coverage', filename + '.html')
  File.open(html_filename, 'w') do |html|
    html.puts('<style>.covered { color: green; }</style>')
    code.each_line(chomp: true).with_index do |code_line, index|
      css_class = covered_lines.include?(index + 1) ? 'covered' : ''
      html.puts("<pre class=\"#{css_class}\">#{(index + 1).to_s.ljust(3)} #{code_line.sub('<', '&lt;')}</pre>")
    end
  end
  index << filename
end

File.open('.coverage/index.html', 'w') do |index_file|
  index_file.puts('<ul>')
  index.each do |filename|
    index_file.puts("<li><a href=\"#{filename}.html\">#{filename}</a></li>")
  end
  index_file.puts('</ul>')
end
