size = ARGV[0].to_i

File.open("K#{size}.in","w") do |f|
  0.upto(size-1) do |i|
    0.upto(size-1) do |j|
      next if i == j
      f.puts("#{i} #{j}")
    end
  end
end