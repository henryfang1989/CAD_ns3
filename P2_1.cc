#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/red-queue.h"


using namespace ns3;

/***********************************************************

Topology:

									|------------d1
									|------------d2
n1-----------|                      |------------d3
n2-----------|						|------------d4
n3----------r1----------r2---------r3------------d5
n4-----------|			|			|------------d6
n5-----------|          |			|------------d7
                        |			|------------d8
               ---------r4--------	|------------d9
                |   |   |   |   | 	|------------d10
				|	|	|	|	|
				m1	m2	m3	m4	m5

**************************************************************/

int main(int argc, char *argv[])
{
	uint32_t nLeftNodes = 5;
	uint32_t nMidNodes = 5;
	uint32_t nRightNodes = 10;
	uint32_t queueSize = 2000;
	uint32_t segSize = 512;
	uint32_t windowSize = 1000;
	std::string dataRate = "5Mbps";
	std::string RdataRate = "5Mbps";
	std::string delay = "10ms";
	std::string Rdelay = "5ms";
	std::string queueType = "droptail";
	//std::string animFile = "p2_animation.xml";

	CommandLine cmd;
	//cmd.AddValue ("nLeftLeaf", "Number of left side leaf nodes", nLeftLeaf);
	//cmd.AddValue ("nMidLeaf", "Number of middle leaf nodes", nMidLeaf);
	//cmd.AddValue ("nRightLeaf", "Number of right side leaf nodes");
	//cmd.AddValue ("animFile", "Name of animation file", animFile);
	cmd.AddValue ("queueSize", "Maxmium size of queue", queueSize);
	cmd.AddValue ("queueType", "Type of queue", queueType);
	cmd.Parse (argc,argv);


	Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));  
  	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
  	Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize)); 
  	Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));

//create nodes
	NodeContainer routers;
	routers.Create (4);
	std::vector<NodeContainer> routerList(3);
	for (uint32_t i=0; i<2; i++)
	{
		routerList[i] = NodeContainer (routers.Get(i), routers.Get(i+1));
	}
	routerList[2] = NodeContainer (routers.Get(1), routers.Get(3));

	NodeContainer leftNodes;
	leftNodes.Create(nLeftNodes);
	std::vector<NodeContainer> leftNodeList(nLeftNodes);
	for (uint32_t i=0; i<nLeftNodes; i++)
	{
		leftNodeList[i] = NodeContainer (leftNodes.Get(i), routers.Get(0));
	}

	NodeContainer rightNodes;
	rightNodes.Create(nRightNodes);
	std::vector<NodeContainer> rightNodeList(nRightNodes);
	for (uint32_t i=0; i<nRightNodes; i++)
	{
		rightNodeList[i] = NodeContainer (routers.Get(2), rightNodes.Get(i));
	}

	NodeContainer midNodes;
	midNodes.Create(nMidNodes);
	std::vector<NodeContainer> midNodeList(nMidNodes);
	for (uint32_t i=0; i<nMidNodes; i++)
	{
		midNodeList[i] = NodeContainer (midNodes.Get(i), routers.Get(3));
	}



//Build link between host to router
	PointToPointHelper HostToRouter;
	HostToRouter.SetDeviceAttribute ("DataRate", StringValue (dataRate));
	HostToRouter.SetChannelAttribute ("Delay", StringValue (delay));
	if (queueType == "droptail")
	{
		HostToRouter.SetQueue ("ns3::DropTailQueue", "Mode", StringValue("QUEUE_MODE_BYTES"));
		HostToRouter.SetQueue ("ns3::DropTailQueue", "MaxBytes", UintegerValue(queueSize));
	}
	else
	{

	}	

//Build link between router to router
	PointToPointHelper RouterToRouter;
	RouterToRouter.SetDeviceAttribute ("DataRate", StringValue (RdataRate));
	RouterToRouter.SetChannelAttribute ("Delay", StringValue (Rdelay));
	if (queueType == "droptail")
	{
		RouterToRouter.SetQueue ("ns3::DropTailQueue", "Mode", StringValue("QUEUE_MODE_BYTES"));
		RouterToRouter.SetQueue ("ns3::DropTailQueue", "MaxBytes", UintegerValue(queueSize));
	}
	else
	{

	}	


//Build the device
	std::vector<NetDeviceContainer> routerDeviceList(3);
	for (uint32_t i=0; i<3; i++)
	{
		routerDeviceList[i] = RouterToRouter.Install(routerList[i]);
	}

	std::vector<NetDeviceContainer> leftDeviceList (nLeftNodes);
	for (uint32_t i=0;i<nLeftNodes;i++)
	{
		leftDeviceList[i] = HostToRouter.Install(leftNodeList[i]);
	}

	std::vector<NetDeviceContainer> rightDeviceList (nRightNodes);
	for (uint32_t i=0;i<nRightNodes;i++)
	{
		rightDeviceList[i] = HostToRouter.Install(rightNodeList[i]);
	}

	std::vector<NetDeviceContainer> midDeviceList (nMidNodes);
	for (uint32_t i=0;i<nMidNodes;i++)
	{
		midDeviceList[i] = HostToRouter.Install(midNodeList[i]);
	}


