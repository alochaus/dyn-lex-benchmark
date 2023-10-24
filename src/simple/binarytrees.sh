START=$(date +%s.%N)
lua simple.lua
END=$(date +%s.%N)
DIFF_LUA=$(echo "$END - $START" | bc)
echo "SIMPLE ON LUA: $DIFF_LUA"

START=$(date +%s.%N)
./simple.exe
END=$(date +%s.%N)
DIFF_CEU=$(echo "$END - $START" | bc)
echo "SIMPLE ON CEU: $DIFF_CEU"

RATIO=$(echo "$DIFF_CEU/$DIFF_LUA" | bc -l)
echo "RATIO: $RATIO"