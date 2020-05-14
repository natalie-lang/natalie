# hashmap
Flexible hashmap implementation in C using open addressing and linear probing for collision resolution.

### Summary
This project came into existence because there are a notable lack of flexible and easy to use data structures available in C.  Sure, higher level languages have built-in libraries, but plenty of embedded projects or higher level libraries start with core C code.  It was undesirable to add a bulky library like Glib as a dependency to my projects, or grapple with a restrictive license agreement.  Searching for "C hashmap" yielded results with questionable algorithms and code quality, projects with difficult or inflexible interfaces, or projects with less desirable licenses.  I decided it was time to create my own.


### Goals
* **To scale gracefully to the full capacity of the numeric primitives in use.**  E.g. on a 32-bit machine, you should be able to load a billion+ entries without hitting any bugs relating to integer overflows.  Lookups on a hashtable with a billion entries should be performed in close to constant time, no different than lookups in a hashtable with 20 entries.  Automatic rehashing occurs and maintains a load factor of 0.75 or less.
* **To provide a clean and easy-to-use interface.**  C data structures often struggle to strike a balance between flexibility and ease of use.  To this end, I provided a generic interface using void pointers for keys and data, and macros to generate type-specific wrapper functions, if desired.
* **To enable easy iteration and safe entry removal during iteration.**  Applications often need these features, and the data structure should not hold them back.  Both an iterator interface and a foreach function was provided to satisfy various use-cases.  This hashmap also uses an open addressing scheme, which has superior iteration performance to a similar hashmap implemented using separate chaining (buckets with linked lists).  This is because fewer instructions are needed per iteration, and array traversal has superior cache performance than linked list traversal.
* **To use a very unrestrictive software license.**  Using no license was an option, but I wanted to allow the code to be tracked, simply for my own edification.  I chose the MIT license because it is the most common open source license in use, and it grants full rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell the code.  Basically, take this code and do what you want with it.  Just be nice and leave the license comment and my name at top of the file.  Feel free to add your name if you are modifying and redistributing.

### Code Example
```C
#include <stdlib.h>
#include <stdio.h>

#include <hashmap.h>

/* Some sample data structure with a string key */
struct blob {
  char key[32];
  size_t data_len;
  unsigned char data[1024];
};

/* Declare type-specific blob_hashmap_* functions with this handy macro */
HASHMAP_FUNCS_CREATE(blob, const char, struct blob)

struct blob *blob_load(void)
{
  struct blob *b;
  /*
   * Hypothetical function that allocates and loads blob structures
   * from somewhere.  Returns NULL when there are no more blobs to load.
   */
  return b;
}

/* Hashmap structure */
struct hashmap map;

int main(int argc, char **argv)
{
  struct blob *b;
  struct hashmap_iter *iter;

  /* Initialize with default string key functions and init size */
  hashmap_init(&map, hashmap_hash_string, hashmap_compare_string, 0);

  /* Load some sample data into the map and discard duplicates */
  while ((b = blob_load()) != NULL) {
    if (blob_hashmap_put(&map, b->key, b) != b) {
      printf("discarding blob with duplicate key: %s\n", b->key);
      free(b);
    }
  }

  /* Lookup a blob with key "AbCdEf" */
  b = blob_hashmap_get(&map, "AbCdEf");
  if (b) {
    printf("Found blob[%s]\n", b->key);
  }

  /* Iterate through all blobs and print each one */
  for (iter = hashmap_iter(&map); iter; iter = hashmap_iter_next(&map, iter)) {
    printf("blob[%s]: data_len %zu bytes\n", blob_hashmap_iter_get_key(iter),
      blob_hashmap_iter_get_data(iter)->data_len);
  }

  /* Remove all blobs with no data */
  iter = hashmap_iter(&map);
  while (iter) {
    b = blob_hashmap_iter_get_data(iter);
    if (b->data_len == 0) {
      iter = hashmap_iter_remove(&map, iter);
      free(b);
    } else {
      iter = hashmap_iter_next(&map, iter);
    }
  }

  /* Free all allocated resources associated with map and reset its state */
  hashmap_destroy(&map);

  return 0;
}

```
