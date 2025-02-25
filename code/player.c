#include <stdbool.h> // bool, true, false
#include <stdlib.h>  // rand
#include <stdio.h>   // printf
#include <math.h>    // sqrt
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
char DEAD = 'D';          // ascii used for the dead runner

extern const int BOMB_TTL; // time to live for bombs

extern bool DEBUG; // true if and only if the game runs in debug mode

const char *students = "BOURDET"; // replace Random with the student names here

void debug(char* s){
  if(DEBUG){
    printf("%s", s);
  }
}

typedef struct min_heap{
  int size; // Nombre d'elements dans le tas
  int capacity; // Taille max du tas
  int* array; // Tableau des elements, on n'a pas besoin d'utiliser une structure plus complique car on utilise un arbre binaire parfait, le pere de i est (i-1)/2
  float* priority; // Prioritee des elements
} min_heap;

typedef struct path{ // Structure qui contient les informations que renvoie A*
  min_heap* heap;
  int* p;
  int* d;
  bool found;
} path;

typedef struct child{ // Structure utilisee pour remonter le chemin trouve par A*
  int pos;
  float distance;
  struct child* parent;
} child;

// Local prototypes (add your own prototypes below)
// Tas-min, afin de pouvoir faire de la recherche de chemin intelligente, https://fr.wikipedia.org/wiki/Tas_binaire
min_heap* create_min_heap(int);
void free_min_heap(min_heap*);
void swap(int*, int*); // Echange les valeurs des deux entiers
void swapf(float*, float*); // Echange les valeurs de deux flottants
void percolate_up(min_heap*, int); // Conserve notre invariance, le min de priorite est dans la racine de l'arbre, utilisee dans un cas d'insertion
void percolate_down(min_heap*, int); // Conserve l'invariant, mais est utilisee en cas de suppression d'un element
void insert(min_heap*, int, float);
void modify_priority(min_heap*, int, float); // key est la valeur dans array de l'element a modifier
int extract_min(min_heap*); // Renvoie l'element de priorite minimale est le retire de la file
bool is_member(min_heap*, int); 

// Outils
float dist(int, int, int, int); // Calcule la distance euclidienne entre deux points (avec leurs coordonnees)
float vdist(int, int, levelinfo); // Calcule la distance euclidienne entre deux points (avec leurs positions dans le level)
character_list get_runner(character_list); // Renvoie le runner parmi les characters
bool is_in_bonus_list(bonus_list, bonus_list); // Verifie si un bonus est dans une liste de bonus, on utilise ses coordonnees
levelinfo add_enemies(levelinfo, character_list, bomb_list); // Ajoute les ennemis et les bombes a la map
bonus_list get_closest_bonus(bonus_list, character_list, bonus_list); // Renvoie le bonus le plus proche du runner, en evitant ceux deja vus
character_list get_closest_enemy(character_list, character_list, levelinfo); // Renvoie l'ennemi le plus proche du runner, sur la meme ligne
bool is_valid_closest(int, action, levelinfo); // Verifie si une action est dangereuse, utilisée pour le mode closest
void special_moves(character_list, character_list, int*, levelinfo); // Gere toutes les actions qui sont déterminées par des conditions spéciales (ennemis, ...)

// A*
path* create_path(int, levelinfo);
void free_path(path*);
bool is_valid(int, action, levelinfo, levelinfo); // Verifie si une action est valide
int weight(action); // Valeur d'une action, utilise pour le calcul de la priorite
int get_new_pos(int, action, levelinfo); // Renvoie la position apres avoir effectue une action
action get_action(int, int, levelinfo); // Renvoie l'action a effectuer pour aller de u a v, si ce n'est pas possible, renvoie NONE
path* a_star(character_list, bonus_list, levelinfo, levelinfo); // Algorithme de recherche de chemin, renvoie le chemin le plus court entre le runner et le bonus
child* find_closest_child(int*, int, int, levelinfo); // Si A* ne trouve pas de chemin, on cherche le chemin qui nous rapproche le plus du bonus

action lode_runner(levelinfo, character_list, bonus_list, bomb_list);


min_heap* create_min_heap(int capacity){
  min_heap* heap = malloc(sizeof(min_heap));
  heap->size = 0;
  heap->capacity = capacity;
  heap->array = malloc(capacity * sizeof(int));
  heap->priority = malloc(capacity * sizeof(int));

  return heap;
}

