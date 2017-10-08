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
#include "ns3/double.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/trace-replay-utility.h"
#include "ns3/trace-replay-client.h"
#include "ns3/trace-replay-server.h"
#include "trace-replay-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TraceReplayHelper");

TraceReplayHelper::TraceReplayHelper (DataRate dataRate)
{
  m_stopTime = Seconds (0);
  m_startTimeOffset = Seconds (0);
  m_dataRate = dataRate;
  m_portNumber = 49153;
  m_traceFilePath = "";
  m_pcapPath = "";

  // m_startTimeJitter is time (in ms) a connection waits before starting, to avoid synchronization between multiple clients.
  // Value is uniform random-variable between 0 and 1000.
  m_startTimeJitter = CreateObject<UniformRandomVariable> ();
  m_startTimeJitter->SetAttribute ("Min", DoubleValue (0));
  m_startTimeJitter->SetAttribute ("Max", DoubleValue (1000));
  NS_LOG_FUNCTION (this);
}

TraceReplayHelper::~TraceReplayHelper ()
{
  m_connMap.clear ();
  m_timeoutMap.clear ();
  m_httpReqMap.clear ();
  NS_LOG_FUNCTION (this);
}

bool
TraceReplayHelper::m_connId::operator< (const m_connId& rhs) const
{
  return this->ipClient < rhs.ipClient
         || (this->ipClient == rhs.ipClient && this->portClient < rhs.portClient)
         || (this->ipClient == rhs.ipClient && this->portClient == rhs.portClient
             && this->ipServer < rhs.ipServer)
         || (this->ipClient == rhs.ipClient && this->portClient == rhs.portClient
             && this->ipServer == rhs.ipServer && this->portServer < rhs.portServer);
}

void
TraceReplayHelper::SetPcap (std::string pcap)
{
  NS_LOG_FUNCTION (this);
  m_pcapPath = pcap;
}

void
TraceReplayHelper::SetTraceFile (std::string traceFile)
{
  NS_LOG_FUNCTION (this);
  m_traceFilePath = traceFile;
}

int64_t
TraceReplayHelper::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);
  m_startTimeJitter->SetStream (stream);
  return 1;
}

void
TraceReplayHelper::SetStopTime (Time stopTime)
{
  NS_LOG_FUNCTION (this);
  m_stopTime = stopTime;
}

void
TraceReplayHelper::SetStartTimeOffset (Time time)
{
  NS_LOG_FUNCTION (this);
  m_startTimeOffset = time;
}

void
TraceReplayHelper::SetPortNumber (uint16_t port)
{
  NS_LOG_FUNCTION (this);
  m_portNumber = port;
}

void
TraceReplayHelper::RunCommands ()
{
  NS_LOG_FUNCTION (this);
  if (!std::ifstream (m_pcapPath.c_str ()))
    {
      std::cerr << "Input pcap file is missing.\n";
      exit (1);
    }

  // Get frame number of all the tcp packets which are also http requests
  std::string command = "tshark -r " + m_pcapPath + " -Y \"http.request.method and tcp\" -n -T fields -e frame.number > httpRequestFile";
  system (command.c_str ());

  // Get details about all the tcp packets
  command = "tshark -r " + m_pcapPath + " -Y \"ip.proto==6 \" -n -T fields -e ip.src -e tcp.srcport -e ip.dst -e tcp.dstport -e tcp.len -e frame.time_relative -e frame.number > tcpPackets.csv";
  system (command.c_str ());

  // Get tcp timeout details for each packet
  command = "tshark -r " + m_pcapPath + " -Y \"ip.proto == 6 and tcp.analysis.rto > 0\" -n -T fields -e frame.number -e tcp.analysis.rto > tcpTimeout";
  system (command.c_str ());
}

void
TraceReplayHelper::ProcessHttpList ()
{
  NS_LOG_FUNCTION (this);
  // Reading httpRequestFile to make a list of frame number of  http requests
  std::string filename = "httpRequestFile";
  std::ifstream infile;
  infile.open (filename.c_str ());
  if (infile.is_open ())
    {
      uint32_t val;
      while (infile >> val)
        {
          m_httpReqMap[val] = true;
        }
    }
  else
    {
      std::cerr << "Error opening httpRequestFile file.\n";
      exit (1);
    }
  infile.close ();
}

