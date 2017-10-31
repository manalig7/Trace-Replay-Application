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

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/trace-replay-utility.h"
#include "trace-replay-client.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TraceReplayClient");
NS_OBJECT_ENSURE_REGISTERED (TraceReplayClient);

TypeId TraceReplayClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TraceReplayClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<TraceReplayClient> ()
  ;
  return tid;
}

TraceReplayClient::TraceReplayClient ()
  : m_socket (0),
    m_peer (),
    m_dataRate (0),
    m_sendEvent (),
    m_connected (false),
    m_totRecByte (0),
    m_totExpByte (0),
    m_totByteCount (0)
{
  NS_LOG_FUNCTION (this);
}

TraceReplayClient::~TraceReplayClient ()
{
  NS_LOG_FUNCTION (this);
  m_numReq.clear ();
  m_expByte.clear ();
  m_packetList.clear ();
  m_parallelConnList.clear ();
  m_socket = 0;
}

void
TraceReplayClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  m_connected = true;
  if (Inet6SocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind6 ();
    }
  else if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  m_socket->Connect (m_peer);
  m_socket->SetConnectCallback (
    MakeCallback (&TraceReplayClient::ConnectionSucceeded, this),
    MakeCallback (&TraceReplayClient::ConnectionFailed, this));
}

void
TraceReplayClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_connected = false;
  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }
  if (m_socket)
    {
      m_socket->Close ();
    }
}

void TraceReplayClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("TraceReplayClient Connection succeeded");
  m_connected = true;
  ScheduleTx ();
}

void TraceReplayClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("TraceReplayClient, Connection Failed");
}

void
TraceReplayClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
TraceReplayClient::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  for (uint32_t index = 0; index < GetNode ()->GetNApplications (); index++)
    {
      Ptr <TraceReplayClient> app = DynamicCast <TraceReplayClient>  (GetNode ()->GetApplication (index));
      if (app && app->GetIpServer () == m_ipServer)
        {
          m_parallelConnList.push_back (app);
        }
    }
  Application::DoInitialize ();
}

Address
TraceReplayClient::GetIpServer () const
{
  NS_LOG_FUNCTION (this);
  return m_ipServer;
}

uint16_t
TraceReplayClient::GetPortServer () const
{
  NS_LOG_FUNCTION (this);
  return m_portServer;
}

uint16_t
TraceReplayClient::GetPortClient () const
{
  NS_LOG_FUNCTION (this);
  return m_portClient;
}

uint32_t
TraceReplayClient::GetTotalByteCount () const
{
  NS_LOG_FUNCTION (this);
  return m_totByteCount;
}

void
TraceReplayClient::SetConnectionId (Address ipClient, uint16_t portClient, Address ipServer, uint16_t portServer)
{
  NS_LOG_FUNCTION (this);
  m_ipClient = ipClient;
  m_ipServer = ipServer;
  m_portClient = portClient;
  m_portServer = portServer;
}

void
TraceReplayClient::Setup (Address address, DataRate dataRate, std::vector<uint32_t> numReq, std::vector<uint32_t> expByte, std::vector<TraceReplayPacket> packetList)
{
  NS_LOG_FUNCTION (this);
  m_peer = address;
  m_dataRate = dataRate;

  // If the conneciton does not have any packet then numReq will be empty.
  // Insert 0 to indicate client that it does not have to send any packet
  m_numReq = numReq;
  if (m_numReq.empty ())
    {
      m_numReq.push_back (0);
    }
  m_numReqIt = m_numReq.begin ();

  // If the conneciton does not have any packet then expByte will be empty.
  // Insert 0 to indicate client that it does not have to receive any packet
  m_expByte = expByte;
  if (m_expByte.empty ())
    {
      m_expByte.push_back (0);
    }
  m_expByteIt = m_expByte.begin ();

  m_packetList = packetList;
  m_packetListIt = m_packetList.begin ();
}