void free_min_heap(min_heap* heap){
  free(heap->array);
  free(heap->priority);
  free(heap);
}

path* create_path(int heap_capacity, levelinfo level){
  path* pat = malloc(sizeof(path));

  pat->heap = create_min_heap(heap_capacity);
  pat->d = malloc(level.xsize * level.ysize * sizeof(int));
  pat->p = malloc(level.xsize * level.ysize * sizeof(int));
  pat->found = false;

  for(int i = 0; i < level.xsize * level.ysize; i++){
    pat->d[i] = 1000000;
    pat->p[i] = -1;
  }

  return pat;
}

void free_path(path* pat){
  free(pat->d);
  free(pat->p);
  free_min_heap(pat->heap);
  free(pat);
}

void swap(int* a, int* b){
  int temp = *a; // Valeur de a
  *a = *b;
  *b = temp;
}

void swapf(float* a, float* b){
  float temp = *a; // Valeur de a
  *a = *b;
  *b = temp;
}

void percolate_up(min_heap* heap, int i){
  while(heap->priority[i] < heap->priority[(i - 1) / 2] && i > 0){ // On remonte le noeud tant que sa priorite est inferieure a celle de son pere
    swap(&heap->array[i], &heap->array[(i - 1) / 2]);
    swapf(&heap->priority[i], &heap->priority[(i - 1) / 2]);
    i = (i - 1) / 2;
  }
}

void percolate_down(min_heap* heap, int i){
  // On descend i jusqu'a ce que sa priorite soit inferieure a celle de ses fils
  int current = i;
  int left = 2 * i + 1;
  int right = 2 * i + 2;

  if(left < heap->size && heap->priority[left] < heap->priority[current]){
    current = left;
  }

  if(right < heap->size && heap->priority[right] < heap->priority[current]){
    current = right;
  }

  if(current != i){
    swap(&heap->array[i], &heap->array[current]);
    swapf(&heap->priority[i], &heap->priority[current]);
    percolate_down(heap, current);
  }
}

void insert(min_heap* heap, int key, float priority){
  if(heap->size == heap->capacity){
    printf("ERROR: Heap is full\n");
    exit(1);
  }

  int i = heap->size;
  heap->array[i] = key;
  heap->priority[i] = priority;
  heap->size++;

  percolate_up(heap, i);
}

void modify_priority(min_heap* heap, int key, float priority){
  int i;
  for(i = 0; i < heap->size; i++){ // On cherche l'index de l'element a modifier
    if(heap->array[i] == key){
      break;
    }
  }

  heap->priority[i] = priority;
  percolate_up(heap, i); // On actualise l'arbre
}

int extract_min(min_heap* heap){
  if(heap->size == 0){
    return -1;
  }

  if(heap->size == 1){
    heap->size--;
    return heap->array[0];
  }

  int root = heap->array[0];
  heap->array[0] = heap->array[heap->size - 1]; // On remplace la racine par l'element en derniere position
  heap->priority[0] = heap->priority[heap->size - 1];
  heap->size--;

  percolate_down(heap, 0); // On actualise l'arbre

  return root;
}

bool is_member(min_heap* heap, int key){
  for(int i = 0; i < heap->size; i++){
    if(heap->array[i] == key){
      return true;
    }
  }

  return false;
}

