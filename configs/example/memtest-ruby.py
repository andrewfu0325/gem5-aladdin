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
# Authors: Ron Dreslinski
#          Brad Beckmann

import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath
import os, optparse, sys
addToPath('../common')
addToPath('../../tests/configs/')
import ruby_config

parser = optparse.OptionParser()

parser.add_option("-a", "--atomic", action="store_true",
                  help="Use atomic (non-timing) mode")
parser.add_option("-b", "--blocking", action="store_true",
                  help="Use blocking caches")
parser.add_option("-l", "--maxloads", metavar="N", default=0,
                  help="Stop after N loads")
parser.add_option("-m", "--maxtick", type="int", default=m5.MaxTick,
                  metavar="T",
                  help="Stop after T ticks")

parser.add_option("-t", "--testers", type="int", metavar="N", default=1,
                  help="number of testers/cores")

parser.add_option("-f", "--functional", type="int", default=0,
                  metavar="PCT",
                  help="Target percentage of functional accesses "
                  "[default: %default]")
parser.add_option("-u", "--uncacheable", type="int", default=0,
                  metavar="PCT",
                  help="Target percentage of uncacheable accesses "
                  "[default: %default]")

parser.add_option("--progress", type="int", default=1000,
                  metavar="NLOADS",
                  help="Progress message interval "
                  "[default: %default]")

(options, args) = parser.parse_args()

if args:
     print "Error: script doesn't take any positional arguments"
     sys.exit(1)

block_size = 64

if options.testers > block_size:
     print "Error: Number of testers %d limited to %d because of false sharing" \
           % (options.testers, block_size)
     sys.exit(1)

cpus = [ MemTest(atomic=options.atomic, max_loads=options.maxloads, \
                 percent_functional=options.functional, \
                 percent_uncacheable=options.uncacheable, \
                 progress_interval=options.progress) \
         for i in xrange(options.testers) ]

# create the desired simulated system
# ruby memory must be at least 16 MB to work with the mem tester
ruby_memory = ruby_config.generate("MI_example-homogeneous.rb",
                                   cores = options.testers,
                                   memory_size = 16,
                                   ports_per_cpu = 1)

system = System(cpu = cpus, funcmem = PhysicalMemory(),
                physmem = ruby_memory)

for cpu in cpus:
     cpu.test = system.physmem.port
     cpu.functional = system.funcmem.port


# -----------------------
# run simulation
# -----------------------

root = Root( system = system )
if options.atomic:
    root.system.mem_mode = 'atomic'
else:
    root.system.mem_mode = 'timing'

# Not much point in this being higher than the L1 latency
m5.ticks.setGlobalFrequency('1ns')

# instantiate configuration
m5.instantiate(root)

# simulate until program terminates
exit_event = m5.simulate(options.maxtick)

print 'Exiting @ tick', m5.curTick(), 'because', exit_event.getCause()
