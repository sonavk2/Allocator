#include "allocator.h"
#include <string.h>

#define MAX_HEAP_SIZE (128 * 1024 * 1024)  

static void *base;        
static size_t used;     

typedef struct Metadata {
  size_t size;                
  int used;                  
  struct Metadata *next_free; 
  struct Metadata *prev;      
  struct Metadata *next;      
} Metadata;

static Metadata *head = NULL; 
static Metadata *last = NULL;     

void allocator_init(void *newbase) {
  base = newbase;
  used = 0;
  head = NULL;
  last= NULL;
}

void allocator_reset() {
  used = 0;
  head = NULL;
  last = NULL;
}

void remove_from_list(Metadata *block) {
  Metadata *prev_free = NULL;
  Metadata *curr = head;
  while (curr) {
    if (curr == block) {
      if (prev_free) {
        prev_free->next_free = curr->next_free;
      } else {
        head = curr->next_free;
      }
      break;
    }
    prev_free = curr;
    curr = curr->next_free;
  }
}

void add_to_list(Metadata *block) {
  block->next_free = head;
  head = block;
}

void split(Metadata *block, size_t size) {
  Metadata *split_block = (Metadata *)((char *)(block + 1) + size);
  split_block->size = block->size - size - sizeof(Metadata);
  split_block->used = 0;

  split_block->prev = block;
  split_block->next = block->next;
  if (split_block->next) {
    split_block->next->prev = split_block;
  } else {
    last = split_block;
  }
  block->next = split_block;
  add_to_list(split_block);

  block->size = size;
}

void merge_next(Metadata *block) {
  Metadata *next_block = block->next;
  if (next_block && !next_block->used) {
    remove_from_list(next_block);
    block->size += sizeof(Metadata) + next_block->size;
    block->next = next_block->next;
    if (block->next) {
      block->next->prev = block;
    } else {
      last = block;
    }
  }
}

Metadata *merge_prev(Metadata *block) {
  Metadata *prev_block = block->prev;
  if (prev_block && !prev_block->used) {
    remove_from_list(prev_block);
    prev_block->size += sizeof(Metadata) + block->size;
    prev_block->next = block->next;
    if (prev_block->next) {
      prev_block->next->prev = prev_block;
    } else {
      last = prev_block;
    }
    block = prev_block;
  }
  return block;
}

void *mymalloc(size_t size) {
  size_t total_size = sizeof(Metadata) + size;
  Metadata *prev_free = NULL;
  Metadata *curr = head;

  while (curr) {
    if (curr->size >= size) {
      if (prev_free) {
        prev_free->next_free = curr->next_free;
      } else {
        head = curr->next_free;
      }
      curr->used = 1;
      if (curr->size >= size + sizeof(Metadata) + 8) {
        split(curr, size);
      }
      return (void *)(curr + 1);
    }
    prev_free = curr;
    curr = curr->next_free;
  }
  if (used + total_size > MAX_HEAP_SIZE) {
    return NULL;  
  }
  Metadata *meta = (Metadata *)((char *)base + used);
  meta->size = size;
  meta->used = 1;
  meta->prev = last;
  meta->next = NULL;

  if (last) {
    last->next = meta;
  }
  last = meta;
  used += total_size;
  return (void *)(meta + 1);
}

void myfree(void *ptr) {
  if (ptr == NULL) {
    return; 
  }
  Metadata *meta = (Metadata *)ptr - 1;
  meta->used = 0;

  merge_next(meta);
  meta = merge_prev(meta);
  if (meta->next == NULL) {
    if (meta->prev) {
      meta->prev->next = NULL;
      last = meta->prev;
    } else {
      last = NULL;
    }
    used = (char *)meta - (char *)base;
  } else {
      add_to_list(meta);
  }
}

void *myrealloc(void *ptr, size_t size) {
  if (!size) {
    myfree(ptr);
    return NULL;
  }
  if (ptr == NULL) {
    return mymalloc(size);
  }
  Metadata *meta = (Metadata *)ptr - 1;
  size_t old_size = meta->size;

  if (size <= old_size) {
    if (old_size - size >= sizeof(Metadata) + 8) {
      split(meta, size);
    }
    return ptr;
} else {
    Metadata *next_meta = meta->next;
    if (next_meta && !next_meta->used &&
      (meta->size + sizeof(Metadata) + next_meta->size) >= size) {
      remove_from_list(next_meta);
      meta->size += sizeof(Metadata) + next_meta->size;
      meta->next = next_meta->next;
      if (meta->next) {
        meta->next->prev = meta;
      } else {
        last = meta;
      }
      if (meta->size >= size + sizeof(Metadata) + 8) {
        split(meta, size);
      }
      return ptr;
    }
    if (meta->next == NULL && used + (size - old_size) <= MAX_HEAP_SIZE) {
      meta->size = size;
      used += size - old_size;
      return ptr;
    }
    void *new_ptr = mymalloc(size);
    if (new_ptr) {
      memcpy(new_ptr, ptr, old_size);
      myfree(ptr);
    }
    return new_ptr;
  }
}
