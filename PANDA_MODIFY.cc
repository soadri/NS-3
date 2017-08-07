#include <fstream>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DashApplication");

//================================================================
// SERVER APPLICATION
//================================================================

class DashServerApp: public Application
{
public:
    DashServerApp();
    virtual ~DashServerApp();
    void Setup(Address address, uint32_t packetSize);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void RxCallback(Ptr<Socket> socket);
    void TxCallback(Ptr<Socket> socket, uint32_t txSpace);
    bool ConnectionCallback(Ptr<Socket> s, const Address &ad);
    void AcceptCallback(Ptr<Socket> s, const Address &ad);
    void SendData();

    bool m_connected;
    Ptr<Socket> m_socket;
    Ptr<Socket> m_peer_socket;

    Address ads;
    Address m_peer_address;
    uint32_t m_remainingData;
    EventId m_sendEvent;
    uint32_t m_packetSize;
    uint32_t m_packetCount;
};

DashServerApp::DashServerApp() :
    m_connected(false), 
    m_socket(0), 
    m_peer_socket(0), 
    ads(), 
    m_peer_address(), 
    m_remainingData(0),
    m_sendEvent(), 
    m_packetSize(0), 
    m_packetCount(0)
{

}

DashServerApp::~DashServerApp()
{
    m_socket = 0;
}

void DashServerApp::Setup(Address address, uint32_t packetSize)
{
    ads = address;
    m_packetSize = packetSize;
}

void DashServerApp::StartApplication()
{
    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

    m_socket->Bind(ads);
    m_socket->Listen();
    m_socket->SetAcceptCallback(
        MakeCallback(&DashServerApp::ConnectionCallback, this),
        MakeCallback(&DashServerApp::AcceptCallback, this));
    m_socket->SetRecvCallback(MakeCallback(&DashServerApp::RxCallback, this));
    m_socket->SetSendCallback(MakeCallback(&DashServerApp::TxCallback, this));
}

void DashServerApp::StopApplication()
{
    m_connected = false;
    if (m_socket)
        m_socket->Close();
    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }
}

bool DashServerApp::ConnectionCallback(Ptr<Socket> socket, const Address &ads)
{
    m_connected = true;
    return true;
}

void DashServerApp::AcceptCallback(Ptr<Socket> socket, const Address &ads)
{
    m_peer_address = ads;
    m_peer_socket = socket;

    socket->SetRecvCallback(MakeCallback(&DashServerApp::RxCallback, this));
    socket->SetSendCallback(MakeCallback(&DashServerApp::TxCallback, this));
}

void DashServerApp::RxCallback(Ptr<Socket> socket)
{
    Address ads;
    Ptr<Packet> pckt = socket->RecvFrom(ads);
    
    if (ads == m_peer_address)
    {
        uint32_t data = 0;
        pckt->CopyData((uint8_t *) &data, 4);

        m_remainingData = data;
        m_packetCount = 0;

        SendData();
    }
}

void DashServerApp::TxCallback(Ptr<Socket> socket, uint32_t txSpace)
{
    if (m_connected)
        Simulator::ScheduleNow (&DashServerApp::SendData, this);
}

void DashServerApp::SendData()
{
    while (m_remainingData > 0)
    {
        // Time to send more
        uint32_t toSend = std::min (m_packetSize, m_remainingData);
        Ptr<Packet> packet = Create<Packet> (toSend);

        int actual = m_peer_socket->Send (packet);

        if (actual > 0)
        {
            m_remainingData -= toSend;
            m_packetCount++;
        }

        if ((unsigned)actual != toSend)
        {
            break;
        }
    }
}

//================================================================
// CLIENT APPLICATION
//================================================================

class DashClientApp: public Application
{
public:

    DashClientApp();
    virtual ~DashClientApp();
    enum
    {
        MAX_BUFFER_SIZE = 30000
    }; // 30 seconds

