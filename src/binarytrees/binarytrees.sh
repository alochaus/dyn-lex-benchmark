N=12

START=$(date +%s.%N)
~/lua-aot-5.4/scripts/run binarytrees $N
END=$(date +%s.%N)
DIFF_LUA=$(echo "$END - $START" | bc)
echo "BINARYTREES ON LUA: $DIFF_LUA"

START=$(date +%s.%N)
./binarytrees.exe
END=$(date +%s.%N)
DIFF_CEU=$(echo "$END - $START" | bc)
echo "BINARYTREES ON CEU: $DIFF_CEU"

RATIO=$(echo "$DIFF_CEU/$DIFF_LUA" | bc -l)
echo "RATIO: $RATIO"