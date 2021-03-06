/*
 * Copyright (c) 2012-2013, 2015 ARM Limited
 * Copyright (c) 2015 Advanced Micro Devices, Inc.
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 *
 * Authors: Steve Reinhardt
 *          Kevin Lim
 */

#ifndef __SIM_SYSCALL_EMUL_HH__
#define __SIM_SYSCALL_EMUL_HH__

#define NO_STAT64 (defined(__APPLE__) || defined(__OpenBSD__) || \
  defined(__FreeBSD__) || defined(__CYGWIN__) || \
  defined(__NetBSD__))

///
/// @file syscall_emul.hh
///
/// This file defines objects used to emulate syscalls from the target
/// application on the host machine.

#ifdef __CYGWIN32__
#include <sys/fcntl.h>  // for O_BINARY
#endif
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <fcntl.h>

#include <cerrno>
#include <string>

#include "base/chunk_generator.hh"
#include "base/intmath.hh"      // for RoundUp
#include "base/misc.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/SyscallVerbose.hh"
#include "mem/page_table.hh"
#include "sim/byteswap.hh"
#include "sim/emul_driver.hh"
#include "sim/process.hh"
#include "sim/sim_exit.hh"
#include "sim/syscall_emul_buf.hh"
#include "sim/syscallreturn.hh"
#include "sim/system.hh"

#include "aladdin/gem5/aladdin_sys_connection.h"
#include "aladdin/gem5/aladdin_sys_constants.h"

///
/// System call descriptor.
///
class SyscallDesc {

  public:

    /// Typedef for target syscall handler functions.
    typedef SyscallReturn (*FuncPtr)(SyscallDesc *, int num,
                           LiveProcess *, ThreadContext *);

    const char *name;   //!< Syscall name (e.g., "open").
    FuncPtr funcPtr;    //!< Pointer to emulation function.
    int flags;          //!< Flags (see Flags enum).
    bool warned;        //!< Have we warned about unimplemented syscall?

    /// Flag values for controlling syscall behavior.
    enum Flags {
        /// Don't set return regs according to funcPtr return value.
        /// Used for syscalls with non-standard return conventions
        /// that explicitly set the ThreadContext regs (e.g.,
        /// sigreturn).
        SuppressReturnValue = 1,
        WarnOnce = 2
    };

    /// Constructor.
    SyscallDesc(const char *_name, FuncPtr _funcPtr, int _flags = 0)
        : name(_name), funcPtr(_funcPtr), flags(_flags), warned(false)
    {
    }

    /// Emulate the syscall.  Public interface for calling through funcPtr.
    void doSyscall(int callnum, LiveProcess *proc, ThreadContext *tc);

    /// Is the WarnOnce flag set?
    bool warnOnce() const {  return (flags & WarnOnce); }
};


//////////////////////////////////////////////////////////////////////
//
// The following emulation functions are generic enough that they
// don't need to be recompiled for different emulated OS's.  They are
// defined in sim/syscall_emul.cc.
//
//////////////////////////////////////////////////////////////////////


/// Handler for unimplemented syscalls that we haven't thought about.
SyscallReturn unimplementedFunc(SyscallDesc *desc, int num,
                                LiveProcess *p, ThreadContext *tc);

/// Handler for unimplemented syscalls that we never intend to
/// implement (signal handling, etc.) and should not affect the correct
/// behavior of the program.  Print a warning only if the appropriate
/// trace flag is enabled.  Return success to the target program.
SyscallReturn ignoreFunc(SyscallDesc *desc, int num,
                         LiveProcess *p, ThreadContext *tc);

/// Target exit() handler: terminate current context.
SyscallReturn exitFunc(SyscallDesc *desc, int num,
                       LiveProcess *p, ThreadContext *tc);

/// Target exit_group() handler: terminate simulation. (exit all threads)
SyscallReturn exitGroupFunc(SyscallDesc *desc, int num,
                       LiveProcess *p, ThreadContext *tc);

/// Target getpagesize() handler.
SyscallReturn getpagesizeFunc(SyscallDesc *desc, int num,
                              LiveProcess *p, ThreadContext *tc);

/// Target brk() handler: set brk address.
SyscallReturn brkFunc(SyscallDesc *desc, int num,
                      LiveProcess *p, ThreadContext *tc);

/// Target close() handler.
SyscallReturn closeFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);

/// Target read() handler.
SyscallReturn readFunc(SyscallDesc *desc, int num,
                       LiveProcess *p, ThreadContext *tc);

/// Target write() handler.
SyscallReturn writeFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);

/// Target lseek() handler.
SyscallReturn lseekFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);

/// Target _llseek() handler.
SyscallReturn _llseekFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);

/// Target gethostname() handler.
SyscallReturn gethostnameFunc(SyscallDesc *desc, int num,
                              LiveProcess *p, ThreadContext *tc);

/// Target getcwd() handler.
SyscallReturn getcwdFunc(SyscallDesc *desc, int num,
                         LiveProcess *p, ThreadContext *tc);

/// Target readlink() handler.
SyscallReturn readlinkFunc(SyscallDesc *desc, int num,
                           LiveProcess *p, ThreadContext *tc,
                           int index = 0);
SyscallReturn readlinkFunc(SyscallDesc *desc, int num,
                           LiveProcess *p, ThreadContext *tc);

/// Target unlink() handler.
SyscallReturn unlinkHelper(SyscallDesc *desc, int num,
                           LiveProcess *p, ThreadContext *tc,
                           int index);
SyscallReturn unlinkFunc(SyscallDesc *desc, int num,
                         LiveProcess *p, ThreadContext *tc);

/// Target mkdir() handler.
SyscallReturn mkdirFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);

/// Target rename() handler.
SyscallReturn renameFunc(SyscallDesc *desc, int num,
                         LiveProcess *p, ThreadContext *tc);


/// Target truncate() handler.
SyscallReturn truncateFunc(SyscallDesc *desc, int num,
                           LiveProcess *p, ThreadContext *tc);


/// Target ftruncate() handler.
SyscallReturn ftruncateFunc(SyscallDesc *desc, int num,
                            LiveProcess *p, ThreadContext *tc);