    void Setup(Address address, uint32_t chunkSize, uint32_t numChunks);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void SendRequest(void);
    void ScheduleFetch(void);
    void GetStatistics(void);
    uint32_t GetNextBitrate();
    void RxCallback(Ptr<Socket> socket);

    void ClientBufferModel(void);
    void GetBufferState(void);
    void RxDrop (Ptr<const Packet> p);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_chunkSize;
    uint32_t m_numChunks;
    uint32_t m_chunkCount;
    int m_bufferSize;
    uint32_t m_bufferPercent;
    double m_bpsAvg;
    uint32_t m_bpsLastChunk;
    std::vector<uint32_t> m_bitrate_array;
    EventId m_fetchEvent;
    EventId m_statisticsEvent;
    bool m_running;
    bool m_requestState;
    uint32_t m_commulativeTime;
    uint32_t m_commulativeSize;
    uint32_t m_lastRequestedSize;
    uint32_t m_sessionData;
    uint32_t m_sessionTime;
    uint32_t m_packetCount;

    // kmw
    EventId m_bufferEvent;
    EventId m_bufferStateEvent;
    uint32_t m_downloadDuration;
    Time m_requestTime;

    double m_throughput;

    // PANDA
    double target;
    double kappa;
    double omega;
    double epsilon;
    double alpha;
    double beta;
    double requestInterval;
    double actualInterRequestInterval;
    int bufMin;
    uint32_t initialDataRate;
    uint32_t currentBitrate;
};

DashClientApp::DashClientApp() :
    m_socket(0), 
    m_peer(), 
    m_chunkSize(0), 
    m_numChunks(0), 
    m_chunkCount(0),
    m_bufferSize(0), 
    m_bufferPercent(0), 
    m_bpsAvg(0.0), 
    m_bpsLastChunk(0),
    m_fetchEvent(), 
    m_statisticsEvent(), 
    m_running(false), 
    m_requestState(false),
    m_commulativeTime(0),
    m_commulativeSize(0), 
    m_lastRequestedSize(0),
    m_sessionData(0), 
    m_sessionTime(0),
    m_packetCount(0),

    m_bufferEvent(),
    m_bufferStateEvent(),
    m_downloadDuration(0),
    m_requestTime(),

    m_throughput(0.0),

    target(0.0),
    kappa(0.56),
    omega(300000),
    epsilon(0.15),
    alpha(0.2),
    beta(0.2),
    requestInterval(0.0),
    actualInterRequestInterval(0.0),
    bufMin(26000),
    initialDataRate(0),
    currentBitrate(0)
{

}

DashClientApp::~DashClientApp()
{
    m_socket = 0;
}

void DashClientApp::Setup(Address address, uint32_t chunkSize,
                          uint32_t numChunks)
{
    m_peer = address;
    m_chunkSize = chunkSize;
    m_numChunks = numChunks;

    //bitrate profile of the content
    m_bitrate_array.push_back(450000);
    m_bitrate_array.push_back(750000);
    m_bitrate_array.push_back(1255000);
    m_bitrate_array.push_back(1950000);
    m_bitrate_array.push_back(3050000);
    m_bitrate_array.push_back(4550000);
    m_bitrate_array.push_back(6450000);

    currentBitrate = m_bitrate_array[0];
}

void DashClientApp::RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void DashClientApp::StartApplication(void)
{
    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&DashClientApp::RxDrop, this));

    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    m_socket->SetRecvCallback(MakeCallback(&DashClientApp::RxCallback, this));

    SendRequest();

    // Monitoring
    m_bufferStateEvent = Simulator::ScheduleNow(&DashClientApp::GetBufferState, this);
}

