include("LibOpenJpeg.jl")
include("LibOpenJpegExtra.jl")
using Libdl
using FixedPointNumbers
using ImageBase
a = LibOpenJpegExtra.decode("sample1.jp2")
# println(unsafe_load(a))
c = unsafe_load(a)
b = LibOpenJpegExtra.imagetorgbbuffer(a)
a1 = unsafe_wrap(Array, b.r, c.x1*c.y1)
a2 = unsafe_wrap(Array, b.g, c.x1*c.y1)
a3 = unsafe_wrap(Array, b.b, c.x1*c.y1)
im = colorview(RGB, float(a1/255), float(a2/255), float(a3/255))
img1 = reshape(im, c.x1, c.y1)'
save("test.jpg", img1)