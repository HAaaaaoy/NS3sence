/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Thorben Ole Hellweg
 * Uni Münster
 */

/*
 * This example program allows one to run ns-3 DSDV and Eff-DSDV (and other
 * protocols) under a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time to reach a steady-state.
 * The number of nodes is 30, of which 10 act as sinks.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 10 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with 11Mbit/s (802.11b) and a Friis loss model.
 * The transmit power is set to 8.9048 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, specifying a value of 3 will cause
 * DSDV to be used and specifying a value of 5 will cause Eff-DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 256b/s each. This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a CSV file with simulation results and can be  items and
 * configured to record extensive simulation information such as
 * - Flowmonitor traces
 * - NetAnim simulation
 * - RoutingTables (including Eff-DSDV alternative tables not included in FlowMon)
 */

#include <fstream>
#include <iostream>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/aomdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/eff-dsdv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/aodvee-module.h"
#include "ns3/aodvKmeans-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-source.h"

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (uint32_t nWifis,
                uint32_t nSinks,
                double totalTime,
                std::string rate,
                std::string phyMode,
				double txp,
				uint32_t width,
  	  	  	  	uint32_t height,
                uint32_t nodeSpeed,
				uint32_t pauseTime,
                uint32_t periodicUpdateInterval,
                uint32_t settlingTime,
                double dataStart,
                bool printRoutes,
                std::string CSVfileName,
				uint32_t protocol,
				bool extensiveOutput,
				bool dsdvBufferEnabled
				//uint32_t seed
				);
  //static void SetMACParam (ns3::NetDeviceContainer & devices,
  //                                 int slotDistance);
  std::string CommandSetup (int argc, char **argv);




private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void CheckThroughput ();
  void SetupMobility();
  void InstallApplications();
  void Cheack(Ptr<FlowMonitor> &monitor, FlowMonitorHelper &flowmon);
//  void AodvPacketTrace (Ptr<const Packet> packet);

  uint32_t port;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
  uint32_t m_nWifis; ///< total number of nodes
  uint32_t m_nSinks; ///< number of receiver nodes
  double m_totalTime; ///< total simulation time (in seconds)
  std::string m_rate; ///< network bandwidth
  std::string m_phyMode; ///< remote station manager data mode
  uint32_t m_width; ///< simulation area x value
  uint32_t m_height; ///< simulation area y value
  uint32_t m_nodeSpeed; ///< mobility model node speed
  uint32_t m_pauseTime; ///< mobility model pause time
  uint32_t m_periodicUpdateInterval; ///< routing update interval
  uint32_t m_settlingTime; ///< routing setting time
  double m_dataStart; ///< time to start data transmissions (seconds)
  uint32_t bytesTotal; ///< total bytes received by all nodes
  uint32_t packetsReceived; ///< total packets received by all nodes
  bool m_printRoutes; ///< print routing table
  std::string m_CSVfileName; ///< CSV file name
  bool m_extensiveOutput;
 // std::ifstream m_file;

  NodeContainer m_nodes; ///< the collection of nodes
  Ipv4InterfaceContainer m_adhocInterfaces;
  NetDeviceContainer m_adhocDevices;
  DeviceEnergyModelContainer m_deviceModels;


public:
  //std::ifstream m_file;
  std::vector<std::string> GetNextLineAndSplitIntoTokens(std::istream& str);
};

uint64_t countAodv = 0, countRreq = 0, countRrep = 0, countRerr = 0, countRrepAck = 0;
Time firstAodv, lastAodv;
uint64_t bytesAodv = 0;
double totalEnergyComsupution = 0;

void
RemainingEnergy (double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now().GetSeconds()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
  totalEnergyComsupution += totalEnergy;
}

