#include <random>
#include <map>
#include <string>
#include <algorithm> 
#include <numeric>   
#include <set>
#include <vector>
#include <memory>    
#include <iomanip>   
#include <sstream>   
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/trace-helper.h"
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("CN_ASSIGNMENT_4_DISCRTE_EVENT_NETWORK_SIMULATION_USING_CUSTOM_TOPOLOGY");

const int num_routers = 4;
const int num_workstations = 5;
const int packet_size = 256;
const double total_simulation_time = 60;
const double fixed_error_rate = 0.00001;
double trafficVals[5][5] = {
    {0.0, 67.0, 56.0, 68.0, 69.0},
    {64.0, 0.0, 63.0, 62.0, 65.0},
    {65.0, 66.0, 0.0, 73.0, 64.0},
    {64.0, 65.0, 63.0, 0.0, 72.0},
    {61.0, 62.0, 63.0, 64.0, 0.0}
};

//                          [TOPOLOGY]
//                    s1         s2        s3
//                     ^         ^         ^    
//      1        2     |3   1   |3       2 |3   1
// s0 <---> R0 <----> R1 <----> R2 <----> R3 <-----> s4      


class Tracker : 

public Tag {

    public:

        static TypeId GetTypeId() {
            static TypeId tid = TypeId("Tracker")
                .SetParent<Tag>()
                .AddConstructor<Tracker>();
            return tid;
        }

        TypeId GetInstanceTypeId() const override { return GetTypeId(); }

        void Serialize(TagBuffer i) const override { i.WriteDouble(m_timestamp.GetSeconds()); }
        void Deserialize(TagBuffer i) override { m_timestamp = Seconds(i.ReadDouble()); }
        uint32_t GetSerializedSize() const override { return 8; }

        void Print(ostream &os) const override { os << "Timestamp: " << m_timestamp.GetSeconds(); }

        void SetTimestamp(Time time) { m_timestamp = time; }
        Time GetTimestamp() const { return m_timestamp; }

    private:
        Time m_timestamp;
};

map<pair<Ipv4Address, Ipv4Address>, vector<double>> mp1;

void Updatemp1(Ipv4Address src, Ipv4Address dest, double delay) {
    auto key = make_pair(src, dest);
    mp1[key].push_back(delay);
}

map<Ipv4Address, string> ip_to_node_name;

