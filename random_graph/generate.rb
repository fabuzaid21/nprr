verts = ARGV[0].to_i
edges = ARGV[1].to_i
num_fils = ARGV[2].to_i

num_fils.times do  |i|
  File.open("./graph_#{verts}_#{edges}_#{i}.in", "w") do |f|
    taken = []
    edges.times do 
      left = rand(verts)
      right = rand(verts)
      next if taken.index [left, right]

      taken << [left, right]
      f.puts("#{left} #{right}")
    end
  end
end