// Function for capturing AODV overhead statistics
void
AodvPacketTrace (Ptr<const Packet> packet)
{
//	std::string packetType;
//	std::ostringstream description;
//	aodvKmeans::TypeHeader tHeader;
//	Ptr<Packet> p = packet->Copy ();
//	p->RemoveHeader (tHeader);
//	if (!tHeader.IsValid ())
//	{
//		NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
//      return; // drop
//	}
//	switch (tHeader.Get ())
//	{
//	case aodvKmeans::MessageType::aodvKmeansTYPE_RREQ:
//	{
//		packetType = "RREQ";
//		aodvKmeans::RreqHeader rreqHeader;
//		p->RemoveHeader (rreqHeader);
//		description << "O:" << rreqHeader.GetOrigin () << " D:" << rreqHeader.GetDst () << " Hop:" << (int)rreqHeader.GetHopCount ();
//		countRreq++;
//		break;
//	}
//	case aodvKmeans::MessageType::aodvKmeansTYPE_RREP:
//    {
//    	packetType = "RREP";
//        aodvKmeans::RrepHeader rrepHeader;
//        p->RemoveHeader (rrepHeader);
//        description << "O:" << rrepHeader.GetOrigin () << " D:" << rrepHeader.GetDst () << " Hop:" << (int)rrepHeader.GetHopCount ();
//        countRrep++;
//        break;
//    }
//	case aodvKmeans::MessageType::aodvKmeansTYPE_RERR:
//	{
//        packetType = "RERR";
//        countRerr++;
//        break;
//	}
//    case aodvKmeans::MessageType::aodvKmeansTYPE_RREP_ACK:
//    {
//    	packetType = "RREP_ACK";
//    	countRrepAck++;
//    	break;
//    }
//	}
	std::string packetType2;
	std::ostringstream description2;
	aodv::TypeHeader tHeader2;
	Ptr<Packet> p2 = packet->Copy ();
	p2->RemoveHeader (tHeader2);
	if (!tHeader2.IsValid ())
	{
		NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader2.Get () << ". Drop");
		return; // drop
	}
	switch (tHeader2.Get ())
	{
	case aodv::MessageType::AODVTYPE_RREQ:
	{
		packetType2 = "RREQ";
		aodv::RreqHeader rreqHeader;
		p2->RemoveHeader (rreqHeader);
		description2 << "O:" << rreqHeader.GetOrigin () << " D:" << rreqHeader.GetDst () << " Hop:" << (int)rreqHeader.GetHopCount ();
		countRreq++;
		break;
	}
	case aodv::MessageType::AODVTYPE_RREP:
	{
		packetType2 = "RREP";
		aodv::RrepHeader rrepHeader;
		p2->RemoveHeader (rrepHeader);
		description2 << "O:" << rrepHeader.GetOrigin () << " D:" << rrepHeader.GetDst () << " Hop:" << (int)rrepHeader.GetHopCount ();
		countRrep++;
		break;
	}
	case aodv::MessageType::AODVTYPE_RERR:
	{
		packetType2 = "RERR";
		countRerr++;
		break;
	}
	case aodv::MessageType::AODVTYPE_RREP_ACK:
	{
		packetType2 = "RREP_ACK";
		countRrepAck++;
		break;
	}
	}
  countAodv++;

  if (countAodv==1)
  {
    firstAodv = Simulator::Now ();
  }
  lastAodv = Simulator::Now ();
  uint64_t packetSize = packet->GetSize ();
  bytesAodv += packetSize;
}


RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0)
{
}

//static inline std::string
//PrintReport (std::string message)
//{
//
//}
inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;
  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();
  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " --------------------------------------------------------received one packet from " << addr.GetIpv4 () << " packet size is " << packet->GetSize();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}


void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_nSinks << ","
      << m_protocolName << ","
      << m_txp << ""
      << std::endl;

  out.close ();
  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
  NS_LOG_UNCOND(Simulator::Now().GetSeconds());
}


Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));
  return sink;
}

