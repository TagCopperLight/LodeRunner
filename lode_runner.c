/*
  Francois Goasoue, Univ Rennes, ENSSAT-IRISA
*/
#include <stdio.h>   // printf
#include <stdbool.h> // bool, true, false
#include <stdlib.h>  // malloc, free, rand, srand
#include <string.h>  // strcpy, strlen
#include <math.h>    // sqrt
#include <unistd.h>  //usleep
#include <time.h>    // time

#include "lode_runner.h" // action, bomberman

// ANSI COLOR SCHEME
#define ANSI_COLOR_RED "\x1b[0;31m"
#define ANSI_COLOR_RED_DIM "\x1b[2;31m"
#define ANSI_COLOR_GREEN "\x1b[0;32m"
#define ANSI_COLOR_GREEN_DIM "\x1b[2;32m"
#define ANSI_COLOR_YELLOW "\x1b[0;33m"
#define ANSI_COLOR_YELLOW_DIM "\x1b[2;33m"
#define ANSI_COLOR_YELLOW_BLINK "\x1b[5;33m"
#define ANSI_COLOR_BLUE "\x1b[0;34m"
#define ANSI_COLOR_BLUE_DIM "\x1b[2;34m"
#define ANSI_COLOR_BLUE_REVERSE "\x1b[7;34m"
#define ANSI_COLOR_MAGENTA "\x1b[0;35m"
#define ANSI_COLOR_MAGENTA_DIM "\x1b[2;35m"
#define ANSI_COLOR_CYAN "\x1b[0;36m"
#define ANSI_COLOR_CYAN_DIM "\x1b[2;36m"
#define ANSI_COLOR_CYAN_REVERSE "\x1b[7;36m"
#define ANSI_COLOR_WHITE "\x1b[0;37m"
#define ANSI_COLOR_WHITE_BLINK "\x1b[5;37m"
#define ANSI_COLOR_WHITE_DIM "\x1b[2;37m"
#define ANSI_COLOR_WHITE_REVERSE "\x1b[7;37m"
#define ANSI_COLOR_RESET "\x1b[0;0m"

// GAME COLOR SCHEME
#define BONUS_COLOR ANSI_COLOR_YELLOW_DIM
#define CABLE_COLOR ANSI_COLOR_WHITE
#define ENEMY_COLOR ANSI_COLOR_RED
#define EXIT_INACTIVE_COLOR ANSI_COLOR_WHITE
#define EXIT_ACTIVE_COLOR ANSI_COLOR_YELLOW_DIM
#define FLOOR_COLOR ANSI_COLOR_CYAN_REVERSE
#define LADDER_COLOR ANSI_COLOR_WHITE
#define PATH_COLOR ANSI_COLOR_RESET
#define RUNNER_COLOR ANSI_COLOR_WHITE
#define WALL_COLOR ANSI_COLOR_BLUE_REVERSE
#define DEFAULT_COLOR ANSI_COLOR_RESET

#define BONUS_ASCII "#"
#define CABLE_ASCII "_"
#define ENEMY_ASCII "&"
#define EXIT_ASCII "="
#define FLOOR_ASCII "*"
#define LADDER_ASCII "="
#define PATH_ASCII " "
#define RUNNER_ASCII "@"
#define WALL_ASCII "+"

#define BONUS_UNICODE "\u25A4"
#define CABLE_UNICODE "\u2581"
#define ENEMY_UNICODE "\u26C7"
#define EXIT_UNICODE "\u2CB6"
#define FLOOR_UNICODE "\u2592"
#define LADDER_UNICODE "\u2CB6"
#define PATH_UNICODE " "
#define RUNNER_UNICODE "\u263A"
#define WALL_UNICODE "\u2593"

const char BOMB = 'H';   // ascii used for the bombs
const char BONUS = 'B';  // ascii used for the bonuses
const char CABLE = 'C';  // ascii used for the cable
const char ENEMY = 'E';  // ascii used for the enemies
const char EXIT = 'X';   // ascii used for the exit
const char FLOOR = 'F';  // ascii used for the floor
const char LADDER = 'L'; // ascii used for the ladder
const char PATH = '.';   // ascii used for the pathes
const char RUNNER = 'R'; // ascii used for runner
const char WALL = 'W';   // ascii used for the walls

const int MAX_MAP_X_SIZE = 1000; // size of the buffer used to read the map file

const int BOMB_TTL = 10;
const int BONUS_SCORE = 1;

bool DEBUG;

extern const char *students; // get student names

typedef struct
{
  bool DEBUG; // game in debug mode if true and in normal mode otherwise
  bool COLOR; // game in vintage colors if true and in B&W otherwise
  bool UNICODE; // game in unicode if true and in ascii otherwise
  int DELAY; // delay between rounds; default speed 100ms
  char *FILENAME; // map file
} gamesettings;

// global variables
gamesettings settings = {false, true, true, 100000, NULL}; // default game settings

levelinfo level; 

character_list cl = NULL;

bonus_list bl = NULL;

bomb_list bombl = NULL;

struct direction_link
{
  action d;
  struct direction_link *next;
};
typedef struct direction_link *direction_list;

struct item_node
{
  char item;
  struct item_node *u;
  struct item_node *d;
  struct item_node *l;
  struct item_node *r;
};
typedef struct item_node item_tree_node;
typedef item_tree_node *item_tree;

int score = 0; // runner score
bool exit_ok = false;

void read_parameters(int, char *[], bool *);
void print_parameters();
void game_credits_printer();
void game_over_printer(unsigned int);
void congratulations_printer();
void map_reader(bool *);
void map_printer(char **);
void game_init(bool *);

// character_list tools
void character_list_insert(char, int, int, action);
character_list character_list_copy();
void character_list_kill(character_list *);
bool is_character_at(char, int, int);
void character_list_printer(character_list);