void DisplayDelays() {
    cout << endl;
    cout << "INFORMATION RELATED TO END TO END DELAYS : " << endl;
    cout << "===============================================================================================================" <<endl;
 map<string, int> storeidxnodes = {{"S0", 0}, {"S1", 1}, {"S2", 2}, {"S3", 3}, {"S4", 4}}; map<int, string> idxtonode = {{0, "S0"}, {1, "S1"}, {2, "S2"}, {3, "S3"}, {4, "S4"}};
    double delaysstored[5][5]; double variances[5][5];
    for (int i = 0; i < 5; ++i){
        for (int j = 0; j < 5; ++j){
            delaysstored[i][j] = -1;
            variances[i][j] = -1;
        }
    }
    for (const auto& it : mp1) {
        const auto& key = it.first;
        const auto& delay_arr = it.second;
        string s = ip_to_node_name.at(key.first); string t = ip_to_node_name.at(key.second);
        if (storeidxnodes.find(s) != storeidxnodes.end() && storeidxnodes.find(t) != storeidxnodes.end()) {
            int srcidx = storeidxnodes[s];
            int destidx = storeidxnodes[t];
            double tot_delay = accumulate(delay_arr.begin(), delay_arr.end(), 0.0);
            double tot_sq_delay = inner_product(delay_arr.begin(), delay_arr.end(), delay_arr.begin(), 0.0);
            double avg_delay = tot_delay / delay_arr.size(); 
            double delay_var = (tot_sq_delay / delay_arr.size()) - (avg_delay * avg_delay);
            delaysstored[srcidx][destidx] = avg_delay;
            variances[srcidx][destidx] = delay_var;
        }
    }
    cout << "Average End-to-End One-Way Delays (seconds):" << endl;
    cout << setw(8) << "Src\\Dst";
    for (int j = 0; j < 5; ++j) {
        cout << setw(8) << idxtonode[j];
    }
    cout << endl;
    for (int i = 0; i < 5; ++i) {
        cout << setw(8) << idxtonode[i];
        for (int j = 0; j < 5; ++j) {
            if (i == j) {
                cout << setw(8) << "-";
            } else if (delaysstored[i][j] >= 0) {
                cout << setw(8) << fixed << setprecision(6) << delaysstored[i][j];
            } else {
                cout << setw(8) << "N/A";
            }
        }
        cout << endl;
    }
    cout << "\nVariance of End-to-End One-Way Delays (seconds^2):" << endl;
    cout << setw(8) << "Src\\Dst";
    for (int j = 0; j < 5; ++j) {
        cout << setw(8) << idxtonode[j];
    }
    cout << endl;
    for (int i = 0; i < 5; ++i) {
        cout << setw(8) << idxtonode[i];
        for (int j = 0; j < 5; ++j) {
            if (i == j) {
                cout << setw(8) << "-";
            } else if (variances[i][j] >= 0) {
                cout << setw(8) << fixed << setprecision(6) << variances[i][j];
            } else {
                cout << setw(8) << "N/A";
            }
        }
        cout << endl;
    }
    cout << "===============================================================================================================" <<endl;
}
void DisplayPacketDrops(Ptr<FlowMonitor> flowmon, Ptr<Ipv4FlowClassifier> classifier, map<Ipv4Address, string> ip_to_node_name) {
    map<FlowId, FlowMonitor::FlowStats> mp2 = flowmon->GetFlowStats(); map<string, int> storeidxnodes = {{"S0", 0}, {"S1", 1}, {"S2", 2}, {"S3", 3}, {"S4", 4}};
    map<int, string> idxtonode = {{0, "S0"}, {1, "S1"}, {2, "S2"}, {3, "S3"}, {4, "S4"}};
    int pktdrprates[5][5];
    for (int i = 0; i < 5; ++i){
        for (int j = 0; j < 5; ++j){
            pktdrprates[i][j] = -1;
        }
    }
    for (const auto &it : mp2) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it.first);
        string s = ip_to_node_name[t.sourceAddress]; string d = ip_to_node_name[t.destinationAddress];
        if (storeidxnodes.find(s) != storeidxnodes.end() && storeidxnodes.find(d) != storeidxnodes.end()) {
            int srcidx = storeidxnodes[s]; int destidx = storeidxnodes[d];
            uint64_t txPackets = it.second.txPackets; uint64_t rxPackets = it.second.rxPackets;
            uint64_t lostPackets = txPackets - rxPackets; double droprate = 0.0;
            if (txPackets > 0) {
                droprate = lostPackets;
            }       
            pktdrprates[srcidx][destidx] = droprate;
        }
    }
    cout << endl; cout << "Packet Drop Rates:" << endl;
    cout << "===============================================================================================================" <<endl;
    cout << setw(8) << "Src\\Dst";
    for (int j = 0; j < 5; ++j) {
        cout << setw(10) << idxtonode[j];
    }
    cout << endl;
    for (int i = 0; i < 5; ++i) {
        cout << setw(8) << idxtonode[i];
        for (int j = 0; j < 5; ++j) {
            if (i == j) {
                cout << setw(10) << "-";
            } else if (pktdrprates[i][j] >= 0) {
                cout << setw(10) << fixed << setprecision(6) << pktdrprates[i][j];
            } else {
                cout << setw(10) << "N/A";
            }
        }
        cout << endl;
    }
    cout << "===============================================================================================================" <<endl;
}

