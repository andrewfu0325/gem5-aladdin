/*
 * Copyright (c) 2010 Gabe Black
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
 * Authors: Gabe Black
 */

#include "arch/x86/types.hh"
#include "sim/serialize.hh"

using namespace X86ISA;
using namespace std;

template <>
void
paramOut(ostream &os, const string &name, ExtMachInst const &machInst)
{
    // Prefixes
    paramOut(os, name + ".legacy", (uint8_t)machInst.legacy);
    paramOut(os, name + ".rex", (uint8_t)machInst.rex);

    // Opcode
    paramOut(os, name + ".opcode.num", machInst.opcode.num);
    paramOut(os, name + ".opcode.prefixA", machInst.opcode.prefixA);
    paramOut(os, name + ".opcode.prefixB", machInst.opcode.prefixB);
    paramOut(os, name + ".opcode.op", (uint8_t)machInst.opcode.op);

    // Modifier bytes
    paramOut(os, name + ".modRM", (uint8_t)machInst.modRM);
    paramOut(os, name + ".sib", (uint8_t)machInst.sib);

    // Immediate fields
    paramOut(os, name + ".immediate", machInst.immediate);
    paramOut(os, name + ".displacement", machInst.displacement);

    // Sizes
    paramOut(os, name + ".opSize", machInst.opSize);
    paramOut(os, name + ".addrSize", machInst.addrSize);
    paramOut(os, name + ".stackSize", machInst.stackSize);
    paramOut(os, name + ".dispSize", machInst.dispSize);

    // Mode
    paramOut(os, name + ".mode", (uint8_t)machInst.mode);
}

template <>
void
paramIn(Checkpoint *cp, const string &section,
        const string &name, ExtMachInst &machInst)
{
    uint8_t temp8;
    // Prefixes
    paramIn(cp, section, name + ".legacy", temp8);
    machInst.legacy = temp8;
    paramIn(cp, section, name + ".rex", temp8);
    machInst.rex = temp8;

    // Opcode
    paramIn(cp, section, name + ".opcode.num", machInst.opcode.num);
    paramIn(cp, section, name + ".opcode.prefixA", machInst.opcode.prefixA);
    paramIn(cp, section, name + ".opcode.prefixB", machInst.opcode.prefixB);
    paramIn(cp, section, name + ".opcode.op", temp8);
    machInst.opcode.op = temp8;

    // Modifier bytes
    paramIn(cp, section, name + ".modRM", temp8);
    machInst.modRM = temp8;
    paramIn(cp, section, name + ".sib", temp8);
    machInst.sib = temp8;;

    // Immediate fields
    paramIn(cp, section, name + ".immediate", machInst.immediate);
    paramIn(cp, section, name + ".displacement", machInst.displacement);

    // Sizes
    paramIn(cp, section, name + ".opSize", machInst.opSize);
    paramIn(cp, section, name + ".addrSize", machInst.addrSize);
    paramIn(cp, section, name + ".stackSize", machInst.stackSize);
    paramIn(cp, section, name + ".dispSize", machInst.dispSize);

    // Mode
    paramIn(cp, section, name + ".mode", temp8);
    machInst.mode = temp8;
}