float dist(int x1, int y1, int x2, int y2){
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

float vdist(int pos1, int pos2, levelinfo level){
  int x1 = pos1 % level.xsize;
  int y1 = pos1 / level.xsize;
  int x2 = pos2 % level.xsize;
  int y2 = pos2 / level.xsize;

  return dist(x1, y1, x2, y2);
}

bool is_valid(int pos, action a, levelinfo level, levelinfo air_level){
  int x = pos % level.xsize;
  int y = pos / level.xsize;
  char** map = level.map;
  char** air_map = air_level.map;

  // On verifie si le joueur n'est pas en l'air
  bool not_in_air = (air_map[y + 1][x] != PATH && air_map[y + 1][x] != CABLE && air_map[y + 1][x] != BOMB) || air_map[y - 1][x] == CABLE;

  switch(a){
    case NONE:
      return true;
    case UP:
      // On ne peut monter que si on est sur une echelle et qu'il n'y a pas de mur au dessus
      if(map[y][x] == LADDER && map[y - 1][x] != WALL && map[y - 1][x] != FLOOR && map[y - 1][x] != ENEMY){
        return true;
      }
      break;
    case DOWN:
      // On ne peut descendre que si il y a une echelle, un chemin ou un cable en dessous
      if ((map[y + 1][x] == LADDER || map[y + 1][x] == PATH || map[y + 1][x] == CABLE) && map[y + 1][x] != ENEMY){
        return true;
      }
      break;
    case LEFT:
      // On ne peut aller a gauche que si il n'y a pas de mur a gauche et que le joueur n'est pas en l'air
      // C'est un petit hack car le moteur avance de plusieurs tour de jeu sans utiliser le code du joueur tant qu'il tombe, mais le A* ne le sait pas,
      // alors on se debrouille pour que la seule action possible soit DOWN
      if(map[y][x - 1] != WALL && map[y][x - 1] != FLOOR && map[y][x - 1] != ENEMY && map[y][x - 1] != DEAD && map[y + 1][x - 1] != ENEMY && not_in_air){
        return true;
      }
      break;
    case RIGHT:
      // De meme pour la droite
      if(map[y][x + 1] != WALL && map[y][x + 1] != FLOOR && map[y][x + 1] != ENEMY && map[y][x + 1] != DEAD && map[y + 1][x + 1] != ENEMY && not_in_air){
        return true;
      }
      break;
    default:
      printf("ERROR: Invalid action\n");
      exit(1);
  }
  return false;
}

bool is_valid_closest(int pos, action a, levelinfo level){
  int x = pos % level.xsize;
  int y = pos / level.xsize;
  char** map = level.map;

  int yc = y + 1;

  switch(a){
    case NONE:
      return true;
    case UP:
      return true;
    case DOWN:
      while(map[yc][x] == PATH){
        yc++;
      }
      if(map[yc][x] == ENEMY){
        return false;
      }
      return true;
    case LEFT:
      while(map[yc][x - 1] == PATH){
        yc++;
      }
      if(map[yc][x - 1] == ENEMY){
        return false;
      }
      return true;
    case RIGHT:
      while(map[yc][x + 1] == PATH){
        yc++;
      }
      if(map[yc][x + 1] == ENEMY){
        return false;
      }
      return true;
    default:
      printf("ERROR: Invalid action\n");
      exit(1);
  }
}

int weight(action a){
  // Les deplacements coutent 1, le reste 0
  switch(a){
    case NONE:
      return 0;
    case UP:
      return 1;
    case DOWN:
      return 1;
    case LEFT:
      return 1;
    case RIGHT:
      return 1;
    case BOMB_LEFT:
      return 0;
    case BOMB_RIGHT:
      return 0;
    default:
      printf("ERROR: Invalid action\n");
      exit(1);
  }      
}

int get_new_pos(int pos, action a, levelinfo level){
  switch(a){
    case NONE:
      return pos;
    case UP:
      return pos - level.xsize;
    case DOWN:
      return pos + level.xsize;
    case LEFT:
      return pos - 1;
    case RIGHT:
      return pos + 1;
    case BOMB_LEFT:
      return pos;
    case BOMB_RIGHT:
      return pos;
    default:
      printf("ERROR: Invalid action\n");
      exit(1);
  }
}

action get_action(int u, int v, levelinfo level){
  int diff = u - v;

  if(diff == level.xsize){ // Aurait ete plus joli avec un switch mais level.xsize n'est pas une constante
    return UP;

  } else if(diff == -level.xsize){
    return DOWN;

  } else if(diff == 1){
    return LEFT;

  } else if(diff == -1){
    return RIGHT;

  } else {
    return NONE;
  }
}

character_list get_runner(character_list characterl){
  character_list current = characterl;

  while(current->c.item != RUNNER){ // Iteration dans une liste chainee, on pourrait avoir une boucle infinie si le runner n'est pas dans la liste
    current = current->next;
  }

  return current;
}

bool is_in_bonus_list(bonus_list bonus_e, bonus_list bonusl){
  bonus_list current = bonusl;

  while(current != NULL){
    if(current->b.x == bonus_e->b.x && current->b.y == bonus_e->b.y){ // Iteration dans une liste chainee
      return true;
    }
    current = current->next;
  }
    
  return false;
}

levelinfo add_enemies(levelinfo level, character_list characterl, bomb_list bombl){
  // Les ennemis et les bombes sont dans des listes chainees, on a besoin que le A* les prenne en compte
  // Pour cela on les ajoute a la map

  bomb_list currentb = bombl;

  while(currentb != NULL){
    level.map[currentb->y][currentb->x] = BOMB;
    currentb = currentb->next;
  }

  character_list current = characterl;

  while(current != NULL){
    if(current->c.item == ENEMY){
      if(level.map[current->c.y][current->c.x] == BOMB || level.map[current->c.y][current->c.x] == DEAD){
        level.map[current->c.y][current->c.x] = DEAD;
      } else {
        level.map[current->c.y][current->c.x] = ENEMY;
      }
    }
    current = current->next;
  }

  return level;
}

levelinfo get_astar_level(levelinfo level, character_list characterl){
  // On cree une copie du niveau pour ne pas modifier l'original
  levelinfo astar_level;
  astar_level.xsize = level.xsize;
  astar_level.ysize = level.ysize;
  astar_level.xexit = level.xexit;
  astar_level.yexit = level.yexit;
  astar_level.map = malloc(astar_level.ysize * sizeof(char*));

  for(int i = 0; i < astar_level.ysize; i++){
    astar_level.map[i] = malloc(astar_level.xsize * sizeof(char));
    for(int j = 0; j < astar_level.xsize; j++){
      astar_level.map[i][j] = level.map[i][j];
    }
  }

  character_list current = characterl;

  while(current != NULL){
    if(current->c.item == ENEMY){
      if(level.map[current->c.y][current->c.x] == DEAD){
        current = current->next;
        continue;
      }
      // On ajoute des murs autour des ennemis pour faciliter la tache du A*
      char left = astar_level.map[current->c.y][current->c.x - 1];
      char right = astar_level.map[current->c.y][current->c.x + 1];
      char down = astar_level.map[current->c.y + 1][current->c.x];
      char up = astar_level.map[current->c.y - 1][current->c.x];

      if(left == PATH || left == LADDER || left == CABLE){
        astar_level.map[current->c.y][current->c.x - 1] = ENEMY;
      }
      if(right == PATH || right == LADDER || right == CABLE){
        astar_level.map[current->c.y][current->c.x + 1] = ENEMY;
      }
      if(down == LADDER){
        astar_level.map[current->c.y + 1][current->c.x] = ENEMY;
      }
      if(up == LADDER || up == CABLE){
        astar_level.map[current->c.y - 1][current->c.x] = ENEMY;
      }
      if(astar_level.map[current->c.y - 1][current->c.x + 1] == CABLE){
        astar_level.map[current->c.y - 1][current->c.x + 1] = ENEMY;
      }
      if (astar_level.map[current->c.y - 1][current->c.x - 1] == CABLE){
        astar_level.map[current->c.y - 1][current->c.x - 1] = ENEMY;
      }
    }
    current = current->next;
  }

  return astar_level;
}

bonus_list get_closest_bonus(bonus_list bonusl, character_list runner, bonus_list already_seen){
  bonus_list closest_bonus = NULL;

  if(bonusl == NULL){
    return NULL;
  }

  float best_dist = 100000;
  bonus_list current = bonusl;

  while(current != NULL){
    if(dist(current->b.x, current->b.y, runner->c.x, runner->c.y) < best_dist && !is_in_bonus_list(current, already_seen)){
      closest_bonus = current;
      best_dist = dist(current->b.x, current->b.y, runner->c.x, runner->c.y);
    }
    current = current->next;
  }

  return closest_bonus;
}

character_list get_closest_enemy(character_list characterl, character_list runner, levelinfo level){
  character_list closest_enemy = NULL;

  if(characterl == NULL) {
    return NULL;
  }

  float best_dist = 100000;
  character_list current = characterl;

  while(current != NULL){
    if(current->c.item == ENEMY){
      if(level.map[current->c.y][current->c.x] == DEAD){
          current = current->next;
          continue;
        }
      bool enemy_above = false;
      bool enemy_below = false;
      if(current->c.x == runner->c.x){
        int y = current->c.y;
        if(current->c.y < runner->c.y){
          enemy_above = true;
          y++;
          while(y < runner->c.y){
            if(level.map[y][current->c.x] != LADDER){
              enemy_above = false;
              break;
            }
            y++;
          }
        } else {
          enemy_below = true;
          y--;
          while(y > runner->c.y){
            if(level.map[y][current->c.x] != LADDER){
              enemy_below = false;
              break;
            }
            y--;
          }
        }
      }
      if(current->c.y == runner->c.y || enemy_above || enemy_below){ // On ne prend en compte que les ennemis sur la meme ligne que le runner, ou ceux qui sont sur la même échelle
        float distance = dist(current->c.x, current->c.y, runner->c.x, runner->c.y);
        if(distance < best_dist && distance < 10){
          closest_enemy = current;
          best_dist = dist(current->c.x, current->c.y, runner->c.x, runner->c.y);
        }
      }
    }
    current = current->next;
  }

  return closest_enemy;
}

path* a_star(character_list runner, bonus_list closest_bonus, levelinfo level, levelinfo air_level){
  path* pat = create_path(100, level);

  // On ajoute le runner a la file
  pat->d[runner->c.y * level.xsize + runner->c.x] = runner->c.y * level.xsize + runner->c.x;
  insert(pat->heap, runner->c.y * level.xsize + runner->c.x, 0);

  int count = 0; // Evite les boucles infinies
  
  while(!(pat->heap->size == 0) && count < 1000){
    int u = extract_min(pat->heap); // Sommet courant, celui avec la priorite minimale

    if(u == closest_bonus->b.y * level.xsize + closest_bonus->b.x){ // On a trouve le bonus, on pourra remonter le chemin grace a pat->p
      pat->found = true;
      break;
    }

    for(int i=1; i<5; i++){ // i represente une action, 1 = UP, 2 = DOWN, 3 = LEFT, 4 = RIGHT
      if(!is_valid(u, i, level, air_level)){ // On verifie que l'action est possible
        continue;
      }
      int v = get_new_pos(u, i, level); // On calcule la position apres l'action
      float h_v = vdist(v, closest_bonus->b.y * level.xsize + closest_bonus->b.x, level); // Heuristique, distance euclidienne entre v et le bonus

      if(pat->d[u] + weight(i) < pat->d[v]){ // Si on a trouve un chemin plus court
        pat->d[v] = pat->d[u] + weight(i);
        pat->p[v] = u;

        if(is_member(pat->heap, v) == false){ // Si v n'est pas dans la file, on l'ajoute
          insert(pat->heap, v, pat->d[v] + h_v);
        } else { // Sinon on modifie sa priorite
          modify_priority(pat->heap, v, pat->d[v] + h_v);
        }
      }
    }
    count++;  
  }
  return pat;
}

child* find_closest_child(int* p, int origin, int destination, levelinfo level){
  // On cherche le chemin qui nous rapproche le plus du bonus
  float min_dist = 10000;
  int min_child = -1;
  child* p_min_child = NULL;

  for(int i = 0; i < level.xsize * level.ysize; i++){ // On cherche tous les noeuds qui ont pour parent origin
    if(p[i] == origin){
      child* v = find_closest_child(p, i, destination, level);
      int distance = v->distance + vdist(i, origin, level);
      if(distance < min_dist){
        min_dist = distance;
        min_child = i;
        p_min_child = v;
      }
    }
  }
  if (min_child == -1){ // Si on n'a pas trouve de noeud enfant, c'est qu'on est proche de la destination
    child* c = malloc(sizeof(child));
    c->pos = -1; c->distance = vdist(origin, destination, level); c->parent = NULL;
    return c;
  } else {
    child* c = malloc(sizeof(child));
    c->pos = min_child; c->distance = min_dist; c->parent = p_min_child;
    return c;
  }
}

void special_moves(character_list runner, character_list closest_enemy, int* move_to_combat, levelinfo level){
  char down_left = level.map[runner->c.y + 1][runner->c.x - 1];
  char down_right = level.map[runner->c.y + 1][runner->c.x + 1];
  char top_left = level.map[runner->c.y - 1][runner->c.x - 1];
  char top_right = level.map[runner->c.y - 1][runner->c.x + 1];
  char left = level.map[runner->c.y][runner->c.x - 1];
  char right = level.map[runner->c.y][runner->c.x + 1];
  char center = level.map[runner->c.y][runner->c.x];

  bool can_right = is_valid(runner->c.y * level.xsize + runner->c.x, RIGHT, level, level);
  bool can_left = is_valid(runner->c.y * level.xsize + runner->c.x, LEFT, level, level);
  can_right = can_right && (down_right != BOMB && down_right != PATH);
  can_left = can_left && (down_left != BOMB && down_left != PATH);

  if(closest_enemy != NULL){
    if(*move_to_combat == -1){
      debug("Entered move to combat\n");
      int distance = runner->c.y - closest_enemy->c.y;
      if(DEBUG){
        printf("Vertical distance to closest enemy: %d\n", distance);
      }
      if(distance == 0){ // Combat horizontal
        debug("Horizontal combat\n");
        distance = runner->c.x - closest_enemy->c.x;
        if(DEBUG){
          printf("Distance to closest enemy: %d\n", distance);
        }
        if(level.map[runner->c.y - 1][runner->c.x] == CABLE && level.map[runner->c.y + 1][runner->c.x] == PATH){
          debug("Cable above, dropping\n");
          *move_to_combat = DOWN;
        } else if(distance > 0 && distance < 4){ // a gauche
          if((down_left == FLOOR || down_left == BOMB) && top_left != CABLE && left != ENEMY){
            if(down_left == BOMB){
              debug("Already bombed, waiting\n");
              *move_to_combat = NONE;
            } else {
              debug("Enemy close on the left, bombing\n");
              *move_to_combat = BOMB_LEFT;
            }
          } else {
            debug("Can't bomb, moving right\n");
            if(can_right) *move_to_combat = RIGHT;
          }
        } else if(distance < 0 && distance > -4){ // a droite
          if((down_right == FLOOR || down_right == BOMB) && top_right != CABLE && right != ENEMY){
            if(down_right == BOMB){
              debug("Already bombed, waiting\n");
              *move_to_combat = NONE;
            } else {
              debug("Enemy close on the right, bombing\n");
              *move_to_combat = BOMB_RIGHT;
            }
          } else {
            debug("Can't bomb, moving left\n");
            if(can_left) *move_to_combat = LEFT;
          }
        }

        if(DEBUG){
          printf("Can right: %d\n", can_right);
          printf("Can left: %d\n", can_left);
        }

        if(can_right){
          if((left == LADDER || center == LADDER || (left == ENEMY && top_left == LADDER)) && distance > 0){
            debug("Moving right, cause of LADDER\n");
            *move_to_combat = RIGHT;
          }
        }
        if(can_left){
          if((right == LADDER || center == LADDER || (right == ENEMY && top_right == LADDER)) && distance < 0){
            debug("Moving left, cause of LADDER\n");
            *move_to_combat = LEFT;
          }
        }
      } else { // Vertical combat
        debug("Vertical combat\n");
        bool can_up = is_valid(runner->c.y * level.xsize + runner->c.x, UP, level, level);
        bool can_down = is_valid(runner->c.y * level.xsize + runner->c.x, DOWN, level, level);
        if(DEBUG){
          printf("Can up: %d\n", can_up);
          printf("Can down: %d\n", can_down);
        }
        if(distance > 0 && can_down){
          debug("Moving down\n");
          *move_to_combat = DOWN;
        } else if(distance < 0 && can_up){
          debug("Moving up\n");
          *move_to_combat = UP;
        }

        if(level.map[runner->c.y][runner->c.x] == PATH || level.map[runner->c.y + 1][runner->c.x] == FLOOR){
          debug("On top of the ladder, or on the bottom\n");
          *move_to_combat = -1;
        }

      }
    }
  }
}

void print_map(levelinfo level, path* pat, int v, int u, bool closest, child* child_closest){

  for(int i = 0; i < level.ysize; i++){
    for(int j = 0; j < level.xsize; j++){
      bool is_runner = u == i * level.xsize + j;
      bool is_bonus = v == i * level.xsize + j;
      if(is_runner){
        printf("R");
      } else if(is_bonus){
        printf("B");
      } else if(closest == true){
        bool in_path = false;
        child* c2 = child_closest;
        while(c2 != NULL){
          if(c2->pos == i * level.xsize + j){
            in_path = true;
            break;
          }
          c2 = c2->parent;
        }

        if(in_path){
          printf("#");
        } else {
          printf("%c", level.map[i][j]);
        }
        
      } else if(pat->found){
        bool in_path = false;
        int v2 = v;
        while(pat->p[v2] != u){
          if(v2 == i * level.xsize + j){
            in_path = true;
            break;
          }
          v2 = pat->p[v2];
        }

        if(pat->p[v2] == u && v2 == i * level.xsize + j){
          in_path = true;
        }
        
        if(in_path){
          printf("*");
        } else {
          printf("%c", level.map[i][j]);
        }
      } else {
        printf("%c", level.map[i][j]);
      }
    }
    printf("\n");
  }
}

action lode_runner(levelinfo level, character_list characterl, bonus_list bonusl, bomb_list bombl){
  character_list runner = get_runner(characterl);
  level = add_enemies(level, characterl, bombl);
  levelinfo astar_level = get_astar_level(level, characterl);

  if(DEBUG){
    path pat2 = {NULL, NULL, NULL, false};
    path* pat3 = &pat2;
    printf("Level : \n");
    print_map(level, pat3, -1, runner->c.y * level.xsize + runner->c.x, false, NULL);
  }

  bonus_list already_seen = NULL;
  bonus_list closest_bonus = get_closest_bonus(bonusl, runner, already_seen);

  bool to_exit = false;
  if(bonusl == NULL) {
    closest_bonus = malloc(sizeof(bonus_list));
    bonus b = {level.xexit, level.yexit};
    closest_bonus->b = b;
    to_exit = true;
  }

  int v;
  int move_to_combat = -1;

  int move_to_closest = -1;
  child* child_closest = NULL;

  int move_to_path = -1;
  int move_to_skipped = -1;

  bool printed = false;

  while(closest_bonus != NULL){
    path* pat = a_star(runner, closest_bonus, astar_level, level);

    if(level.map[closest_bonus->b.y][closest_bonus->b.x] == ENEMY){
      int runner_pos = runner->c.y * level.xsize + runner->c.x;
      int closest_bonus_pos = closest_bonus->b.y * level.xsize + closest_bonus->b.x;

      child* c = find_closest_child(pat->p, runner_pos, closest_bonus_pos, level);

      move_to_skipped = get_action(runner_pos, c->pos, level);

      bonus_list tmp = malloc(sizeof(bonus_list));
      tmp->b = closest_bonus->b;
      tmp->next = already_seen;
      already_seen = tmp;
      if(!to_exit){
        closest_bonus = get_closest_bonus(bonusl, runner, already_seen);
      }
      continue;
    }

    if(move_to_closest == -1){
      int runner_pos = runner->c.y * level.xsize + runner->c.x;
      int closest_bonus_pos = closest_bonus->b.y * level.xsize + closest_bonus->b.x;

      child* c = find_closest_child(pat->p, runner_pos, closest_bonus_pos, level);

      move_to_closest = get_action(runner_pos, c->pos, level);
      child_closest = c;
    }

    if(pat->found){
      if(DEBUG){
        printf("A* Level : \n");
        print_map(astar_level, pat, closest_bonus->b.y * level.xsize + closest_bonus->b.x, runner->c.y * level.xsize + runner->c.x, false, NULL);
        printf("A* found\n");
        printed = true;
      }
      v = closest_bonus->b.y * level.xsize + closest_bonus->b.x;
      while(pat->p[v] != runner->c.y * level.xsize + runner->c.x){
        v = pat->p[v];
      }
      free_path(pat);

      move_to_path = get_action(runner->c.y * level.xsize + runner->c.x, v, level);

      break;
    } else if(pat->heap->size != 0){
      printf("ERROR: Path is longer than heap size\n");
      exit(1);
    } else {
      special_moves(runner, get_closest_enemy(characterl, runner, level), &move_to_combat, level);
    }

    bonus_list tmp = malloc(sizeof(bonus_list));
    tmp->b = closest_bonus->b;
    tmp->next = already_seen;
    already_seen = tmp;
    if(!to_exit){
      closest_bonus = get_closest_bonus(bonusl, runner, already_seen);
    } else {
      closest_bonus = NULL;
    }

    free_path(pat);
  }

  if(DEBUG && !printed){
    path pat2 = {NULL, NULL, NULL, false};
    path* pat3 = &pat2;
    printf("A* level : \n");
    bool closest = move_to_path == -1 && move_to_combat == -1 && move_to_closest != -1; 
    print_map(astar_level, pat3, -1, runner->c.y * level.xsize + runner->c.x, closest, child_closest);
  }

  if(move_to_path != -1){
    debug("Path\n");
    return move_to_path;
  } else if(move_to_combat != -1){
    debug("Combat\n");
    return move_to_combat;
  } else if(move_to_closest != -1){
    debug("Closest\n");
    if(is_valid_closest(runner->c.y * level.xsize + runner->c.x, move_to_closest, astar_level)){
      return move_to_closest;
    }
    return NONE;
  } else {
    debug("Skipped\n");
    return move_to_skipped;
  }
}