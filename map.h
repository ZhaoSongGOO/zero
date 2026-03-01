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
  int count;
} Map;

Map *new_map();
void free_map(Map *map);

void map_insert(Map *map, const char *key, void *value);
MapPair *map_get(Map *map, const char *key);

typedef void (*map_handler)(MapPair *pair, void *);

void map_foreach(Map *map, map_handler handler, void *);

#endif
