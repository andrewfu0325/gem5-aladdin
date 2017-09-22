
#include <cmath>
#include <random>

#include "arch/generic/mmu_cache.hh"
#include "base/bitfield.hh"

static std::random_device rd;
static std::mt19937_64 e2(rd());
static std::uniform_int_distribution<long long int> dist(
         std::llround(std::pow(2,61)), std::llround(std::pow(2,62)));

MMUCache::MMUCache() : hit(0), miss(0) {
  CR3Reg = dist(e2);
  CR3Reg &= ~mask(12);
  CR3Reg &= mask(30);
  PageTable *L4PT = new PageTable(CR3Reg);
  emulVirtMem.insert(std::make_pair(CR3Reg, L4PT));
  pseudoLRU = new PseudoLRUPolicy(1, size);
}

MMUCache::~MMUCache() {
  for(auto it : emulVirtMem) {
    delete (it.second);
  }
  delete pseudoLRU;
}

MMUCache::PageTable*
MMUCache::getTopLevelPageTable() {
  assert(emulVirtMem.find(CR3Reg) != emulVirtMem.end());
  return emulVirtMem[CR3Reg];
}

void
MMUCache::pageWalker(Addr vaddr) {
  //printf("Page walking for vaddr: %llx\n", vaddr);
  vaddr &= mask(48);
  PTIdx L4Idx = (PTIdx)(vaddr >> 39);
  PTIdx L3Idx = (PTIdx)((vaddr >> 30) & mask(9));
  PTIdx L2Idx = (PTIdx)((vaddr >> 21) & mask(9));

  PageTable *L4PT = getTopLevelPageTable();

  //printf("L4Idx: %llx\n", L4Idx);
  PageTable *L3PT = walkPageTable(L4PT, L4Idx);
  //printf("L3Idx: %llx\n", L3Idx);
  PageTable *L2PT = walkPageTable(L3PT, L3Idx);
  //printf("L2Idx: %llx\n", L2Idx);
  walkPageTable(L2PT, L2Idx);
  //printf("MMU Cache Hit: %lu\n", hit);
  //printf("MMU Cache Miss: %lu\n", miss);
  //printf("MMU Hit Rate: %f\n", (float)hit / ((float)hit + (float)miss));
}

MMUCache::PageTable*
MMUCache::walkPageTable(PageTable *pt, PTIdx idx) {
  MMUCacheTag tag = (idx << 3) + pt->startAddr;
  //printf("MMUCacheTag: %llx\n", tag);
  auto entry = lookup(tag);
  PageTable *nextPt = entry.second;
  if (nextPt == nullptr) {
    //printf("MMU Cache Miss!\n");
    miss++;
    // Generate a new page table for the first-touch page table
    if(pt->nPT.find(idx) == pt->nPT.end()) {
      Addr pageAddr = dist(e2);
      pageAddr &= ~mask(12);
      pageAddr &= mask(29);
      while(emulVirtMem.find(pageAddr) != emulVirtMem.end()) {
        pageAddr = dist(e2);
        pageAddr &= ~mask(12);
        pageAddr &= mask(30);
      }
      nextPt = new PageTable(pageAddr);
      emulVirtMem.insert(std::make_pair(pageAddr, nextPt));
      pt->nPT.insert(std::make_pair(idx, nextPt));
    } else {
      nextPt = pt->nPT[idx];
    }
    auto assoc = pseudoLRU->getVictim(0);
    cache[assoc] = std::make_pair(tag, nextPt);
    pseudoLRU->touch(0, assoc, Tick(hit + miss));

  } else {
    //printf("MMU Cache Hit!\n");
    hit++;
    pseudoLRU->touch(0, entry.first, Tick(hit + miss));
    nextPt = entry.second;
  }
  return nextPt;
}

std::pair<uint64_t, MMUCache::PageTable*>
MMUCache::lookup(MMUCacheTag tag) {
  for(int i = 0; i < size; i++) {
    if(cache[i].first == tag)
      return std::make_pair(i, cache[i].second);
  }
  return std::make_pair(-1, nullptr);
}

bool
MMUCache::isTagPresent(MMUCacheTag tag) {
  for(int i = 0; i < size; i++) {
    if(cache[i].first == tag)
      return true;
  }
  return false;
}