/// Target truncate64() handler.
SyscallReturn truncate64Func(SyscallDesc *desc, int num,
                             LiveProcess *p, ThreadContext *tc);

/// Target ftruncate64() handler.
SyscallReturn ftruncate64Func(SyscallDesc *desc, int num,
                              LiveProcess *p, ThreadContext *tc);


/// Target umask() handler.
SyscallReturn umaskFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);


/// Target chown() handler.
SyscallReturn chownFunc(SyscallDesc *desc, int num,
                        LiveProcess *p, ThreadContext *tc);


/// Target fchown() handler.
SyscallReturn fchownFunc(SyscallDesc *desc, int num,
                         LiveProcess *p, ThreadContext *tc);

/// Target dup() handler.
SyscallReturn dupFunc(SyscallDesc *desc, int num,
                      LiveProcess *process, ThreadContext *tc);

/// Target fcntl() handler.
SyscallReturn fcntlFunc(SyscallDesc *desc, int num,
                        LiveProcess *process, ThreadContext *tc);

/// Target fcntl64() handler.
SyscallReturn fcntl64Func(SyscallDesc *desc, int num,
                        LiveProcess *process, ThreadContext *tc);

// Aladdin handler function shared between 32-bit and 64-bit fcntl emulations.
void fcntlAladdinHandler(LiveProcess *process, ThreadContext *tc);

// Our cache forwarding mechanism for ACC-Task Data shared between 32-bit and 64-bit fcntl emulations.
void fcntlRegAccTaskDataForCache(LiveProcess *process, ThreadContext *tc);

/// Target setuid() handler.
SyscallReturn setuidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getpid() handler.
SyscallReturn getpidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getuid() handler.
SyscallReturn getuidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getgid() handler.
SyscallReturn getgidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getppid() handler.
SyscallReturn getppidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target geteuid() handler.
SyscallReturn geteuidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getegid() handler.
SyscallReturn getegidFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target clone() handler.
SyscallReturn cloneFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target access() handler
SyscallReturn accessFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);
SyscallReturn accessFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc,
                               int index);

/// Futex system call
///  Implemented by Daniel Sanchez
///  Used by printf's in multi-threaded apps
template <class OS>
SyscallReturn
futexFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
          ThreadContext *tc)
{
    int index_uaddr = 0;
    int index_op = 1;
    int index_val = 2;
    int index_timeout = 3;

    uint64_t uaddr = process->getSyscallArg(tc, index_uaddr);
    int op = process->getSyscallArg(tc, index_op);
    int val = process->getSyscallArg(tc, index_val);
    uint64_t timeout = process->getSyscallArg(tc, index_timeout);

    std::map<uint64_t, std::list<ThreadContext *> * >
        &futex_map = tc->getSystemPtr()->futexMap;

    DPRINTF(SyscallVerbose, "In sys_futex: Address=%llx, op=%d, val=%d\n",
            uaddr, op, val);

    op &= ~OS::TGT_FUTEX_PRIVATE_FLAG;

    if (op == OS::TGT_FUTEX_WAIT) {
        if (timeout != 0) {
            warn("sys_futex: FUTEX_WAIT with non-null timeout unimplemented;"
                 "we'll wait indefinitely");
        }

        uint8_t *buf = new uint8_t[sizeof(int)];
        tc->getMemProxy().readBlob((Addr)uaddr, buf, (int)sizeof(int));
        int mem_val = *((int *)buf);
        delete buf;

        if(val != mem_val) {
            DPRINTF(SyscallVerbose, "sys_futex: FUTEX_WAKE, read: %d, "
                                    "expected: %d\n", mem_val, val);
            return -OS::TGT_EWOULDBLOCK;
        }

        // Queue the thread context
        std::list<ThreadContext *> * tcWaitList;
        if (futex_map.count(uaddr)) {
            tcWaitList = futex_map.find(uaddr)->second;
        } else {
            tcWaitList = new std::list<ThreadContext *>();
            futex_map.insert(std::pair< uint64_t,
                            std::list<ThreadContext *> * >(uaddr, tcWaitList));
        }
        tcWaitList->push_back(tc);
        DPRINTF(SyscallVerbose, "sys_futex: FUTEX_WAIT, suspending calling "
                                "thread context\n");
        tc->suspend();
        return 0;
    } else if (op == OS::TGT_FUTEX_WAKE){
        int wokenUp = 0;
        std::list<ThreadContext *> * tcWaitList;
        if (futex_map.count(uaddr)) {
            tcWaitList = futex_map.find(uaddr)->second;
            while (tcWaitList->size() > 0 && wokenUp < val) {
                tcWaitList->front()->activate();
                tcWaitList->pop_front();
                wokenUp++;
            }
            if(tcWaitList->empty()) {
                futex_map.erase(uaddr);
                delete tcWaitList;
            }
        }
        DPRINTF(SyscallVerbose, "sys_futex: FUTEX_WAKE, activated %d waiting "
                                "thread contexts\n", wokenUp);
        return wokenUp;
    } else {
        warn("sys_futex: op %d is not implemented, just returning...", op);
        return 0;
    }

}


/// Pseudo Funcs  - These functions use a different return convension,
/// returning a second value in a register other than the normal return register
SyscallReturn pipePseudoFunc(SyscallDesc *desc, int num,
                             LiveProcess *process, ThreadContext *tc);

/// Target getpidPseudo() handler.
SyscallReturn getpidPseudoFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getuidPseudo() handler.
SyscallReturn getuidPseudoFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);

/// Target getgidPseudo() handler.
SyscallReturn getgidPseudoFunc(SyscallDesc *desc, int num,
                               LiveProcess *p, ThreadContext *tc);


/// A readable name for 1,000,000, for converting microseconds to seconds.
const int one_million = 1000000;
/// A readable name for 1,000,000,000, for converting nanoseconds to seconds.
const int one_billion = 1000000000;

/// Approximate seconds since the epoch (1/1/1970).  About a billion,
/// by my reckoning.  We want to keep this a constant (not use the
/// real-world time) to keep simulations repeatable.
const unsigned seconds_since_epoch = 1000000000;