int
main (int argc, char *argv[])
{
	  uint32_t nWifis = 50;
	  uint32_t nSinks = 10;
	  double totalTime = 300.0;
	  std::string rate ("256bps");
	  std::string phyMode ("DsssRate11Mbps");
	  uint32_t nodeSpeed = 25; // in m/s
	  uint32_t pauseTime = 5; // in m/s
	  uint32_t width = 1500; // in m
	  uint32_t height = 1500; // in m
	  std::string appl = "all";
	  uint32_t periodicUpdateInterval = 15;
	  uint32_t settlingTime = 6;
	  double dataStart = 0.0;
	  bool printRoutingTable = true;
	  std::string CSVfileName = "Manet_Compare";
	  bool traceMobility;
	  uint32_t protocol = 2;    //2,7,8
	  bool extensiveOutput = false;
	  bool dsdvBufferEnabled = true;
	 // uint32_t seed;

	  int runs = 10;
	  std::string config = "";

	  //SeedManager::SetSeed (1234);

	  RoutingExperiment experiment;
	  CommandLine cmd;
	  cmd.AddValue ("configFile", "Path to the config file", config);
	  cmd.AddValue ("runs", "Number of runs for each configuration[Default:10]", runs);
	  cmd.AddValue ("nWifis", "Number of wifi nodes[Default:30]", nWifis);
	  cmd.AddValue ("nSinks", "Number of wifi sink nodes[Default:10]", nSinks);
	  cmd.AddValue ("totalTime", "Total Simulation time[Default:100]", totalTime);
	  cmd.AddValue ("phyMode", "Wifi Phy mode[Default:DsssRate11Mbps]", phyMode);
	  cmd.AddValue ("rate", "CBR traffic rate[Default:8kbps]", rate);
	  cmd.AddValue ("pauseTime", "Pause time in RandomWayPoint model[Default:0]", pauseTime);
	  cmd.AddValue ("nodeSpeed", "Node speed in RandomWayPoint model[Default:10]", nodeSpeed);
	  cmd.AddValue ("width", "x value in RandomWayPoint model[Default:300]", width);
	  cmd.AddValue ("height", "y value in RandomWayPoint model[Default:1500]", height);
	  cmd.AddValue ("periodicUpdateInterval", "Periodic Interval Time[Default=15]", periodicUpdateInterval);
	  cmd.AddValue ("settlingTime", "Settling Time before sending out an update for changed metric[Default=6]", settlingTime);
	  cmd.AddValue ("dataStart", "Time at which nodes start to transmit data[Default=50.0]", dataStart);
	  cmd.AddValue ("printRoutingTable", "print routing table for nodes[Default:1]", printRoutingTable);
	  cmd.AddValue ("CSVfileName", "The name of the CSV output file name[Default:Manet_Compare]", CSVfileName);
	  cmd.AddValue ("traceMobility", "Enable mobility tracing", traceMobility);
	  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR;5=EFFDSDV", protocol);
	  cmd.AddValue ("extensiveOutput", "Additional out, including animation, routing tables and route tracking. NEEDS LOT OF FILE SPACE![Default=0]", extensiveOutput);
	  cmd.AddValue ("dsdvBufferEnabled", "Enables DSDV Buffer Features[Default=1]", dsdvBufferEnabled);
	  cmd.Parse (argc, argv);
//	  double txp = 8.9048;
	  double txp = 16.0206;
  if (config != "")
  {
	  std::ifstream file (config);
	  std::vector<std::string> v;
	  v = experiment.GetNextLineAndSplitIntoTokens(file);
	  //we ignore the first line
	  int simNumber = 1;
	  v = experiment.GetNextLineAndSplitIntoTokens(file);
	 	  while (!v.empty() && v.size()>1){
	 		  NS_LOG_UNCOND(  "Currently running simulation: " << std::to_string(simNumber));// << simNumber << "\n";
	 		  runs = std::stoi(std::string(v[0]));
	 		  nWifis = std::stoi(std::string(v[1]));
	 		  nSinks = std::stoi(std::string(v[2]));
	 		  totalTime = std::stod(std::string(v[3]));
	 		  nodeSpeed = std::stoi(std::string(v[4]));
	 		  periodicUpdateInterval = std::stoi(std::string(v[5]));
	 		  settlingTime = std::stoi(std::string(v[6]));
	 		  dataStart = std::stod(std::string(v[7]));
	 		  protocol = std::stoi(std::string(v[8]));
	 		  width = std::stoi(std::string(v[9]));
	 		  height = std::stoi(std::string(v[10]));
	 		  pauseTime = std::stoi(std::string(v[11]));
	 		  while(runs>0)
	 		  {
	 			  RoutingExperiment toRun;
	 			  //SeedManager::SetRun(seed);
	 			  NS_LOG_UNCOND( "  Remaining runs: "<< std::to_string(runs));
	 			  toRun.Run (nWifis, nSinks, totalTime, rate, phyMode, txp, nodeSpeed, pauseTime, width, height, periodicUpdateInterval, settlingTime, dataStart, printRoutingTable, CSVfileName, protocol, extensiveOutput, dsdvBufferEnabled);
	 			  runs = runs-1;
	 		  }
	 		 v = experiment.GetNextLineAndSplitIntoTokens(file);
	 		 simNumber ++;
	 	  }
	  file.close();
  }
  else
  {
	  experiment.Run(nWifis, nSinks, totalTime, rate, phyMode, txp, nodeSpeed, pauseTime, width, height, periodicUpdateInterval, settlingTime, dataStart, printRoutingTable, CSVfileName, protocol, extensiveOutput, dsdvBufferEnabled);
  }
}

