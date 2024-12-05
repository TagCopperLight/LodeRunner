#!/bin/sh

congrats_count=0
over_count=0
over_seeds=""
curr_seed=$(date +%s)
total_games=500

for i in $(seq 1 $total_games);
do
  # Execute the program and capture its output
  output=$(./lode_runner -delay 0 -debug off -seed $curr_seed level3.map)
  curr_seed=$(expr $curr_seed + 1)

  # Check if the output contains "Congrats" or "Over"
  echo "$output" | grep -q "Congrats"
  if [ $? -eq 0 ]; then
    congrats_count=$(expr $congrats_count + 1)
    seed=$(echo "$output" | grep -o "Seed: [0-9]*" | cut -d' ' -f2)
    echo "Seed: $seed"
  fi

  echo "$output" | grep -q "Over"
  if [ $? -eq 0 ]; then
    over_count=$(expr $over_count + 1)
    seed=$(echo "$output" | grep -o "Seed: [0-9]*" | cut -d' ' -f2)
    echo "Seed: $seed"

    if [ -z "$over_seeds" ]; then
      over_seeds="$seed"
    else
      over_seeds="$over_seeds, $seed"
    fi
  fi
  echo "Progress: $i/$total_games"
done

winrate=$(expr $congrats_count \* 100 / $total_games)

# Output the results
echo "Congrats appeared $congrats_count times"
echo "Over appeared $over_count times"
echo "Seeds for 'Over':$over_seeds"
echo "Winrate: $winrate%"