/// Helper function to convert current elapsed time to seconds and
/// microseconds.
template <class T1, class T2>
void
getElapsedTimeMicro(T1 &sec, T2 &usec)
{
    uint64_t elapsed_usecs = curTick() / SimClock::Int::us;
    sec = elapsed_usecs / one_million;
    usec = elapsed_usecs % one_million;
}

/// Helper function to convert current elapsed time to seconds and
/// nanoseconds.
template <class T1, class T2>
void
getElapsedTimeNano(T1 &sec, T2 &nsec)
{
    uint64_t elapsed_nsecs = curTick() / SimClock::Int::ns;
    sec = elapsed_nsecs / one_billion;
    nsec = elapsed_nsecs % one_billion;
}

//////////////////////////////////////////////////////////////////////
//
// The following emulation functions are generic, but need to be
// templated to account for differences in types, constants, etc.
//
//////////////////////////////////////////////////////////////////////

#if NO_STAT64
    typedef struct stat hst_stat;
    typedef struct stat hst_stat64;
#else
    typedef struct stat hst_stat;
    typedef struct stat64 hst_stat64;
#endif

//// Helper function to convert a host stat buffer to a target stat
//// buffer.  Also copies the target buffer out to the simulated
//// memory space.  Used by stat(), fstat(), and lstat().

template <typename target_stat, typename host_stat>
static void
convertStatBuf(target_stat &tgt, host_stat *host, bool fakeTTY = false)
{
    using namespace TheISA;

    if (fakeTTY)
        tgt->st_dev = 0xA;
    else
        tgt->st_dev = host->st_dev;
    tgt->st_dev = TheISA::htog(tgt->st_dev);
    tgt->st_ino = host->st_ino;
    tgt->st_ino = TheISA::htog(tgt->st_ino);
    tgt->st_mode = host->st_mode;
    if (fakeTTY) {
        // Claim to be a character device
        tgt->st_mode &= ~S_IFMT;    // Clear S_IFMT
        tgt->st_mode |= S_IFCHR;    // Set S_IFCHR
    }
    tgt->st_mode = TheISA::htog(tgt->st_mode);
    tgt->st_nlink = host->st_nlink;
    tgt->st_nlink = TheISA::htog(tgt->st_nlink);
    tgt->st_uid = host->st_uid;
    tgt->st_uid = TheISA::htog(tgt->st_uid);
    tgt->st_gid = host->st_gid;
    tgt->st_gid = TheISA::htog(tgt->st_gid);
    if (fakeTTY)
        tgt->st_rdev = 0x880d;
    else
        tgt->st_rdev = host->st_rdev;
    tgt->st_rdev = TheISA::htog(tgt->st_rdev);
    tgt->st_size = host->st_size;
    tgt->st_size = TheISA::htog(tgt->st_size);
    tgt->st_atimeX = host->st_atime;
    tgt->st_atimeX = TheISA::htog(tgt->st_atimeX);
    tgt->st_mtimeX = host->st_mtime;
    tgt->st_mtimeX = TheISA::htog(tgt->st_mtimeX);
    tgt->st_ctimeX = host->st_ctime;
    tgt->st_ctimeX = TheISA::htog(tgt->st_ctimeX);
    // Force the block size to be 8k. This helps to ensure buffered io works
    // consistently across different hosts.
    tgt->st_blksize = 0x2000;
    tgt->st_blksize = TheISA::htog(tgt->st_blksize);
    tgt->st_blocks = host->st_blocks;
    tgt->st_blocks = TheISA::htog(tgt->st_blocks);
}

// Same for stat64

template <typename target_stat, typename host_stat64>
static void
convertStat64Buf(target_stat &tgt, host_stat64 *host, bool fakeTTY = false)
{
    using namespace TheISA;

    convertStatBuf<target_stat, host_stat64>(tgt, host, fakeTTY);
#if defined(STAT_HAVE_NSEC)
    tgt->st_atime_nsec = host->st_atime_nsec;
    tgt->st_atime_nsec = TheISA::htog(tgt->st_atime_nsec);
    tgt->st_mtime_nsec = host->st_mtime_nsec;
    tgt->st_mtime_nsec = TheISA::htog(tgt->st_mtime_nsec);
    tgt->st_ctime_nsec = host->st_ctime_nsec;
    tgt->st_ctime_nsec = TheISA::htog(tgt->st_ctime_nsec);
#else
    tgt->st_atime_nsec = 0;
    tgt->st_mtime_nsec = 0;
    tgt->st_ctime_nsec = 0;
#endif
}

//Here are a couple convenience functions
template<class OS>
static void
copyOutStatBuf(SETranslatingPortProxy &mem, Addr addr,
        hst_stat *host, bool fakeTTY = false)
{
    typedef TypedBufferArg<typename OS::tgt_stat> tgt_stat_buf;
    tgt_stat_buf tgt(addr);
    convertStatBuf<tgt_stat_buf, hst_stat>(tgt, host, fakeTTY);
    tgt.copyOut(mem);
}

template<class OS>
static void
copyOutStat64Buf(SETranslatingPortProxy &mem, Addr addr,
        hst_stat64 *host, bool fakeTTY = false)
{
    typedef TypedBufferArg<typename OS::tgt_stat64> tgt_stat_buf;
    tgt_stat_buf tgt(addr);
    convertStat64Buf<tgt_stat_buf, hst_stat64>(tgt, host, fakeTTY);
    tgt.copyOut(mem);
}