void
RoutingExperiment::Run (uint32_t nWifis, uint32_t nSinks, double totalTime, std::string rate,
        std::string phyMode,double txp, uint32_t nodeSpeed, uint32_t pauseTime, uint32_t width, uint32_t height, uint32_t periodicUpdateInterval, uint32_t settlingTime,
        double dataStart, bool printRoutes, std::string CSVfileName, uint32_t protocol, bool extensiveOutput, bool dsdvBufferEnabled)
{
  Packet::EnablePrinting ();
  m_nSinks = nSinks;
  m_txp = txp;
  m_CSVfileName = CSVfileName;
  m_nWifis = nWifis;
  m_totalTime = totalTime;
  m_rate = rate;
  m_phyMode = phyMode;
  m_nodeSpeed = nodeSpeed;
  m_pauseTime = pauseTime;
  m_width = width;
  m_height = height;
  m_periodicUpdateInterval = periodicUpdateInterval;
  m_settlingTime = settlingTime;
  m_dataStart = dataStart;
  m_printRoutes = printRoutes;
  m_protocolName = "protocol";
  m_protocol = protocol;
  m_extensiveOutput = extensiveOutput;
  std::string tr_name ("_Manet_" + std::to_string(m_nWifis)
  	  	  	  	  	   + "Nodes_"+ std::to_string(m_nSinks)
  	  	  	  	  	   + "Sinks" + std::to_string(int(m_totalTime)) + "SimTime");

  Config::SetDefault ("ns3::OnOffApplication::PacketSize",StringValue ("512"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  EffDsdvHelper effdsdv;
  DsrMainHelper dsrMain;
  AodveeHelper aodvee;
  aodvKmeansHelper aodvkmeans;
  AomdvHelper aomdv;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  switch (m_protocol)
  {
    case 1:
    	list.Add (olsr, 100);
    	m_protocolName = "OLSR";
    	break;
    case 2:
    	list.Add (aodv, 100);
    	m_protocolName = "AODV";
    	break;
    case 3:
    	dsdv.Set("EnableBuffering",BooleanValue(dsdvBufferEnabled));
    	list.Add (dsdv, 100);
    	m_protocolName = "DSDV";
    	break;
    case 4:
    	m_protocolName = "DSR";
    	break;
    case 5:
    	list.Add (effdsdv, 100);
    	m_protocolName = "EFFDSDV";
      break;
    case 6:
    	list.Add (aodvee, 100);
    	m_protocolName = "AODVee";
    	break;
    case 7:
    	list.Add (aodvkmeans ,100);
    	m_protocolName = "AODVKmeans";
    	break;
    case 8:
    	list.Add (aomdv, 100);
    	m_protocolName = "AOMDV";
    	break;
    default:
    	NS_FATAL_ERROR ("No such protocol:" << m_protocol);
  }

  tr_name = m_protocolName + tr_name;
  m_nodes.Create (nWifis);
  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
//  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");zhege default limian you
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");

//  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(300.0));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0) );
  wifiPhy.Set ("RxGain", DoubleValue (0) );

  //transmission range 250m
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-71.9842));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-74.9842));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  m_adhocDevices = wifi.Install (wifiPhy, wifiMac, m_nodes);

  //energymodel
  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000.0));
  basicSourceHelper.Set ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (1.0)));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (m_nodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (1.6));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (1.2));
  radioEnergyHelper.Set ("IdleCurrentA", DoubleValue (1.15));
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.001));
//  radioEnergyHelper.Set ("SwitchingCurrentA", DoubleValue (1.0));
//  radioEnergyHelper.Set ("CcaBusyCurrentA", DoubleValue (0.000056));
  // install device model
  m_deviceModels = radioEnergyHelper.Install (m_adhocDevices, sources);
  /***************************************************************************/
  /** connect trace sources **/
  /***************************************************************************/
  // all sources are connected to node 1
  // energy source
  Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (1));
  basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
  // device energy model
  Ptr<DeviceEnergyModel> basicRadioModelPtr =
  basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
  NS_ASSERT (basicRadioModelPtr != NULL);
  basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));


  /***************************************************************************/
  if (m_protocol < 4 || m_protocol >= 5)
  {
      internet.SetRoutingHelper (list);
      internet.Install (m_nodes);
  }
  else
  {
	  internet.Install (m_nodes);
      dsrMain.Install (dsr, m_nodes);
  }
  SetupMobility ();

  NS_LOG_INFO ("assigning ip address");
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
//  Ipv4InterfaceContainer adhocInterfaces;
  m_adhocInterfaces = addressAdhoc.Assign (m_adhocDevices);
  InstallApplications();
  /**
   * Uncomment the following lines to switch to a n*(n-1) communication model
   * Useful to analyse protocols performance under high load situations
   */
