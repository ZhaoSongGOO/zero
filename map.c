#include "map.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Map *new_map() {
  Map *m = (Map *)malloc(sizeof(Map));
  m->root = NULL;
  m->count = 0;
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

void map_insert(Map *map, const char *key, void *value) {
  map->count += 1;
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
MapPair *map_get(Map *map, const char *key) {
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

void _map_foreach_kernel(MapPair *pair, map_handler handler, void *data) {
  if (pair == NULL) {
    return;
  }

  handler(pair, data);

  _map_foreach_kernel(pair->left, handler, data);
  _map_foreach_kernel(pair->right, handler, data);
}

void map_foreach(Map *map, map_handler handler, void *data) {
  _map_foreach_kernel(map->root, handler, data);
}
