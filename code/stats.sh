#!/bin/sh

congrats_count=0
over_count=0
total_games=100

for i in $(seq 1 $total_games);
do
  # Execute the program and capture its output
  output=$(./lode_runner -delay 0 -debug off level0.map)

  # Check if the output contains "Congrats" or "Over"
  echo "$output" | grep -q "_____                             _         _       _   _                 _"
  if [ $? -eq 0 ]; then
    congrats_count=$(expr $congrats_count + 1)
  fi

  echo "$output" | grep -q " _____                        _____"
  if [ $? -eq 0 ]; then
    over_count=$(expr $over_count + 1)
  fi
  echo "Progress: $i/$total_games"
done

winrate=$(expr $congrats_count \* 100 / $total_games)

# Output the results
echo "Congrats appeared $congrats_count times"
echo "Over appeared $over_count times"
echo "Winrate: $winrate%"