//install IP address
 	InternetStackHelper internet;
 	internet.InstallAll ();

  	Ipv4AddressHelper ipv4;
  	
  	std::vector<Ipv4InterfaceContainer> routerInterfaceList(3);
  	for (uint32_t i=0;i<3;i++)
  	{
  		std::ostringstream subnet;
      	subnet<<"10.1."<<i<<".0";
      	ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      	routerInterfaceList[i] = ipv4.Assign (routerDeviceList[i]);
  	}

  	std::vector<Ipv4InterfaceContainer> leftInterfaceList(nLeftNodes);
  	for (uint32_t i=0;i<nLeftNodes;i++)
  	{
  		std::ostringstream subnet;
      	subnet<<"10.1."<<i+3<<".0";
      	ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      	leftInterfaceList[i] = ipv4.Assign (leftDeviceList[i]);
  	}

  	std::vector<Ipv4InterfaceContainer> midInterfaceList(nMidNodes);
  	for (uint32_t i=0;i<nMidNodes;i++)
  	{
  		std::ostringstream subnet;
      	subnet<<"10.1."<<i+3+nLeftNodes<<".0";
      	ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      	midInterfaceList[i] = ipv4.Assign (midDeviceList[i]);
  	}

    std::vector<Ipv4InterfaceContainer> rightInterfaceList(nRightNodes);
  	for (uint32_t i=0;i<nRightNodes;i++)
  	{
  		std::ostringstream subnet;
      	subnet<<"10.1."<<i+3+nLeftNodes+nMidNodes<<".0";
      	ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      	rightInterfaceList[i] = ipv4.Assign (rightDeviceList[i]);
  	}

//turn on global static routing
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


	uint32_t start = 0;
	uint32_t stop = 10.0;
	uint32_t tcpPort = 50;
	uint32_t udpPort = 60;

//create tcp source
	ApplicationContainer leftClientApps;
	for (uint32_t i=0; i<nLeftNodes; i++)
	{
		BulkSendHelper leftClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (rightInterfaceList[i].GetAddress(1), tcpPort));
		leftClientHelper.SetAttribute ("MaxBytes", UintegerValue(0));
		leftClientHelper.SetAttribute ("SendSize", UintegerValue(512));
		leftClientApps.Add (leftClientHelper.Install (leftNodes.Get(i)));
	}

//start tcp source
	leftClientApps.Start (Seconds(start));
	leftClientApps.Stop (Seconds(stop));

//create tcp sink
	std::vector<ApplicationContainer> tcpsinkApps (nLeftNodes);
	for(uint32_t i=0; i<nLeftNodes; i++)
    {
    		PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
      		tcpsinkApps[i] = sink.Install (rightNodes.Get (i));
    }

//create udp sink
   	
   	std::vector<ApplicationContainer> udpsinkApps (nMidNodes);
   	for (uint32_t i=0; i<nMidNodes; i++)
   	{
   		UdpServerHelper server (udpPort);
   		udpsinkApps[i] = server.Install (rightNodes.Get(i+nLeftNodes));
   		udpsinkApps[i].Start (Seconds(start));
		udpsinkApps[i].Stop (Seconds(stop));
   	}


//create udp source
   	uint32_t MaxPacketSize = 128;
   	Time interPacketInterval = Seconds (0.01);
   	uint32_t MaxPacketCount = 320000;

	ApplicationContainer midClientApps;
	for (uint32_t i=0; i<nMidNodes; i++)
	{
		UdpClientHelper midClientHelper(Address(rightInterfaceList[i+nLeftNodes].GetAddress(1)), udpPort);
		midClientHelper.SetAttribute ("MaxPackets", UintegerValue (MaxPacketCount));
		midClientHelper.SetAttribute ("Interval", TimeValue (interPacketInterval));
		midClientHelper.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
		midClientApps.Add (midClientHelper.Install (midNodes.Get(i)));
		//http://www.nsnam.org/doxygen/classns3_1_1_udp_client.html
	}
//start udp source
	midClientApps.Start (Seconds(start));
	midClientApps.Stop (Seconds(stop));

	


 //Create animation file
  AnimationInterface anim("mynetanim.xml");
  for (uint32_t i=0; i<=2; i++)
  {
    anim.SetConstantPosition (routers.Get(i), 100*(i+1), 200);
  }
  anim. SetConstantPosition (routers.Get(3), 200, 300);
  for (uint32_t i=0; i<nLeftNodes; i++)
  {
    anim.SetConstantPosition (leftNodes.Get(i), 0, 150+20*i );
  }
  for (uint32_t i=0; i<nMidNodes; i++)
  {
    anim.SetConstantPosition (midNodes.Get(i), 150+20*i, 400);
  }
  for (uint32_t i=0;i<nRightNodes;i++)
  {
  	anim.SetConstantPosition (rightNodes.Get(i), 400, 100+20*i);
  }


	Simulator::Stop (Seconds (10));
	Simulator::Run ();


	Ptr<PacketSink> sink1;
  for (uint32_t i=0; i<nLeftNodes;i++)
  {
    sink1 = (tcpsinkApps[i].Get (0))->GetObject<PacketSink> ();
    double goodput = (sink1->GetTotalRx ())/10;
    std::cout << "goodput: " << goodput <<std::endl;
    //std::cout << "flow " << i << " windowSize " << windowSize << " queueSize " << queueSize <<" segSize " << segSize <<" goodput " << goodput <<  std::endl;
  }


  Ptr<UdpServer> sink2;
  for (uint32_t i=0; i<nMidNodes;i++)
  {
    sink2 = (udpsinkApps[i].Get(0))->GetObject<UdpServer> ();
    double goodput = (sink2->GetReceived())*MaxPacketSize/10;
    std::cout << "goodput: " << goodput <<std::endl;
    //std::cout << "flow " << i << " windowSize " << windowSize << " queueSize " << queueSize <<" segSize " << segSize <<" goodput " << goodput <<  std::endl;
  }
  
      Simulator::Destroy ();
}