void
TraceReplayHelper::ProcessTimeoutList ()
{
  NS_LOG_FUNCTION (this);
  // Reading tcpTimeoutFile file to make a list of timed out packets
  std::string filename = "tcpTimeout";
  std::ifstream infile;
  infile.open (filename.c_str ());
  if (infile.is_open ())
    {
      uint32_t val;
      while (infile >> val)
        {
          m_timeoutMap[val] = true;
        }
    }
  else
    {
      std::cerr << "Error opening tcpTimeout file.\n";
      exit (1);
    }
  infile.close ();
}

double
TraceReplayHelper::CalculatePacketDelay (uint32_t frameNum, bool timeOut, bool httpReq, double currTime, double packetTime)
{
  double delay = 0;
  // if it is time out packet then delay will be 0 as TraceReplay is trying to simulate only application layer delays
  if (!timeOut)
    {
      double httpDelay = 0;
      double sshDelay = 0;
      if (httpReq)
        {
          // if packet is http request then delay will be time between this and last packet
          httpDelay = packetTime - currTime;
        }
      if (packetTime - currTime > 1)
        {
          // sshdelay is considered only when it is > 1 seconds
          sshDelay = packetTime - currTime;
        }
      delay = std::max (httpDelay, sshDelay);
    }
  if (delay < 0.000001)
    {
      return 0;
    }
  return delay;
}

void
TraceReplayHelper::ProcessPacket (m_connId id, uint32_t packetSize, double packetTime, uint32_t frameNum)
{
  TraceReplayPacket packet;
  packet.SetSize (packetSize);

  bool clientPacket;

  // If id is present in m_connMap then its Client packet, otherwise Server packet
  if (m_connMap.find (id) != m_connMap.end ())
  {
    clientPacket = true;
    if (m_connMap[id].packetC2S)
    {
      // last packet in connection was also from client to server
      m_connMap[id].packetCount += 1;
      m_connMap[id].byteCount += packetSize;
    }
    else
    {
      // last packet in connection was from server to client.
      // Therefore this packet is a new request.
      // Update server details.
      m_connMap[id].numRep.push_back (m_connMap[id].packetCount);
      m_connMap[id].expByteServer.push_back (m_connMap[id].byteCount);
      m_connMap[id].packetC2S = true;
      m_connMap[id].packetCount = 1;
      m_connMap[id].byteCount = packetSize;
    }
  }
  else
  {
    // packet is from server to client, reverse ip and port
    Address tmp1 = id.ipClient;
    id.ipClient = id.ipServer;
    id.ipServer = tmp1;
    uint32_t tmp2 = id.portClient;
    id.portClient = id.portServer;
    id.portServer = tmp2;

    clientPacket = false;
    if (!m_connMap[id].packetC2S)
    {
      m_connMap[id].packetCount += 1;
      m_connMap[id].byteCount += packetSize;
    }
    else
    {
      m_connMap[id].numReq.push_back (m_connMap[id].packetCount);
      m_connMap[id].expByteClient.push_back (m_connMap[id].byteCount);
      m_connMap[id].packetC2S = false;
      m_connMap[id].packetCount = 1;
      m_connMap[id].byteCount = packetSize;
    }
  }

  double delay = CalculatePacketDelay (frameNum, m_timeoutMap[frameNum], m_httpReqMap[frameNum],
                                         (m_connMap[id].currTime).GetSeconds (), packetTime);
  packet.SetDelay (Seconds (delay));
  if ((packet.GetDelay ()).IsStrictlyPositive ())
  {
    // delay > 0 seconds
    // Therefore get the list of all parallel connections and total packets sent by so far
    std::map<m_connId, m_connInfo>::iterator it;
    for (it = m_connMap.begin (); it != m_connMap.end (); it++)
    {
      // A parallel connection is that in which src and dst ips are same
      // but src and dst port are different.
      // Ignore the connection if total packet send by it so far is 0
      if ((it->first).ipClient == id.ipClient && (it->first).ipServer == id.ipServer
            && ((it->first).portClient != id.portClient || (it->first).portServer != id.portServer)
            && it->second.totByteCount > 0)
      {
        packet.AddParallelConnection ((it->first).portClient, (it->first).portServer, (it->second).totByteCount);
      }
    }
  }

  if (clientPacket)
  {
    m_connMap[id].clientPackets.push_back (packet);
  }
  else
  {
    m_connMap[id].serverPackets.push_back (packet);
  }

  m_connMap[id].currTime = Seconds (packetTime);
  // Increament total packet sent by connection so far
  m_connMap[id].totByteCount += packetSize;
}

