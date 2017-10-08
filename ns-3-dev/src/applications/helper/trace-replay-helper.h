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

#ifndef TRACE_REPLAY_HELPER_H
#define TRACE_REPLAY_HELPER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <map>
#include <regex>

namespace ns3 {

class TraceReplayPacket;
class Address;

/**
 * \brief TraceReplayHelper make it easier to analyse input pcap file
 * and initialize connections between TraceReplayClient and TraceReplayServer
 *
 */
class TraceReplayHelper
{
public:
  /**
   * \brief TraceReplayHelper make it easier to analyse input pcap file
   * and initialize connections between TraceReplayClient and TraceReplayServer
   *
   * \param DataRate data rate for client and server
   */
  TraceReplayHelper (DataRate dataRate);
  ~TraceReplayHelper (void);

  /**
   * \brief This method sets the input the pcap file. It is optional if the trace file is provided.
   * If both pcap and trace file are provided, then trace file will be ignored,
   * and new trace file will be created from the input pcap.
   * For simulating large number of nodes, it is advisable to first create a trace file from running
   * simulation for a single node and then use 'SetTraceFile' to provide trace file (skipping 'SetPcap').
   * This would save the pcap processing time for each nodes.
   *
   * \param pcap File name of the input pcap
   */
  void SetPcap (std::string pcap);

  /**
   * \brief This method sets the input the trace file.
   * TraceReplay will use this trace file, instead of creating new
   * trace file from input pcap.
   *
   * \param traceFile File name of the input trace file
   */
  void SetTraceFile (std::string traceFile);

  /**
   * \brief Assign a fixed random variable stream number to the random variables used by this model.
   *
   * Use to avoid synchronization between multiple clients start time.
   *
   * \param stream first stream index to use
   *
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  /**
   * \brief This methods sets the stop time for the connection.
   *
   * \param time Stop time for application
   */
  void SetStopTime (Time time);

  /**
   * \brief This methods sets the start time offset for the connection.
   *
   * Start time for each connection is read from trace file. This function will set additional
   * offset for start time.
   *
   * \param time Start time offset for application
   */
  void SetStartTimeOffset (Time time);

  /**
   * \brief This methods sets the starting port number for client.
   *
   * All the connections for client will start from this port number
   *
   * \param port Starting port number for client
   */
  void SetPortNumber (uint16_t port);

  /**
   * \brief Creates the trace file, if not present, and initializes all client-server pairs
   *
   * This method reads the traceFile.txt file and initializes all client-server connections.
   *
   * \param clientNode pointer to client node
   * \param remoteNode pointer to server node
   * \param remoteAddress Server Ip address
   *
   */
  void Install (Ptr<Node> clientNode, Ptr<Node> remoteNode, Address remoteAddress);

private:
  std::string     m_pcapPath;       //!< Path to input pcap file
  std::string     m_traceFilePath;  //!< Path to input trace file
  Time            m_stopTime;       //!< Stop time
  Time            m_startTimeOffset; //!< Start time offset
  DataRate        m_dataRate;       //!< Data Rate
  uint16_t        m_portNumber;     //!< Starting port number for connections
  struct          m_connId          //!< Struct to uniquely identify a connection
  {
    Address       ipClient;         //!< Real IP address of client
    uint16_t      portClient;       //!< Real port number of client
    Address       ipServer;         //!< Real IP address of server
    uint16_t      portServer;       //!< Real port number of server
    /**
     * \brief < opertator for m_connId.
     */
    bool operator< (const m_connId& rhs) const;
  };
  struct          m_connInfo        //!< Struct containing details about the connection
  {
    Time            startTime;      //!< Start time of connections
    Time            currTime;       //!< Time of the last packet
    uint32_t        packetCount;    //!< Total count of packets in current cycle
    uint32_t        byteCount;      //!< Total count of bytes in current cycle
    uint32_t        totByteCount;   //!< Total count of bytes seen in the connection
    bool            packetC2S;      //!< Indicate whether last packet was client to server or not

    std::vector<TraceReplayPacket>    clientPackets;    //!< List of client's packet
    std::vector<TraceReplayPacket>    serverPackets;    //!< List of server's packet
    std::vector<uint32_t>             numReq;           //!< List of #packets to send as request
    std::vector<uint32_t>             expByteClient;    //!< List of #byte expected to receive as reply
    std::vector<uint32_t>             numRep;           //!< List of #packets to send as reply
    std::vector<uint32_t>             expByteServer;    //!< List of #byte expected to receive as request
  };

  std::map<uint32_t, bool>        m_httpReqMap;     //!< list of frame numbers for packet which are http request
  std::map<uint32_t, bool>        m_timeoutMap;     //!< list of frame numbers for packet which were timed out
  std::map<m_connId, m_connInfo>  m_connMap;        //!< list of all tcp connections with details
  Ptr<RandomVariableStream>       m_startTimeJitter;//!< random number stream for start time

  /**
   * \brief Converts the input pcap file to formatted trace file (tarceFile.txt)
   *
   */
  void ConvertPcapToTrace ();

  /**
   * \brief Runs the necessary tshark commands to read the input pcap file
   *
   */
  void RunCommands ();

  /**
   * \brief Processes the httpRequest file to make list of http request packets in pcap
   *
   */
  void ProcessHttpList ();

  /**
   * \brief Processes the timeout file to make list of timed out packets in pcap
   *
   */
  void ProcessTimeoutList ();

  /**
   * \brief Reads each packet details, calculate delay and maps it to a tcp connection
   *
   * If the ip and port numbers for packet is not present in map, then it's a packet from
   * new connection, therefore insert conn_id into map
   *
   */
  void ProcessPacketList ();

  /**
   * \brief Prints the trace file
   *
   * Any line starting with '#' is comment.
   *
   */
  void PrintTraceFile ();

  /**
   * \brief Delets all the temporary files
   *
   */
  void DeleteTmpFiles ();

  /**
   * \brief Calculates delay for the packet
   *
   *
   * \param frameNum frame number of the packet
   * \param timeOut True if packet was timed out
   * \param httpReq True if packet is a http request
   * \param currTime Time of the last packet in the connection
   * \param packetTime Packet time
   *
   * \returns Calculated delay for the packet
   */
  double CalculatePacketDelay (uint32_t frameNum, bool timeOut, bool httpReq, double currTime, double packetTime);

  /**
   * \brief Matches the packet with the its connection and update connection details
   *
   *
   * \param id m_connId of the packet
   * \param size size of the packet
   * \param time time of the packet
   * \param frameNum frame number of the packet
   *
   */
  void ProcessPacket (m_connId id, uint32_t size, double time, uint32_t frameNum);
  /**
   * \brief Checks the input file for regular expression match.
   * Skips the comment lines (starting with '#').
   *
   *
   * \param infile input file stream
   * \param reg Regular expression
   *
   * \returns line (string) if regex matches
   */
  std::string CheckRegex (std::ifstream& infile, std::regex reg);
};

} // namespace ns3
#endif /* TRACE_REPLAY_HELPER_H */