// bonus_list tools
void bonus_list_insert(char, int, int);
bonus_list bonus_list_copy();
void bonus_list_kill(bonus_list *);
void bonus_list_printer(bonus_list);
bool is_bonus_at(int, int);

// bomb_list tools
void bomb_list_insert(int, int);
bomb_list bomb_list_copy();
void bomb_list_kill(bomb_list *);
void bomb_list_printer();
bool is_bomb_at(int, int);

char **map_clone_decorated();
char **map_copy(char **, int, int);
void map_free(char **, int, int);
void action_printer(action);
void add_characters_to_map(char **);
void add_bonuses_to_map(char **);
void add_bombs_to_map(char **);
void add_exit_to_map(char **);

bool process_move(character *, action);
bool is_runner_alive();
bool is_runner_exited();
bool can_do(character, action);
void update_bomb_list();
void process_bombs();
bomb_list bomb_to_remove();
void remove_from_bomb_list(bomb_list);
void kill_character_at(int, int);
character_list character_to_remove(int, int);
void remove_from_character_list(character_list);
void kill_bonus_at(int, int);
bonus_list bonus_to_remove(int, int);
void remove_from_bonus_list(bonus_list);
action enemy(character);
item_tree tree_of_paths_to_runner(character, character, int, int, direction_list);
action action_to_shortest_path(item_tree);
int shortest_path(item_tree);
int min4(int[4]);
void item_tree_printer(item_tree);
void kill_item_tree(item_tree *);
bool already_been_there(direction_list);
void direction_list_kill(direction_list *); 
void direction_list_printer(direction_list); 
character *get_character_at(int, int);

int main(int argc, char *argv[])
{
  bool validparameters;
  bool maploaded;
  bool validgame;
  char **mapclone = NULL;
  bool dead = false;
  bool exited = false;
  character_list cltmp;
  character_list characterltmp;
  bonus_list bonusltmp;
  bomb_list bombltmp;
  action a = -1;
  levelinfo map_info;
  bool runner_round_only=false;

  unsigned int seed = (unsigned int)time(NULL);
  seed = 1728659117;

  printf("Seed: %u\n", seed);

  srand(seed);

  read_parameters(argc, argv, &validparameters);
  if (validparameters)
  {
    map_reader(&maploaded);
  }

  if (validparameters && maploaded)
  {
    print_parameters();
    game_credits_printer();
    map_printer(level.map);
    game_init(&validgame);

    if (!validgame)
      printf("Invalid game level!\n");
  }

  if (validparameters && maploaded && validgame)
  {
    DEBUG = settings.DEBUG;
    map_info.xexit=level.xexit;
    map_info.yexit=level.yexit;
    map_info.xsize=level.xsize;
    map_info.ysize=level.ysize;
    
    do
    {
      cltmp = cl;
      while (cltmp != NULL && !dead && !exited)
      {
        if ((cltmp->c).item == RUNNER)
        {
          map_info.map = map_copy(level.map, level.xsize, level.ysize);
          characterltmp=character_list_copy();
          bonusltmp=bonus_list_copy();
          bombltmp=bomb_list_copy();
          a = lode_runner(map_info, characterltmp, bl, bombl);
          map_free(map_info.map, level.xsize, level.ysize);
          character_list_kill(&characterltmp);
          bonus_list_kill(&bonusltmp);
          bomb_list_kill(&bombltmp);

          if (settings.DEBUG)
          {
            printf("[Engine] Runner action is ");
            action_printer(a);
            printf("\n");
          }
          dead = !process_move(&(cltmp->c), a);
          exited = (exit_ok && level.xexit == cltmp->c.x && level.yexit == cltmp->c.y);
        }
        else if ((cltmp->c).item == ENEMY && !runner_round_only)
        {
          a = enemy(cltmp->c);
          if (settings.DEBUG)
          {
            printf("[Engine] Enemy action is ");
            action_printer(a);
            printf("\n");
          }
          dead = !process_move(&(cltmp->c), a);
        }
        cltmp->c.d=a;
        cltmp = cltmp->next;
      }
      
      process_bombs();
      runner_round_only=!runner_round_only;

      if (!settings.DEBUG)
        printf("\033[%dA", level.ysize + 1); // Move up X lines;
      
      mapclone = map_clone_decorated();
      map_printer(mapclone);
      map_free(mapclone, level.xsize, level.ysize);
      usleep(settings.DELAY);

      dead = dead || !is_runner_alive();
      if (!dead)
        exited = is_runner_exited();
    } while (!dead && !exited);

    if (dead)
      game_over_printer(seed);
    else
      congratulations_printer();
  }
  return 0;
}

char **map_clone_decorated()
{
  char **mapclone = map_copy(level.map, level.xsize, level.ysize);
  add_characters_to_map(mapclone);
  add_bonuses_to_map(mapclone);
  add_bombs_to_map(mapclone);
  add_exit_to_map(mapclone);
  return mapclone;
}

/*
  Tools for parameters
*/

