// $Id: db_table.h,v 1.45.2.10 2007/05/31 22:03:18 korpela Exp $
// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <ctime>
#include <vector>
#include <map>


template <typename T>
class row_cache {
  private:
    class cache_element {
      public:
      T item;
      time_t insert_time;
      cache_element(const T &val) : item(val), insert_time(time(0)) {};
      cache_element &operator =(const T &val) {
        if (&item != &val) {
          item=val;
          insert_time=time(0);
        }
        return *this;
      };
    };
    int size;
    std::vector<cache_element> elements;
    typedef typename std::vector<cache_element>::iterator element_iterator;
    typedef typename std::map<sqlint8_t,element_iterator>::iterator id_iterator;
    typedef typename std::multimap<time_t,element_iterator>::iterator time_iterator;
    std::map<sqlint8_t,element_iterator> id_index;
    std::multimap<time_t,element_iterator> time_index;
    time_t ttl;
    size_t hits;
    size_t misses;

  public:
  // constructors
    row_cache(int newsize=1024, time_t newttl=3600) : 
      size(newsize), ttl(newttl), hits(0), misses(0) { invalidate_all(); };

    ~row_cache() {
#ifndef _NDEBUG
      print_stats();
#endif
    }; 

    void print_stats() {
      printf("Cache statistics on %s\n",T::table_name);
      printf("%ld hits    %ld misses   %f percent hit rate\n",
	  misses, hits, (double)hits/(hits+misses));
    }

    void invalidate_all() { 
      elements.clear(); 
      elements.reserve(size);
      time_index.clear(); 
      id_index.clear(); 
    };

    void set_size(int newsize) {
      size=newsize;
      invalidate_all();
    }

    int get_size() {
      return(size);
    }

    void set_ttl(time_t newttl) {
      ttl=newttl;
    }

    time_t get_ttl() {
     return(ttl);
    }

    void insert(const T &val) {
      element_iterator inserted;
      if (elements.size() < size) {
        // not full yet, just push_back()
        elements.push_back(val);
        inserted=elements.end()-1;
      } else {
        // We're full so we'll find the oldest element to overwrite.
        time_iterator oldest=time_index.begin();
        inserted=oldest->second;
        // remove it from the indexes
        id_index.erase(id_index.find(inserted->item.id));
        time_index.erase(oldest);
        // overwrite the old cache item with the new one.
        *inserted=val;
      }
      // add the new item to the indexes.
      id_index[inserted->item.id]=inserted;
      time_index.insert(std::pair<time_t,element_iterator>(inserted->insert_time,inserted));
    };
      
    T *find(sqlint8_t id) {
      id_iterator i=id_index.find(id);
      if ((i == id_index.end()) || (i->second->insert_time+ttl < time(0))) {
	misses++;
	return NULL;
      }
      hits++;
      return &(i->second->item);
    };
};