/// Target ioctl() handler.  For the most part, programs call ioctl()
/// only to find out if their stdout is a tty, to determine whether to
/// do line or block buffering.  We always claim that output fds are
/// not TTYs to provide repeatable results.
template <class OS>
SyscallReturn
ioctlFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
          ThreadContext *tc)
{
    int index = 0;
    int fd = process->getSyscallArg(tc, index);
    unsigned req = process->getSyscallArg(tc, index);

    DPRINTF(SyscallVerbose, "ioctl(%d, 0x%x, ...)\n", fd, req);

    Process::FdMap *fdObj = process->sim_fd_obj(fd);

    if ((fdObj == NULL || fd < 0 || process->sim_fd(fd) < 0) && fd != ALADDIN_FD) {
        // doesn't map to any simulator fd: not a valid target fd
        return -EBADF;
    }

    if (OS::isTtyReq(req)) {
        return -ENOTTY;
    }

    if (fd == ALADDIN_FD) {
      if (req == DUMP_STATS || req == RESET_STATS) {
        size_t max_desc_len = 100;

        // Read the description string out of simulated memory.  We make the
        // char buffer one character longer than the max length so that we can
        // set the last character to the terminating character in case the
        // string actually exceeds the max length allowed.
        Addr desc_addr = (Addr) process->getSyscallArg(tc, index);
        std::string stat_final_desc;
        if (desc_addr != 0) {
          SETranslatingPortProxy& memProxy = tc->getMemProxy();
          uint8_t* desc_buf = new uint8_t[max_desc_len+2];
          memProxy.readBlob(desc_addr, desc_buf, max_desc_len);
          desc_buf[max_desc_len] = static_cast<uint8_t>(0);

          char* stats_desc = (char*) desc_buf;
          stat_final_desc = stats_desc;
        }

        // Create the final string to pass to exitSimLoop.
        std::string exit_sim_loop_reason = (req == DUMP_STATS) ?
            DUMP_STATS_EXIT_SIM_SIGNAL + stat_final_desc :
            RESET_STATS_EXIT_SIM_SIGNAL + stat_final_desc;

        exitSimLoop(exit_sim_loop_reason);
      } else {
        // Translate the finish flag pointer to a physical address that Aladdin
        // will write to when execution is completed.
        Addr paddr;
        Addr finish_flag = (Addr) process->getSyscallArg(tc, index);
        process->pTable->translate(finish_flag, paddr);
        // We need the context and thread id of the calling thread.
        process->system->activateAccelerator(
            req, paddr, tc->contextId(), tc->threadId());
      }
      return -ENOTTY;
    }

    if (fdObj->driver != NULL) {
        return fdObj->driver->ioctl(process, tc, req);
    }

    warn("Unsupported ioctl call: ioctl(%d, 0x%x, ...) @ \n",
         fd, req, tc->pcState());
    return -ENOTTY;
}

template <class OS>
static SyscallReturn
openFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
         ThreadContext *tc, int index)
{
    std::string path;

    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index)))
        return -EFAULT;

    int tgtFlags = process->getSyscallArg(tc, index);
    int mode = process->getSyscallArg(tc, index);
    int hostFlags = 0;

    // translate open flags
    for (int i = 0; i < OS::NUM_OPEN_FLAGS; i++) {
        if (tgtFlags & OS::openFlagTable[i].tgtFlag) {
            tgtFlags &= ~OS::openFlagTable[i].tgtFlag;
            hostFlags |= OS::openFlagTable[i].hostFlag;
        }
    }

    // any target flags left?
    if (tgtFlags != 0)
        warn("Syscall: open: cannot decode flags 0x%x", tgtFlags);

#ifdef __CYGWIN32__
    hostFlags |= O_BINARY;
#endif

    // Adjust path for current working directory
    path = process->fullPath(path);

    DPRINTF(SyscallVerbose, "opening file %s\n", path.c_str());

    if (startswith(path, "/dev/")) {
        std::string filename = path.substr(strlen("/dev/"));
        if (filename == "sysdev0") {
            // This is a memory-mapped high-resolution timer device on Alpha.
            // We don't support it, so just punt.
            warn("Ignoring open(%s, ...)\n", path);
            return -ENOENT;
        }

        EmulatedDriver *drv = process->findDriver(filename);
        if (drv != NULL) {
            // the driver's open method will allocate a fd from the
            // process if necessary.
            return drv->open(process, tc, mode, hostFlags);
        }

        // fall through here for pass through to host devices, such as
        // /dev/zero
    }

    int fd;
    int local_errno;
    if (startswith(path, "/proc/") || startswith(path, "/system/") ||
        startswith(path, "/platform/") || startswith(path, "/sys/")) {
        // It's a proc/sys entry and requires special handling
        fd = OS::openSpecialFile(path, process, tc);
        local_errno = ENOENT;
     } else {
        // open the file
        fd = open(path.c_str(), hostFlags, mode);
        local_errno = errno;
     }

    if (fd == -1)
        return -local_errno;

    return process->alloc_fd(fd, path.c_str(), hostFlags, mode, false);
}

/// Target open() handler.
template <class OS>
SyscallReturn
openFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
         ThreadContext *tc)
{
    return openFunc<OS>(desc, callnum, process, tc, 0);
}

/// Target openat() handler.
template <class OS>
SyscallReturn
openatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
         ThreadContext *tc)
{
    int index = 0;
    int dirfd = process->getSyscallArg(tc, index);
    if (dirfd != OS::TGT_AT_FDCWD)
        warn("openat: first argument not AT_FDCWD; unlikely to work");
    return openFunc<OS>(desc, callnum, process, tc, 1);
}

/// Target unlinkat() handler.
template <class OS>
SyscallReturn
unlinkatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
             ThreadContext *tc)
{
    int index = 0;
    int dirfd = process->getSyscallArg(tc, index);
    if (dirfd != OS::TGT_AT_FDCWD)
        warn("unlinkat: first argument not AT_FDCWD; unlikely to work");

    return unlinkHelper(desc, callnum, process, tc, 1);
}

/// Target facessat() handler
template <class OS>
SyscallReturn
faccessatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
        ThreadContext *tc)
{
    int index = 0;
    int dirfd = process->getSyscallArg(tc, index);
    if (dirfd != OS::TGT_AT_FDCWD)
        warn("faccessat: first argument not AT_FDCWD; unlikely to work");
    return accessFunc(desc, callnum, process, tc, 1);
}

/// Target readlinkat() handler
template <class OS>
SyscallReturn
readlinkatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
        ThreadContext *tc)
{
    int index = 0;
    int dirfd = process->getSyscallArg(tc, index);
    if (dirfd != OS::TGT_AT_FDCWD)
        warn("openat: first argument not AT_FDCWD; unlikely to work");
    return readlinkFunc(desc, callnum, process, tc, 1);
}

