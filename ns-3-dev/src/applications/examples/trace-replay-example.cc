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
 *
 */

// Command to run simulation:
// Copy src/applications/examples/trace-replay-example.cc to scratch folder
// ./waf --run "scratch/trace-replay-example --pcapPath=src/applications/examples/trace-replay-sample.pcap --nWifi=1"
// pcapPath : path to input pcap file
// traceFilePath : path to input trace file
// nWifi    : Number of wifi client to simulate
//
// Default Network Topology
//
//     Wifi 10.1.3.0
//                   AP
//    *    *    *    *
//    |    |    |    |       10.1.1.0
// nWifi   n2   n1   n0 ------------------ Server
//                        point-to-point
//
// This example creates network as shown above. All the clients
// are share 802.11g wireless network and connect to a single server.
// TraceReplayHelper installs TraceReplayClient on each client and
// corresponding TraceReplayServer, for each connection.
//
// The input pcap file must contain only one client.
// Different nodes can be given different pcap file as input to simulate 
// different application/user behavior.
// TraceReplay will analyze each connection present in pcap file
// (such as number of connection, application/user delays for packet, size of request etc)
// and will replay it for every node which is given that pcap as input.
//
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TraceReplayExample");

int
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  uint32_t nWifi = 1;
  std::string pcapPath = "";
  std::string traceFilePath = "";

  CommandLine cmd;
  cmd.AddValue ("pcapPath", "path to input pcap file", pcapPath);
  cmd.AddValue ("traceFilePath", "path to input trace file", traceFilePath);
  cmd.AddValue ("nWifi", "Number of client", nWifi);
  cmd.Parse (argc, argv);

  Time stopTime = Seconds(1000);

  NS_LOG_INFO ("Create p2p nodes.");
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NS_LOG_INFO ("Create p2p channels.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100MBps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("3ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NS_LOG_INFO ("Create wifi nodes.");
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  NS_LOG_INFO ("Create wireless channel.");
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  WifiHelper wifi;

  wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (2.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (2),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (1));
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);

  NS_LOG_INFO ("Create TraceReplay on each wifi client and server pair.");
  for (uint32_t i = 0; i < nWifi; i++)
    {
      TraceReplayHelper application (DataRate ("25MBps"));
      application.SetPcap (pcapPath);
      application.AssignStreams (i);
      application.SetStopTime (stopTime);
      application.SetPortNumber (49153 + 200 * i);
      application.Install (wifiStaNodes.Get (i), p2pNodes.Get (1), Address (p2pInterfaces.GetAddress (1)));
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (stopTime);
  pointToPoint.EnablePcapAll ("traceReplayTest");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  return 0;
}