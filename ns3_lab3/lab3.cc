#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lab3");

int main (int argc, char *argv[]) {
  // parse command line parameters
  CommandLine cmd;
  cmd.Parse (argc,argv);
  // log application info
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  
  // create p2p nodes: n0(A), n1(B), n2(C)
  NodeContainer p2pNodes1, p2pNodes2;
  p2pNodes1.Create (2);  // n0(A) n1(B)
  p2pNodes2.Add( p2pNodes1.Get(1) );  // n1(B)
  p2pNodes2.Create (1);  // n2(C)
  
  // create p2p channel between A and B and C
  PointToPointHelper pointToPoint;
  // p2p network between n0 and n1
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint.Install (p2pNodes1);
  // p2p network2 between n1 and n2
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint.Install (p2pNodes2);
  
  // create wifi nodes: n3 n4 n5 | n6 n7 n8 
  // wifi1 node net
  NodeContainer wifiNodes1;
  wifiNodes1.Create (3);  // n3 n4 n5
  NodeContainer wifiApNode1 = p2pNodes1.Get (0);  // n0
  // wifi2 node net
  NodeContainer wifiNodes2;
  wifiNodes2.Create (3);  // n6 n7 n8
  NodeContainer wifiApNode2 = p2pNodes2.Get (1);  // n2 
  
  // create wifi network
  // two wifi network: wifi1 and wifi2
  // wifi1 channel
  YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy1 = YansWifiPhyHelper::Default ();
  phy1.SetChannel (channel1.Create ());
  // wifi1 network
  WifiHelper wifi1;
  wifi1.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiMacHelper mac1;
  Ssid ssid1 = Ssid ("WLAN-SSID-1");
  // wifi1 station network
  mac1.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid1),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices1;
  staDevices1 = wifi1.Install (phy1, mac1, wifiNodes1);
  // wifi1 ap network
  mac1.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid1));
  NetDeviceContainer apDevices1;
  apDevices1 = wifi1.Install (phy1, mac1, wifiApNode1);
  // wifi2 channel
  YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
  phy2.SetChannel (channel2.Create ());
  // wifi2 network
  WifiHelper wifi2;
  wifi2.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiMacHelper mac2;
  Ssid ssid2 = Ssid ("WLAN-SSID-2");
  // wifi2 station network
  mac2.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices2;
  staDevices2 = wifi2.Install (phy2, mac2, wifiNodes2);
  // wifi2 ap network
  mac2.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));
  NetDeviceContainer apDevices2;
  apDevices2 = wifi2.Install (phy2, mac2, wifiApNode2);
  
  // setup station mobility
  // wifi1 mobility
  MobilityHelper mobility1;
  mobility1.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  // wifi1 sta
  mobility1.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility1.Install (wifiNodes1);
  // wifi1 ap
  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (wifiApNode1);
  // wifi2 mobility
  MobilityHelper mobility2;
  mobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  // wifi2 sta
  mobility2.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility2.Install (wifiNodes2);
  // wifi2 ap
  mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility2.Install (wifiApNode2);
  
  // intall internet stack
  InternetStackHelper stack;
  stack.Install (wifiApNode1);       // n0
  stack.Install (wifiNodes1);     // n3 n4 n5
  stack.Install (wifiApNode2);       // n2
  stack.Install (wifiNodes2);     // n6 n7 n8 
  stack.Install (p2pNodes1.Get(1));  // n1

  // setup address
  Ipv4AddressHelper address;
  
  // set p2p network1 
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign (p2pDevices1);

  // set p2p network2
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (p2pDevices2);

  //set apdevice1 address
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces1;
  staInterfaces1 = address.Assign (staDevices1);
  address.Assign (apDevices1);
  
  //set apdevice2 address
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces2;
  staInterfaces2 = address.Assign (staDevices2);
  address.Assign (apDevices2);
  
  // setup application layer
  // set UDP server as node8
  // UDP server function: echo
  UdpEchoServerHelper echoServer (7143); // port:9

  ApplicationContainer serverApps = echoServer.Install (wifiNodes2.Get (2));  // n8
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  
  // set UDP client as node3
  // set remote address and port
  // set num of packets as 2
  // set size of interval as 1
  // set size of packetsize as 1024
  UdpEchoClientHelper echoClient (staInterfaces2.GetAddress (2), 7143);  // n8:9
  echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (wifiNodes1.Get (0));  // n3
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // setup routing system
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));
  // enable trace p2p packets
  pointToPoint.EnablePcapAll ("lab3");
  // enable trace wifi packets
  phy1.EnablePcap("lab3", staDevices1.Get(0));
  phy2.EnablePcap("lab3", staDevices2.Get(2));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