void
TraceReplayHelper::ProcessPacketList ()
{
  // Reading packetFile to get details about each individual packet and map them to a connection
  std::string filename = "tcpPackets.csv";
  std::ifstream infile;
  infile.open (filename.c_str ());
  if (infile.is_open ())
    {
      std::string line;
      while (std::getline (infile, line))
        {
          std::istringstream iss (line);
          std::string ipSrc;
          uint16_t portSrc;
          std::string ipDest;
          uint16_t portDest;
          uint32_t packetSize;
          uint32_t frameNum;
          double packetTime;

          iss >> ipSrc >> portSrc >> ipDest >> portDest;
          iss >> packetSize >> packetTime >> frameNum;

          m_connId id;
          if (std::regex_search (ipSrc.begin (), ipSrc.end (), std::regex ("^[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+$")))
            {
              // Ipv4 address
              id.ipClient = Ipv4Address (ipSrc.c_str ());
            }
          else
            {
              // Ipv6 address
              id.ipClient = Ipv6Address (ipSrc.c_str ());
            }
          id.portClient = portSrc;
          if (std::regex_search (ipDest.begin (), ipDest.end (), std::regex ("^[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+$")))
            {
              id.ipServer = Ipv4Address (ipDest.c_str ());
            }
          else
            {
              id.ipServer = Ipv6Address (ipDest.c_str ());
            }
          id.portServer = portDest;

          // If the packet is from server to client then ip and port numbers for source and destination will be reversed
          m_connId idReverse;
          idReverse.ipClient = id.ipServer;
          idReverse.portClient = id.portServer;
          idReverse.ipServer = id.ipClient;
          idReverse.portServer = id.portClient;
          // if the ip and port number for packet are not present in map then insert it as new connection
          if (m_connMap.find (id) == m_connMap.end () && m_connMap.find (idReverse) == m_connMap.end ())
          {
            m_connInfo info;
            info.startTime = Seconds (packetTime);
            info.packetC2S = true;
            info.packetCount = 0;
            info.byteCount = 0;
            info.currTime = Seconds (packetTime);
            info.totByteCount = 0;
            m_connMap[id] = info;
          }

          if (packetSize == 0)
          {
            // ignore 0 byte packets
            continue;
          }
          ProcessPacket (id, packetSize, packetTime, frameNum);
        }
    }
  else
    {
      std::cerr << "Error opening tcpPackets.csv file.\n";
      exit (1);
    }
  infile.close ();
}

