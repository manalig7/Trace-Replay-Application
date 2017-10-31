/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Indian Institute of Technology Bombay
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Prakash Agrawal <prakashagr@cse.iitb.ac.in, prakash9752@gmail.com>
 *         Prof. Mythili Vutukuru <mythili@cse.iitb.ac.in>
 * Refrence: https://goo.gl/Z4ZW2K
 */

#include "trace-replay-utility.h"
namespace ns3 {

TraceReplayPacket::TraceReplayPacket ()
{
  m_size = 0;
  m_delay = Seconds (0);
}

TraceReplayPacket::~TraceReplayPacket ()
{
  m_parallelConnList.clear ();
}

uint32_t
TraceReplayPacket::GetByteCount (uint16_t portClient, uint16_t portServer) const
{
  uint32_t expectedByte = 0;
  for (uint32_t i = 0; i < m_parallelConnList.size (); i++)
    {
      if (m_parallelConnList[i].m_srcPort == portClient && m_parallelConnList[i].m_dstPort == portServer)
        {
          expectedByte = m_parallelConnList[i].m_byteCount;
          break;
        }
    }
  return expectedByte;
}

uint32_t
TraceReplayPacket::GetNumParallelConnection () const
{
  return m_parallelConnList.size ();
}

uint32_t
TraceReplayPacket::GetByteCount (uint32_t i) const
{
  return m_parallelConnList[i].m_byteCount;
}

std::pair<uint16_t, uint16_t>
TraceReplayPacket::GetConnectionId (uint32_t i) const
{
  return std::make_pair (m_parallelConnList[i].m_srcPort, m_parallelConnList[i].m_dstPort);
}

void
TraceReplayPacket::AddParallelConnection (uint16_t srcPort, uint16_t dstPort, uint32_t count)
{
  ParallelConnectionInfo parallelConn;
  parallelConn.m_srcPort = srcPort;
  parallelConn.m_dstPort = dstPort;
  parallelConn.m_byteCount = count;
  m_parallelConnList.push_back (parallelConn);
}

Time
TraceReplayPacket::GetDelay () const
{
  return m_delay;
}

uint32_t
TraceReplayPacket::GetSize () const
{
  return m_size;
}

void
TraceReplayPacket::SetDelay (Time delay)
{
  m_delay = delay;
}

void
TraceReplayPacket::SetSize (uint32_t size)
{
  m_size = size;
}

} // namespace ns3