//  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
//  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
//  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
//
//  for (uint32_t i = 0; i < m_nSinks; i++)
//    {
//      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), m_nodes.Get (i));
//
//      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
//      onoff1.SetAttribute ("Remote", remoteAddress);
//
//      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
//      ApplicationContainer temp = onoff1.Install (m_nodes.Get (i + nSinks));
//      temp.Start (Seconds (m_dataStart));
//      temp.Stop (Seconds (m_totalTime));
//    }

  std::stringstream ss;
  ss << nWifis;
  std::string nodes = ss.str ();

  std::stringstream ss2;
  ss2 << nodeSpeed;
  std::string sNodeSpeed = ss2.str ();

  std::stringstream ss3;
  ss3 << /*nodePause*/0;
  std::string sNodePause = ss3.str ();

  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();

  AsciiTraceHelper ascii;

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();

  NS_LOG_INFO ("Run Simulation.");

  AnimationInterface anim (tr_name + "_animation.xml");
  Ptr<OutputStreamWrapper> routingStream;
  if (true)
  {
	  //MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));
	  //anim.SetBackgroundImage("/home/yh/NS3/ns-allinone-3.30.1/ns-3.30.1/hjl/UAVswarmBackground.jpg", 0, 0, 2.3, 3.8, 1);
	  for(uint32_t i = 0; i<m_nWifis; i++)
	  {
//		anim.UpdateNodeDescription(i,std::to_string(i+1));
//		//xiugaijiedianyangshi
	  	anim.UpdateNodeSize (i, 150, 150);
		uint32_t resourceId = anim.AddResource("/home/yh/NS3/ns-allinone-3.30.1/ns-3.30.1/hjl/UAVicon.png");
		anim.UpdateNodeImage (i, resourceId);//optional
	  }
//	  	anim.SetBackgroundImage("/home/yh/NS3/ns-allinone-3.30.1/ns-3.30.1/hjl/UAVswarmBackground.jpg", -1, 1, 0.5, 0.5, 1);
	    anim.SetMaxPktsPerTraceFile(5000);
	    anim.SetMobilityPollInterval (Seconds (1));
	    //anim.EnablePacketMetadata (true);
	    anim.EnableWifiPhyCounters(Seconds(0),Seconds(m_totalTime));
	    anim.EnableIpv4L3ProtocolCounters(Seconds(0),Seconds(m_totalTime));

	    anim.EnableIpv4RouteTracking (tr_name+"_rt.xml", Seconds (0), Seconds (m_totalTime), Seconds (5)); //Optional
	    //routingStream = Create<OutputStreamWrapper> ((tr_name + ".routes"), std::ios::out);
	    //effdsdv.PrintRoutingTableAllEvery(Seconds(1),routingStream);
  }