/// Target renameat() handler.
template <class OS>
SyscallReturn
renameatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
             ThreadContext *tc)
{
    int index = 0;

    int olddirfd = process->getSyscallArg(tc, index);
    if (olddirfd != OS::TGT_AT_FDCWD)
        warn("renameat: first argument not AT_FDCWD; unlikely to work");

    std::string old_name;

    if (!tc->getMemProxy().tryReadString(old_name,
                                         process->getSyscallArg(tc, index)))
        return -EFAULT;

    int newdirfd = process->getSyscallArg(tc, index);
    if (newdirfd != OS::TGT_AT_FDCWD)
        warn("renameat: third argument not AT_FDCWD; unlikely to work");

    std::string new_name;

    if (!tc->getMemProxy().tryReadString(new_name,
                                         process->getSyscallArg(tc, index)))
        return -EFAULT;

    // Adjust path for current working directory
    old_name = process->fullPath(old_name);
    new_name = process->fullPath(new_name);

    int result = rename(old_name.c_str(), new_name.c_str());
    return (result == -1) ? -errno : result;
}

/// Target sysinfo() handler.
template <class OS>
SyscallReturn
sysinfoFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
         ThreadContext *tc)
{

    int index = 0;
    TypedBufferArg<typename OS::tgt_sysinfo>
        sysinfo(process->getSyscallArg(tc, index));

    sysinfo->uptime=seconds_since_epoch;
    sysinfo->totalram=process->system->memSize();

    sysinfo.copyOut(tc->getMemProxy());

    return 0;
}

/// Target chmod() handler.
template <class OS>
SyscallReturn
chmodFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
          ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }

    uint32_t mode = process->getSyscallArg(tc, index);
    mode_t hostMode = 0;

    // XXX translate mode flags via OS::something???
    hostMode = mode;

    // Adjust path for current working directory
    path = process->fullPath(path);

    // do the chmod
    int result = chmod(path.c_str(), hostMode);
    if (result < 0)
        return -errno;

    return 0;
}


/// Target fchmod() handler.
template <class OS>
SyscallReturn
fchmodFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    int index = 0;
    int fd = process->getSyscallArg(tc, index);
    if (fd < 0 || process->sim_fd(fd) < 0) {
        // doesn't map to any simulator fd: not a valid target fd
        return -EBADF;
    }

    uint32_t mode = process->getSyscallArg(tc, index);
    mode_t hostMode = 0;

    // XXX translate mode flags via OS::someting???
    hostMode = mode;

    // do the fchmod
    int result = fchmod(process->sim_fd(fd), hostMode);
    if (result < 0)
        return -errno;

    return 0;
}

/// Target mremap() handler.
template <class OS>
SyscallReturn
mremapFunc(SyscallDesc *desc, int callnum, LiveProcess *process, ThreadContext *tc)
{
    int index = 0;
    Addr start = process->getSyscallArg(tc, index);
    uint64_t old_length = process->getSyscallArg(tc, index);
    uint64_t new_length = process->getSyscallArg(tc, index);
    uint64_t flags = process->getSyscallArg(tc, index);
    uint64_t provided_address = 0;
    bool use_provided_address = flags & OS::TGT_MREMAP_FIXED;

    if (use_provided_address)
        provided_address = process->getSyscallArg(tc, index);

    if ((start % TheISA::PageBytes != 0) ||
        (provided_address % TheISA::PageBytes != 0)) {
        warn("mremap failing: arguments not page aligned");
        return -EINVAL;
    }

    new_length = roundUp(new_length, TheISA::PageBytes);

    if (new_length > old_length) {
        if ((start + old_length) == process->mmap_end &&
            (!use_provided_address || provided_address == start)) {
            uint64_t diff = new_length - old_length;
            process->allocateMem(process->mmap_end, diff);
            process->mmap_end += diff;
            return start;
        } else {
            if (!use_provided_address && !(flags & OS::TGT_MREMAP_MAYMOVE)) {
                warn("can't remap here and MREMAP_MAYMOVE flag not set\n");
                return -ENOMEM;
            } else {
                uint64_t new_start = use_provided_address ?
                    provided_address : process->mmap_end;
                process->pTable->remap(start, old_length, new_start);
                warn("mremapping to new vaddr %08p-%08p, adding %d\n",
                     new_start, new_start + new_length,
                     new_length - old_length);
                // add on the remaining unallocated pages
                process->allocateMem(new_start + old_length,
                                     new_length - old_length,
                                     use_provided_address /* clobber */);
                if (!use_provided_address)
                    process->mmap_end += new_length;
                if (use_provided_address &&
                    new_start + new_length > process->mmap_end) {
                    // something fishy going on here, at least notify the user
                    // @todo: increase mmap_end?
                    warn("mmap region limit exceeded with MREMAP_FIXED\n");
                }
                warn("returning %08p as start\n", new_start);
                return new_start;
            }
        }
    } else {
        if (use_provided_address && provided_address != start)
            process->pTable->remap(start, new_length, provided_address);
        process->pTable->unmap(start + new_length, old_length - new_length);
        return use_provided_address ? provided_address : start;
    }
}

/// Target stat() handler.
template <class OS>
SyscallReturn
statFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
         ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

    struct stat hostBuf;
    int result = stat(path.c_str(), &hostBuf);

    if (result < 0)
        return -errno;

    copyOutStatBuf<OS>(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}


/// Target stat64() handler.
template <class OS>
SyscallReturn
stat64Func(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index)))
        return -EFAULT;
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

#if NO_STAT64
    struct stat  hostBuf;
    int result = stat(path.c_str(), &hostBuf);
#else
    struct stat64 hostBuf;
    int result = stat64(path.c_str(), &hostBuf);
#endif

    if (result < 0)
        return -errno;

    copyOutStat64Buf<OS>(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}


/// Target fstatat64() handler.
template <class OS>
SyscallReturn
fstatat64Func(SyscallDesc *desc, int callnum, LiveProcess *process,
              ThreadContext *tc)
{
    int index = 0;
    int dirfd = process->getSyscallArg(tc, index);
    if (dirfd != OS::TGT_AT_FDCWD)
        warn("fstatat64: first argument not AT_FDCWD; unlikely to work");

    std::string path;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index)))
        return -EFAULT;
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

