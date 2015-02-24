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
#include "ns3/random-variable-stream.h"
//////////////////////////////////

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
	uint32_t nRightNodes = 15;
	uint32_t queueSize = 2000;
	uint32_t segSize = 512;
	uint32_t windowSize = 2000;
	uint32_t maxPacketSize = 512;

	double minTh = 3000;
	double maxTh = 4000;



	std::string dataRate = "5Mbps";
	std::string RdataRate = "2Mbps";
	std::string delay = "5ms";
	std::string Rdelay = "10ms";
	std::string queueType = "droptail";
	std::string udpDateRate ="0.1Mbps";

	CommandLine cmd;
	cmd.AddValue ("queueSize", "Maxmium size of queue", queueSize);
	cmd.AddValue ("queueType", "Type of queue", queueType);
	cmd.AddValue ("segSize", "SegmentSize", segSize);
	cmd.AddValue ("windowSize", "TCP Window Size", windowSize);
	cmd.AddValue ("maxPacketSize", "Maximum packet size for source application", maxPacketSize);
	cmd.AddValue ("minTh", "min threshold of RED", minTh);
	cmd.AddValue ("maxTh", "max threshold of RED", maxTh);
	cmd.AddValue ("udpDateRate", "date rate for udp source", udpDateRate);
	cmd.Parse (argc,argv);


	Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));  
  	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
  	Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (windowSize)); 
  	Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));

//create nodes
	NodeContainer routers;
	routers.Create (3);
	std::vector<NodeContainer> routerList(3);
	for (uint32_t i=0; i<2; i++)
	{
		routerList[i] = NodeContainer (routers.Get(i), routers.Get(i+1));
	}
//	routerList[2] = NodeContainer (routers.Get(1), routers.Get(3));

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
		midNodeList[i] = NodeContainer (midNodes.Get(i), routers.Get(1));
	}





    Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
    Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (queueSize));
    Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (50));


//Build link between host to router
	PointToPointHelper HostToRouter;
	HostToRouter.SetDeviceAttribute ("DataRate", StringValue (dataRate));
	HostToRouter.SetChannelAttribute ("Delay", StringValue (delay));
	if (queueType == "droptail")
	{
		HostToRouter.SetQueue ("ns3::DropTailQueue", 
										"Mode", StringValue("QUEUE_MODE_BYTES"),
										"MaxBytes", UintegerValue(queueSize));
	}
	else
	{
		HostToRouter.SetQueue ("ns3::RedQueue", 
								"MinTh", DoubleValue (minTh),
								"MaxTh", DoubleValue (maxTh), 
								"LinkBandwidth", StringValue(RdataRate),
								"LinkDelay", StringValue(Rdelay));																					
	}	

//Build link between router to router
	PointToPointHelper RouterToRouter;
	RouterToRouter.SetDeviceAttribute ("DataRate", StringValue (RdataRate));
	RouterToRouter.SetChannelAttribute ("Delay", StringValue (Rdelay));
	if (queueType == "droptail")
	{
		RouterToRouter.SetQueue ("ns3::DropTailQueue", 
								 	"Mode", StringValue("QUEUE_MODE_BYTES"),
									"MaxBytes", UintegerValue(queueSize));
	}
	else
	{
		RouterToRouter.SetQueue ("ns3::RedQueue", 
								   "MinTh", DoubleValue (minTh),
								   "MaxTh", DoubleValue (maxTh),
				   				   "LinkBandwidth", StringValue(RdataRate),
								   "LinkDelay", StringValue(Rdelay));
		}	


//Build the device
	std::vector<NetDeviceContainer> routerDeviceList(2);
	for (uint32_t i=0; i<2; i++)
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
  	
  	std::vector<Ipv4InterfaceContainer> routerInterfaceList(2);
  	for (uint32_t i=0;i<2;i++)
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


	//set the random seed
	 RngSeedManager::SetSeed ( 11223344 );
	


	//generate random start times for source applications
	 Ptr<UniformRandomVariable> RandomNum = CreateObject<UniformRandomVariable> ();
	 RandomNum->SetAttribute ("Stream", IntegerValue(6110));
	 RandomNum->SetAttribute ("Min", DoubleValue(0.0));
	 RandomNum->SetAttribute ("Max", DoubleValue(0.1));  



	std::vector<double>tcpstart(nLeftNodes); //start time for left nodes
	std::vector<double>udpstart(nMidNodes); //start time for middle node
	std::vector<double>udpstart2(nLeftNodes); //start time for left node udp flow


