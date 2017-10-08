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
#include "trace-replay-server.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("TraceReplayServer");
NS_OBJECT_ENSURE_REGISTERED (TraceReplayServer);

TypeId TraceReplayServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TraceReplayServer")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<TraceReplayServer> ()
  ;
  return tid;
}

TraceReplayServer::TraceReplayServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_connected = false;
  m_totRecByte = 0;
  m_totExpByte = 0;
  m_totByteCount = 0;
}

TraceReplayServer::~TraceReplayServer ()
{
  NS_LOG_FUNCTION (this);
  m_numRep.clear ();
  m_expByte.clear ();
  m_packetList.clear ();
  m_parallelConnList.clear ();
  m_socket = 0;
}

void
TraceReplayServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  m_connected = true;

  if (m_socket == 0)
    {
      m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
      m_socket->Bind (m_local);
      m_socket->Listen ();
    }

  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&TraceReplayServer::HandleAccept, this));
}

void
TraceReplayServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  m_connected = false;

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetAcceptCallback (
        MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
        MakeNullCallback<void, Ptr<Socket>, const Address &> () );
    }
}

void TraceReplayServer::HandleAccept (Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION (this);
  if (m_totExpByte == 0)
    {
      ScheduleTx (socket);
    }
  else
    {
      socket->SetRecvCallback (MakeCallback (&TraceReplayServer::ReceivePacket, this));
    }
  socket->SetCloseCallbacks (MakeCallback (&TraceReplayServer::HandleSuccessClose, this),
                             MakeNullCallback<void, Ptr<Socket> > () );
}

void TraceReplayServer::HandleSuccessClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  socket->Close ();
  socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
  socket->SetCloseCallbacks (MakeNullCallback<void, Ptr<Socket> > (),
                             MakeNullCallback<void, Ptr<Socket> > () );
}

void
TraceReplayServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
TraceReplayServer::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  for (uint32_t index = 0; index < GetNode ()->GetNApplications (); index++)
    {
      Ptr <TraceReplayServer> app = DynamicCast <TraceReplayServer>  (GetNode ()->GetApplication (index));
      if (app && app->GetIpServer () == m_ipServer)
        {
          // Two connections are considered parallel if both source and destination Ip's
          // are same as other connection
          m_parallelConnList.push_back (app);
        }
    }
  Application::DoInitialize ();
}

Address
TraceReplayServer::GetIpServer () const
{
  NS_LOG_FUNCTION (this);
  return m_ipServer;
}

uint16_t
TraceReplayServer::GetPortServer () const
{
  NS_LOG_FUNCTION (this);
  return m_portServer;
}

uint16_t
TraceReplayServer::GetPortClient () const
{
  NS_LOG_FUNCTION (this);
  return m_portClient;
}

uint32_t
TraceReplayServer::GetTotalByteCount () const
{
  NS_LOG_FUNCTION (this);
  return m_totByteCount;
}

void
TraceReplayServer::SetConnectionId (Address ipClient, uint16_t portClient, Address ipServer, uint16_t portServer)
{
  NS_LOG_FUNCTION (this);
  m_ipClient = ipClient;
  m_ipServer = ipServer;
  m_portClient = portClient;
  m_portServer = portServer;
}

void
TraceReplayServer::Setup (Address address, DataRate dataRate, std::vector<uint32_t> numRep, std::vector<uint32_t> expByte, std::vector<TraceReplayPacket> packetList)
{
  NS_LOG_FUNCTION (this);
  m_local = address;
  m_dataRate = dataRate;

  // If the conneciton does not have any packet then numRep will be empty.
  // Insert 0 to indicate server that it does not have to send any packet
  m_numRep = numRep;
  if (m_numRep.empty ())
    {
      m_numRep.push_back (0);
    }
  m_numRepIt = m_numRep.begin ();

  // If the conneciton does not have any packet then expByte will be empty.
  // Insert 0 to indicate server that it does not have to receive any packet
  m_expByte = expByte;
  if (m_expByte.empty ())
    {
      m_expByte.push_back (0);
    }
  m_expByteIt = m_expByte.begin ();
  // Update m_totExpByte, as server will start in receive mode
  m_totExpByte = *(m_expByteIt++);

  m_packetList = packetList;
  m_packetListIt = m_packetList.begin ();
}