void read_parameters(int argc, char *argv[], bool *pvalidparameters)
{
  char errormessage[] = "Usage: lode_runner [-debug on/off] [-display color/bw] [-encoding ascii/unicode] [-delay integer] level_file \n\a";
  int i = 1;
  int delay;
  if (argc == 1)
    *pvalidparameters = false;
  else
  {
    *pvalidparameters = true;
    while (i < argc - 2 && *pvalidparameters)
    {
      if (strncmp(argv[i], "-debug", 6) == 0 && argc > i + 1)
      {
        if (strcmp(argv[i + 1], "on") == 0)
        {
          settings.DEBUG = true;
        }
        else if (strcmp(argv[i + 1], "off") == 0)
        {
          settings.DEBUG = false;
        }
        else
          *pvalidparameters = false;
        i = i + 2;
      }
      else if (strncmp(argv[i], "-display", 8) == 0 && argc > i + 1)
      {
        if (strcmp(argv[i + 1], "color") == 0)
        {
          settings.COLOR = true;
        }
        else if (strcmp(argv[i + 1], "bw") == 0)
        {
          settings.COLOR = false;
        }
        else
          *pvalidparameters = false;
        i = i + 2;
      }
      else if (strncmp(argv[i], "-encoding", 9) == 0 && argc > i + 1)
      {
        if (strcmp(argv[i + 1], "ascii") == 0)
        {
          settings.UNICODE = false;
        }
        else if (strcmp(argv[i + 1], "unicode") == 0)
        {
          settings.UNICODE = true;
        }
        else
          *pvalidparameters = false;
        i = i + 2;
      }
      else if (strncmp(argv[i], "-delay", 6) == 0 && argc > i + 1)
      {
        if (sscanf(argv[i + 1], "%d", &delay) == 1)
        {
          settings.DELAY = delay;
        }
        else
          *pvalidparameters = false;
        i = i + 2;
      }
      else
        *pvalidparameters = false;
    }
    if (settings.DEBUG)
      settings.COLOR = false;
    if (i == argc - 1)
      settings.FILENAME = argv[i];
    else
      *pvalidparameters = false;
  }
  if (!*pvalidparameters)
    printf("%s", errormessage);
}

void print_parameters()
{
  printf("[debug=");
  if (settings.DEBUG)
    printf("on");
  else
    printf("off");
  printf(", display=");
  if (settings.COLOR)
    printf("color");
  else
    printf("bw");
  printf(", encoding=");
  if (settings.UNICODE)
    printf("unicode");
  else
    printf("ascii");
  printf(", delay=%d", settings.DELAY);
  printf(", filename=%s", settings.FILENAME);
  printf("]\n");
}

/*
  Tools for map
 */

void map_reader(bool *pmaploaded)
{
  FILE *pf;
  char buffer[MAX_MAP_X_SIZE];

  level.map = NULL;
  level.ysize = 0;
  *pmaploaded = true;
  pf = fopen(settings.FILENAME, "r");
  if (pf == NULL)
  {
    printf("Error: file %s not found!\a\n", settings.FILENAME);
    *pmaploaded = false;
  }
  else if (fscanf(pf, "%s", buffer) == 0)
  {
    *pmaploaded = false;
    fclose(pf);
  }
  else
  {
    level.xsize = strlen(buffer);
    while (*pmaploaded && !feof(pf))
    {
      if (level.xsize == strlen(buffer))
      {
        level.ysize = level.ysize + 1;
        level.map = realloc(level.map, level.ysize * sizeof(char *));
        level.map[level.ysize - 1] = malloc((level.xsize + 1) * sizeof(char));
        strcpy(level.map[level.ysize - 1], buffer);
        if (fscanf(pf, "%s", buffer) == 0)
        {
          *pmaploaded = false;
        }
      }
      else
      {
        *pmaploaded = false;
        printf("Error: %s is not well-formed (rows do not have same size)\a\n", settings.FILENAME);
      }
    }
    fclose(pf);
  }
}

