#include <stdio.h>

extern int dimensions, *width, numvlinks, numnvlinks, numplinks, *part,
  *cum_part;
extern int NODES;

int *map;
extern char THREAD_MAP[40];

make_thread_map(num_threads) {
  if (strcmp("BLOCK", THREAD_MAP) == 0)
    make_mesh_map(num_threads);
  else
    if (strcmp("LINEAR", THREAD_MAP) == 0)
      make_linear_map(num_threads);
  else
    read_map_file(THREAD_MAP, num_threads);
}

read_map_file(f, num_threads)
     char f[];
{
  FILE *fmap;
  int i;
  fmap = fopen(f, "r");
  if (!fmap) {
    printf("Map file not found.\n");
    exit(1);
  }
  map = (int *)malloc(sizeof(int)*num_threads);
  for (i=0; i<num_threads; i++)
    fscanf(fmap, "%d", &map[i]);
  fclose(fmap);
}

make_linear_map(num_threads)
{
  int id;
  
  map = (int *)malloc(sizeof(int)*num_threads);
  if (NODES < num_threads)
    for (id=0; id<num_threads; id++) 
      map[id] = id*NODES/num_threads;
  else
    for (id=0; id<num_threads; id++) 
      map[id] = id;
}

make_mesh_map(num_threads) 
{
  int id, i, j, k, l, m, node;
  int thread_row, thread_col;
  int rows, cols, frows, fcols;
  int r, c;
  int root;

  root = (int)sqrt((double)num_threads);
  
  thread_row=0;
  thread_col=0;
  map = (int *)malloc(sizeof(int)*num_threads);
  k=0;
  node = 0;
  cols = num_threads/root/width[0];
  fcols = (num_threads/root)%width[0];
  rows = root/width[1];
  frows = root%width[1];
  for (j=0; j<num_threads; j++) {
    thread_row = j/root;
    thread_col = j%root;
    c = thread_col/cols;
    c -= (c*fcols-root/2)/root;
    r = thread_row/rows;
    r -= (r*frows-root/2)/root;
    map[j] = r*width[0]+c;
  }
}

map_thread(id) {
  return map[id];
}
