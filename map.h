#ifndef ZERO_MAP_H
#define ZERO_MAP_H

struct map_pair {
  const char *key;
  void *value;
  struct map_pair *left;
  struct map_pair *right;
};

#define MapPair struct map_pair

typedef struct {
  MapPair *root;
} Map;

Map *new_map();
void free_map(Map *map);

void insert(Map *map, const char *key, void *value);
MapPair *get(Map *map, const char *key);

#endif
