#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");
int main (int argc, char *argv[])
{
	CommandLine cmd;
	//cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
	//cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse (argc, argv);

	//if (verbose)
	//{
	//LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
	//LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
	//}

	//nCsma = nCsma == 0 ? 1 : nCsma; // makes sure you have at least one “extra” node.
	
	LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    
    //nodes
	NodeContainer p2pNodes,p2pNodes2;
	p2pNodes.Create (2);
	p2pNodes2.Add(p2pNodes.Get(1));
	p2pNodes.Create(1);
	
	//p2p channel
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (p2pNodes);

	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	NetDeviceContainer p2pDevices2;
	p2pDevices2 = pointToPoint.Install (p2pNodes2);
	
	//stack
	InternetStackHelper stack;
	stack.Install (p2pNodes.Get (0));
	stack.Install (p2pNodes2);

	Ipv4AddressHelper address,address2;
	address.SetBase ("192.168.10.0  ", "255.255.255.0");
	address2.SetBase ("192.168.50.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterfaces,p2pInterfaces2;
	p2pInterfaces = address.Assign (p2pDevices);	
	p2pInterfaces2 = address2.Assign(p2pDevices2);
	
	//onoff server
	Address remoteAddress (InetSocketAddress (p2pInterfaces2.GetAddress(1),1999));
	OnOffHelper onOffHelper ("ns3::TcpSocketFactory", remoteAddress);  
	onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	onOffHelper.SetAttribute("DataRate", StringValue("50000bps"));
	onOffHelper.SetAttribute("PacketSize", StringValue("512"));
	onOffHelper.SetAttribute("MaxBytes", StringValue("2000"));
	
	// Deploy the application and run
	ApplicationContainer serverApp = onOffHelper.Install (p2pNodes.Get (0));
	serverApp.Start (Seconds (1.0));
	serverApp.Stop (Seconds (3.0));
	
	// Setup SinkPacket Server
	Address LocalAddress (InetSocketAddress (p2pInterfaces2.GetAddress(1), 1999));
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", LocalAddress);
	ApplicationContainer clientApps = packetSinkHelper.Install (p2pNodes2.Get (1));
	clientApps.Start (Seconds (1.0));
	clientApps.Stop (Seconds (3.0));
	
	
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	pointToPoint.EnablePcapAll ("lab2");

	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}