void DisplayqueuemaxVals(const map<string, uint32_t>& queuemaxVals) {
    cout << "MAXIMUM QUEUE LENGTH CONNECTING ROUTER-CHANNEL : "<<endl; cout << "===============================================================================================================" << endl;
    for (const auto &it : queuemaxVals) {
        cout << "ROUTER_ID-INTERFACE_ID \"" << it.first << "\" MAX LENGTH " << it.second << endl;
    }
    cout << "===============================================================================================================" << endl;
}


class distri {
private:
    random_device rd; mt19937 gen; Ptr<Node> source; Ptr<Node> destination;uint16_t port;double avgPacketpersec; uint32_t packetSize; Ptr<UniformRandomVariable> uniformRng; Ptr<Socket> m_socket; Ipv4Address destAddress;
public:
    distri(Ptr<Node> src, Ptr<Node> dest, uint16_t p, double avgPackets, uint32_t pktSize)
        : gen(rd()), source(src), destination(dest), port(p),
          avgPacketpersec(avgPackets), packetSize(pktSize) {
        uniformRng = CreateObject<UniformRandomVariable>(); uniformRng->SetAttribute("Min", DoubleValue(0.0));
        uniformRng->SetAttribute("Max", DoubleValue(1.0)); m_socket = Socket::CreateSocket(source, TypeId::LookupByName("ns3::UdpSocketFactory"));
        destAddress = destination->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    }

    void generatepack(double start, double stop) {
        Simulator::Schedule(Seconds(start), &distri::generatePackets, this, start, stop);
    }

private:
    void generatePackets(double start, double stop) {
        if (Simulator::Now().GetSeconds() >= stop) {
            return;
        }
        poisson_distribution<int> dist(avgPacketpersec); int packets = dist(gen);
        for (int i = 0; i < packets; i++) {
            double randomval = uniformRng->GetValue(0.0, 1.0);
            Simulator::Schedule(Seconds(randomval), &distri::sendpack, this);
        }
         Simulator::Schedule(Seconds(1.0), &distri::generatePackets, this, start, stop);
    }

    void sendpack() {
        Ptr<Packet> packet = Create<Packet>(packetSize); Tracker tag; tag.SetTimestamp(Simulator::Now()); packet->AddByteTag(tag);
        InetSocketAddress remote = InetSocketAddress(destAddress, port); m_socket->Connect(remote); m_socket->Send(packet);
    }
};

map<string, uint32_t> queuemaxVals;

void QueueLengthTrace(string context, uint32_t oldValue, uint32_t newValue)
{
    if (queuemaxVals.find(context) == queuemaxVals.end()) {
        queuemaxVals[context] = newValue;
    } else {
        if (newValue > queuemaxVals[context]) {
            queuemaxVals[context] = newValue;
        }
    }
}

void SetupQueueTracing(Ptr<NetDevice> netDevice, string deviceName)
{
    Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(netDevice);
    if (p2pDevice)
    {
        Ptr<Queue<Packet>> queue = p2pDevice->GetQueue();
        if (queue)
        {
            queue->TraceConnect("PacketsInQueue", deviceName, MakeCallback(&QueueLengthTrace));
        }

    }
}

void ReceivePacket(Ptr<Socket> socket) {
    Ptr<Packet> packet; Address from;
    while ((packet = socket->RecvFrom(from))) {
        Tracker tag;
        if (packet->FindFirstMatchingByteTag(tag)) {
            Time sendTime = tag.GetTimestamp();
            Time receiveTime = Simulator::Now();
            double delay = (receiveTime - sendTime).GetSeconds();

            Ipv4Address srcAddress = InetSocketAddress::ConvertFrom(from).GetIpv4();
            Ipv4Address destAddress = socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
            Updatemp1(srcAddress, destAddress, delay);
        }
    }
}

