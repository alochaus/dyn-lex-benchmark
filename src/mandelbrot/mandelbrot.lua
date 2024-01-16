-- Mandelbrot benchmark from benchmarks game
-- https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/mandelbrot.html
--
-- Translated from the Java version found at
-- https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/mandelbrot-java-1.html
--
--  * I don't use any explicit buffering. In my experiments it speeds up the
--    program by less than 10%, while considerably increasing the complexity.
--  * The LuaJIT version needs to be separate, due to the lack of bitwise ops.
--  * The output is in the Netpbm file format. Use an image viewer to view the picture.

local function mandelbrot(N)
    local bits  = 0
    local nbits = 0

    local delta = 2.0 / N
    for y = 0, N-1 do
        local Ci = y*delta - 1.0
        for x = 0, N-1 do
            local Cr = x*delta - 1.5

            local bit = 0x1
            local Zr  = 0.0
            local Zi  = 0.0
            local Zr2 = 0.0
            local Zi2 = 0.0
            for _ = 1, 50 do
                Zi = 2.0 * Zr * Zi + Ci
                Zr = Zr2 - Zi2 + Cr
                Zi2 = Zi * Zi
                Zr2 = Zr * Zr
                if Zi2 + Zr2 > 4.0 then
                    bit = 0x0
                    break
                end
            end

            bits = (bits << 1) | bit
            nbits = nbits + 1

            if nbits == 8 then
                io.write(bits)
                bits  = 0
                nbits = 0
            end
        end

        if nbits > 0 then
            bits  = bits << (8 - nbits)
            io.write(bits)
            bits  = 0
            nbits = 0
        end
    end
end

local function main(N)
    io.write(string.format("P4\n%d %d\n", N, N))
    mandelbrot(N)
end

main(100)