#if NO_STAT64
    struct stat  hostBuf;
    int result = stat(path.c_str(), &hostBuf);
#else
    struct stat64 hostBuf;
    int result = stat64(path.c_str(), &hostBuf);
#endif

    if (result < 0)
        return -errno;

    copyOutStat64Buf<OS>(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}


/// Target fstat64() handler.
template <class OS>
SyscallReturn
fstat64Func(SyscallDesc *desc, int callnum, LiveProcess *process,
            ThreadContext *tc)
{
    int index = 0;
    int fd = process->getSyscallArg(tc, index);
    Addr bufPtr = process->getSyscallArg(tc, index);
    if (fd < 0 || process->sim_fd(fd) < 0) {
        // doesn't map to any simulator fd: not a valid target fd
        return -EBADF;
    }

#if NO_STAT64
    struct stat  hostBuf;
    int result = fstat(process->sim_fd(fd), &hostBuf);
#else
    struct stat64  hostBuf;
    int result = fstat64(process->sim_fd(fd), &hostBuf);
#endif

    if (result < 0)
        return -errno;

    copyOutStat64Buf<OS>(tc->getMemProxy(), bufPtr, &hostBuf, (fd == 1));

    return 0;
}


/// Target lstat() handler.
template <class OS>
SyscallReturn
lstatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
          ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

    struct stat hostBuf;
    int result = lstat(path.c_str(), &hostBuf);

    if (result < 0)
        return -errno;

    copyOutStatBuf<OS>(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}

/// Target lstat64() handler.
template <class OS>
SyscallReturn
lstat64Func(SyscallDesc *desc, int callnum, LiveProcess *process,
            ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

#if NO_STAT64
    struct stat hostBuf;
    int result = lstat(path.c_str(), &hostBuf);
#else
    struct stat64 hostBuf;
    int result = lstat64(path.c_str(), &hostBuf);
#endif

    if (result < 0)
        return -errno;

    copyOutStat64Buf<OS>(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}

/// Target fstat() handler.
template <class OS>
SyscallReturn
fstatFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
          ThreadContext *tc)
{
    int index = 0;
    int fd = process->sim_fd(process->getSyscallArg(tc, index));
    Addr bufPtr = process->getSyscallArg(tc, index);

    DPRINTF(SyscallVerbose, "fstat(%d, ...)\n", fd);

    if (fd < 0)
        return -EBADF;

    struct stat hostBuf;
    int result = fstat(fd, &hostBuf);

    if (result < 0)
        return -errno;

    copyOutStatBuf<OS>(tc->getMemProxy(), bufPtr, &hostBuf, (fd == 1));

    return 0;
}


/// Target statfs() handler.
template <class OS>
SyscallReturn
statfsFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }
    Addr bufPtr = process->getSyscallArg(tc, index);

    // Adjust path for current working directory
    path = process->fullPath(path);

    struct statfs hostBuf;
    int result = statfs(path.c_str(), &hostBuf);

    if (result < 0)
        return -errno;

    OS::copyOutStatfsBuf(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}


/// Target fstatfs() handler.
template <class OS>
SyscallReturn
fstatfsFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
            ThreadContext *tc)
{
    int index = 0;
    int fd = process->sim_fd(process->getSyscallArg(tc, index));
    Addr bufPtr = process->getSyscallArg(tc, index);

    if (fd < 0)
        return -EBADF;

    struct statfs hostBuf;
    int result = fstatfs(fd, &hostBuf);

    if (result < 0)
        return -errno;

    OS::copyOutStatfsBuf(tc->getMemProxy(), bufPtr, &hostBuf);

    return 0;
}


/// Target writev() handler.
template <class OS>
SyscallReturn
writevFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    int index = 0;
    int fd = process->getSyscallArg(tc, index);
    if (fd < 0 || process->sim_fd(fd) < 0) {
        // doesn't map to any simulator fd: not a valid target fd
        return -EBADF;
    }

    SETranslatingPortProxy &p = tc->getMemProxy();
    uint64_t tiov_base = process->getSyscallArg(tc, index);
    size_t count = process->getSyscallArg(tc, index);
    struct iovec hiov[count];
    for (size_t i = 0; i < count; ++i) {
        typename OS::tgt_iovec tiov;

        p.readBlob(tiov_base + i*sizeof(typename OS::tgt_iovec),
                   (uint8_t*)&tiov, sizeof(typename OS::tgt_iovec));
        hiov[i].iov_len = TheISA::gtoh(tiov.iov_len);
        hiov[i].iov_base = new char [hiov[i].iov_len];
        p.readBlob(TheISA::gtoh(tiov.iov_base), (uint8_t *)hiov[i].iov_base,
                   hiov[i].iov_len);
    }

    int result = writev(process->sim_fd(fd), hiov, count);

    for (size_t i = 0; i < count; ++i)
        delete [] (char *)hiov[i].iov_base;

    if (result < 0)
        return -errno;

    return result;
}


/* madvise syscall that just ignores the advice given. */
template <class OS>
SyscallReturn
madviseFunc(SyscallDesc *desc, int num, LiveProcess *p, ThreadContext *tc)
{
  return 0;
}