void
TraceReplayHelper::PrintTraceFile ()
{
  std::ofstream file;
  file.open ("traceFile.txt");
  // Comments for trace file
  file << "# ------------------------------------------------\n";
  file << "# Trace file: traceFile.txt\n";
  file << "# File structure:-\n";
  file << "# Number of client\n";
  file << "# For each client {\n";
  file << "# \tNumber of connection\n";
  file << "# \tFor each connection {\n";
  file << "# \t\tIp_Client\tPort_Client\tIp_server\tPort_Server\tStart_Time\n";
  file << "# \t\tNumber of packets from client to server\n";
  file << "# \t\tFor each packet from client to server {\n";
  file << "# \t\t\tPacket_Size\tPacket_Delay\n";
  file << "# \t\t}\n";
  file << "# \t\tNumber of client request\n";
  file << "# \t\tFor each request {\n";
  file << "# \t\t\tNumber of packets to send before going to receive mode\n";
  file << "# \t\t}\n";
  file << "# \t\tNumber of server response\n";
  file << "# \t\tFor each response {\n";
  file << "# \t\t\tNumber of bytes to receive before going to send mode\n";
  file << "# \t\t}\n";
  file << "# \t\tNumber of packet from server to client\n";
  file << "# \t\tFor each packet from server to client {\n";
  file << "# \t\t\tPacket_Size\tPacket_Delay\n";
  file << "# \t\t}\n";
  file << "# \t\tNumber of server response\n";
  file << "# \t\tFor each response {\n";
  file << "# \t\t\tNumber of packets to send before going to receive mode\n";
  file << "# \t\t}\n";
  file << "# \t\tNumber of client request\n";
  file << "# \t\tFor each request {\n";
  file << "# \t\t\tNumber of bytes to receive before going to send mode\n";
  file << "# \t\t}\n";
  file << "# \t}\n";
  file << "# }\n";
  file << "# ------------------------------------------------\n";
  // Print number of connection
  file << m_connMap.size () << std::endl;
  // Iterate over each connection and print details
  std::map<m_connId, m_connInfo>::iterator it;
  for (it = m_connMap.begin (); it != m_connMap.end (); it++)
    {
      // Update the final packet and byte counts.
      if (it->second.packetC2S && it->second.totByteCount > 0)
        {
          it->second.numReq.push_back (it->second.packetCount);
          it->second.expByteClient.push_back (it->second.byteCount);
        }
      else if (it->second.totByteCount > 0)
        {
          it->second.numRep.push_back (it->second.packetCount);
          it->second.expByteServer.push_back (it->second.byteCount);
        }

      if (Ipv4Address::IsMatchingType ((it->first).ipClient))
        {
          file << Ipv4Address::ConvertFrom ((it->first).ipClient) << "\t";
        }
      else
        {
          file << Ipv6Address::ConvertFrom ((it->first).ipClient) << "\t";
        }
      file << (it->first).portClient << "\t";
      if (Ipv4Address::IsMatchingType ((it->first).ipServer))
        {
          file << Ipv4Address::ConvertFrom ((it->first).ipServer) << "\t";
        }
      else
        {
          file << Ipv6Address::ConvertFrom ((it->first).ipServer) << "\t";
        }
      file << (it->first).portServer << "\t";
      file << (it->second).startTime.GetSeconds () << std::endl;

      uint32_t numPacket = (it->second).clientPackets.size ();
      file << numPacket << std::endl;
      for (uint32_t i = 0; i < numPacket; i++)
        {
          // print details of each packet
          TraceReplayPacket packet = (it->second).clientPackets[i];
          file << packet.GetSize () << "\t" << (packet.GetDelay ()).GetSeconds () << std::endl;
          if ((packet.GetDelay ()).IsStrictlyPositive ())
            {
              uint32_t numParallelCon = packet.GetNumParallelConnection ();
              file << numParallelCon << std::endl;
              for (uint32_t j = 0; j < numParallelCon; j++)
                {
                  std::pair<uint16_t, uint16_t> connId = packet.GetConnectionId (j);
                  file << connId.first << "\t" << connId.second << "\t" << packet.GetByteCount (j) << std::endl;
                }
            }
        }

      uint32_t size = (it->second).numReq.size ();
      file << size << std::endl;
      for (uint32_t i = 0; i < size; i++)
        {
          file << (it->second).numReq[i] << std::endl;
        }

      size = (it->second).expByteServer.size ();
      file << size << std::endl;
      for (uint32_t i = 0; i < size; i++)
        {
          file << (it->second).expByteServer[i] << std::endl;
        }

      numPacket = (it->second).serverPackets.size ();
      file << numPacket << std::endl;
      for (uint32_t i = 0; i < numPacket; i++)
        {
          // print details of each packet
          TraceReplayPacket packet = (it->second).serverPackets[i];
          file << packet.GetSize () << "\t" << (packet.GetDelay ()).GetSeconds () << std::endl;
          if ((packet.GetDelay ()).IsStrictlyPositive ())
            {
              uint32_t numParallelCon = packet.GetNumParallelConnection ();
              file << numParallelCon << std::endl;
              for (uint32_t j = 0; j < numParallelCon; j++)
                {
                  std::pair<uint16_t, uint16_t> connId = packet.GetConnectionId (j);
                  file << connId.first << "\t" << connId.second << "\t" << packet.GetByteCount (j) << std::endl;
                }
            }
        }

      size = (it->second).numRep.size ();
      file << size << std::endl;
      for (uint32_t i = 0; i < size; i++)
        {
          file << (it->second).numRep[i] << std::endl;
        }

      size = (it->second).expByteClient.size ();
      file << size << std::endl;
      for (uint32_t i = 0; i < size; i++)
        {
          file << (it->second).expByteClient[i] << std::endl;
        }
    }
  file.close ();
}

void
TraceReplayHelper::DeleteTmpFiles ()
{
  // Remove all the temporary files
  std::remove ("httpRequestFile");
  std::remove ("tcpPackets.csv");
  std::remove ("tcpTimeout");
}

void
TraceReplayHelper::ConvertPcapToTrace ()
{
  RunCommands ();
  ProcessHttpList ();
  ProcessTimeoutList ();
  ProcessPacketList ();
  PrintTraceFile ();
  DeleteTmpFiles ();
}

