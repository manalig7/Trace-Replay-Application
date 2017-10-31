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

#ifndef TRACE_REPLAY_PACKET_H
#define TRACE_REPLAY_PACKET_H

#include "ns3/nstime.h"
#include <iostream>
#include <vector>
#include <map>

namespace ns3 {

/**
 * \brief TraceReplayPacket is a packet container for TraceReplay.
 *
 * A TraceReplayPacket is defined by its size and delay (taken from input pcap).
 * Delay for a packet is application delay (inter packet gap greater than 1sec)
 * or user think times (such as inter packet gap between HTTP requests).
 * It is the idel time after which the packet was sent.
 * TraceReplay tries to schedule the packet after 'delay'
 * to create realistic simulation. 
 *
 * If the 'delay' is greater than 0 seconds, it also stores
 * list of bytes seen by all parallel connections (parallel 
 * to the connection to which packet belongs) till
 * the instant when packet was sent. This info is used
 * to check the progress of these parallel connections
 * during simulation. If any of the parallel connection
 * has not made desired progress, than packet will be delayed further. 
 *
 */
class TraceReplayPacket
{
public:
  TraceReplayPacket ();
  virtual ~TraceReplayPacket ();
  /**
   * \brief Returns the number of parallel connection for the packet
   *
   * \returns Count of parallel connections for the packet
   */
  uint32_t GetNumParallelConnection () const;
  /**
   * \brief Returns the #Bytes for i'th parallel connection
   *
   * \param i index of the parallel connection

   * \returns #Bytes for i'th parallel connection
   */
  uint32_t GetByteCount (uint32_t i) const;

  /**
   * \brief Returns source and destination port number of i'th parallel connection
   *
   * \param i index for m_parallelConnList
   *
   * \returns source and destination port number pair of i'th parallel connection
   */
  std::pair<uint16_t, uint16_t> GetConnectionId (uint32_t i) const;

  /**
   * \brief Add a parallel connection to m_parallelConnList
   *
   * Parallel connections are those connections for which source and
   * destination Ip's are same, but port number's are different.
   *
   * \param srcPort Source port number of parallel connection
   * \param dstPort Destination port number of parallel connection
   * \param byteCount #Bytes seen (sent + received) by connection at that moment
   */
  void AddParallelConnection (uint16_t srcPort, uint16_t dstPort, uint32_t byteCount);

  /**
   * \brief Set size of TraceReplayPacket
   *
   * \param size size of packet
   */
  void SetSize (uint32_t size);

  /**
   * \brief Set delay of TraceReplayPacket
   *
   * Delay for a packet is application delay (inter packet time gap greater than 1sec)
   * or user think times (such as inter packet gap between HTTP requests).
   * It is the idel time after which the packet was sent.
   * TraceReplay tries to schedule the packet after 'delay'
   * to create realistic simulation.
   *
   * \param delay delay of packet
   */
  void SetDelay (Time delay);

  /**
   * \brief Return size of TraceReplayPacket
   *
   * \returns Size of the packet
   */
  uint32_t GetSize () const;

  /**
   * \brief Return delay of TraceReplayPacket
   *
   * \returns delay for the packet
   */
  Time GetDelay () const;

  /**
   * \brief Returns byte count for connection between portClient and portServer
   *
   * \param portClient Port number of client
   * \param portServer Port number of server
   *
   * \returns total number of bytes seen (sent+recieved) by the connection in the pcap
   */
  uint32_t GetByteCount (uint16_t portClient, uint16_t portServer) const;

private:
  struct ParallelConnectionInfo
  {
    uint16_t       m_srcPort;     //!< client's port number of parallel connection
    uint16_t       m_dstPort;     //!< server's port number of parallel connection
    uint32_t       m_byteCount;   //!< #packet send by connection at that moment
  };
  uint32_t         m_size;        //!< Size of packet
  Time             m_delay;       //!< Delay of packet
  std::vector<ParallelConnectionInfo> m_parallelConnList;   //!< List of parallel connections for packet
};
} // namespace ns3
#endif