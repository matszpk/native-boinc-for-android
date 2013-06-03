
#ifndef _TRACK_MEM_H_
#define _TRACK_MEM_H_

#include <cstdio>
#include <new>

template <typename T> 
class track_mem {
#ifdef DEBUG_ALLOCATIONS
  private:
    static const char *name;
    static int ref_count;
  public:
    track_mem(const char *n="unknown") {
      name=n;
      fprintf(stderr,"%s #%d: allocated 0x%lx bytes at 0x%p\n",name,++ref_count,sizeof(T),this);
      fflush(stderr);
    };
    ~track_mem() {
      fprintf(stderr,"%s #%d: freed 0x%lx bytes at 0x%p\n",name,this->ref_count--,sizeof(T),this);
      fflush(stderr);
    };
#else 
  public:
    track_mem(const char *n=0) {};
    ~track_mem() {};
#endif
};

#ifdef DEBUG_ALLOCATIONS
template <typename T>
const char *track_mem<T>::name;

template <typename T>
int track_mem<T>::ref_count;
#endif

#endif