//  Config::Connect("/NodeList/*/$ns3::aodvKmeans::RoutingProtocol/Tx", MakeCallback(&MacTxCallback));
  Config::ConnectWithoutContext("/NodeList/*/$ns3::aodvKmeans::RoutingProtocol/Tx", MakeCallback(&AodvPacketTrace));
  Config::ConnectWithoutContext("/NodeList/*/$ns3::aodv::RoutingProtocol/Tx", MakeCallback(&AodvPacketTrace));


  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();


  int epoch=1;
  for (DeviceEnergyModelContainer::Iterator iter = m_deviceModels.Begin (); iter != m_deviceModels.End (); iter ++)
  {
	  double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
	  NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
			  << "s) Total energy consumed by radio = " << energyConsumed << "J");
	  NS_ASSERT (energyConsumed <= 5000.0);
	  std::ofstream statistics;
	    std::ifstream f(m_CSVfileName+"_"+m_protocolName+"everyNodeEnergy"+".csv");
	    std::string header = "";
	    if (!f.good())
	    {
	  	 statistics.open(m_CSVfileName+"_"+m_protocolName+"everyNodeEnergy"+".csv", std::ios_base::app);
	  	 header = header + "Protocol,"
	  			 + "Nodes,"
	  			 + "Sinks,"
	  			 + "Simulation Time,"
	  			 + "DataStart,"
	  			 + "NodeSpeed,"
	  			 + "PauseTime,"
	  			 + "SimulationArea,"
	  			 + "Every EnergyConsumption";
	  	 statistics << header << std::endl;
	  	 statistics.close();
	    }
	    statistics.open(m_CSVfileName+"_"+m_protocolName+"everyNodeEnergy"+".csv", std::ios_base::app);
	      statistics << m_protocolName << ","
	      				<< m_nWifis << ","
	      	            << m_nSinks << ","
	      	            << m_totalTime << ","
	      	            << m_dataStart << ","
	  					<< m_nodeSpeed << ","
	  					<< m_pauseTime << ","
	  					<< width <<" x "<< height << ","
	  					<< energyConsumed << std::endl;
	      statistics.close();
	      epoch++;
  }

  Cheack(flowmon, flowmonHelper);
  flowmon->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();
  double txPacketsum = 0;
  double txBytesum = 0;
  double rxPacketsum = 0;
  double rxBytesum = 0;
  double routingPacketSum = 0;
  double lostPacketSum = 0;
  double throughput = 0;
  double delaySum = 0;
  double meanDelay = 0;
  double hopCount = 0;
  double transmittedBitrate = 0;
  uint32_t applicationTrafficFlows = 0; // all flows with dst port 9
  uint32_t emptyFlows = 0; //measures flows over which no data gets transmitted at all
  uint32_t infThroughputs = 0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	  if (t.destinationPort == 9)
	  {
		  applicationTrafficFlows +=1;
		  txPacketsum += i->second.txPackets;
		  txBytesum += i->second.txBytes;
		  rxPacketsum += i->second.rxPackets;
		  rxBytesum += i-> second.rxPackets;
		  lostPacketSum += i->second.lostPackets;

		  delaySum += i->second.delaySum.GetMilliSeconds();
		  if(i->second.rxPackets>0)
		  {
			  meanDelay += i->second.delaySum.GetMilliSeconds()/i->second.rxPackets;
			  hopCount += 1+(i->second.timesForwarded/(double)i->second.rxPackets);
			  double tmpThroughput = ((i->second.rxBytes * 8.0 / ((i->second.timeLastRxPacket-i->second.timeFirstRxPacket).GetSeconds())) / 1000) ;
			  if (!std::isinf(tmpThroughput)){
				  throughput += tmpThroughput;
			  }
			  else
			  {
				  infThroughputs += 1;
			  }
		  }
		  else
		  {
			  //meanDelay += i->second.delaySum.GetMilliSeconds();
			  emptyFlows +=1;
		  }
		  transmittedBitrate += ((i->second.txBytes * 8.0 / ((i->second.timeLastTxPacket-i->second.timeFirstTxPacket).GetSeconds())) / 1000) ;
	  }
	  else
	  {
		  routingPacketSum += i->second.txPackets;
	  }
  }

  std::ofstream statistics;
  std::ifstream f(m_CSVfileName+"_"+m_protocolName+".csv");
  std::string header = "";
  if (!f.good())
  {
	 statistics.open(m_CSVfileName+"_"+m_protocolName+".csv", std::ios_base::app);
	 header = header + "Protocol,"
			 + "Nodes,"
			 + "Sinks,"
			 + "Simulation Time,"
			 + "DataStart,"
			 + "NodeSpeed,"
			 + "PauseTime,"
			 + "SimulationArea,"
			 + "Tx_Packets,"
			 + "Rx_Packets,"
			 + "PDR,"
			 + "Routing_Packets_(not_including_broadcasts),"
			 + "LostPackets,"
			 + "Mean_End-to-End_Delay_in_ms,"
			 + "Transmitted_Bitrate_in_kbps,"
			 + "Throughput_in_kbps,"
			 + "Mean_Throughput_in_bps,"
			 + "Flows_without_transmission,"
			 + "Mean_Hop_Count,"
			 + "AodvOverhead,"
			 + "Total EnergyConsumption";
	 statistics << header << std::endl;
	 statistics.close();
  }
  statistics.open(m_CSVfileName+"_"+m_protocolName+".csv", std::ios_base::app);
    statistics << m_protocolName << ","
    				<< m_nWifis << ","
    	            << m_nSinks << ","
    	            << m_totalTime << ","
    	            << m_dataStart << ","
					<< m_nodeSpeed << ","
					<< m_pauseTime << ","
					<< width <<" x "<<height << ","
    	            << txPacketsum << ","
					<< rxPacketsum << ","
    	            << (rxPacketsum>0 ? ((rxPacketsum *100.0) / txPacketsum) : 0) << ","
    	            << routingPacketSum << ","
    	            << lostPacketSum << ","
    	            << (applicationTrafficFlows>0 ? meanDelay/(applicationTrafficFlows-emptyFlows) : meanDelay )<< ","
    	            << transmittedBitrate << ","
    	            << throughput << ","
					<< (throughput>0 ? (throughput*1000)/(applicationTrafficFlows-emptyFlows) : 0) << ","
					<< emptyFlows << ","
					<< (hopCount>0 ? (hopCount/(applicationTrafficFlows-emptyFlows)) : 0) << ","
					<< (double)bytesAodv/1000.0 <<","
					<< totalEnergyComsupution << std::endl;
    statistics.close();
    if (m_extensiveOutput)
      {
    	flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);
      }
  Simulator::Destroy ();
}