void DashClientApp::RxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet = socket->Recv();
    m_commulativeSize += packet->GetSize();
    m_packetCount++;

    if (m_commulativeSize >= m_lastRequestedSize)
    {
        m_chunkCount++;
        m_requestState = false;
 
        // Estimating
        m_downloadDuration = Simulator::Now().GetMilliSeconds() - m_requestTime.GetMilliSeconds();

        m_bpsLastChunk = (m_commulativeSize * 8) / m_downloadDuration;
        m_bpsLastChunk = m_bpsLastChunk * 1000;

        if (m_bpsAvg == 0)
            target = m_bpsAvg = initialDataRate = m_bpsLastChunk; 

        else {
            if (target == m_bpsLastChunk) { 
                // do nothing 
            } 
            
	    else if (m_bpsLastChunk > target + omega)
                target = target + kappa * omega * actualInterRequestInterval;
            
	    else {
                target = target + kappa * ((double)m_bpsLastChunk - target) * actualInterRequestInterval;

                if (target < initialDataRate)
                    target = initialDataRate;
            }

            // Smoothing
            m_bpsAvg = m_bpsAvg * (1 - alpha) + target * alpha;
        }

        m_bufferSize += m_chunkSize * 1000;
        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        // Scheduling
        Simulator::ScheduleNow(&DashClientApp::ScheduleFetch, this);

        // Monitoring
        m_statisticsEvent = Simulator::ScheduleNow(&DashClientApp::GetStatistics, this);

        // Start ClientBufferModel
        if (m_chunkCount == 1)
        {
            Time tNext("1ms");
            m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::ClientBufferModel, this);
        }

        m_commulativeSize = 0;
        m_packetCount = 0;
    }
}