/// Target mmap() handler.
///
/// We don't really handle mmap().  If the target is mmaping an
/// anonymous region or /dev/zero, we can get away with doing basically
/// nothing (since memory is initialized to zero and the simulator
/// doesn't really check addresses anyway).
///
template <class OS>
SyscallReturn
mmapFunc(SyscallDesc *desc, int num, LiveProcess *p, ThreadContext *tc)
{
    int index = 0;
    Addr start = p->getSyscallArg(tc, index);
    uint64_t length = p->getSyscallArg(tc, index);
    index++; // int prot = p->getSyscallArg(tc, index);
    int flags = p->getSyscallArg(tc, index);
    int tgt_fd = p->getSyscallArg(tc, index);
    int offset = p->getSyscallArg(tc, index);

    if (length > 0x100000000ULL)
        warn("mmap length argument %#x is unreasonably large.\n", length);

    if (!(flags & OS::TGT_MAP_ANONYMOUS)) {
        Process::FdMap *fd_map = p->sim_fd_obj(tgt_fd);
        if (!fd_map || fd_map->fd < 0) {
            warn("mmap failing: target fd %d is not valid\n", tgt_fd);
            return -EBADF;
        }

        if (fd_map->filename != "/dev/zero") {
            // This is very likely broken, but leave a warning here
            // (rather than panic) in case /dev/zero is known by
            // another name on some platform
            warn("allowing mmap of file %s; mmap not supported on files"
                 " other than /dev/zero\n", fd_map->filename);
        }
    }

    length = roundUp(length, TheISA::PageBytes);

    if ((start  % TheISA::PageBytes) != 0 ||
        (offset % TheISA::PageBytes) != 0) {
        warn("mmap failing: arguments not page-aligned: "
             "start 0x%x offset 0x%x",
             start, offset);
        return -EINVAL;
    }

    // are we ok with clobbering existing mappings?  only set this to
    // true if the user has been warned.
    bool clobber = false;

    // try to use the caller-provided address if there is one
    bool use_provided_address = (start != 0);

    if (use_provided_address) {
        // check to see if the desired address is already in use
        if (!p->pTable->isUnmapped(start, length)) {
            // there are existing mappings in the desired range
            // whether we clobber them or not depends on whether the caller
            // specified MAP_FIXED
            if (flags & OS::TGT_MAP_FIXED) {
                // MAP_FIXED specified: map attempt fails
                return -EINVAL;
            } else {
                // MAP_FIXED not specified: ignore suggested start address
                warn("mmap: ignoring suggested map address 0x%x\n", start);
                use_provided_address = false;
            }
        }
    }

    if (!use_provided_address) {
        // no address provided, or provided address unusable:
        // pick next address from our "mmap region"
        if (OS::mmapGrowsDown()) {
            start = p->mmap_end - length;
            p->mmap_end = start;
        } else {
            start = p->mmap_end;
            p->mmap_end += length;
        }
    }

    p->allocateMem(start, length, clobber);

    // Copy the file contents to the simulated memory space.
    int sim_fd = p->sim_fd(tgt_fd);
    if (tgt_fd != -1 && sim_fd != -1) {
      DPRINTF(SyscallVerbose, "mmap sim_fd = %d, tgt_fd = %d. Allocated "
              "memory from %#x-%#x.\n", sim_fd, tgt_fd, start, start+length);
      int npages = divCeil(length, (int64_t)TheISA::PageBytes);
      uint8_t* buf = new uint8_t[npages * TheISA::PageBytes];

      // We need to preserve the user's view of the file descriptor's offset,
      // so we get the current offset, move the fd to the mmap argument's
      // offset, read the whole file, and then restore the original offset.
      off_t curr_fd_offset = lseek(sim_fd, 0, SEEK_CUR);
      lseek(sim_fd, offset, SEEK_SET);
      ssize_t success = read(sim_fd, buf, npages * TheISA::PageBytes);
      if (success == -1) {
        perror("Could not read from mmapped file");
        fatal("mmap: failed for fd %d.", tgt_fd);
      }
      lseek(sim_fd, curr_fd_offset, SEEK_SET);

      // Write to simulated memory space.
      SETranslatingPortProxy& memProxy = tc->getMemProxy();
      memProxy.writeBlob(start, buf, npages * TheISA::PageBytes);
      delete[] buf;
    } else {
      DPRINTF(SyscallVerbose, "mmap sim_fd = %d, tgt_fd = %d, so "
              "file contents are not being copied.\n", sim_fd, tgt_fd);
    }

    return start;
}


/// Target munmap() handler.
template <class OS>
SyscallReturn
munmapFunc(SyscallDesc *desc, int num, LiveProcess *p, ThreadContext *tc)
{
    int index = 0;
    Addr addr = p->getSyscallArg(tc, index);
    size_t len = p->getSyscallArg(tc, index);

    if (addr % TheISA::PageBytes != 0 ||
        len % TheISA::PageBytes != 0) {
        warn("mmap failing: arguments not page-aligned: "
             "start 0x%x length 0x%x", addr, len);
        return -EINVAL;
    }
    DPRINTF(SyscallVerbose, "munmap region from %#x-%#x.\n", addr, addr+len);

    // Remove entries from the page table.
    p->pTable->unmap(addr, len);

    // We can't reclaim space from the "mmap region" because another mmap call
    // could have incremented the head pointer, and moving the head pointer
    // could then cause collisions with regions that are already mapped. gem5
    // does't do mmap correctly, basically, and for the size of inputs that
    // we're using, this won't matter.
    return 0;
}

/// Target getrlimit() handler.
template <class OS>
SyscallReturn
getrlimitFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
        ThreadContext *tc)
{
    int index = 0;
    unsigned resource = process->getSyscallArg(tc, index);
    TypedBufferArg<typename OS::rlimit> rlp(process->getSyscallArg(tc, index));

    switch (resource) {
        case OS::TGT_RLIMIT_STACK:
            // max stack size in bytes: make up a number (8MB for now)
            rlp->rlim_cur = rlp->rlim_max = 8 * 1024 * 1024;
            rlp->rlim_cur = TheISA::htog(rlp->rlim_cur);
            rlp->rlim_max = TheISA::htog(rlp->rlim_max);
            break;

        case OS::TGT_RLIMIT_DATA:
            // max data segment size in bytes: make up a number
            rlp->rlim_cur = rlp->rlim_max = 256 * 1024 * 1024;
            rlp->rlim_cur = TheISA::htog(rlp->rlim_cur);
            rlp->rlim_max = TheISA::htog(rlp->rlim_max);
            break;

        default:
            warn("getrlimit: unimplemented resource %d", resource);
            return -EINVAL;
            break;
    }

    rlp.copyOut(tc->getMemProxy());
    return 0;
}