void map_printer(char **map)
{
  int l;
  int c;

  if (settings.COLOR)
    printf(ANSI_COLOR_WHITE);
  printf("SCORE: %2d", score);
  if(settings.DEBUG && exit_ok) printf(" (exit is active)");
  if (settings.COLOR)
    printf(ANSI_COLOR_RESET);
  printf("\n");

  for (l = 0; l < level.ysize; l++)
  {
    for (c = 0; c < level.xsize; c++)
    {
      if (map[l][c] == BONUS)
      {
        if (settings.COLOR)
          printf(BONUS_COLOR);
        if (settings.UNICODE)
          printf(BONUS_UNICODE ANSI_COLOR_RESET);
        else
          printf(BONUS_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == CABLE)
      {
        if (settings.COLOR)
          printf(CABLE_COLOR);
        if (settings.UNICODE)
          printf(CABLE_UNICODE ANSI_COLOR_RESET);
        else
          printf(CABLE_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == ENEMY)
      {
        if (settings.COLOR)
          printf(ENEMY_COLOR);
        if (settings.UNICODE)
          printf(ENEMY_UNICODE ANSI_COLOR_RESET);
        else
          printf(ENEMY_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == EXIT)
      {
        if (settings.COLOR && !exit_ok)
          printf(EXIT_INACTIVE_COLOR);
        else if (settings.COLOR && exit_ok)
          printf(EXIT_ACTIVE_COLOR);
        if (settings.UNICODE)
          printf(EXIT_UNICODE ANSI_COLOR_RESET);
        else
          printf(EXIT_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == FLOOR)
      {
        if (settings.COLOR)
          printf(FLOOR_COLOR);
        if (settings.UNICODE)
          printf(FLOOR_UNICODE ANSI_COLOR_RESET);
        else
          printf(FLOOR_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == LADDER)
      {
        if (settings.COLOR)
          printf(LADDER_COLOR);
        if (settings.UNICODE)
          printf(LADDER_UNICODE ANSI_COLOR_RESET);
        else
          printf(LADDER_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == PATH || map[l][c] == BOMB)
      {
        if (settings.COLOR)
          printf(PATH_COLOR);
        if (settings.UNICODE)
          printf(PATH_UNICODE ANSI_COLOR_RESET);
        else
          printf(PATH_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == RUNNER)
      {
        if (settings.COLOR)
          printf(RUNNER_COLOR);
        if (settings.UNICODE)
          printf(RUNNER_UNICODE ANSI_COLOR_RESET);
        else
          printf(RUNNER_ASCII ANSI_COLOR_RESET);
      }
      else if (map[l][c] == WALL)
      {
        if (settings.COLOR)
          printf(WALL_COLOR);
        if (settings.UNICODE)
          printf(WALL_UNICODE ANSI_COLOR_RESET);
        else
          printf(WALL_ASCII ANSI_COLOR_RESET);
      }
      else
        printf("%c", map[l][c]);
    }
    printf("\n");
  }
}

/*
  Game initialization
*/

void game_init(bool *pvalidgame)
{
  int l;
  int c;
  character_list ptmp;
  character_list pm;

  *pvalidgame = true;

  // looking for characters in the map
  for (l = 0; l < level.ysize; l++)
  {
    for (c = 0; c < level.xsize; c++)
    {
      if (level.map[l][c] == RUNNER)
      {
        character_list_insert(RUNNER, c, l, -1);
        level.map[l][c] = PATH;
        // if(settings.DEBUG) printf("%c found at x=%d, y=%d\n",RUNNER,c,l);
      }
      else if (level.map[l][c] == ENEMY)
      {
        character_list_insert(ENEMY, c, l, -1);
        level.map[l][c] = PATH;
        // if(settings.DEBUG) printf("%c found at x=%d, y=%d\n",ENEMY,c,l);
      }
      else if (level.map[l][c] == EXIT)
      {
        level.xexit = c;
        level.yexit = l;
        // if(settings.DEBUG) printf("%c found at x=%d, y=%d\n",EXIT,c,l);
      }
      else if (level.map[l][c] == BONUS)
      {
        bonus_list_insert(BONUS, c, l);
        level.map[l][c] = PATH;
        // if(settings.DEBUG) printf("%c found at x=%d, y=%d\n",BONUS,c,l);
      }
    }
  }

  // put RUNNER in 1st position in the character list
  ptmp = cl;
  pm = cl;

  if (ptmp == NULL)
  {
    *pvalidgame = false;
  }
  else if ((ptmp->c).item != RUNNER)
  {
    while (ptmp->next != NULL && (ptmp->next)->c.item != RUNNER)
    {
      ptmp = ptmp->next;
    }
    if (ptmp->next == NULL)
    {
      *pvalidgame = false;
    }
    else
    {
      pm = ptmp->next;
      ptmp->next = pm->next;
      pm->next = cl;
      cl = pm;
    }
  }

  // if(settings.DEBUG) character_list_printer(cl);

  // if(settings.DEBUG) printf("Exit set at x=%d and y=%d\n",level.xexit,level.yexit);

  /* if(settings.DEBUG) { */
  /*   printf("Bonuses: "); */
  /*   bonus_list_printer(bl); */
  /*   printf("\n"); */
  /* } */
}

void character_list_insert(char item, int x, int y, action d)
{
  character c;
  character_list l;
  c.item = item;
  c.x = x;
  c.y = y;
  c.d = d;

  l = malloc(sizeof(struct character_link));
  l->c = c;
  l->next = cl;
  cl = l;

  // if(settings.DEBUG) character_list_printer(cl);
}

character_list character_list_copy()
{
  character_list pctmp=cl;
  character_list pccopy;
  character_list newpcl=NULL;
  character_list newpclend=NULL;

  while(pctmp!=NULL)
  {
    pccopy=malloc(sizeof(struct character_link));
    pccopy->c=pctmp->c;
    pccopy->next=NULL;
    if(newpcl==NULL) 
      newpcl=pccopy;
    else 
      newpclend->next=pccopy;
    newpclend=pccopy;      
    pctmp=pctmp->next;
  }

  return newpcl;
}

void character_list_kill(character_list * pcl)
{
  character_list ptmp;
  while(*pcl!=NULL)
  {
    ptmp=*pcl;
    *pcl=(*pcl)->next;
    free(ptmp);
  }
}

bool is_character_at(char item, int x, int y)
{
  character_list pc = cl;
  bool ok = false;
  while (pc != NULL && !ok)
  {
    if (pc->c.item == item && pc->c.x == x && pc->c.y == y)
      ok = true;
    pc = pc->next;
  }
  return ok;
}

void character_list_printer(character_list l)
{
  printf("->");
  while (l != NULL)
  {
    printf("(%c,%d,%d,%d)->", (l->c).item, (l->c).x, (l->c).y, (l->c).d);
    l = l->next;
  }
  printf("\n");
}

void bonus_list_insert(char item, int x, int y)
{
  bonus b;
  bonus_list l;
  b.x = x;
  b.y = y;

  l = malloc(sizeof(struct bonus_link));
  l->b = b;
  l->next = bl;
  bl = l;
}

bonus_list bonus_list_copy()
{
  bonus_list bltmp=bl;
  bonus_list newbl=NULL;
  bonus_list newbltmp=NULL;

  while(bltmp!=NULL)
  {
    newbltmp=malloc(sizeof(struct bonus_link));
    newbltmp->b=bltmp->b;
    newbltmp->next=newbl;
    newbl=newbltmp;
    bltmp=bltmp->next;
  }
  return newbl;
}

void bonus_list_kill(bonus_list * pbl)
{
  bonus_list ptmp;
  while(*pbl!=NULL)
  {
    ptmp=*pbl;
    *pbl=(*pbl)->next;
    free(ptmp);
  }
}

void bonus_list_printer(bonus_list l)
{
  printf("->");
  while (l != NULL)
  {
    printf("(%d,%d)->", (l->b).x, (l->b).y);
    l = l->next;
  }
  printf("\n");
}

bool is_bonus_at(int x, int y)
{
  bool found = false;
  bonus_list l = bl;
  while (l != NULL && !found)
  {
    if ((l->b).x == x && (l->b).y == y)
      found = true;
    l = l->next;
  }
  return found;
}

bool is_bomb_at(int x, int y)
{
  bool found = false;
  bomb_list l = bombl;
  while (l != NULL && !found)
  {
    if (l->x == x && l->y == y)
      found = true;
    l = l->next;
  }
  return found;
}


char **map_copy(char **map, int xsize, int ysize)
{
  int l, c;
  char **copy = malloc(ysize * sizeof(char *));

  for (l = 0; l < ysize; l++)
  {
    copy[l] = malloc(xsize * sizeof(char));
    for (c = 0; c < xsize; c++)
    {
      copy[l][c] = map[l][c];
    }
  }
  return copy;
}

void map_free(char **map, int xsize, int ysize)
{
  int l;

  for (l = 0; l < ysize; l++)
  {
    free(map[l]);
  }
  free(map);
}

void game_credits_printer()
{
  if (settings.COLOR)
    printf(ANSI_COLOR_BLUE);
  printf("    __              __        ____                             \n");
  printf("   / /   ____  ____/ /__     / __ \\__  ______  ____  ___  _____\n");
  printf("  / /   / __ \\/ __  / _ \\   / /_/ / / / / __ \\/ __ \\/ _ \\/ ___/\n");
  printf(" / /___/ /_/ / /_/ /  __/  / _, _/ /_/ / / / / / / /  __/ /    \n");
  printf("/_____/\\____/\\__,_/\\___/  /_/ |_|\\__,_/_/ /_/_/ /_/\\___/_/     \n");
  printf("version 3.14159265359...\n");
  printf("by Pr Francois Goasdoue, Shaman team, ENSSAT-IRISA, Univ Rennes\n");

  if (settings.COLOR)
    printf(ANSI_COLOR_WHITE);
  printf("Artificial Intelligence by ");
  if (settings.COLOR)
    printf(ANSI_COLOR_WHITE);
  printf("%s\n\n", students);

  if (settings.COLOR)
    printf(ANSI_COLOR_RESET);
}

void game_over_printer(unsigned int seed)
{
  if (settings.COLOR)
    printf(ANSI_COLOR_RED);
  printf(" _____                        _____                \n");
  printf("|  __ \\                      |  _  |               \n");
  printf("| |  \\/ __ _ _ __ ___   ___  | | | |_   _____ _ __ \n");
  printf("| | __ / _` | '_ ` _ \\ / _ \\ | | | \\ \\ / / _ \\ '__|\n");
  printf("| |_\\ \\ (_| | | | | | |  __/ \\ \\_/ /\\ V /  __/ |   \n");
  printf(" \\____/\\__,_|_| |_| |_|\\___|  \\___/  \\_/ \\___|_|   \n");
  if (settings.COLOR)
    printf(ANSI_COLOR_RESET);
  printf("Seed: %u\n", seed);
}

void congratulations_printer()
{
  if (settings.COLOR)
    printf(ANSI_COLOR_GREEN);
  printf(" _____                             _         _       _   _                 _ \n");
  printf("/  __ \\                           | |       | |     | | (_)               | |\n");
  printf("| /  \\/ ___  _ __   __ _ _ __ __ _| |_ _   _| | __ _| |_ _  ___  _ __  ___| |\n");
  printf("| |    / _ \\| '_ \\ / _` | '__/ _` | __| | | | |/ _` | __| |/ _ \\| '_ \\/ __| |\n");
  printf("| \\__/\\ (_) | | | | (_| | | | (_| | |_| |_| | | (_| | |_| | (_) | | | \\__ \\_|\n");
  printf(" \\____/\\___/|_| |_|\\__, |_|  \\__,_|\\__|\\__,_|_|\\__,_|\\__|_|\\___/|_| |_|___(_)\n");
  printf("                    __/ |                                                    \n");
  printf("                   |___/                                                     \n");
  if (settings.COLOR)
    printf(ANSI_COLOR_RESET);
}

void action_printer(action a)
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

bool can_do(character c, action a)
{
  bool cando = false;

  switch (a)
  {
  case NONE:
    cando = true;
    break;
  case UP:
    if (level.map[c.y][c.x] == LADDER)
      cando = true;
    break;
  case DOWN:
    if (level.map[c.y + 1][c.x] == LADDER || level.map[c.y + 1][c.x] == PATH)
      cando = true;
    break;
  case LEFT:
    if (level.map[c.y][c.x - 1] != WALL && level.map[c.y][c.x - 1] != FLOOR)
      cando = true;
    break;
  case RIGHT:
    if (level.map[c.y][c.x + 1] != WALL && level.map[c.y][c.x + 1] != FLOOR)
      cando = true;
    break;
  case BOMB_LEFT:
    if (level.map[c.y + 1][c.x - 1] == FLOOR && level.map[c.y][c.x - 1] == PATH)
      cando = true;
    break;
  case BOMB_RIGHT:
    if (level.map[c.y + 1][c.x + 1] == FLOOR && level.map[c.y][c.x + 1] == PATH)
      cando = true;
    break;
  }

  return cando;
}

void add_characters_to_map(char **map)
{
  character_list l = cl;
  while (l != NULL)
  {
    map[(l->c).y][(l->c).x] = (l->c).item;
    l = l->next;
  }
}

void add_bonuses_to_map(char **map)
{
  bonus_list l = bl;
  while (l != NULL)
  {
    if (map[(l->b).y][(l->b).x] == PATH)
      map[(l->b).y][(l->b).x] = BONUS;
    l = l->next;
  }
}

void add_bombs_to_map(char **map)
{
  bomb_list l = bombl;
  while (l != NULL)
  {
    if (map[l->y][l->x] == FLOOR)
      map[l->y][l->x] = BOMB;
    l = l->next;
  }
}

void add_exit_to_map(char **map)
{
  if (map[level.yexit][level.xexit] == LADDER && exit_ok)
    map[level.yexit][level.xexit] = EXIT;
}

bool process_move(character *pc, action a)
{
  bool ok = false;
  char **mapclone = NULL;
  if (pc->item == RUNNER)
  {
    switch (a)
    {
    case NONE:
      ok = true;
      break;
    case UP:
      if (can_do(*pc, UP))
      {
        if (!is_character_at(ENEMY, pc->x, pc->y - 1))
        {
          ok = true;
          pc->y--;
          if (is_bonus_at(pc->x, pc->y))
          {
            score += BONUS_SCORE;
            kill_bonus_at(pc->x, pc->y);
          }
        }
      }
      break;
    case DOWN:
      if (can_do(*pc, DOWN))
      {
        pc->y++;
        while (level.map[pc->y - 1][pc->x] != CABLE && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
        {
          if (!settings.DEBUG)
            printf("\033[%dA", level.ysize + 1); // Move up X lines;
          mapclone = map_clone_decorated();
          map_printer(mapclone);
          map_free(mapclone, level.xsize, level.ysize);
          usleep(settings.DELAY);
          pc->y++;
          if (is_bonus_at(pc->x, pc->y))
          {
            score += BONUS_SCORE;
            kill_bonus_at(pc->x, pc->y);
          }
        }
        if (!is_character_at(ENEMY, pc->x, pc->y) && !is_bomb_at(pc->x, pc->y))
          ok = true;
      }
      break;
    case LEFT:
      if (can_do(*pc, LEFT))
      {
        pc->x--;
        if (is_bonus_at(pc->x, pc->y))
        {
          score += BONUS_SCORE;
          kill_bonus_at(pc->x, pc->y);
        }
        while (level.map[pc->y - 1][pc->x] != CABLE && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
        {
          pc->y++;
          if (is_bonus_at(pc->x, pc->y))
          {
            score += BONUS_SCORE;
            kill_bonus_at(pc->x, pc->y);
          }
          if (!settings.DEBUG)
            printf("\033[%dA", level.ysize + 1); // Move up X lines;
          mapclone = map_clone_decorated();
          map_printer(mapclone);
          map_free(mapclone, level.xsize, level.ysize);
          usleep(settings.DELAY);
        }
        if (!is_character_at(ENEMY, pc->x, pc->y) && !is_bomb_at(pc->x, pc->y))
          ok = true;
      }
      break;
    case RIGHT:
      if (can_do(*pc, RIGHT))
      {
        pc->x++;
        if (is_bonus_at(pc->x, pc->y))
        {
          score += BONUS_SCORE;
          kill_bonus_at(pc->x, pc->y);
        }
        while (level.map[pc->y - 1][pc->x] != CABLE && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
        {
          pc->y++;
          if (is_bonus_at(pc->x, pc->y))
          {
            score += BONUS_SCORE;
            kill_bonus_at(pc->x, pc->y);
          }
          if (!settings.DEBUG)
            printf("\033[%dA", level.ysize + 1); // Move up X lines;
          mapclone = map_clone_decorated();
          map_printer(mapclone);
          map_free(mapclone, level.xsize, level.ysize);
          usleep(settings.DELAY);
        }
        if (!is_character_at(ENEMY, pc->x, pc->y) && !is_bomb_at(pc->x, pc->y))
          ok = true;
      }
      break;
    case BOMB_LEFT:
      if (can_do(*pc, BOMB_LEFT))
      {
        bomb_list_insert(pc->x - 1, pc->y + 1);
        ok = true;
      }
      break;
    case BOMB_RIGHT:
      if (can_do(*pc, BOMB_RIGHT))
      {
        bomb_list_insert(pc->x + 1, pc->y + 1);
        ok = true;
      }
      break;
    }
    if (bl == NULL)
      exit_ok = true;
  }
  else // enemy
  {
    ok = true;
    switch (a)
    {
    case UP:
      pc->y--;
      if (is_character_at(RUNNER, pc->x, pc->y))
        ok = false;
      break;
    case DOWN:
      pc->y++;
      if (is_character_at(RUNNER, pc->x, pc->y))
        ok = false;
      while (level.map[pc->y - 1][pc->x] != CABLE && !is_bomb_at(pc->x, pc->y) && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
      {
        if (!settings.DEBUG)
          printf("\033[%dA", level.ysize + 1); // Move up X lines;
        mapclone = map_clone_decorated();
        map_printer(mapclone);
        map_free(mapclone, level.xsize, level.ysize);
        usleep(settings.DELAY);
        pc->y++;
        if (is_character_at(RUNNER, pc->x, pc->y))
          ok = false;
      }
      break;
    case LEFT:
      pc->x--;
      if (is_character_at(RUNNER, pc->x, pc->y))
        ok = false;
      while (level.map[pc->y - 1][pc->x] != CABLE && !is_bomb_at(pc->x, pc->y) && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
      {
        if (!settings.DEBUG)
          printf("\033[%dA", level.ysize + 1); // Move up X lines;
        mapclone = map_clone_decorated();
        map_printer(mapclone);
        map_free(mapclone, level.xsize, level.ysize);
        usleep(settings.DELAY);
        pc->y++;
        if (is_character_at(RUNNER, pc->x, pc->y))
          ok = false;
      }
      break;
    case RIGHT:
      pc->x++;
      if (is_character_at(RUNNER, pc->x, pc->y))
        ok = false;
      while (level.map[pc->y - 1][pc->x] != CABLE && !is_bomb_at(pc->x, pc->y) && (level.map[pc->y + 1][pc->x] == PATH || level.map[pc->y + 1][pc->x] == CABLE || (is_bomb_at(pc->x, pc->y + 1) && !is_character_at(ENEMY, pc->x, pc->y + 1))))
      {
        if (!settings.DEBUG)
          printf("\033[%dA", level.ysize + 1); // Move up X lines;
        mapclone = map_clone_decorated();
        map_printer(mapclone);
        map_free(mapclone, level.xsize, level.ysize);
        usleep(settings.DELAY);
        pc->y++;
        if (is_character_at(RUNNER, pc->x, pc->y))
          ok = false;
      }
      break;
    default:
      break;
    }
  }
  return ok;
}

void kill_bonus_at(int x, int y)
{
  bonus_list pb;

  pb = bonus_to_remove(x, y);
  if (pb != NULL)
  {
    remove_from_bonus_list(pb);
  }
}

bonus_list bonus_to_remove(int x, int y)
{
  bonus_list pb = bl;

  while (pb != NULL && (pb->b.x != x || pb->b.y != y))
    pb = pb->next;
  return pb;
}

void remove_from_bonus_list(bonus_list pb)
{
  bonus_list l = bl;
  if (bl == pb)
  {
    bl = bl->next;
    free(pb);
  }
  else
  {
    while (l->next != pb)
      l = l->next;
    l->next = pb->next;
    free(pb);
  }
}

void bomb_list_insert(int x, int y)
{
  bomb_list pbtmp = malloc(sizeof(struct bomb_link));
  pbtmp->delay = BOMB_TTL;
  pbtmp->x = x;
  pbtmp->y = y;
  pbtmp->next = bombl;
  bombl = pbtmp;
}

void bomb_list_kill(bomb_list * pbl) 
{
  bomb_list bltmp;
  while(*pbl!=NULL)
  {
    bltmp=*pbl;
    *pbl=(*pbl)->next;
    free(bltmp);
  }
}

void process_bombs()
{
  bomb_list pb = bombl;
  character *pc;
  int x;
  int y;

  while (pb != NULL)
  {
    pb->delay--;
    if (pb->delay == 0)
    {
      pc = get_character_at(pb->x, pb->y);
      if (pc != NULL)
      {
        do
        {
          x = (rand() % (level.xsize-2))+1;
          y = (rand() % (level.ysize-2))+1;
        } while (!(level.map[y][x] == PATH && level.map[y+1][x] == FLOOR && !is_character_at(RUNNER, x, y) && !is_character_at(RUNNER, x-1, y) && !is_character_at(RUNNER, x+1, y) && !is_character_at(ENEMY, x, y) && !is_bonus_at(x,y)));
        pc->x = x;
        pc->y = y;
      }
    }
    pb = pb->next;
  }
  update_bomb_list();
}

character *get_character_at(int x, int y)
{
  character_list pc = cl;
  bool found = false;
  while (pc != NULL && !found)
  {
    if (pc->c.x == x && pc->c.y == y)
      found = true;
    else
      pc = pc->next;
  }
  if (found)
    return &(pc->c);
  else
    return NULL;
}

void kill_character_at(int x, int y)
{
  character_list pc;

  do
  {
    pc = character_to_remove(x, y);
    if (pc != NULL)
      remove_from_character_list(pc);
  } while (pc != NULL);
}

character_list character_to_remove(int x, int y)
{
  character_list pc = cl;

  while (pc != NULL && (pc->c.x != x || pc->c.y != y))
    pc = pc->next;
  return pc;
}

void remove_from_character_list(character_list pc)
{
  character_list l = cl;
  if (cl == pc)
  {
    cl = cl->next;
    free(pc);
  }
  else
  {
    while (l->next != pc)
      l = l->next;
    l->next = pc->next;
    free(pc);
  }
}

void update_bomb_list()
{
  bomb_list pb;
  // if(settings.DEBUG) bomb_list_printer();
  do
  {
    pb = bomb_to_remove();
    if (pb != NULL)
      remove_from_bomb_list(pb);
  } while (pb != NULL);
}

bomb_list bomb_to_remove()
{
  bomb_list pb = bombl;

  while (pb != NULL && pb->delay != 0)
    pb = pb->next;
  return pb;
}

void remove_from_bomb_list(bomb_list pb)
{
  bomb_list l = bombl;
  if (l == pb)
  {
    bombl = bombl->next;
    free(pb);
  }
  else
  {
    while (l->next != pb)
      l = l->next;
    l->next = pb->next;
    free(pb);
  }
}



bomb_list bomb_list_copy()
{
  bomb_list bltmp=bombl;
  bomb_list newbl=NULL;
  bomb_list newbltmp=NULL;

  while(bltmp!=NULL)
  {
    newbltmp=malloc(sizeof(struct bomb_link));
    newbltmp->x=bltmp->x;
    newbltmp->y=bltmp->y;
    newbltmp->delay=bltmp->delay;
    newbltmp->next=newbl;
    newbl=newbltmp;
    bltmp=bltmp->next;
  }
  return newbl;
}

void bomb_list_printer()
{
  bomb_list l = bombl;

  printf("Bomb list bombl->");
  while (l != NULL)
  {
    printf("%d@(x=%d,y=%d)->", l->delay, l->x, l->y);
    l = l->next;
  }
  printf("\n");
}

bool is_runner_exited()
{
  bool exit = false;
  if (exit_ok && cl->c.x == level.xexit && cl->c.y == level.yexit)
    exit = true;
  return exit;
}

bool is_runner_alive()
{
  character_list l;
  bool alive = true;

  if (cl == NULL)
    alive = false;
  else if ((cl->c).item == RUNNER)
  {
    l = cl->next;
    while (l != NULL && alive)
    {
      if ((l->c).x == (cl->c).x && (l->c).y == (cl->c).y)
        alive = false;
      l = l->next;
    }
  }
  else
    alive = false;
  return alive;
}

action enemy(character e)
{
  item_tree t;
  character_list ptmp = cl;
  action a;
  bool found = false;

  if (is_bomb_at(e.x, e.y))
    a = NONE;
  else
  {
    while (ptmp != NULL && !found)
    {
      if ((ptmp->c).item == RUNNER)
        found = true;
    }
    if (found)
    {
      t = tree_of_paths_to_runner(e, ptmp->c, e.x, e.y, NULL);
      a = action_to_shortest_path(t);
      if((a==UP && is_character_at(ENEMY,e.x,e.y-1)) || (a==DOWN && is_character_at(ENEMY,e.x,e.y+1)) || (a==LEFT && is_character_at(ENEMY,e.x-1,e.y)) || (a==RIGHT && is_character_at(ENEMY,e.x+1,e.y))) a=NONE;
      kill_item_tree(&t);
    }
    else
      a = NONE;
  }
  return a;
}

item_tree tree_of_paths_to_runner(character e, character r, int x, int y, direction_list l)
{
  item_tree t = NULL;
  item_tree tu = NULL;
  item_tree td = NULL;
  item_tree tl = NULL;
  item_tree tr = NULL;
  direction_list ltmp;

  if (already_been_there(l) || level.map[y][x] == WALL || level.map[y][x] == FLOOR)
  {
    return NULL;
  }
  else if (x == r.x && y == r.y)
  {
    t = malloc(sizeof(item_tree_node));
    t->item = RUNNER;
    t->u = NULL;
    t->d = NULL;
    t->l = NULL;
    t->r = NULL;
    return t;
  }
  else
  {
    ltmp = malloc(sizeof(struct direction_link));
    ltmp->next = l;
    if (level.map[y][x] == LADDER)
    {
      ltmp->d = UP;
      tu = tree_of_paths_to_runner(e, r, x, y - 1, ltmp);
    }
    if (level.map[y + 1][x] == LADDER)
    {
      ltmp->d = DOWN;
      td = tree_of_paths_to_runner(e, r, x, y + 1, ltmp);
    }
    if (level.map[y - 1][x] == CABLE || level.map[y + 1][x] == FLOOR || level.map[y + 1][x] == LADDER || (is_bomb_at(x, y + 1) && is_character_at(ENEMY, x, y + 1)))
    {
      ltmp->d = LEFT;
      tl = tree_of_paths_to_runner(e, r, x - 1, y, ltmp);
    //}
    //if (level.map[y - 1][x] == CABLE || level.map[y + 1][x] == FLOOR || level.map[y + 1][x] == LADDER || (is_bomb_at(x, y + 1) && is_character_at(ENEMY, x, y + 1)))
    //{
      ltmp->d = RIGHT;
      tr = tree_of_paths_to_runner(e, r, x + 1, y, ltmp);
    }
    free(ltmp);
    if (tu != NULL || td != NULL || tl != NULL || tr != NULL)
    {
      t = malloc(sizeof(item_tree_node));
      t->item = level.map[y][x];
      t->u = tu;
      t->d = td;
      t->l = tl;
      t->r = tr;
    }
    return t;
  }
}

bool already_been_there(direction_list l)
{
  int x = 0;
  int y = 0;
  bool cycle = false;
  while (l != NULL && !cycle)
  {
    if (l->d == UP)
      y--;
    else if (l->d == DOWN)
      y++;
    else if (l->d == LEFT)
      x--;
    else if (l->d == RIGHT)
      x++;
    if (x == 0 && y == 0)
      cycle = true;
    else
      l = l->next;
  }
  return cycle;
}

action action_to_shortest_path(item_tree t)
{
  int l[4];
  int min;
  action a;
  if (t == NULL)
    a = NONE;
  else
  {
    l[0] = shortest_path(t->u);
    l[1] = shortest_path(t->d);
    l[2] = shortest_path(t->l);
    l[3] = shortest_path(t->r);
    min = min4(l);

    /* 
    if (settings.DEBUG)
    {
      printf("Min distance is : %d\n", l[min]);
    }
    */
   
    if (min == 0)
      a = UP;
    else if (min == 1)
      a = DOWN;
    else if (min == 2)
      a = LEFT;
    else
      a = RIGHT;
  }
  return a;
}

int shortest_path(item_tree t)
{
  int l[4];

  if (t == NULL)
    return 0;
  else
  {
    l[0] = shortest_path(t->u);
    l[1] = shortest_path(t->d);
    l[2] = shortest_path(t->l);
    l[3] = shortest_path(t->r);
    return 1 + l[min4(l)];
  }
}

int min4(int t[4])
{
  int i;
  int min = 0;
  // if(settings.DEBUG) printf("Min between: %d ", t[0]);
  for (i = 1; i < 4; i++)
  {
    // if(settings.DEBUG) printf("%d ", t[i]);
    if ((t[i] < t[min] && t[i] > 0) || t[min] == 0)
      min = i;
  }
  // if(settings.DEBUG) printf("\nMin is: %d\n", t[min]);
  return min;
}

void item_tree_printer(item_tree t)
{
  if (t == NULL)
    printf("0");
  else
  {
    printf("<%c,", t->item);
    item_tree_printer(t->u);
    printf(",");
    item_tree_printer(t->d);
    printf(",");
    item_tree_printer(t->l);
    printf(",");
    item_tree_printer(t->r);
    printf(">");
  }
}

void kill_item_tree(item_tree *pt)
{
  if (*pt != NULL)
  {
    kill_item_tree(&((*pt)->u));
    kill_item_tree(&((*pt)->d));
    kill_item_tree(&((*pt)->l));
    kill_item_tree(&((*pt)->r));
    free(*pt);
    *pt = NULL;
  }
}

void direction_list_kill(direction_list * pl) 
{
  direction_list dl;
  while(*pl!=NULL)
  {
    dl=*pl;
    *pl=(*pl)->next;
    free(dl);
  }
}

void direction_list_printer(direction_list l) 
{
  printf("->");
  while(l!=NULL)
  {
    action_printer(l->d);
    printf("->");
    l=l->next;
  }
}