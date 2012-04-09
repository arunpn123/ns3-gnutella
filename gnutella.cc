//#include "SimpleSimulation.h"
#include "ns3/buffer.h"
#include <iostream>
#include <ios>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-star-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-star.h"
#include "GnutellaApp.h"

NS_LOG_COMPONENT_DEFINE ("GnutellaSimulation");

void testResultSet()
{
	ns3::Buffer buf(1024);
	File f1("derp", buf);
	File f2("second", buf);
	
	std::vector<File*> result;
	result.push_back(&f1);
	result.push_back(&f2);
	
	FileContainer files;
	files.AddFile(&f1);
	files.AddFile(&f2);
	
	
	Result* result_set = new Result[result.size()];
	for (size_t i = 0; i < result.size(); i++)
	{
		result_set[i].file_index = (uint32_t) files.GetIndexByFile(result[i]);
		result_set[i].file_size = result[i]->GetData().GetSize();
		result_set[i].shared_file_name = result[i]->GetFileName();
	}
	
	for (size_t i = 0; i < result.size(); i++)
		NS_LOG_INFO (result_set[i].file_index << " " << result_set[i].file_size << " "
					<< result_set[i].shared_file_name);
}

void testDescriptor()
{
    const uint8_t descriptor[23] = 
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        //Payload descriptor
        0,
        //TTL
        4,
        //Hops
        2,
        //Payload_length
        0,0,0,0};

    Descriptor * d = Descriptor::Create(descriptor);
    if(d->GetType() == Descriptor::PING)
        NS_LOG_INFO ("Created PING Type");

    ns3::Buffer b;
    b.AddAtStart(d->GetSerializedSize());
    d->Serialize(b.Begin());

    uint8_t i_buf[23];

    b.CopyData(i_buf, 23);

    int cmp = memcmp(i_buf, descriptor, 23);

    if(cmp == 0)
        NS_LOG_INFO ("PING Serialized Correctly");
    else
        NS_LOG_INFO ("PING Serialization Failed at: " << std::dec << cmp);
    
}

void simOne(uint32_t nNodes)

{
    NS_LOG_INFO ("Starting Simulation");
    NodeContainer nodes;
    nodes.Create(nNodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    //Create an ethernet channel between the nodes
    CsmaHelper eth;
    eth.SetChannelAttribute("DataRate", DataRateValue(DataRate(5000000)));

    eth.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    eth.SetDeviceAttribute("Mtu", UintegerValue(1400));

    NetDeviceContainer devices = eth.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipf = ipv4.Assign(devices);

    ApplicationContainer apps;

    NS_LOG_INFO ("Creating Apps");

    Ptr<GnutellaApp> app;
    InetSocketAddress addr(ipf.GetAddress(0), DEFAULT_LISTENING_PORT);
    Address nullAddress;
    app = CreateObject<GnutellaApp>(addr, nullAddress);
//    NS_LOG_INFO ("Created App " << app);

    nodes.Get(0)->AddApplication(app);
    apps.Add(app);

    for(unsigned i=1; i<nodes.GetN(); i++)
    {
        Ptr<GnutellaApp> app;

        InetSocketAddress addr(ipf.GetAddress(i), DEFAULT_LISTENING_PORT);
        uint32_t bOff = NormalVariable().GetValue()*1000.0;
        if(bOff == 0)
            bOff = 1;
        
        InetSocketAddress boot(ipf.GetAddress((i+bOff) % (i)), DEFAULT_LISTENING_PORT);
        while (boot == addr)
        {
			bOff = NormalVariable().GetValue()*1000.0;
			if(bOff == 0)
				bOff = 1;
			boot.SetIpv4((ipf.GetAddress((i+bOff) % ipf.GetN())));
		}

        app = CreateObject<GnutellaApp>(addr, boot);
        nodes.Get(i)->AddApplication(app);
        apps.Add(app);
    }

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> fm = fmh.InstallAll();

    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (40.0));

    Simulator::Stop(Seconds(40.0));
    Simulator::Run ();

    std::map<FlowId, FlowMonitor::FlowStats> fs = fm->GetFlowStats();

    std::map<FlowId, FlowMonitor::FlowStats>::iterator it;

    uint32_t delay = 0;
    double lost_packets = 0;
    double qdv = 0;
    double j = 0;

    for(it = fs.begin(); it != fs.end(); it++)
    {
        lost_packets += (*it).second.lostPackets;
        qdv += (*it).second.jitterSum.GetSeconds();
        delay += (*it).second.delaySum.GetSeconds();
        j++;
    }
    
    NS_LOG_INFO ("Average Number of Lost Packets: " << lost_packets/j);
    NS_LOG_INFO ("Average PDV: " << qdv/j);
    NS_LOG_INFO ("Average Delay: " << delay/j);

    uint64_t totalqrt = 0;
    uint32_t totaln = 0;
    for (uint32_t n = 0; n < nodes.GetN(); n++)
    {
        Ptr<Node> node = nodes.Get(n);
        Ptr<GnutellaApp> gapp = Ptr<GnutellaApp> (dynamic_cast<GnutellaApp *> (PeekPointer (node->GetApplication(0))));
	if (gapp != NULL && gapp->GetAverageQueryResponseTime() > 0) {
	        totalqrt += gapp->GetAverageQueryResponseTime();
                totaln++;
        }
    }

    // It's in nanoseconds, so convert to milliseconds
    NS_LOG_INFO ("Average Query Response Time: " << (double) totalqrt/totaln/1000000 << " ms based on " << totaln << " samples");
    
    Simulator::Destroy ();

}


int main(int argc, char *argv[])
{
    uint32_t nPeers = 2;
    CommandLine cmd;
    cmd.AddValue("nPeers", "Number of peers to run simulation with", nPeers);
    cmd.Parse(argc, argv);

    if(nPeers > 254)
        NS_LOG_ERROR ("Error: Cannot run with more than 254 peers");

    LogComponentEnable ("GnutellaSimulation", LOG_LEVEL_INFO);
    //LogComponentEnable ("GnutellaDescriptor", LOG_LEVEL_INFO);
//    LogComponentEnable ("GnutellaPingDescriptor", LOG_LEVEL_INFO);
	  //LogComponentEnable ("GnutellaQueryHitDescriptor", LOG_LEVEL_INFO);
//    LogComponentEnable ("GnutellaFileContainer", LOG_LEVEL_INFO);
	  LogComponentEnable ("GnutellaApp", LOG_LEVEL_INFO);
      LogComponentEnable ("QueryLatencyContainer", LOG_LEVEL_INFO);

      simOne(nPeers);
}