std::string
TraceReplayHelper::CheckRegex (std::ifstream& infile, std::regex reg)
{
  std::string line;
  while (getline (infile, line))
    {
      if (std::regex_search (line.begin (), line.end (), std::regex ("^#")))
        {
          continue;
        }
      else
        {
          break;
        }
    }
  if (std::regex_search (line.begin (), line.end (), reg))
    {
      return line;
    }
  else
    {
      std::cerr << "Input trace file is corrupted!";
      exit (1);
    }
}

void
TraceReplayHelper::Install (Ptr<Node> clientNode, Ptr<Node> remoteNode, Address remoteAddress)
{
  NS_LOG_FUNCTION (this);

  std::string filename = "";
  if (std::ifstream (m_pcapPath.c_str ()))
    {
      // Valid pcap file. Overwrite trace file (if present)
      ConvertPcapToTrace ();
      filename = "traceFile.txt";
    }
  else if (std::ifstream (m_traceFilePath.c_str ()))
    {
      // Valid trace file.
      filename = m_traceFilePath;
    }
  else
    {
      std::cerr << "No valid pcap or trace file.\n";
      exit (1);
    }

  std::ifstream infile (filename.c_str ());
  if (!infile.is_open ())
    {
      std::cerr << "Error opening trace file.\n";
      exit (1);
    }

  uint32_t numConn = 0; // number of connection per client
  std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
  std::istringstream iss (line);
  iss >> numConn;

  // for each connection read the data and initialize client-server connection pair
  for (uint32_t j = 0; j < numConn; j++)
    {
      // real ip and port numbers
      std::string ipClientTmp = "";
      uint16_t portClient = 0;
      std::string ipServerTmp = "";
      uint16_t portServer = 0;
      double startTime = 0.0;
      {
        std::string line = CheckRegex (infile, std::regex ("^*\t[0-9]+\t*[0-9]+\t[0-9]+[.]?[0-9]*$"));
        std::istringstream iss (line);
        iss >> ipClientTmp >> portClient >> ipServerTmp >> portServer >> startTime;
      }

      Address ipClient;
      if (std::regex_search (ipClientTmp.begin (), ipClientTmp.end (), std::regex ("^[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+$")))
        {
          ipClient = Ipv4Address (ipClientTmp.c_str ());
        }
      else
        {
          ipClient = Ipv6Address (ipClientTmp.c_str ());
        }

      Address ipServer;
      if (std::regex_search (ipServerTmp.begin (), ipServerTmp.end (), std::regex ("^[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+$")))
        {
          ipServer = Ipv4Address (ipServerTmp.c_str ());
        }
      else
        {
          ipServer = Ipv6Address (ipServerTmp.c_str ());
        }

      // Each connections will get port number sequentially starting from m_portNumber.
      uint16_t portNumber = m_portNumber + j;
      Address address;
      if (Ipv4Address::IsMatchingType (remoteAddress) == true)
        {
          address = InetSocketAddress (Ipv4Address::ConvertFrom (remoteAddress), portNumber);
        }
      else
        {
          address = Inet6SocketAddress (Ipv6Address::ConvertFrom (remoteAddress), portNumber);
        }
      // Client to server connection
      {
        uint16_t numPacket = 0;
        {
          std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
          std::istringstream iss (line);
          iss >> numPacket;
        }
        {
          std::vector<TraceReplayPacket> packetList;
          for (uint16_t k = 0; k < numPacket; k++)
            {
              TraceReplayPacket packet;
              uint32_t packetSize = 0;
              double delay = 0.0;
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+\t[0-9]+[.]?[0-9]*$"));
                std::istringstream iss (line);
                iss >> packetSize >> delay;
              }
              if (delay > 0)
                {
                  uint32_t n = 0; // number of parallel connection
                  {
                    std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                    std::istringstream iss (line);
                    iss >> n;
                  }
                  for (uint32_t i = 0; i < n; i++)
                    {
                      uint16_t srcPort = 0;
                      uint16_t dstPort = 0;
                      uint32_t count = 0;
                      {
                        std::string line = CheckRegex (infile, std::regex ("^[0-9]+\t[0-9]+\t[0-9]+$"));
                        std::istringstream iss (line);
                        iss >> srcPort >> dstPort >> count;
                      }
                      packet.AddParallelConnection (srcPort, dstPort, count);
                    }
                }

              packet.SetSize (packetSize);
              packet.SetDelay (Seconds (delay));
              packetList.push_back (packet);
            }

          uint32_t nRequest = 0; // number of requests
          {
            std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
            std::istringstream iss (line);
            iss >> nRequest;
          }
          std::vector<uint32_t> numReq;
          for (uint32_t k = 0; k < nRequest; k++)
            {
              uint32_t count = 0; // number of packets to send per request
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                std::istringstream iss (line);
                iss >> count;
              }
              numReq.push_back (count);
            }

          uint32_t nReply = 0; // number of Reply from server
          {
            std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
            std::istringstream iss (line);
            iss >> nReply;
          }
          std::vector<uint32_t> expByte;
          for (uint32_t k = 0; k < nReply; k++)
            {
              uint32_t count; // number of total bytes to receive, as reply, for each request
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                std::istringstream iss (line);
                iss >> count;
              }
              expByte.push_back (count);
            }

          // Initialize TraceReplayClient
          Ptr<TraceReplayClient> client = CreateObject<TraceReplayClient> ();
          client->SetConnectionId (ipClient, portClient, ipServer, portServer);
          client->Setup (address, m_dataRate, numReq, expByte, packetList);
          // Start time of connection is :
          // Actual start time taken from trace file +
          // offset set by user +
          // jitter to avoid synchronization (max 1 second)
          client->SetStartTime (Seconds (startTime) + m_startTimeOffset + MilliSeconds (m_startTimeJitter->GetValue ()));
          client->SetStopTime (Seconds (m_stopTime));
          clientNode->AddApplication (client);
        }
      }
      // Server to client connection
      {
        uint16_t numPacket = 0;
        {
          std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
          std::istringstream iss (line);
          iss >> numPacket;
        }
        {
          std::vector<TraceReplayPacket> packetList;
          for (uint16_t k = 0; k < numPacket; k++)
            {
              TraceReplayPacket packet;
              uint32_t packetSize = 0;
              double delay = 0.0;
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+\t[0-9]+[.]?[0-9]*$"));
                std::istringstream iss (line);
                iss >> packetSize >> delay;
              }
              if (delay > 0)
                {
                  uint32_t n = 0; // number of parallel connection
                  {
                    std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                    std::istringstream iss (line);
                    iss >> n;
                  }
                  for (uint32_t i = 0; i < n; i++)
                    {
                      uint16_t srcPort = 0;
                      uint16_t dstPort = 0;
                      uint32_t count = 0;
                      {
                        std::string line = CheckRegex (infile, std::regex ("^[0-9]+\t[0-9]+\t[0-9]+$"));
                        std::istringstream iss (line);
                        iss >> srcPort >> dstPort >> count;
                      }
                      packet.AddParallelConnection (srcPort, dstPort, count);
                    }
                }

              packet.SetSize (packetSize);
              packet.SetDelay (Seconds (delay));
              packetList.push_back (packet);
            }

          uint32_t nReply = 0; // number of reply
          {
            std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
            std::istringstream iss (line);
            iss >> nReply;
          }
          std::vector<uint32_t> numRep;
          for (uint32_t k = 0; k < nReply; k++)
            {
              uint32_t count; // number of packets to send in each reply
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                std::istringstream iss (line);
                iss >> count;
              }
              numRep.push_back (count);
            }

          uint32_t nRequest = 0; // number of request
          std::vector<uint32_t> expByte;
          {
            std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
            std::istringstream iss (line);
            iss >> nRequest;
          }
          for (uint32_t k = 0; k < nRequest; k++)
            {
              uint32_t count = 0; // total bytes expected in each request
              {
                std::string line = CheckRegex (infile, std::regex ("^[0-9]+$"));
                std::istringstream iss (line);
                iss >> count;
              }
              expByte.push_back (count);
            }

          // Initiliaze TraceReplayServer
          Ptr<TraceReplayServer> server = CreateObject<TraceReplayServer> ();
          server->SetConnectionId (ipClient, portClient, ipServer, portServer);
          server->Setup (address, m_dataRate, numRep, expByte, packetList);
          server->SetStartTime (Seconds (0.0));
          server->SetStopTime (Seconds (m_stopTime));
          remoteNode->AddApplication (server);
        }
      }
    }
  infile.close ();
}
} // namespace ns3