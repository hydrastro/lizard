#!/usr/bin/evn bash=10
total=0

for i in $(seq 1 "$N"); do
  t=$({ time ./build/lizard < /tmp/pow3.scm  >/dev/null; } 2>&1 | awk '/real/ {print $2}')
  seconds=$(echo "$t" | awk -Fm '{print ($1 * 60) + $2}')
  total=$(echo "$total + $seconds" | bc)
done

echo "Average: $(echo "scale=4; $total / $N" | bc) seconds"
