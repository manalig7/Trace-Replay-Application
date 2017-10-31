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

#ifndef TRACE_REPLAY_CLIENT_H
#define TRACE_REPLAY_CLIENT_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include <vector>
#include <cstdlib>
#include <map>

namespace ns3 {

class Address;
class Socket;
class TraceReplayPacket;

/**
 * \ingroup applications
 * \class TraceReplayClient
 * \brief TraceReplayClient class acts as a tcp client for TraceReplay.
 *
 * TraceReplayClient acts as a tcp client, which initializes connection to
 * TraceReplayServer and communicates with it in a request-reply mode.
 * In each cycle, it sends *m_numReq number of packets, from m_packetList,
 * as request and waits for server to send *m_expByte as reply.
 * Before sending a packet, if the delay for the packet > 0 seconds,
 * it also checks the progress of all parallel connections
 * between same client IP and server IP.
 * If any of the parallel connection has not made sufficient progress
 * then it reschedules the packet for later time,
 * otherwise continues normal operation.
 *
 */

class TraceReplayClient : public Application
{

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TraceReplayClient ();

  ~TraceReplayClient ();

  /**
   * \brief This method initializes the connection's ids (real ip and port address of client and server).
   *
   * \param ipClient real ip address of client
   * \param portClient real port number of client
   * \param ipServer real ip address of corresponding server
   * \param portServer real port number of corresponding server
   */
  void SetConnectionId (Address ipClient, uint16_t portClient, Address ipServer, uint16_t portServer);

  /**
   * \brief This method initializes the client object.
   *
   * \param address peer address
   * \param dataRate The datarate of the sever
   * \param numReq Vector containing number of packet to send in each request
   * \param expByte Vector containing the number of bytes expected as reply for each request
   * \param packetList Vector containing client's TraceReplayPacket
   */
  void Setup (Address address, DataRate dataRate, std::vector<uint32_t> numReq, std::vector<uint32_t> expByte, std::vector<TraceReplayPacket> packetList);

  /**
   * \brief Returns real Ip address of server in the original connection
   *
   * \returns Address of server in trace file
   */
  Address GetIpServer (void) const;

  /**
   * \brief Returns port number of server in the original connection
   *
   * \returns Port number of server in trace file
   */
  uint16_t GetPortServer (void) const;

  /**
   * \brief Returns port number of client in the original connection
   *
   * \returns Port number of client in trace file
   */
  uint16_t GetPortClient (void) const;

  /**
   * \brief Returns total number of bytes seen in the connection (sent + received)
   *
   * \returns Total number of bytes seen in the connection (sent + received)
   */
  uint32_t GetTotalByteCount (void) const;

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class
  virtual void StartApplication (void); // Called at time specified by Start
  virtual void StopApplication (void);  // Called at time specified by Stop
  virtual void DoInitialize (void);

  /**
   * \brief Connection Succeeded (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Connection Failed (called by Socket through a callback)
   * \param socket the connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  /**
   * \brief This method schedules next packet for sending
   *
   * If there is a packet in send queue (ie *numReq != 0), schedule
   * it for sending. If delay for the packet is > 0 seconds
   * then schedule the packet after that delay.
   * Otherwise update the m_totExpByte and go to receive mode.
   *
   */
  void ScheduleTx (void);
  /**
   * \brief This method is called to send packet
   *
   * If the delay (packet.delay) for the packet is > 0 Seconds, then
   * check all the parallel connections (packet.m_parallelConnList) between
   * same client IP and server IP. If any of the parallel connection has
   * not made desired progress or there is not enough buffer space available
   * then reschedule the packet for later time.
   * After sending the packet check whether there are more packet in send
   * queue (ie m_numReq != 0), if so then call ScheduleTx again.
   * Otherwise update m_expByte and go to receive mode.
   *
   * \param packet TraceReplayPacket containg packet details
   */
  void SendPacket (TraceReplayPacket packet);

  /**
   * \brief This method is to receive a packet
   *
   * Receive packets from server.
   * If m_totRecByte < m_totExpByte, wait for server to send more packet.
   * After that if there are packets in send queue, call ScheduleTx,
   * otherwise close the connection.
   *
   * \param socket Associated socket
   */
  void ReceivePacket (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  DataRate        m_dataRate;     //!< Data rate
  EventId         m_sendEvent;    //!< Event Id of SendPacket
  bool            m_connected;    //!< True if running
  Address         m_ipClient;     //!< Real IP address of client
  uint16_t        m_portClient;   //!< Real port address of client
  Address         m_ipServer;     //!< Real IP address of server
  uint16_t        m_portServer;   //!< Real port address of server
  uint32_t        m_totRecByte;   //!< Total number of bytes recieved so far
  uint32_t        m_totExpByte;   //!< Total number of bytes expected to receive
  uint32_t        m_totByteCount; //!< Total number of bytes seen in connection (sent + received)

  std::vector<uint32_t>             m_numReq;       //!< List of #packets to send as requests
  std::vector<uint32_t>             m_expByte;      //!< List of total bytes expected to receive for each set of request
  std::vector<TraceReplayPacket>    m_packetList;   //!< List of packets

  std::vector<uint32_t>::iterator             m_numReqIt;        //!< m_numReq iterator
  std::vector<uint32_t>::iterator             m_expByteIt;       //!< m_expByte iterator
  std::vector<TraceReplayPacket>::iterator    m_packetListIt;    //!< m_packetList iterator
  std::vector<Ptr<TraceReplayClient> >        m_parallelConnList; //!< List of all parallel connections
};
} // namespace ns3
#endif /* TRACE_REPLAY_CLIENT_H */