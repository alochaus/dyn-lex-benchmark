#!/bin/bash


for i in {1..10}
do

    START=$(date +%s.%N)
    lua simple.lua
    END=$(date +%s.%N)
    DIFF_LUA=$(echo "$END - $START" | bc)
    echo "SIMPLE ON LUA: $DIFF_LUA"
    echo "$DIFF_LUA" >> lua_results.csv
done


for i in {1..10}
do

    START=$(date +%s.%N)
    ./simple.exe
    END=$(date +%s.%N)
    DIFF_CEU=$(echo "$END - $START" | bc)
    echo "SIMPLE ON CEU: $DIFF_CEU"
    echo "$DIFF_CEU" >> ceu_results.csv
done


avg_lua=$(awk '{ total += $1 } END { printf("%.4f", total/10) }' lua_results.csv)
avg_ceu=$(awk '{ total += $1 } END { printf("%.4f", total/10) }' ceu_results.csv)

echo "Average Lua Time: $avg_lua"
echo "Average Ceu Time: $avg_ceu"
