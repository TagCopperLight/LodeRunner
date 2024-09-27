#include <stdbool.h> // bool, true, false
#include <stdlib.h>  // rand
#include <stdio.h>   // printf
#include "lode_runner.h" // types and prototypes used to code the game

// global declarations used by the game engine
extern const char BOMB;  // ascii used for the bomb
extern const char BONUS;  // ascii used for the bonuses
extern const char CABLE;  // ascii used for the cable
extern const char ENEMY;  // ascii used for the ennemies
extern const char EXIT;   // ascii used for the exit
extern const char FLOOR;  // ascii used for the floor
extern const char LADDER; // ascii used for the ladder
extern const char PATH;   // ascii used for the pathes
extern const char RUNNER; // ascii used for runner
extern const char WALL;   // ascii used for the walls

extern const int BOMB_TTL; // time to live for bombs

extern bool DEBUG; // true if and only if the game runs in debug mode

const char *students = "Random"; // replace Radom with the student names here

// local prototypes (add your own prototypes below)
void print_action(action); 

/* 
  function to code: it may use as many modules (functions and procedures) as needed
  Input (see lode_runner.h for the type descriptions): 
    - level provides the information for the game level
    - characterl is the linked list of all the characters (runner and enemies)
    - bonusl is the linked list of all the bonuses that have not been collected yet
    - bombl is the linked list of all the bombs that are still active
  Output
    - the action to perform
*/
action lode_runner(
  levelinfo level,
  character_list characterl,
  bonus_list bonusl,
  bomb_list bombl
  )
{
  action a; // action to choose and then return
  bool ok; // boolean to control the do while loops
  
  int x; // runner's x position
  int y; // runner's y position

  character_list pchar=characterl; // iterator on the character list

  // looking for the runner ; we know s.he is in the list
  ok=false; // ok will become true when the runner will be found
  do
  { 
    if(pchar->c.item==RUNNER) // runner found
    {
      x=pchar->c.x; 
      y=pchar->c.y;
      ok=true;
    }
    else // otherwise move on next character
      pchar=pchar->next;
  } while(!ok);

  ok=false; // ok will become true when a valid action will be guessed
  do
  {
    a = rand() % 7; // randomly guess a integer between 0 and 6 as we have 7 possible actions
    switch (a)
    {
    case NONE:
      ok = true; // it's always possible, though often useless, to do nothing ;-)
      break;
    case UP:
      if (level.map[y][x] == LADDER)
        ok = true; // it's possible to go up if on a ladder
      break;
    case DOWN:
      if (level.map[y + 1][x] == LADDER || level.map[y + 1][x] == PATH)
        ok = true; // it's possible to go down if there is a ladder or nothing (jump) below 
      break;
    case LEFT:
      if (level.map[y][x - 1] != WALL && level.map[y][x - 1] != FLOOR)
        ok = true; // it's possible to go left if there is no wall or floor
      break;
    case RIGHT:
      if (level.map[y][x + 1] != WALL && level.map[y][x + 1] != FLOOR)
        ok = true; // it's possible to go right if there is no wall or floor
      break;
    case BOMB_LEFT: 
      if (level.map[y + 1][x - 1] == FLOOR && level.map[y][x - 1] == PATH)
        ok = true; // it's possible to bomb left if there is some floor that can be destroyed
      break;
    case BOMB_RIGHT:
      if (level.map[y + 1][x + 1] == FLOOR && level.map[y][x + 1] == PATH)
        ok = true; // it's possible to bomb right if there is some floor that can be destroyed
      break;
    }
    if(DEBUG) // only when the game is in debug mode
    {
      printf("[Player] Candidate action ");
      print_action(a);
      if(ok) 
        printf(" is valid"); 
      else 
        printf(" not valid");
      printf(".\n");
    }
  } while (!ok);

  return a; // action to perform
}

/*
  Procedure that print the action name based on its enum type value
  Input:
    - the action a to print
*/
void print_action(action a)
{
  switch (a)
  {
  case NONE:
    printf("NONE");
    break;
  case UP:
    printf("UP");
    break;
  case DOWN:
    printf("DOWN");
    break;
  case LEFT:
    printf("LEFT");
    break;
  case RIGHT:
    printf("RIGHT");
    break;
  case BOMB_LEFT:
    printf("BOMB_LEFT");
    break;
  case BOMB_RIGHT:
    printf("BOMB_RIGHT");
    break;
  }
}