void
TraceReplayClient::SendPacket (TraceReplayPacket packet)
{
  NS_LOG_FUNCTION (this);

  if ((packet.GetDelay ()).IsStrictlyPositive ())
    {
      // If dalay > 0 seconds, check all parallel connections for progress
      bool okToSend = true;
      for (uint32_t i = 0; i < m_parallelConnList.size (); i++)
        {
          uint32_t expected = packet.GetByteCount (m_parallelConnList[i]->GetPortClient (), m_parallelConnList[i]->GetPortServer ());
          uint32_t current = m_parallelConnList[i]->GetTotalByteCount ();
          if (current < expected)
            {
              okToSend = false;
              break;
            }
        }
      if (!okToSend)
        {
          // Retry to send after 0.0001 seconds
          Time tNext (Seconds (0.00001));
          NS_LOG_LOGIC ("Parallel connections have not made desired progress. Scheduling next event at time "
            << (Simulator::Now () + tNext));
          Simulator::Schedule (tNext, &TraceReplayClient::SendPacket, this, packet);
          return;
        }
    }

  if (m_socket->GetTxAvailable () < packet.GetSize ())
    {
      // Not enough buffer available
      Time tNext = m_dataRate.CalculateBytesTxTime (packet.GetSize ());
      NS_LOG_LOGIC ("Buffer not available. Scheduling next event at time " << (Simulator::Now () + tNext));
      Simulator::Schedule (tNext, &TraceReplayClient::SendPacket, this, packet);
      return;
    }

  // Send packet
  NS_LOG_LOGIC ("ClientIp " << m_ipClient << " ClientPort " << m_portClient << " sending packet of size " << packet.GetSize ());
  Ptr<Packet> packetTmp = Create<Packet> (packet.GetSize ());
  m_socket->Send (packetTmp);

  // Update the total byte count for this connection
  m_totByteCount += packet.GetSize ();
  if (*m_numReqIt > 0)
    {
      // more packets are in send queue, schedule next packet
      ScheduleTx ();
    }
  else
    {
      // update expected number of total bytes to be received
      m_totExpByte = *(m_expByteIt++);
      m_totRecByte = 0;
      // No more packets to receive. Close connection
      if (m_totExpByte == 0)
        {
          m_socket->Close ();
          m_connected = false;
          return;
        }
      // go to receive mode
      m_socket->SetRecvCallback (MakeCallback (&TraceReplayClient::ReceivePacket, this));
    }
}

void
TraceReplayClient::ScheduleTx (void)
{
  NS_LOG_FUNCTION (this);
  if (m_connected && *m_numReqIt > 0)
    {
      // Decrement number of packets to be send
      *m_numReqIt -= 1;

      TraceReplayPacket packet = *(m_packetListIt++);
      if ((packet.GetDelay ()).IsStrictlyPositive ())
        {
          // schedule packet after 'delay'
          Time tNext = packet.GetDelay () + m_dataRate.CalculateBytesTxTime (packet.GetSize ());
          NS_LOG_LOGIC ("Packet sending scheduled at time " << (Simulator::Now () + tNext));
          m_sendEvent = Simulator::Schedule (tNext, &TraceReplayClient::SendPacket, this, packet);
        }
      else
        {
          SendPacket (packet);
          return;
        }
    }
  else if (*m_numReqIt == 0)
    {
      // No more packet to send, update expected number total bytes to be received
      m_totExpByte = *(m_expByteIt++);
      m_totRecByte = 0;
      if (m_totExpByte == 0)
        {
          // No more packet to send or receive
          m_socket->Close ();
          m_connected = false;
        }
      else
        {
          // go to receive mode
          m_socket->SetRecvCallback (MakeCallback (&TraceReplayClient::ReceivePacket, this));
        }
    }
}

void
TraceReplayClient::ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  // receive packet
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          NS_LOG_LOGIC ("ClientIp " << m_ipClient << " ClientPort " << m_portClient << " received packet of size " << packet->GetSize () << " at " << Simulator::Now ());
          // update total bytes received
          m_totRecByte += packet->GetSize ();
          m_totByteCount += packet->GetSize ();
        }
    }
  if (m_totRecByte < m_totExpByte)
    {
      // keep receiving packet
      m_socket->SetRecvCallback (MakeCallback (&TraceReplayClient::ReceivePacket, this));
    }
  else if (m_numReqIt != m_numReq.end () && ++m_numReqIt != m_numReq.end ())
    {
      // go to send mode
      ScheduleTx ();
    }
  else
    {
      // No more packet to send or receive
      StopApplication ();
    }
}
} // namespace ns3