#include <string>
#include <fstream>
#include <cassert>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/animation-interface.h"
#include "ns3/node.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P1");



int main (int argc, char *argv[])
{
  uint32_t queueSize = 8000;
  uint32_t segSize = 128;
  uint32_t windowSize= 2000;
  uint32_t nFlows = 10;
  uint16_t port=10;
    
  CommandLine cmd;
  cmd.AddValue("queueSize","queue size",queueSize);
  cmd.AddValue("segSize","link rate",segSize);
  cmd.AddValue("windowSize","window size",windowSize);
  cmd.AddValue("nFlows","number of flows",nFlows);
  cmd.Parse(argc,argv);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));  
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize)); 
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
  
  //set the random seed
  RngSeedManager::SetSeed ( 11223344 );
  
  // Create the bottleneck nodes
  NodeContainer nodes;
  nodes.Create (2);
  NodeContainer n0n1 (nodes.Get (0), nodes.Get (1));

  //Create source nodes
  NodeContainer SourceNodes;
  SourceNodes.Create (nFlows);

  //Create sink nodes
  NodeContainer SinkNodes;
  SinkNodes.Create (nFlows);
  
  std::vector<NodeContainer> SourceList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      SourceList[i] = NodeContainer (SourceNodes.Get (i), nodes.Get(0));
    }

  std::vector<NodeContainer> SinkList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      SinkList[i] = NodeContainer (nodes.Get(1), SinkNodes.Get (i));
    }

  // Create the point-to-point links required by the topology
  PointToPointHelper p2p1;
  p2p1.SetDeviceAttribute ("DataRate", StringValue("5Mbps"));
  p2p1.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
  p2p1.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));
  p2p1.SetChannelAttribute ("Delay", TimeValue(MilliSeconds (10)));

  PointToPointHelper p2p2;
  p2p2.SetDeviceAttribute ("DataRate", StringValue("1Mbps"));
  p2p2.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
  p2p2.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES"));
  p2p2.SetChannelAttribute ("Delay", TimeValue(MilliSeconds (20)));

//create device for bottleneck link
  NetDeviceContainer d0d1 = p2p2.Install (n0n1);
 
 //create devices for source links 
  std::vector<NetDeviceContainer> SourceDeviceList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      SourceDeviceList[i] = p2p1.Install (SourceList[i]);
    }
  
  //create device for sink links
  std::vector<NetDeviceContainer> SinkDeviceList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      SinkDeviceList[i] = p2p1.Install (SinkList[i]);
    }


  // Install the internet stack on the nodes
  InternetStackHelper internet;
  internet.InstallAll ();

//assign IP address to bottleneck device
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interf01 = ipv4.Assign (d0d1);

//assign IP addresses to source devices
  std::vector<Ipv4InterfaceContainer> SourceInterfaceList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      std::ostringstream subnet;
      subnet<<"10.1."<<i+2<<".0";
      ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      SourceInterfaceList[i] = ipv4.Assign (SourceDeviceList[i]);
    }

//assisgn IP addresses to sink devices
  std::vector<Ipv4InterfaceContainer> SinkInterfaceList (nFlows);
  for(uint32_t i=0; i<(nFlows); i++)
    {
      std::ostringstream subnet;
      subnet<<"10.1."<<i+20<<".0";
      ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      SinkInterfaceList[i] = ipv4.Assign (SinkDeviceList[i]);
    }

  // Turn on global static routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
//generate random start times for source applications
  Ptr<UniformRandomVariable> RandomNum = CreateObject<UniformRandomVariable> ();
  RandomNum->SetAttribute ("Stream", IntegerValue(6110));
  RandomNum->SetAttribute ("Min", DoubleValue(0.0));
  RandomNum->SetAttribute ("Max", DoubleValue(0.1));  

  std::vector<double> start(nFlows); 
  double stop = 10.0;
  //build applications for source nodes
   std::vector<ApplicationContainer> sourceApps (nFlows);
   for(uint32_t i=0; i<(nFlows); i++)
    {
     BulkSendHelper source ("ns3::TcpSocketFactory",
                           InetSocketAddress (SinkInterfaceList[i].GetAddress(1), port));
     source.SetAttribute ("MaxBytes", UintegerValue (0));
     source.SetAttribute ("SendSize", UintegerValue (512));
     sourceApps[i] = source.Install (SourceNodes.Get (i));
     start[i] = RandomNum->GetValue();
     sourceApps[i].Start (Seconds (start[i]));
    }

    // Create application for sink nodes
    std::vector<ApplicationContainer> sinkApps (nFlows);
    for(uint32_t i=0; i<(nFlows); i++)
    {
     PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), port));
      sinkApps[i] = sink.Install (SinkNodes.Get (i));
    }
/*
  //Create animation file
  AnimationInterface anim("mynetanim.xml");
  for (uint32_t i=0; i<=1; i++)
  {
    anim.SetConstantPosition (nodes.Get(i), 100*(i+1), 200);
  }
  for (uint32_t i=0; i<nFlows; i++)
  {
    anim.SetConstantPosition (SourceNodes.Get(i), 0, 200+10*i );
  }
  for (uint32_t i=0; i<nFlows; i++)
  {
    anim.SetConstantPosition (SinkNodes.Get(i), 300, 200-10*i );
  }
*/
// Run the simulation
 Simulator::Stop (Seconds (10.0));
 Simulator::Run ();

//get the receive data from sink applications
    Ptr<PacketSink> sink1;
  for (uint32_t i=0; i<(nFlows);i++)
  {
    sink1 = (sinkApps[i].Get (0))->GetObject<PacketSink> ();
    double goodput = (sink1->GetTotalRx ())/(stop - start[i]);
    std::cout << "flow " << i << " windowSize " << windowSize << " queueSize " << queueSize <<" segSize " << segSize <<" goodput " << goodput <<  std::endl;
  }
      Simulator::Destroy ();
}
