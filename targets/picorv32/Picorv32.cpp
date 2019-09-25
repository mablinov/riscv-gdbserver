// GDB RSP server PicoRV32 CPU model wrapper: definition

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
// Contributor Ian Bolton <ian.bolton@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <chrono>
#include <cstdint>
#include <iostream>

#include "Picorv32.h"
#include "Picorv32Impl.h"
#include "Vtestbench_testbench.h"

#include "Vtestbench_picorv32__C1_EF1_EH1_EE1.h"

using std::chrono::duration;
using std::chrono::system_clock;
using std::chrono::time_point;

// The program counter is handled a little differently to the rest of the
// register file on Picorv32.
static const int RISCV_PC_REGNUM   = 32;

// Run for 10000 cycles at a time during continued execution.
static const size_t RUN_SAMPLE_PERIOD = 10000;


//! @param[in] wantVcd  TRUE if we want a VCD generated, false otherwise.

Picorv32::Picorv32 (TraceFlags * flags) :
  ITarget (flags),
  mServer (nullptr),
  mFlags (flags)
{
  mPicorv32Impl = new Picorv32Impl (flags);

}	// Picorv32::Picorv32 ()


Picorv32::~Picorv32()
{
  delete mPicorv32Impl;
}

ITarget::ResumeRes
Picorv32::resume (ResumeType step)
{
  return resume(step, duration <double>::zero ());
}

ITarget::ResumeRes
Picorv32::resume (ResumeType step,
        std::chrono::duration <double> timeout)
{
  time_point <system_clock, duration <double> > timeout_end =
    system_clock::now () + timeout;

  switch (step)
  {
  case ResumeType::STEP:
    if (mPicorv32Impl->step ())
    {
      return ResumeRes::TIMEOUT;
    } else {
      return ResumeRes::INTERRUPTED;
    }
    break;
  case ResumeType::CONTINUE:
    for (;;)
    {
      for (size_t i = 0; i < RUN_SAMPLE_PERIOD; i++)
      {
        if (mPicorv32Impl->step ())
        {
          return ResumeRes::INTERRUPTED;
        }
      }

      if (timeout_end < system_clock::now ())
      {
        return ResumeRes::TIMEOUT;
      }
    }
    break;
  case ResumeType::STOP:
    // Do nothing. We are already "stopped"?
    break;
  }
  return ResumeRes::NONE;
}

ITarget::ResumeRes
Picorv32::terminate ()
{
  // FIXME: Any action required here? I don't think so..
  return ResumeRes::NONE;
}

ITarget::ResumeRes
Picorv32::reset (ITarget::ResetType  type  __attribute__ ((unused)) )
{
  delete mPicorv32Impl;
  mPicorv32Impl = new Picorv32Impl (mFlags);

  if (mPicorv32Impl)
  {
    return ResumeRes::SUCCESS;
  } else {
    return ResumeRes::FAILURE;
  }
}

uint64_t
Picorv32::getCycleCount () const
{
  return mPicorv32Impl->getCycleCount ();
}
uint64_t
Picorv32::getInstrCount () const
{
  return mPicorv32Impl->getInstrCount ();
}

std::size_t
Picorv32::readRegister (const int reg, uint32_t & value) const
{
  if (RISCV_PC_REGNUM == reg)
  {
    value = mPicorv32Impl->readProgramAddr ();
  } else {
    value = mPicorv32Impl->readReg(reg);
  }

  return 4;
}

std::size_t
Picorv32::writeRegister (const int reg, const uint32_t  value)
{
  if (RISCV_PC_REGNUM == reg) {
    mPicorv32Impl->writeProgramAddr (value);
  } else {
    mPicorv32Impl->writeReg(reg, value);
  }
  return 4;
}

std::size_t
Picorv32::read (const uint32_t addr,
                uint8_t * buffer,
                const std::size_t  size) const
{
  size_t i;
  for (i = 0; i < size; i++)
    buffer[i] = mPicorv32Impl->readMem (addr + i);
  return i;
}

std::size_t
Picorv32::write (const uint32_t addr,
                 const uint8_t * buffer,
                 const std::size_t size)
{
  size_t i;
  for (i = 0; i < size; i++)
    mPicorv32Impl->writeMem (addr + i, buffer[i]);
  return i;
}

bool
Picorv32::insertMatchpoint (const uint32_t  addr, const MatchType matchType)
{
  std::cerr << "insertMatchpoint NOT IMPLEMENTED" << std::endl;
  return false;
}

bool
Picorv32::removeMatchpoint (const uint32_t  addr, const MatchType matchType)
{
  std::cerr << "removeMatchpoint NOT IMPLEMENTED" << std::endl;
  return false;
}

bool
Picorv32::command (const std::string cmd, std::ostream & stream)
{
  std::cerr << "command NOT IMPLEMENTED" << std::endl;
  return false;
}


void
Picorv32::gdbServer (GdbServer *server)
{
  mServer = server;
}


//! Return a timestamp.

//! This is needed to support the $time function in Verilog.  This in turn is
//! needed for VCD output.

//! Pass through to the implementation class.

//! @return  The current simulation time in seconds.

double
Picorv32::timeStamp ()
{
  return mPicorv32Impl->timeStamp ();

}	// Picorv32::timeStamp ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