void
TraceReplayServer::SendPacket (Ptr<Socket> socket, TraceReplayPacket packet)
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
          Simulator::Schedule (tNext, &TraceReplayServer::SendPacket, this, socket, packet);
          return;
        }
    }

  if (socket->GetTxAvailable () < packet.GetSize ())
    {
      // Not enough buffer available
      Time tNext = m_dataRate.CalculateBytesTxTime (packet.GetSize ());
      NS_LOG_LOGIC ("Buffer not available. Scheduling next event at time " << (Simulator::Now () + tNext));
      Simulator::Schedule (tNext, &TraceReplayServer::SendPacket, this, socket, packet);
      return;
    }

  // Send packet
  NS_LOG_LOGIC ("ServerIp " << m_ipServer << " ServerPort " << m_portServer << " sending packet of size " << packet.GetSize ());
  Ptr<Packet> packetTmp = Create<Packet> (packet.GetSize ());
  socket->Send (packetTmp);

  // Update total byte count for this connection
  m_totByteCount += packet.GetSize ();
  if (*m_numRepIt > 0)
    {
      // more packets are in send queue, schedule next packet
      ScheduleTx (socket);
    }
  else
    {
      // update expected number of total bytes to be received
      m_totExpByte = *(m_expByteIt++);
      m_totRecByte = 0;
      if (m_numRepIt != m_numRep.end () && *m_numRepIt == 0)
        {
          // update the number of packets to send in next run
          ++m_numRepIt;
        }
      // go to receive mode
      socket->SetRecvCallback (MakeCallback (&TraceReplayServer::ReceivePacket, this));
    }
}

void
TraceReplayServer::ScheduleTx (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  if (m_connected && *m_numRepIt > 0)
    {
      // Decrement number of packets to be send
      *m_numRepIt -= 1;

      TraceReplayPacket packet = *(m_packetListIt++);
      if ((packet.GetDelay ()).IsStrictlyPositive ())
        {
          // schedule packet after 'delay'
          Time tNext = packet.GetDelay () + m_dataRate.CalculateBytesTxTime (packet.GetSize ());
          NS_LOG_LOGIC ("Packet sending scheduled at time " << (Simulator::Now () + tNext));
          Simulator::Schedule (tNext, &TraceReplayServer::SendPacket, this, socket, packet);
        }
      else
        {
          SendPacket (socket, packet);
          return;
        }
    }
  else if (*m_numRepIt == 0)
    {
      // No more packet to send, update expected number total bytes to be received
      m_totExpByte = *(m_expByteIt++);
      m_totRecByte = 0;
      // go to recieve mode
      m_socket->SetRecvCallback (MakeCallback (&TraceReplayServer::ReceivePacket, this));
    }
}

void
TraceReplayServer::ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  // receive packet
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          NS_LOG_LOGIC ("ServerIp " << m_ipServer << " ServerPort " << m_portServer << " received packet of size " << packet->GetSize () << " at " << Simulator::Now ());
          // update total bytes received
          m_totRecByte += packet->GetSize ();
          m_totByteCount += packet->GetSize ();
        }
    }
  // keep recieving packet
  if (m_totRecByte < m_totExpByte)
    {
      socket->SetRecvCallback (MakeCallback (&TraceReplayServer::ReceivePacket, this));
    }
  else if (m_numRepIt != m_numRep.end () && *m_numRepIt > 0)
    {
      // go to send mode
      ScheduleTx (socket);
    }
}
} // namespace ns3