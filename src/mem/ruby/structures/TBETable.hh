/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MEM_RUBY_STRUCTURES_TBETABLE_HH__
#define __MEM_RUBY_STRUCTURES_TBETABLE_HH__

#include <iostream>

#include "base/hashmap.hh"
#include "mem/ruby/common/Address.hh"

extern int DmaReadHit;
extern int DmaReadMiss;
extern std::unordered_set<uint64_t> Evictions;

template<class ENTRY>
class TBETable
{
  public:
    TBETable(int number_of_TBEs)
        : m_number_of_TBEs(number_of_TBEs)
    {
    }

    bool isPresent(const Address& address) const;
    void allocate(const Address& address);
    void deallocate(const Address& address);
    bool
    areNSlotsAvailable(int n) const
    {
        return (m_number_of_TBEs - m_map.size()) >= n;
    }

    ENTRY* lookup(const Address& address);

    // Print cache contents
    void print(std::ostream& out) const;
    void dmaReadHit(const Address& address){
        printf("Hit: 0x%x\n", address.getAddress());
        DmaReadHit++;
    }
    void dmaReadMiss(const Address& address){
        printf("Miss: 0x%x\n", address.getAddress());
        DmaReadMiss++;
    }
    void outstandingReq(){
        printf("Outstanding Req: %d/%d\n", m_map.size(), m_number_of_TBEs);
    }
    // Record Evictions
    void insertEviction(const Address& address){
            printf("Maximum size of set: %u\n", Evictions.max_size());
            printf("size of set: %u\n", Evictions.size());
            printf("Evict: 0x%x\n", address.getAddress());
            Evictions.insert(address.getAddress());
    }
  private:
    // Private copy constructor and assignment operator
    TBETable(const TBETable& obj);
    TBETable& operator=(const TBETable& obj);

    // Data Members (m_prefix)
    m5::hash_map<Address, ENTRY> m_map;


  private:
    int m_number_of_TBEs;
};

template<class ENTRY>
inline std::ostream&
operator<<(std::ostream& out, const TBETable<ENTRY>& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

template<class ENTRY>
inline bool
TBETable<ENTRY>::isPresent(const Address& address) const
{
    assert(address == line_address(address));
    assert(m_map.size() <= m_number_of_TBEs);
    return !!m_map.count(address);
}

template<class ENTRY>
inline void
TBETable<ENTRY>::allocate(const Address& address)
{
//    printf("Allocated Address: %#8x\n", address.getAddress());
    assert(!isPresent(address));
    assert(m_map.size() < m_number_of_TBEs);
    m_map[address] = ENTRY();
//    printf("Outstanding Req: %d/%d\n", m_map.size(), m_number_of_TBEs);
}

template<class ENTRY>
inline void
TBETable<ENTRY>::deallocate(const Address& address)
{
//    printf("Deallocated Address: %#8x\n", address.getAddress());
    assert(isPresent(address));
    assert(m_map.size() > 0);
    m_map.erase(address);
//    printf("Outstanding Req: %d/%d\n", m_map.size(), m_number_of_TBEs);
}

// looks an address up in the cache
template<class ENTRY>
inline ENTRY*
TBETable<ENTRY>::lookup(const Address& address)
{
  if(m_map.find(address) != m_map.end()) return &(m_map.find(address)->second);
  return NULL;
}


template<class ENTRY>
inline void
TBETable<ENTRY>::print(std::ostream& out) const
{
}

#endif // __MEM_RUBY_STRUCTURES_TBETABLE_HH__
