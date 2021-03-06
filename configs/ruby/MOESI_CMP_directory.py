# Copyright (c) 2006-2007 The Regents of The University of Michigan
# Copyright (c) 2009 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Brad Beckmann

import math
import m5
from m5.objects import *
from m5.defines import buildEnv
from Ruby import create_topology
from Ruby import send_evicts
from Cluster import Cluster 

#
# Note: the L1 Cache latency is only used by the sequencer on fast path hits
#
class L1Cache(RubyCache):
    latency = 3 # dummy, for Seqs only

#
# Note: the L2 Cache latency is not currently used
#
class L2Cache(RubyCache):
    latency = 15 # dummy, only for Seqs, L2 is not connected with Seqs

def define_options(parser):
    return

def create_system(options, full_system, system, dma_ports, ruby_system):

    if buildEnv['PROTOCOL'] != 'MOESI_CMP_directory':
        panic("This script requires the MOESI_CMP_directory protocol to be built.")

    cpu_sequencers = []
    accel_sequencers = []
    
    #
    # The ruby network creation expects the list of nodes in the system to be
    # consistent with the NetDest list.  Therefore the l1 controller nodes must be
    # listed before the directory nodes and directory nodes before dma nodes, etc.
    #
    l1_cntrl_nodes = []
    accel_l1_cntrl_nodes = []
    l2_cntrl_nodes = []
    dir_cntrl_nodes = []
    dma_cntrl_nodes = []

    #
    # Must create the individual controllers before the network to ensure the
    # controller constructors are called before the network constructor
    #
    #l2_bits = int(math.log(options.num_l2caches, 2))
    block_size_bits = int(math.log(options.cacheline_size, 2))
    # Add one L2 cache for every 4-additional cores
    #assert(options.num_l2caches == options.num_cpus / 4 + (1 if options.num_cpus % 4 else 0))

    cpu_clusters = [Cluster() for i in xrange(options.num_l2caches)]
    dir_cluster = Cluster()
    ClusterNum = 0;

    for i in xrange(options.num_cpus):
        #
        # First create the Ruby objects associated with this cpu
        #

        # Group 4 cores into one clutser
        # ClusterNum = i / 4;

        l1i_cache = L1Cache(size = options.l1i_size,
                            assoc = options.l1i_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = True,
                            dataArrayBanks = options.l1i_banks,
                            tagArrayBanks = options.l1i_banks,
                            resourceStalls = False)
        l1d_cache = L1Cache(size = options.l1d_size,
                            assoc = options.l1d_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = False,
                            dataArrayBanks = options.l1d_banks,
                            tagArrayBanks = options.l1d_banks,
                            resourceStalls = False)

        l1_cntrl = L1Cache_Controller(version = i,
                                      L1Icache = l1i_cache,
                                      L1Dcache = l1d_cache,
                                      l2_select_num_bits = 0,
                                      ClusterNum = ClusterNum,
                                      send_evictions = send_evicts(options),
                                      transitions_per_cycle = options.ports,
                                      clk_domain=system.cpu[i].clk_domain,
                                      ruby_system = ruby_system,
                                      number_of_TBEs = options.l1_mshrs,
                                      enableCtr = options.enableCtr)

        cpu_seq = RubySequencer(version = i,
                                icache = l1i_cache,
                                dcache = l1d_cache,
                                clk_domain=system.cpu[i].clk_domain,
                                ruby_system = ruby_system)

        l1_cntrl.sequencer = cpu_seq
        exec("ruby_system.l1_cntrl%d = l1_cntrl" % i)

        # Add controllers and sequencers to the appropriate lists
        cpu_sequencers.append(cpu_seq)
        l1_cntrl_nodes.append(l1_cntrl)

        # Connect the L1 controllers and the network
        l1_cntrl.requestFromL1Cache =  ruby_system.network.slave
        l1_cntrl.responseFromL1Cache =  ruby_system.network.slave
        l1_cntrl.requestToL1Cache =  ruby_system.network.master
        l1_cntrl.responseToL1Cache =  ruby_system.network.master

        # Add l1_cntrl to cpu_cluster for later CrossBar creation
        cpu_clusters[ClusterNum].add(l1_cntrl)

    # Add accelerators to Ruby interconnection
    
    for i in xrange(len(system.datapaths)):
        #
        # First create the Ruby objects associated with this accelerator
        #
        #ClusterNum = i / 4;

        l1i_cache = L1Cache(size = system.datapaths[i].cacheSize,
                            assoc = options.l1i_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = True,
                            dataArrayBanks = options.l1i_banks,
                            tagArrayBanks = options.l1i_banks,
                            resourceStalls = False)
        accel_l1d_cache = L1Cache(size = system.datapaths[i].cacheSize,
                            assoc = options.l1d_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = False,
                            dataArrayBanks = options.l1d_banks,
                            tagArrayBanks = options.l1d_banks,
                            resourceStalls = False)

        accel_l1_cntrl = L1Cache_Controller(version = i + options.num_cpus,
                                      L1Icache = l1i_cache,
                                      L1Dcache = accel_l1d_cache,
                                      l2_select_num_bits = 0,
                                      ClusterNum = ClusterNum,
                                      send_evictions = send_evicts(options),
                                      transitions_per_cycle = options.ports,
                                      clk_domain=system.datapaths[i].clk_domain,
                                      ruby_system = ruby_system,
                                      number_of_TBEs = options.l1_mshrs)

        accel_seq = RubySequencer(version = i + options.num_cpus,
                                icache = l1i_cache,
                                dcache = accel_l1d_cache,
                                clk_domain=system.datapaths[i].clk_domain,
                                ruby_system = ruby_system)

        accel_l1_cntrl.sequencer = accel_seq
        exec("ruby_system.accel_l1_cntrl%d = accel_l1_cntrl" % (i + options.num_cpus))

        # Add controllers and sequencers to the appropriate lists
        accel_sequencers.append(accel_seq)
        accel_l1_cntrl_nodes.append(accel_l1_cntrl)

        # Connect the L1 controllers and the network
        accel_l1_cntrl.requestFromL1Cache =  ruby_system.network.slave
        accel_l1_cntrl.responseFromL1Cache =  ruby_system.network.slave
        accel_l1_cntrl.requestToL1Cache =  ruby_system.network.master
        accel_l1_cntrl.responseToL1Cache =  ruby_system.network.master
    
        # Add accel_l1_cntrl to corresponding cpu_cluster
        cpu_clusters[ClusterNum].add(accel_l1_cntrl)


    #l2_index_start = block_size_bits + l2_bits
    l2_index_start = block_size_bits

    for i in xrange(options.num_l2caches):
        #
        # First create the Ruby objects associated with this cpu
        #

        l2_cache = L2Cache(size = options.l2_size,
                           tagAccessLatency = options.l2_tag_latency,
                           dataAccessLatency = options.l2_data_latency,
                           assoc = options.l2_assoc,
                           start_index_bit = l2_index_start,
                           dataArrayBanks = options.l2_banks,
                           tagArrayBanks = options.l2_banks,
                           resourceStalls = False)

        l2_cntrl = L2Cache_Controller(version = i,
                                      L2cache = l2_cache,
                                      transitions_per_cycle = options.ports,
                                      ruby_system = ruby_system,
                                      number_of_TBEs = options.l2_mshrs)
        
        exec("ruby_system.l2_cntrl%d = l2_cntrl" % i)
        l2_cntrl_nodes.append(l2_cntrl)

        # Connect the L2 controllers and the network
        l2_cntrl.GlobalRequestFromL2Cache = ruby_system.network.slave
        l2_cntrl.L1RequestFromL2Cache = ruby_system.network.slave
        l2_cntrl.responseFromL2Cache = ruby_system.network.slave

        l2_cntrl.GlobalRequestToL2Cache = ruby_system.network.master
        l2_cntrl.L1RequestToL2Cache = ruby_system.network.master
        l2_cntrl.responseToL2Cache = ruby_system.network.master

        # Add l2_cntrl to corresponding cpu_cluster
        #cpu_clusters[i].add(l2_cntrl)
        dir_cluster.add(l2_cntrl)


    phys_mem_size = sum(map(lambda r: r.size(), system.mem_ranges))
    assert(phys_mem_size % options.num_dirs == 0)
    mem_module_size = phys_mem_size / options.num_dirs


    # Run each of the ruby memory controllers at a ratio of the frequency of
    # the ruby system.
    # clk_divider value is a fix to pass regression.
    ruby_system.memctrl_clk_domain = DerivedClockDomain(
                                          clk_domain=ruby_system.clk_domain,
                                          clk_divider=3)

    dir_size = MemorySize('0B')
    dir_size.value = mem_module_size
    pf_size = MemorySize(options.dir_size)
    pf_size.value = pf_size.value * 2
    dir_bits = int(math.log(options.num_dirs, 2))
    pf_bits = int(math.log(pf_size.value, 2))

    if dir_bits > 0:
        pf_start_bit = dir_bits + block_size_bits - 1
    else:
        pf_start_bit = block_size_bits


    for i in xrange(options.num_dirs):

        pf = RubyCache(
            tagAccessLatency = options.dir_latency,
            dataAccessLatency = options.dir_latency,
            dataArrayBanks = options.dir_banks,
            tagArrayBanks = options.dir_banks,
            latency = 1, # dummy
    	      size = '1GB',
            assoc = options.dir_assoc,
            resourceStalls = False)

        dir_cntrl = Directory_Controller(version = i,
                                         directory = RubyDirectoryMemory(
                                             version = i, size = dir_size),
                                         probeFilter = pf,
                                         transitions_per_cycle = options.ports,
                                         ruby_system = ruby_system,
                                         number_of_TBEs = options.dir_mshrs,
                                         probe_filter_enabled = False)

        exec("ruby_system.dir_cntrl%d = dir_cntrl" % i)
        dir_cntrl_nodes.append(dir_cntrl)

        # Connect the directory controllers and the network
        dir_cntrl.requestToDir = ruby_system.network.master
        dir_cntrl.responseToDir = ruby_system.network.master
        dir_cntrl.responseFromDir = ruby_system.network.slave
        dir_cntrl.forwardFromDir = ruby_system.network.slave

        dir_cluster.add(dir_cntrl)


    for i, dma_port in enumerate(dma_ports):
        #
        # Create the Ruby objects associated with the dma controller
        #

        dma_seq = DMASequencer(version = i,
                               ruby_system = ruby_system,
                               slave = dma_port,
                               max_outstanding_requests = options.dma_outstanding_requests)
        
        dma_cntrl = DMA_Controller(version = i,
                                   dma_sequencer = dma_seq,
                                   transitions_per_cycle = options.ports,
                                   ruby_system = ruby_system,
                                   number_of_TBEs = options.dma_outstanding_requests)

        exec("ruby_system.dma_cntrl%d = dma_cntrl" % i)
        dma_cntrl_nodes.append(dma_cntrl)

        # Connect the dma controller to the network
        dma_cntrl.responseFromDir = ruby_system.network.master
        dma_cntrl.reqToDir = ruby_system.network.slave
        dma_cntrl.respToDir = ruby_system.network.slave

        dir_cluster.add(dma_cntrl)

    all_cntrls = l1_cntrl_nodes + \
                 accel_l1_cntrl_nodes + \
                 l2_cntrl_nodes + \
                 dir_cntrl_nodes + \
                 dma_cntrl_nodes

    # Create the io controller and the sequencer
    if full_system:
        io_seq = DMASequencer(version=len(dma_ports), ruby_system=ruby_system)
        ruby_system._io_port = io_seq
        io_controller = DMA_Controller(version = len(dma_ports),
                                       dma_sequencer = io_seq,
                                       ruby_system = ruby_system)
        ruby_system.io_controller = io_controller

        # Connect the dma controller to the network
        io_controller.responseFromDir = ruby_system.network.master
        io_controller.reqToDir = ruby_system.network.slave
        io_controller.respToDir = ruby_system.network.slave

        all_cntrls = all_cntrls + [io_controller]

    for i in xrange(len(cpu_clusters)):
        dir_cluster.add(cpu_clusters[i]) 

    return (cpu_sequencers, accel_sequencers,dir_cntrl_nodes, dma_cntrl_nodes, dir_cluster)