int main(int argc, char *argv[])
{
    NodeContainer total_nodes; total_nodes.Create(num_workstations + num_routers); 
    Ptr<Node> s0 = total_nodes.Get(0); Ptr<Node> s1 = total_nodes.Get(1); Ptr<Node> s2 = total_nodes.Get(2); Ptr<Node> s3 = total_nodes.Get(3); Ptr<Node> s4 = total_nodes.Get(4); Ptr<Node> R0 = total_nodes.Get(5); Ptr<Node> R1 = total_nodes.Get(6);Ptr<Node> R2 = total_nodes.Get(7); Ptr<Node> R3 = total_nodes.Get(8);

    NodeContainer srp_00 = NodeContainer(s0, R0); NodeContainer srp_11 = NodeContainer(s1, R1);NodeContainer srp_22 = NodeContainer(s2, R2); NodeContainer srp_33 = NodeContainer(s3, R3); NodeContainer srp_43 = NodeContainer(s4, R3);

    NodeContainer rrp_01 = NodeContainer(R0, R1); NodeContainer rrp_12 = NodeContainer(R1, R2); NodeContainer rrp_23 = NodeContainer(R2, R3);

    PointToPointHelper link_type1mbps; PointToPointHelper link_type2mbps; PointToPointHelper link_type3mbps;

    link_type1mbps.SetDeviceAttribute("DataRate", StringValue("1Mbps")); link_type1mbps.SetChannelAttribute("Delay", StringValue("1ms")); link_type1mbps.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));

    link_type2mbps.SetDeviceAttribute("DataRate", StringValue("2Mbps")); link_type2mbps.SetChannelAttribute("Delay", StringValue("1ms")); link_type2mbps.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("200p"));

    link_type3mbps.SetDeviceAttribute("DataRate", StringValue("3Mbps")); link_type3mbps.SetChannelAttribute("Delay", StringValue("1ms")); link_type3mbps.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("300p"));

    NetDeviceContainer topology_links;
    NetDeviceContainer edge_s0_r0 = link_type1mbps.Install(srp_00);NetDeviceContainer edge_r0_r1 = link_type2mbps.Install(rrp_01);NetDeviceContainer edge_s1_r1 = link_type3mbps.Install(srp_11);NetDeviceContainer edge_r1_r2 = link_type1mbps.Install(rrp_12);NetDeviceContainer edge_s2_r2 = link_type3mbps.Install(srp_22);NetDeviceContainer edge_r2_r3 = link_type2mbps.Install(rrp_23);
    NetDeviceContainer edge_s3_r3 = link_type3mbps.Install(srp_33);NetDeviceContainer edge_s4_r3 = link_type1mbps.Install(srp_43);

    topology_links.Add(edge_s0_r0); topology_links.Add(edge_r0_r1);
    topology_links.Add(edge_s1_r1); topology_links.Add(edge_r1_r2);
    topology_links.Add(edge_s2_r2); topology_links.Add(edge_r2_r3);
    topology_links.Add(edge_s3_r3); topology_links.Add(edge_s4_r3);

    InternetStackHelper internet; internet.Install(total_nodes);

    Ipv4AddressHelper address; Ipv4InterfaceContainer interfaces;
    address.SetBase("10.0.0.0", "255.255.255.0");interfaces.Add(address.Assign(edge_s0_r0));
    address.SetBase("10.0.1.0", "255.255.255.0");interfaces.Add(address.Assign(edge_r0_r1));
    address.SetBase("10.0.2.0", "255.255.255.0");interfaces.Add(address.Assign(edge_s1_r1));
    address.SetBase("10.0.3.0", "255.255.255.0");interfaces.Add(address.Assign(edge_r1_r2));
    address.SetBase("10.0.4.0", "255.255.255.0");interfaces.Add(address.Assign(edge_s2_r2));
    address.SetBase("10.0.5.0", "255.255.255.0");interfaces.Add(address.Assign(edge_r2_r3));
    address.SetBase("10.0.6.0", "255.255.255.0");interfaces.Add(address.Assign(edge_s3_r3));
    address.SetBase("10.0.7.0", "255.255.255.0");interfaces.Add(address.Assign(edge_s4_r3));

    Ipv4Address s0Address = s0->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); Ipv4Address s1Address = s1->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    Ipv4Address s2Address = s2->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();Ipv4Address s3Address = s3->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    Ipv4Address s4Address = s4->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    ip_to_node_name = {
        {s0Address, "S0"},
        {s1Address, "S1"},
        {s2Address, "S2"},
        {s3Address, "S3"},
        {s4Address, "S4"},
        {R0->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), "R0"},
        {R1->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), "R1"},
        {R2->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), "R2"},
        {R3->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), "R3"}
    };

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel>();
    error_model->SetAttribute("ErrorRate", DoubleValue(0.00001));

    for (uint32_t i = 0; i < topology_links.GetN(); ++i) {
        Ptr<NetDevice> device = topology_links.Get(i);
        device->SetAttribute("ReceiveErrorModel", PointerValue(error_model));
    }

    uint16_t port = 9;

    for (uint32_t i = 0; i < num_workstations; ++i) {
        Ptr<Node> node = total_nodes.Get(i);
        Ptr<Socket> recvSink = Socket::CreateSocket(node, TypeId::LookupByName("ns3::UdpSocketFactory"));
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
        recvSink->Bind(local);
        recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
    }

    vector<unique_ptr<distri>> trafficGenerators;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (i == j) {
                continue;
            }
            if (trafficVals[i][j] <= 0) {
                continue;
            }
            trafficGenerators.emplace_back(
                make_unique<distri>(total_nodes.Get(i), total_nodes.Get(j), port, trafficVals[i][j], packet_size)
            );
            trafficGenerators.back()->generatepack(2.0, total_simulation_time);
        }
    }

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> routingStream = ascii.CreateFileStream("routing_table.txt");
    for (uint32_t i = 0; i < total_nodes.GetN(); ++i) {
        Ptr<Node> node = total_nodes.Get(i);
        Ipv4GlobalRoutingHelper::PrintRoutingTableAt(Seconds(2.0), node, routingStream);
    }

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> flowmon = fmh.InstallAll();

    AnimationInterface anim("network-animation2.xml");
    anim.SetConstantPosition(s0, 0.0, 0.0);anim.SetConstantPosition(R0, 10.0, 0.0);
    anim.SetConstantPosition(R1, 20.0, 0.0);anim.SetConstantPosition(R2, 30.0, 0.0);
    anim.SetConstantPosition(R3, 40.0, 0.0);anim.SetConstantPosition(s1, 20.0, -10.0);
    anim.SetConstantPosition(s2, 30.0, -10.0);anim.SetConstantPosition(s3, 40.0, -10.0);
    anim.SetConstantPosition(s4, 50.0, 0.0);

    SetupQueueTracing(edge_s0_r0.Get(1), "R0-edge_s0_r0");SetupQueueTracing(edge_r0_r1.Get(0), "R0-edge_r0_r1");

    SetupQueueTracing(edge_r0_r1.Get(1), "R1-edge_r0_r1");SetupQueueTracing(edge_s1_r1.Get(1), "R1-edge_s1_r1");
    SetupQueueTracing(edge_r1_r2.Get(0), "R1-edge_r1_r2");SetupQueueTracing(edge_r1_r2.Get(1), "R2-edge_r1_r2");
    SetupQueueTracing(edge_s2_r2.Get(1), "R2-edge_s2_r2");SetupQueueTracing(edge_r2_r3.Get(0), "R2-edge_r2_r3");
    SetupQueueTracing(edge_r2_r3.Get(1), "R3-edge_r2_r3");SetupQueueTracing(edge_s3_r3.Get(1), "R3-edge_s3_r3");
    SetupQueueTracing(edge_s4_r3.Get(1), "R3-edge_s4_r3");

    Simulator::Stop(Seconds(total_simulation_time));
    Simulator::Run();
    
    DisplayDelays();
    
    flowmon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    DisplayPacketDrops(flowmon, classifier, ip_to_node_name);
    DisplayqueuemaxVals(queuemaxVals);
    
    Simulator::Destroy();
    return 0;
}