/// Target clock_gettime() function.
template <class OS>
SyscallReturn
clock_gettimeFunc(SyscallDesc *desc, int num, LiveProcess *p, ThreadContext *tc)
{
    int index = 1;
    //int clk_id = p->getSyscallArg(tc, index);
    TypedBufferArg<typename OS::timespec> tp(p->getSyscallArg(tc, index));

    getElapsedTimeNano(tp->tv_sec, tp->tv_nsec);
    tp->tv_sec += seconds_since_epoch;
    tp->tv_sec = TheISA::htog(tp->tv_sec);
    tp->tv_nsec = TheISA::htog(tp->tv_nsec);

    tp.copyOut(tc->getMemProxy());

    return 0;
}

/// Target gettimeofday() handler.
template <class OS>
SyscallReturn
gettimeofdayFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
        ThreadContext *tc)
{
    int index = 0;
    TypedBufferArg<typename OS::timeval> tp(process->getSyscallArg(tc, index));

    getElapsedTimeMicro(tp->tv_sec, tp->tv_usec);
    tp->tv_sec += seconds_since_epoch;
    tp->tv_sec = TheISA::htog(tp->tv_sec);
    tp->tv_usec = TheISA::htog(tp->tv_usec);

    tp.copyOut(tc->getMemProxy());

    return 0;
}


/// Target utimes() handler.
template <class OS>
SyscallReturn
utimesFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    std::string path;

    int index = 0;
    if (!tc->getMemProxy().tryReadString(path,
                process->getSyscallArg(tc, index))) {
        return -EFAULT;
    }

    TypedBufferArg<typename OS::timeval [2]>
        tp(process->getSyscallArg(tc, index));
    tp.copyIn(tc->getMemProxy());

    struct timeval hostTimeval[2];
    for (int i = 0; i < 2; ++i)
    {
        hostTimeval[i].tv_sec = TheISA::gtoh((*tp)[i].tv_sec);
        hostTimeval[i].tv_usec = TheISA::gtoh((*tp)[i].tv_usec);
    }

    // Adjust path for current working directory
    path = process->fullPath(path);

    int result = utimes(path.c_str(), hostTimeval);

    if (result < 0)
        return -errno;

    return 0;
}
/// Target getrusage() function.
template <class OS>
SyscallReturn
getrusageFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
              ThreadContext *tc)
{
    int index = 0;
    int who = process->getSyscallArg(tc, index); // THREAD, SELF, or CHILDREN
    TypedBufferArg<typename OS::rusage> rup(process->getSyscallArg(tc, index));

    rup->ru_utime.tv_sec = 0;
    rup->ru_utime.tv_usec = 0;
    rup->ru_stime.tv_sec = 0;
    rup->ru_stime.tv_usec = 0;
    rup->ru_maxrss = 0;
    rup->ru_ixrss = 0;
    rup->ru_idrss = 0;
    rup->ru_isrss = 0;
    rup->ru_minflt = 0;
    rup->ru_majflt = 0;
    rup->ru_nswap = 0;
    rup->ru_inblock = 0;
    rup->ru_oublock = 0;
    rup->ru_msgsnd = 0;
    rup->ru_msgrcv = 0;
    rup->ru_nsignals = 0;
    rup->ru_nvcsw = 0;
    rup->ru_nivcsw = 0;

    switch (who) {
      case OS::TGT_RUSAGE_SELF:
        getElapsedTimeMicro(rup->ru_utime.tv_sec, rup->ru_utime.tv_usec);
        rup->ru_utime.tv_sec = TheISA::htog(rup->ru_utime.tv_sec);
        rup->ru_utime.tv_usec = TheISA::htog(rup->ru_utime.tv_usec);
        break;

      case OS::TGT_RUSAGE_CHILDREN:
        // do nothing.  We have no child processes, so they take no time.
        break;

      default:
        // don't really handle THREAD or CHILDREN, but just warn and
        // plow ahead
        warn("getrusage() only supports RUSAGE_SELF.  Parameter %d ignored.",
             who);
    }

    rup.copyOut(tc->getMemProxy());

    return 0;
}

/// Target times() function.
template <class OS>
SyscallReturn
timesFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    int index = 0;
    TypedBufferArg<typename OS::tms> bufp(process->getSyscallArg(tc, index));

    // Fill in the time structure (in clocks)
    int64_t clocks = curTick() * OS::M5_SC_CLK_TCK / SimClock::Int::s;
    bufp->tms_utime = clocks;
    bufp->tms_stime = 0;
    bufp->tms_cutime = 0;
    bufp->tms_cstime = 0;

    // Convert to host endianness
    bufp->tms_utime = TheISA::htog(bufp->tms_utime);

    // Write back
    bufp.copyOut(tc->getMemProxy());

    // Return clock ticks since system boot
    return clocks;
}

/// Target time() function.
template <class OS>
SyscallReturn
timeFunc(SyscallDesc *desc, int callnum, LiveProcess *process,
           ThreadContext *tc)
{
    typename OS::time_t sec, usec;
    getElapsedTimeMicro(sec, usec);
    sec += seconds_since_epoch;

    int index = 0;
    Addr taddr = (Addr)process->getSyscallArg(tc, index);
    if(taddr != 0) {
        typename OS::time_t t = sec;
        t = TheISA::htog(t);
        SETranslatingPortProxy &p = tc->getMemProxy();
        p.writeBlob(taddr, (uint8_t*)&t, (int)sizeof(typename OS::time_t));
    }
    return sec;
}

template <class OS>
SyscallReturn
clockGetResFunc(SyscallDesc *desc, int callnum, LiveProcess *p, ThreadContext *tc)
{
  typename OS::time_t sec;
  int index = 1;
  Addr timespec_addr = (Addr) p->getSyscallArg(tc, index);
  if (timespec_addr != 0) {
    SETranslatingPortProxy &proxy = tc->getMemProxy();
    /* Use a fixed clock precision. */
    sec = 0;
    long nsec = 1;
    proxy.writeBlob(timespec_addr, (uint8_t*)&sec, (int) sizeof(typename OS::time_t));
    proxy.writeBlob(timespec_addr + sizeof(typename OS::time_t),
                    (uint8_t *)&nsec, (int)sizeof(long));
  }
  return 0;
}

#endif // __SIM_SYSCALL_EMUL_HH__
