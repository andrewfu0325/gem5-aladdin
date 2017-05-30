
#include <inttypes.h>
#include <unordered_map>

#include "base/types.hh"
#include "mem/ruby/structures/PseudoLRUPolicy.hh"

#ifndef __MMU_CACHE_HH__
#define __MMU_CACHE_HH__

typedef uint64_t MMUCacheTag;
typedef uint64_t PTIdx;

class MMUCache {
  public:
    MMUCache();
    ~MMUCache();

    class PageTable {
      public:
        PageTable(Addr _startAddr) : 
          startAddr(_startAddr) {}
        Addr startAddr;
        std::unordered_map<PTIdx, PageTable*> nPT;
    };

    bool isTagPresent(MMUCacheTag Tag);
    void pageWalker(Addr vaddr);
    PageTable *walkPageTable(PageTable *pg, PTIdx idx);
    PageTable *getTopLevelPageTable();

  private:
    std::pair<uint64_t, PageTable*> lookup(MMUCacheTag Tag);

    const static size_t size = 12;
    uint64_t hit;
    uint64_t miss;

    std::unordered_map<Addr, PageTable*> emulVirtMem;
    typedef std::pair<MMUCacheTag, PageTable*> Entries;
    Entries cache[size];
    
    PseudoLRUPolicy *pseudoLRU;
    Addr CR3Reg;


  public:

};

#endif // __MMU_CACHE_HH__
