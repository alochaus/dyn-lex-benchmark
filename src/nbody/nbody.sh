#!/bin/bash

N=4

for i in {1..N}
do

    START=$(date +%s.%N)
    lua nbody.lua 100
    END=$(date +%s.%N)
    DIFF_LUA=$(echo "$END - $START" | bc)
    echo "SIMPLE ON LUA: $DIFF_LUA"
    echo "$DIFF_LUA" >> lua_results.csv
done


ceu ./nbody.ceu
for i in {1..N}
do

    START=$(date +%s.%N)
    ./nbody.exe
    END=$(date +%s.%N)
    DIFF_CEU=$(echo "$END - $START" | bc)
    echo "SIMPLE ON CEU: $DIFF_CEU"
    echo "$DIFF_CEU" >> ceu_results.csv
done


vstk ./nbody.ceu
for i in {1..N}
do

    START=$(date +%s.%N)
    ./nbody.exe
    END=$(date +%s.%N)
    DIFF_CEU=$(echo "$END - $START" | bc)
    echo "SIMPLE ON VSTK: $DIFF_CEU"
    echo "$DIFF_CEU" >> vstk_results.csv
done


avg_lua=$(awk '{ total += $1 } END { printf("%.4f", total/10) }' lua_results.csv)
avg_ceu=$(awk '{ total += $1 } END { printf("%.4f", total/10) }' ceu_results.csv)
avg_vstk=$(awk '{ total += $1 } END { printf("%.4f", total/10) }' vstk_results.csv)

echo "Average Lua Time: $avg_lua"
echo "Average Ceu Time: $avg_ceu"
echo "Average Vstk Time: $avg_vstk"

