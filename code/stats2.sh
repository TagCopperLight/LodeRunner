#!/bin/sh

total_games=1000
parallel_games=50  # Number of games to run in parallel
tmp_dir=$(mktemp -d)  # Temporary directory for storing results

# Function to process a single game
process_game() {
  game_id=$1
  output=$(./lode_runner -delay 0 -debug off level4.map)

  # Check if the output contains "Congrats" (indicating a win)
  if echo "$output" | grep -q "_____                             _         _       _   _                 _"; then
    win=1
  else
    win=0
  fi

  # Extract the number of moves from the output
  moves=$(echo "$output" | grep -o "Moves: [0-9]*" | awk '{print $2}')
  if [ -z "$moves" ]; then
    moves=0  # Default to 0 if moves are not found
  fi

  # Extract the number of bombs from the output
    bombs=$(echo "$output" | grep -o "Bombs: [0-9]*" | awk '{print $2}')
    if [ -z "$bombs" ]; then
        bombs=0  # Default to 0 if bombs are not found
    fi

  # Write results to a temporary file
  echo "$win $moves $bombs" > "$tmp_dir/result_$game_id"
}

# Run games in parallel
for i in $(seq 1 $total_games); do
  process_game "$i" &  # Launch the game in the background
  sleep 0.05

  # Ensure only a limited number of games run in parallel
  if [ $((i % parallel_games)) -eq 0 ]; then
    wait  # Wait for the current batch to complete
    echo "Progress: $i/$total_games"
  fi
done

# Wait for any remaining background processes to complete
wait

# Aggregate results
wins=0
total_moves=0
total_bombs=0
for result_file in "$tmp_dir"/result_*; do
  read -r win moves bombs < "$result_file"
  wins=$((wins + win))
  total_moves=$((total_moves + moves))
  total_bombs=$((total_bombs + bombs))
done

# Calculate winrate and average moves as floats
winrate=$(echo "scale=1; ($wins * 100) / $total_games" | bc)
average_moves=$(echo "scale=1; $total_moves / $total_games" | bc)
average_bombs=$(echo "scale=1; $total_bombs / $total_games" | bc)

# Output the results
echo "Congrats appeared $wins times"
echo "Winrate: $winrate%"
echo "Average moves per game: $average_moves"
echo "Average bombs per game: $average_bombs"

# Clean up temporary files
rm -rf "$tmp_dir"
