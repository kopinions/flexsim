/*****************************************************************/
/* Flit sim memory management unit                               */
/*****************************************************************/
#include <stdio.h>
#define min_blocksize  32
#define pool_size      1024

typedef struct mem {
  struct mem *next;
  int size;
} mem_block;

typedef struct pool_list {
  struct pool_list *next;
} plist;

plist *Pool_List;
mem_block *pool, *avail, *busy;
mem_block *allocate_pool();

/*----------------------------------*/
/* init_mem()                       */
/*----------------------------------*/
init_mem() {
  pool = allocate_pool();
  avail = pool;
  busy = NULL;
}

/*----------------------------------*/
/* avail_mem()                      */
/*----------------------------------*/
avail_mem() {
  avail_ptr(Pool_List);
}

/*----------------------------------*/
/* avail_ptr()                      */
/*----------------------------------*/
avail_ptr(p) 
     plist *p;
{
  if (p->next)
    avail_ptr(p->next);
  free(p);
}

/*----------------------------------*/
/* allocate_pool()                  */
/*----------------------------------*/
mem_block *allocate_pool() {
  mem_block *tmp;
  plist *pl;

  pl = (plist *)malloc(pool_size*min_blocksize + sizeof(plist));
  pl->next = Pool_List;
  Pool_List = pl;
  tmp = (mem_block *)(pl+1);
  if (tmp) {
    tmp->next = NULL;
    tmp->size = pool_size*min_blocksize - sizeof(mem_block);
  } else {
    printf("allocate_pool failed.\n");
    exit(1);
  }
  return tmp;
}

/*----------------------------------*/
/* allocate()                       */
/*----------------------------------*/
char *allocate(size)
     int size;
{
  mem_block *f, *n, *l;
  char *c;
  f = avail;
  l = NULL;
  if (size > min_blocksize*pool_size) {
    printf("allocation request too large.\n");
    exit(1);
  }
  while (f->size < size) {	/* Search avail list */
    if (f->next == NULL)
      f->next = allocate_pool();
    l = f;
    f = f->next;
  }
  c = (char *)f;
  if (min_blocksize + sizeof(mem_block) < f->size - size) {
    /* Break large block into two fragments */
    if (l != NULL) {
      l->next = (mem_block *)(c + size + sizeof(mem_block));
      l->next->size = (f->size - size -  sizeof(mem_block));
      l->next->next = f->next;
    } else {
      avail = (mem_block *)(c + size + sizeof(mem_block));
      avail->size = (f->size - size -  sizeof(mem_block));
      avail->next = f->next;
    }
    f->size = size;
    /*    f->next = busy;
	  busy = f; */
    return (char *)(f+1);
  } else {
    /* Return the whole block */
    if (l)
      l->next = f->next;
    else {
      if (f->next)
	avail = f->next;
      else
	avail = allocate_pool();
    }
    /*    f->next = busy;
	  busy = f; */
    return (char *)(f+1);
  }
}

/*----------------------------------*/
/* deallocate()                     */
/*----------------------------------*/
deallocate(p)
  char *p;
{
  mem_block *f, *b;
  f = ((mem_block *)p) - 1;

  /* Remove from busy list */
/*  b = busy;
  while (b->next != f) {
    b = b->next;
  }
  b->next = f->next; */

  /* Add to avail list */
  f->next = avail;
  avail = f;
}
