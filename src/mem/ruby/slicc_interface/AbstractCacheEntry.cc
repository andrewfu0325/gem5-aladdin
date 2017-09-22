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

#include "mem/ruby/slicc_interface/AbstractCacheEntry.hh"

AbstractCacheEntry::AbstractCacheEntry()
{
    m_Permission = AccessPermission_NotPresent;
    m_Address.setAddress(0);
    m_locked = -1;
    enableCtr = 0;
}

AbstractCacheEntry::~AbstractCacheEntry()
{
}

void
AbstractCacheEntry::changePermission(AccessPermission new_perm)
{
    AbstractEntry::changePermission(new_perm);
    if ((new_perm == AccessPermission_Invalid) ||
        (new_perm == AccessPermission_NotPresent)) {
        m_locked = -1;
    }
}

void
AbstractCacheEntry::setCtr(int _enableCtr)
{  
  enableCtr = (bool)_enableCtr;
}

void 
AbstractCacheEntry::setVirtAddr(Packet *pkt)
{
    if(pkt->req->hasVaddr() && forwardAccTaskData) {
      Address reqVaddr(pkt->req->getVaddr());
      reqVaddr.makeLineAddress();
      m_vAddress = reqVaddr;
        /*if(cpuPtr->checkAccTaskData(pkt->req->getVaddr())) {
          printf("Set virtual Address for Acc-task data: %x\n", pkt->req->getVaddr());
          printf("Set virtual Address(Line) for Acc-task data: %x\n", m_vAddress.getAddress());
        }*/
        auto vaddr = m_vAddress.getAddress();
        if(cpuPtr->checkAccTaskData(vaddr) && pkt->isWrite()) {
          cpuPtr->incAccTaskDataInCache(vaddr);
          cpuPtr->incAccTaskDataCtr(vaddr, pkt->getSize());
        }
      hasVaddr = true;
    } else {
      hasVaddr = false;
    }
}

static unsigned accumBytes = 0;

bool 
AbstractCacheEntry::checkAccTaskData()
{
    if(forwardAccTaskData && hasVaddr) {
			auto range = cpuPtr->getAccTaskDataRange();
			auto vaddr = m_vAddress.getAddress();
			if(cpuPtr->checkAccTaskDataCtr(vaddr) && enableCtr) {
				printf("Evict req %x fall in Acc-task data range %x - %x(ctr enable)\n",
						vaddr, range.first, range.second); 
				return true;
			}
			if(cpuPtr->checkAccTaskData(vaddr)) {
        accumBytes += 64;
				printf("Evict req %x fall in Acc-task data range %x - %x(ctr disable), accum: %d\n",
						vaddr, range.first, range.second, accumBytes); 
        cpuPtr->decAccTaskDataInCache(vaddr);
        return true;
			}
			return false;
    } else {
        return false;
    }
}

Address 
AbstractCacheEntry::getVirtAddr()
{
    assert(hasVaddr);
    return m_vAddress;
}
