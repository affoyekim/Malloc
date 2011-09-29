extern "C" void * malloc (size_t sz);
extern "C" void free (void * ptr);
extern "C" void * calloc (size_t num, size_t sz);
extern "C" void * realloc (void * ptr, size_t sz) throw();

enum { PAGESIZE = 4096 };
enum { MIN_OBJECT_SIZE = 8};
enum { MAX_OBJECT_SIZE = 1024};
enum { MAX_OBJECTS = 512};

typedef char FreeObject;

struct BibopHeader{
  /// How big objects are in this bibop space.
  size_t _objectSize;

  /// The previous bibop in a linked list.
  BibopHeader * _prev;

  /// The next bibop in a list.
  BibopHeader * _next;

  /// How many items are available for allocation.
  int _available;

  /// The free list of available chunks.
  FreeObject * _freeList; 
  
  /// A dummy value to enforce double-word alignment.
  double _dummy;
};


class Allocator {
public:
  Allocator (void);
  void * malloc (size_t sz);
  void free (void * ptr);
  static size_t getSize (void * ptr);
private:
};
 
