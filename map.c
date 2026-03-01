#include "map.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Map *new_map() {
  Map *m = (Map *)malloc(sizeof(Map));
  m->root = NULL;
  return m;
}

MapPair *new_map_pair(const char *key, void *value) {
  MapPair *pair = (MapPair *)malloc(sizeof(MapPair));
  pair->left = NULL;
  pair->right = NULL;
  pair->key = key;
  pair->value = value;
  return pair;
}

// TODO:free memory, do nothing now.
void free_map(Map *map) {}

void insert(Map *map, const char *key, void *value) {
  if (map->root == NULL) {
    map->root = new_map_pair(key, value);
    return;
  }
  MapPair *pair = map->root;
  while (1) {
    int r = strcmp(pair->key, key);
    if (r < 0) {
      if (pair->right == NULL) {
        MapPair *np = new_map_pair(key, value);
        pair->right = np;
        return;
      } else {
        pair = pair->right;
      }
    } else if (r > 0) {
      if (pair->left == NULL) {
        MapPair *np = new_map_pair(key, value);
        pair->left = np;
        return;
      } else {
        pair = pair->left;
      }
    } else {
      pair->value = value;
      return;
    }
  }
}
MapPair *get(Map *map, const char *key) {
  MapPair *start = map->root;

  while (start != NULL) {
    int r = strcmp(start->key, key);
    if (r < 0) {
      start = start->right;
    } else if (r > 0) {
      start = start->left;
    } else {
      return start;
    }
  }
  return start;
}