//	double stop = 10.0;
	uint32_t tcpPort = 50;
	uint32_t udpPort = 60;


//create tcp source
	std::vector<ApplicationContainer> leftClientApps(nLeftNodes);
	for (uint32_t i=0; i<nLeftNodes; i++)
	{
		BulkSendHelper leftClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (rightInterfaceList[i].GetAddress(1), tcpPort));
		leftClientHelper.SetAttribute ("MaxBytes", UintegerValue(0));
		leftClientHelper.SetAttribute ("SendSize", UintegerValue(maxPacketSize));
		leftClientApps[i].Add (leftClientHelper.Install (leftNodes.Get(i)));
		tcpstart[i] = RandomNum->GetValue();
		leftClientApps[i].Start (Seconds(tcpstart[i]));
	//	leftClientApps[i].Stop (Seconds(stop));
	}


//create tcp sink for left nodes
	std::vector<ApplicationContainer> tcpsinkApps (nLeftNodes);
	for(uint32_t i=0; i<nLeftNodes; i++)
    {
    		PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
      		tcpsinkApps[i] = sink.Install (rightNodes.Get (i));
    }

//Create udp sink for middle nodes
    std::vector<ApplicationContainer> udpsinkApps (nMidNodes);
	for(uint32_t i=0; i<nMidNodes; i++)
    {
    		PacketSinkHelper sink ("ns3::UdpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), udpPort));
      		udpsinkApps[i] = sink.Install (rightNodes.Get (i + nLeftNodes));
    }

//Create udp sink for left nodes
 	std::vector<ApplicationContainer> udpsinkApps2 (nLeftNodes);
	for(uint32_t i=0; i<nLeftNodes; i++)
    {
    		PacketSinkHelper sink ("ns3::UdpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), udpPort));
      		udpsinkApps2[i] = sink.Install (rightNodes.Get (i + nLeftNodes + nMidNodes));
    }


//Create udp source for middle nodes
	std::vector<ApplicationContainer> midClientApps (nMidNodes);
	for (uint32_t i=0; i<nMidNodes; i++)
	{
		OnOffHelper midClientHelper ("ns3::UdpSocketFactory", Address ());
		midClientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
		midClientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
		midClientHelper.SetAttribute ("PacketSize", UintegerValue (maxPacketSize));
		midClientHelper.SetAttribute ("DataRate", StringValue (udpDateRate));
		AddressValue remoteAddress (InetSocketAddress (rightInterfaceList[i+nLeftNodes].GetAddress(1), udpPort));
		midClientHelper.SetAttribute ("Remote", remoteAddress);
		midClientApps[i].Add (midClientHelper.Install (midNodes.Get(i)));
		udpstart[i] = RandomNum->GetValue();
		midClientApps[i].Start(Seconds(udpstart[i]));
	}


//Create udp source for left nodes
		std::vector<ApplicationContainer> udpleftClientApps (nLeftNodes);
	for (uint32_t i=0; i<nLeftNodes; i++)
	{
		OnOffHelper udpleftClientHelper ("ns3::UdpSocketFactory", Address ());
		udpleftClientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
		udpleftClientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
		udpleftClientHelper.SetAttribute ("PacketSize", UintegerValue (maxPacketSize));
		udpleftClientHelper.SetAttribute ("DataRate", StringValue (udpDateRate));
		AddressValue remoteAddress (InetSocketAddress (rightInterfaceList[i+nLeftNodes+nMidNodes].GetAddress(1), udpPort));
		udpleftClientHelper.SetAttribute ("Remote", remoteAddress);
		udpleftClientApps[i].Add (udpleftClientHelper.Install (leftNodes.Get(i)));
		udpstart2[i] = RandomNum->GetValue();
		udpleftClientApps[i].Start(Seconds(udpstart2[i]));
	}







/*
 //Create animation file
  AnimationInterface anim("mynetanim.xml");
  for (uint32_t i=0; i<=2; i++)
  {
    anim.SetConstantPosition (routers.Get(i), 100*(i+1), 200);
  }
 // anim. SetConstantPosition (routers.Get(3), 200, 300);
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
*/

	Simulator::Stop (Seconds (10));
	Simulator::Run ();

	std::cout<<std::endl;
	if(queueType == "droptail")
		{
			std::cout<<"host to router link: "<<" DataRate: "<<dataRate<<" Delay: "<<delay<<std::endl<<"bottleneck link: "<<" DataRate: "<<RdataRate<<" Delay: "<<Rdelay<<std::endl;
			std::cout<<"queueType: "<<queueType<<" queueSize: "<<queueSize<<std::endl;
			std::cout<<"TCP flows: "<<" windowSize: "<<windowSize<<" SegmentSize: "<<segSize<<std::endl;
		
			std::cout<<"UDP flows: "<< " maxPacketSize: "<<maxPacketSize<<std::endl;
		}
	if(queueType =="RED")
		{
			std::cout<<"host to router link: "<<" DataRate: "<<dataRate<<" Delay: "<<delay<<std::endl<<"bottleneck link: "<<" DataRate: "<<RdataRate<<" Delay: "<<Rdelay<<std::endl;
			std::cout<<"queueType: "<<queueType<<" queueLimit: "<<queueSize<<" MaxTh: "<<maxTh<<" MinTh: "<<minTh<<std::endl;
			std::cout<<"TCP flows: "<<" windowSize: "<<windowSize<<" SegmentSize: "<<segSize<<std::endl;
			std::cout<<"UDP flows: "<< " maxPacketSize: "<<maxPacketSize<<std::endl;
		}
		
	

//Get throughout for left nodes with tcp flows
	Ptr<PacketSink> sink1;
	double tcpTotalRecv = 0;
  for (uint32_t i=0; i<nLeftNodes;i++)
  {
    sink1 = (tcpsinkApps[i].Get (0))->GetObject<PacketSink> ();
    double goodput = (sink1->GetTotalRx ())/(10-tcpstart[i]);
    std::cout << "TCP goodput: " << goodput <<std::endl;
    tcpTotalRecv = tcpTotalRecv + goodput;       
  }
  	std::cout<<"			Left_nodes_TCP_flows_average_goodput: "<<tcpTotalRecv/nLeftNodes<<std::endl<<std::endl;


//Get throughout for middle nodes with udp flows
  Ptr<PacketSink> sink2;
  double udpTotalRecv =0;
  for (uint32_t i=0; i<nMidNodes;i++)
  {
    sink2 = (udpsinkApps[i].Get(0))->GetObject<PacketSink> ();
    double goodput = (sink2->GetTotalRx())/(10-udpstart[i]);
    std::cout << "UDP goodput: " << goodput <<std::endl; 
    udpTotalRecv =udpTotalRecv + goodput;
  }
  	std::cout<<"			Middle_nodes_UDP_flows-average_goodput: "<<udpTotalRecv/nMidNodes<<std::endl<<std::endl;



//Get throughout for left nodes with udp flows
  	 Ptr<PacketSink> sink3;
  double udpTotalRecv2 =0;
  for (uint32_t i=0; i<nLeftNodes;i++)
  {
    sink3 = (udpsinkApps2[i].Get(0))->GetObject<PacketSink> ();
    double goodput = (sink3->GetTotalRx())/(10-udpstart2[i]);
    std::cout << "UDP goodput: " << goodput <<std::endl; 
    udpTotalRecv2 = udpTotalRecv2 + goodput;
  }
  	std::cout<<"			Left_nodes_UDP_flows_average_goodput: "<<udpTotalRecv2/nLeftNodes<<std::endl<<std::endl<<"						total_average_goodput: "<<(tcpTotalRecv + udpTotalRecv + udpTotalRecv2) / nRightNodes<<std::endl<<std::endl;







      Simulator::Destroy ();
}