void
RoutingExperiment::SetupMobility ()
{
	 MobilityHelper mobilityAdhoc;
//	  std::ostringstream speedConstantRandomVariableStream;
//	  std::ostringstream pauseConstantRandomVariableStream;
//	  std::ostringstream xValueConstantRandomVariableStream;
//	  std::ostringstream yValueConstantRandomVariableStream;
//	  int64_t streamIndex = 0; // used to get consistent mobility across scenarios
//
//	      ObjectFactory pos;
//	      pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
//	      pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"));
//	      pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"));
//
//	      Ptr<PositionAllocator> positionAlloc = pos.Create()->GetObject<PositionAllocator>();
//	      streamIndex += positionAlloc->AssignStreams(streamIndex);
//
//	      std::stringstream ssSpeed;
//	      ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << m_nodeSpeed << "]";
//	      std::stringstream ssPause;
//	      ssPause << "ns3::ConstantRandomVariable[Constant=" << m_pauseTime << "]";
//
//	      mobilityAdhoc.SetMobilityModel(
//	          "ns3::RandomWaypointMobilityModel",
//	          "Speed", StringValue(ssSpeed.str()),
//	          "Pause", StringValue(ssPause.str()),
//	          "PositionAllocator", PointerValue(positionAlloc)
//	      );
//	      mobilityAdhoc.SetPositionAllocator(positionAlloc);
//	      mobilityAdhoc.Install(m_nodes);
//	      streamIndex += mobilityAdhoc.AssignStreams(m_nodes, streamIndex);
//
//
//	      NS_UNUSED(streamIndex);



	 //wo shang yici yong de
	   int64_t streamIndex = 0; // used to get consistent mobility across scenarios
//	   //
	   ObjectFactory pos;
	   pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
	   pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500]"));
	   pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500]"));

	   Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
	   streamIndex += taPositionAlloc->AssignStreams (streamIndex);
//////
	   std::stringstream ssSpeed;
	   ssSpeed << "ns3::UniformRandomVariable[Min=5.0|Max=" << m_nodeSpeed << "]";
//////
	   std::stringstream ssPause;
	   ssPause << "ns3::ConstantRandomVariable[Constant=" << m_pauseTime << "]";
	   mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
	                                   "Speed", StringValue (ssSpeed.str()),
	                                   "Pause", StringValue (ssPause.str()),
	                                   "PositionAllocator", PointerValue(taPositionAlloc));
	   mobilityAdhoc.SetPositionAllocator (taPositionAlloc);

	   mobilityAdhoc.Install (m_nodes);
	   streamIndex += mobilityAdhoc.AssignStreams (m_nodes, streamIndex);
	   NS_UNUSED (streamIndex); // From this point, streamIndex is unused

//	   ////GaussMarkovMobilityModel
//	   Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
//	   streamIndex += taPositionAlloc->AssignStreams (streamIndex);



//	   mobilityAdhoc.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
//	     "Bounds", BoxValue (Box (0, 1500, 0, 1500, 0, 150)),
//	     "TimeStep", TimeValue (Seconds (0.5)),
//	     "Alpha", DoubleValue (0.5),
////	     "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=5|Max=30]"),
//		 "MeanVelocity", StringValue (ssSpeed.str()),
//	     "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
//	     "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.3]"),
//	     "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=1|Bound=10]"),
//	     "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=1|Bound=10]"),
//	     "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=1|Bound=10]"));
//
//	   mobilityAdhoc.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
//	     "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1500]"),
//	     "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1500]"),
//	     "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150]"));
//	   mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
	   mobilityAdhoc.Install (m_nodes);
}

void
RoutingExperiment::InstallApplications ()
{
	/**
	 * Uncomment to experiment with n*(n-1) communication model
	 */
//  for (uint32_t i = 0; i <= m_nSinks - 1; i++ )
//    {
//      Ptr<Node> node = NodeList::GetNode (i);
//      Ipv4Address nodeAddress = node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
//      Ptr<Socket> sink = SetupPacketReceive (nodeAddress, node);
//    }
//
//  for (uint32_t clientNode = 0; clientNode <= m_nWifis - 1; clientNode++ )
//    {
//      for (uint32_t j = 0; j <= m_nSinks - 1; j++ )
//        {
//          OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (m_adhocInterfaces.GetAddress (j), port)));
//          onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
//          onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
//
//          if (j != clientNode)
//            {
//              ApplicationContainer apps1 = onoff1.Install (m_nodes.Get (clientNode));
//              Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
//              apps1.Start (Seconds (var->GetValue (m_dataStart, m_dataStart + 1)));
//              apps1.Stop (Seconds (m_totalTime));
//            }
//        }
//    }
	  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
	  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
	  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));