void DashClientApp::StopApplication(void)
{
    m_running = false;

    if (m_bufferEvent.IsRunning())
    {
        Simulator::Cancel(m_bufferEvent);
    }

    if (m_fetchEvent.IsRunning())
    {
        Simulator::Cancel(m_fetchEvent);
    }

    if (m_statisticsEvent.IsRunning())
    {
        Simulator::Cancel(m_statisticsEvent);
    }

    if (m_bufferStateEvent.IsRunning())
    {
        Simulator::Cancel(m_bufferStateEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void DashClientApp::SendRequest(void)
{
    uint32_t nextRate = GetNextBitrate();
    if (nextRate == 0)
    {
        return;
    }
    uint32_t bytesReq = (nextRate * m_chunkSize) / 8;
    Ptr<Packet> packet = Create<Packet>((uint8_t *) &bytesReq, 4);
    m_lastRequestedSize = bytesReq;
    m_socket->Send(packet);
    m_requestState = true;

    currentBitrate = nextRate;

    m_requestTime = Simulator::Now();
}

uint32_t DashClientApp::GetNextBitrate()
{
    uint32_t bandwidth, requestBitrate;
    double safetyMargin;

    // Stop Event
    if (m_chunkCount >= m_numChunks)
    {
        return 0;
    }
    bandwidth = m_bpsAvg;

    // Buffer Control
    if (m_bufferPercent == 0)
    {
        bandwidth = 0;
    }
    requestBitrate = m_bitrate_array[0];

    // Quantizing
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (bandwidth <= m_bitrate_array[i])
        {
            break;
        }
        requestBitrate = m_bitrate_array[i];
    }

    // Dead-zone quantizer
    if (currentBitrate < requestBitrate)
        safetyMargin = epsilon * m_bpsAvg;    
    else if (currentBitrate > requestBitrate)
        safetyMargin = 0;
    else
        return requestBitrate;

    if (requestBitrate <= m_bpsAvg - safetyMargin) {
        // do nothing
    } else
        requestBitrate = currentBitrate;

    return requestBitrate;
}

void DashClientApp::ScheduleFetch(void)
{
    if (m_running)
    {
        // Scheduling
        if (m_requestState == false) {
            requestInterval = (GetNextBitrate() * m_chunkSize) / m_bpsAvg + beta * ((m_bufferSize - bufMin) / 1000);
            if (requestInterval < 0) requestInterval = 0;
	    
	    actualInterRequestInterval = std::max(requestInterval, (double)m_downloadDuration)/1000;

            Time tNext(Seconds(requestInterval));
            m_fetchEvent = Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
        }
    }
}

void DashClientApp::ClientBufferModel(void)
{
    if (m_running)
    {
        m_bufferSize -= 1;
        if (m_bufferSize < 0)
            m_bufferSize = 0;

        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        Time tNext("1ms");
        m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::ClientBufferModel, this);
    }
}

void DashClientApp::GetStatistics()
{
    NS_LOG_UNCOND ("=======START=========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
                   " target: " << target <<
                   " bpsAverage: " << m_bpsAvg <<
                   " bpsLastChunk: " << m_bpsLastChunk / 1000 <<
                   " lastBitrate: " << (m_lastRequestedSize * 8) / m_chunkSize <<
                   " chunkCount: " << m_chunkCount <<
                   " totalChunks: " << m_numChunks <<
                   " downloadDuration: " << m_downloadDuration << 
                   " requestInterval: " << requestInterval);
}

void DashClientApp::GetBufferState()
{
    m_throughput = m_commulativeSize * 8 / 1 / 1000;

    NS_LOG_UNCOND ("=======BUFFER========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetSeconds () <<
                   " bufferSize: " << m_bufferSize / 1000 <<
                   " hasThroughput: " << m_throughput <<
                   " estimatedBW: " << m_bpsAvg / 1000 <<
                   " videoLevel: " << (m_lastRequestedSize * 8) / m_chunkSize / 1000);
    
    Time tNext("1s");
    m_statisticsEvent = Simulator::Schedule(tNext,
                                            &DashClientApp::GetBufferState, this);
}

//=================================================================
// SIMULATION
//================================================================

int main(int argc, char *argv[])
{
    LogComponentEnable("DashApplication", LOG_LEVEL_ALL);

    // Config::SetDefault("ns3::RedQueue::Mode", StringValue("QUEUE_MODE_BYTES"));
    // Config::SetDefault("ns3::RedQueue::QueueLimit", UintegerValue(100 * 512));

    PointToPointHelper bottleNeck;
    bottleNeck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleNeck.SetChannelAttribute("Delay", StringValue("20ms"));
    bottleNeck.SetQueue("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_BYTES"));
    // bottleNeck.SetQueue("ns3::RedQueue",
    //                     "MinTh", DoubleValue(30),
    //                     "MaxTh", DoubleValue(90),
    //                     "QW", DoubleValue(0.25));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointDumbbellHelper dB(10, pointToPointLeaf, 10, pointToPointLeaf,
                                  bottleNeck);

    // install stack
    InternetStackHelper stack;
    dB.InstallStack(stack);

    // assign IP addresses
    dB.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    uint16_t serverPort = 8080;

   // Cross Traffic CBR

   //OnOffHelper onoff1("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(3), serverPort));
   //onoff1.SetConstantRate(DataRate("6500kb/s"));

   //ApplicationContainer cbrApp1 = onoff1.Install(dB.GetLeft(3));
   //cbrApp1.Start(Seconds(150.0));
   //cbrApp1.Stop(Seconds(250.0));

   //PacketSinkHelper cbrSink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   //cbrApp1 = cbrSink1.Install(dB.GetRight(3));
   //cbrApp1.Start(Seconds(150.0));
   //cbrApp1.Stop(Seconds(250.0));

    // DASH server
    Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    serverApp1->Setup(bindAddress1, 512);
    dB.GetLeft(1)->AddApplication(serverApp1);
    serverApp1->SetStartTime(Seconds(0.0));
    serverApp1->SetStopTime(Seconds(400.0));

    // DASH client
    Address serverAddress1(
        InetSocketAddress(dB.GetLeftIpv4Address(1), serverPort));
    Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
    clientApp1->Setup(serverAddress1, 2, 512);
    dB.GetRight(1)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.0));
    clientApp1->SetStopTime(Seconds(400.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(400.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
