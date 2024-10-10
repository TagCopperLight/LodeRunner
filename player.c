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

extern const int BOMB_TTL; // time to live for bombs

extern bool DEBUG; // true if and only if the game runs in debug mode

const char *students = "BOURDET"; // replace Radom with the student names here

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
character_list get_closest_enemy(character_list, character_list); // Renvoie l'ennemi le plus proche du runner, sur la meme ligne

// A*
path* create_path(int, levelinfo);
void free_path(path*);
bool is_valid(int, action, levelinfo); // Verifie si une action est valide
int weight(action); // Valeur d'une action, utilise pour le calcul de la priorite
int get_new_pos(int, action, levelinfo); // Renvoie la position apres avoir effectue une action
action get_action(int, int, levelinfo); // Renvoie l'action a effectuer pour aller de u a v, si ce n'est pas possible, renvoie NONE
path* a_star(character_list, bonus_list, levelinfo); // Algorithme de recherche de chemin, renvoie le chemin le plus court entre le runner et le bonus
child find_closest_child(int*, int, int, levelinfo); // Si A* ne trouve pas de chemin, on cherche le chemin qui nous rapproche le plus du bonus

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

bool is_valid(int pos, action a, levelinfo level){
  int x = pos % level.xsize;
  int y = pos / level.xsize;
  char** map = level.map;

  // On verifie si le joueur n'est pas en l'air
  bool not_in_air = (map[y + 1][x] != PATH && map[y + 1][x] != CABLE) || map[y - 1][x] == CABLE;

  switch(a){
    case NONE:
      return true;
    case UP:
      // On ne peut monter que si on est sur une echelle et qu'il n'y a pas de mur au dessus
      if(map[y][x] == LADDER && map[y - 1][x] != WALL && map[y - 1][x] != FLOOR){
        return true;
      }
      break;
    case DOWN:
      // On ne peut descendre que si il y a une echelle, un chemin ou un cable en dessous
      if (map[y + 1][x] == LADDER || map[y + 1][x] == PATH || map[y + 1][x] == CABLE){
        return true;
      }
      break;
    case LEFT:
      // On ne peut aller a gauche que si il n'y a pas de mur a gauche et que le joueur n'est pas en l'air
      // C'est un petit hack car le moteur avance de plusieurs tour de jeu sans utiliser le code du joueur tant qu'il tombe, mais le A* ne le sait pas,
      // alors on se debrouille pour que la seule action possible soit DOWN
      if(map[y][x - 1] != WALL && map[y][x - 1] != FLOOR && not_in_air){
        return true;
      }
      break;
    case RIGHT:
      // De meme pour la droite
      if(map[y][x + 1] != WALL && map[y][x + 1] != FLOOR && not_in_air){
        return true;
      }
      break;
    default:
      printf("ERROR: Invalid action\n");
      exit(1);
  }
  return false;
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
  // Pour cela on les ajoute a la map, les ennemis sont remplaces par des murs, les bombes par des bombes
  character_list current = characterl;

  while(current != NULL){
    if(current->c.item == ENEMY){
      level.map[current->c.y][current->c.x] = WALL;
      // On ajoute des murs autour des ennemis s'ils sont sur une echelle ou un cable pour faciliter la tache du A*
      if(level.map[current->c.y + 1][current->c.x] == LADDER){
        level.map[current->c.y + 1][current->c.x] = WALL;
      }
      if(level.map[current->c.y - 1][current->c.x] == LADDER){
        level.map[current->c.y - 1][current->c.x] = WALL;
      }
      if(level.map[current->c.y - 1][current->c.x] == CABLE){
        level.map[current->c.y - 1][current->c.x] = WALL;
      }
      if(level.map[current->c.y - 1][current->c.x + 1] == CABLE){
        level.map[current->c.y - 1][current->c.x + 1] = WALL;
      }
      if (level.map[current->c.y - 1][current->c.x - 1] == CABLE){
        level.map[current->c.y - 1][current->c.x - 1] = WALL;
      }
    }
    current = current->next;
  }

  bomb_list currentb = bombl;

  while(currentb != NULL){
    level.map[currentb->y][currentb->x] = BOMB;
    currentb = currentb->next;
  }

  return level;
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

character_list get_closest_enemy(character_list characterl, character_list runner){
  character_list closest_enemy = NULL;

  if(characterl == NULL) {
    return NULL;
  }

  float best_dist = 100000;
  character_list current = characterl;

  while(current != NULL){
    if(current->c.item == ENEMY){
      if (current->c.y != runner->c.y){ // On ne prend en compte que les ennemis sur la meme ligne que le runner, ceux en hauteur ne sont pas dangereux
        current = current->next;
        continue;
      }
      if(dist(current->c.x, current->c.y, runner->c.x, runner->c.y) < best_dist){
        closest_enemy = current;
        best_dist = dist(current->c.x, current->c.y, runner->c.x, runner->c.y);
      }
    }
    current = current->next;
  }

  return closest_enemy;
}

path* a_star(character_list runner, bonus_list closest_bonus, levelinfo level){
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
      if(!is_valid(u, i, level)){ // On verifie que l'action est possible
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

child find_closest_child(int* p, int origin, int destination, levelinfo level){
  // On cherche le chemin qui nous rapproche le plus du bonus
  float min_dist = 10000;
  int min_child = -1;

  for(int i = 0; i < level.xsize * level.ysize; i++){ // On cherche tous les noeuds qui ont pour parent origin
    if(p[i] == origin){
      child v = find_closest_child(p, i, destination, level);
      int distance = v.distance + vdist(i, origin, level);
      if(distance < min_dist){
        min_dist = distance;
        min_child = i;
      }
    }
  }
  if (min_child == -1){ // Si on n'a pas trouve de noeud enfant, c'est qu'on est proche de la destination
    child c = {origin, vdist(origin, destination, level)};
    return c;
  } else {
    child c = {min_child, min_dist};
    return c;
  }
}

void special_moves(character_list runner, character_list closest_enemy, bonus_list closest_bonus, path* pat, int* move_to_closest, int* move_to_combat, levelinfo level){
  if(closest_enemy != NULL){
    if(*move_to_combat == -1){
      int distance = runner->c.x - closest_enemy->c.x;
      if(distance > 0 && distance < 4){ // a gauche
        *move_to_combat = BOMB_LEFT;
      } else if(distance < 0 && distance > -4){ // a droite
        *move_to_combat = BOMB_RIGHT;
      }
    }
  } else {
    if(*move_to_closest == -1){
      int runner_pos = runner->c.y * level.xsize + runner->c.x;
      int closest_bonus_pos = closest_bonus->b.y * level.xsize + closest_bonus->b.x;

      child c = find_closest_child(pat->p, runner_pos, closest_bonus_pos, level);

      *move_to_closest = get_action(runner_pos, c.pos, level);
    }
  }
}

action lode_runner(levelinfo level, character_list characterl, bonus_list bonusl, bomb_list bombl){
  character_list runner = get_runner(characterl);
  level = add_enemies(level, characterl, bombl);
  
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

  while(closest_bonus != NULL){
    if(level.map[closest_bonus->b.y][closest_bonus->b.x] == WALL){
      bonus_list tmp = malloc(sizeof(bonus_list));
      tmp->b = closest_bonus->b;
      tmp->next = already_seen;
      already_seen = tmp;
      if(!to_exit){
        closest_bonus = get_closest_bonus(bonusl, runner, already_seen);
      }
      continue;
    }

    path* pat = a_star(runner, closest_bonus, level);

    if(pat->found){
      v = closest_bonus->b.y * level.xsize + closest_bonus->b.x;
      while(pat->p[v] != runner->c.y * level.xsize + runner->c.x){
        v = pat->p[v];
      }
      free_path(pat);
      break;
    } else if(pat->heap->size != 0){
      printf("ERROR: Path is longer than heap size\n");
      exit(1);
    } else {
      special_moves(runner, get_closest_enemy(characterl, runner), closest_bonus, pat, &move_to_closest, &move_to_combat, level);
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

  if(move_to_combat != -1){
    printf("Combat\n");
    return move_to_combat;
  } else if(move_to_closest != -1){
    printf("Closest\n");
    return move_to_closest;
  } else {
    printf("A*\n");
    return get_action(runner->c.y * level.xsize + runner->c.x, v, level);
  }
}

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