//	   for (int i = 0; i < m_nSinks; i++)
//	   {
//	       Ptr<Socket> sink = SetupPacketReceive (m_adhocInterfaces.GetAddress (i), m_nodes.Get (i));
//
//	       AddressValue remoteAddress (InetSocketAddress (m_adhocInterfaces.GetAddress (i), port));
//	       onoff1.SetAttribute ("Remote", remoteAddress);
//
//	       Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
//	       ApplicationContainer temp = onoff1.Install (m_nodes.Get (i + m_nSinks));
//	       temp.Start (Seconds (var->GetValue (0.0,1.0)));
//	       temp.Stop (Seconds (m_totalTime));
//	   }

//	   effientlimiande
	  for (uint32_t i = 0; i < m_nSinks; i++)
	    {
		  uint32_t tmp;
		  if (m_nSinks == m_nWifis)
		  {
			 tmp = i + (uint32_t)(m_nSinks/2) ;
		  }
		  else
		  {
			tmp  = i + m_nSinks;
		  }
		  if (tmp >= m_nWifis){
			  tmp = tmp - m_nWifis;
		  }
	      Ptr<Socket> sink = SetupPacketReceive (m_adhocInterfaces.GetAddress (i), m_nodes.Get (i));
	      //argurement is server's ip and address
	      AddressValue remoteAddress (InetSocketAddress (m_adhocInterfaces.GetAddress (i), port));
	      onoff1.SetAttribute ("Remote", remoteAddress);
	      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
	      ApplicationContainer temp = onoff1.Install (m_nodes.Get (tmp));//client App
	      temp.Start (Seconds (var->GetValue (0.0,1.0)));
	      temp.Stop (Seconds (m_totalTime));
	    }
}

void
RoutingExperiment::Cheack(Ptr<FlowMonitor> &monitor, FlowMonitorHelper &flowmon)
{
	uint32_t SentPackets = 0;
	uint32_t ReceivedPackets = 0;
	uint32_t LostPackets = 0;

	int j=0;
	float AvgThroughput = 0;
	Time Jitter;
	Time Delay;
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

		NS_LOG_UNCOND("----Flow ID:" <<iter->first);
		NS_LOG_UNCOND("Src Addr" <<t.sourceAddress << "Dst Addr "<< t.destinationAddress);
		NS_LOG_UNCOND("Sent Packets=" <<iter->second.txPackets);
		NS_LOG_UNCOND("Received Packets =" <<iter->second.rxPackets);
		NS_LOG_UNCOND("Lost Packets =" <<iter->second.txPackets-iter->second.rxPackets);
		NS_LOG_UNCOND("Packet delivery ratio =" <<iter->second.rxPackets*100/iter->second.txPackets << "%");
		NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%");
		NS_LOG_UNCOND("Delay =" <<iter->second.delaySum);
		NS_LOG_UNCOND("Jitter =" <<iter->second.jitterSum);
		NS_LOG_UNCOND("Throughput =" <<iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");

		SentPackets = SentPackets +(iter->second.txPackets);
		ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
		LostPackets = LostPackets + (iter->second.txPackets-iter->second.rxPackets);
		AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024);
		Delay = Delay + (iter->second.delaySum);
		Jitter = Jitter + (iter->second.jitterSum);

		j=j+1;
	}
		AvgThroughput = AvgThroughput/j;
		NS_LOG_UNCOND("--------Total Results of the simulation----------"<<std::endl);
		NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
		NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
		NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
		NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Average Throughput =" << AvgThroughput<< "Kbps");
		NS_LOG_UNCOND("End to End Delay =" << Delay);
		NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
		NS_LOG_UNCOND("Total Flod id " << j);
		NS_LOG_UNCOND("Total controlOverHead " << (double)bytesAodv/1000.0 << "Kbps");
		NS_LOG_UNCOND("Total EnergyConsumption " << totalEnergyComsupution << "J");

		monitor->SerializeToXmlFile("manet-routing.xml", true, true);
}

std::vector<std::string> RoutingExperiment::GetNextLineAndSplitIntoTokens(std::istream& str)
{
    std::vector<std::string>   result;
    std::string                line;
    std::getline(str,line);

    std::stringstream          lineStream(line);
    std::string                cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        result.push_back("");
    }
    return result;
}


