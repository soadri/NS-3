#include <fstream>
#include <vector>
#include <math.h>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

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
    // NS_LOG_UNCOND("Server: connection callback ");

    m_connected = true;
    return true;
}

void DashServerApp::AcceptCallback(Ptr<Socket> socket, const Address &ads)
{
    // NS_LOG_UNCOND("Server: accept callback ");
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
        // NS_LOG_UNCOND("Server: pckt size requested " << data);

        m_remainingData = data;
        m_packetCount = 0;

        /* NS_LOG_UNCOND("Server: pckt send start time " <<
                      Simulator::Now().GetMilliSeconds()); */
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
// CLIENT1 APPLICATION
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
    int32_t m_bufferSize;
    uint32_t m_bufferPercent;
    uint32_t m_bpsAvg;
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
};

DashClientApp::DashClientApp() :
    m_socket(0), 
    m_peer(), 
    m_chunkSize(0), 
    m_numChunks(0), 
    m_chunkCount(0),
    m_bufferSize(0), 
    m_bufferPercent(0), 
    m_bpsAvg(0), 
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
    m_requestTime()
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
    m_bitrate_array.push_back(500000);
    m_bitrate_array.push_back(1000000);
    m_bitrate_array.push_back(1500000);
    m_bitrate_array.push_back(2000000);
    m_bitrate_array.push_back(2500000);
    m_bitrate_array.push_back(3000000);
    m_bitrate_array.push_back(3500000);	
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

    // Scheduling
    Time tNext(Seconds(0));
    m_fetchEvent = Simulator::Schedule(tNext, &DashClientApp::ScheduleFetch, this);
    SendRequest();

    m_bufferStateEvent = Simulator::ScheduleNow(&DashClientApp::GetBufferState, this);
}

void DashClientApp::RxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet = socket->Recv();
    m_commulativeSize += packet->GetSize();
    m_packetCount++;

    if (m_commulativeSize >= m_lastRequestedSize)
    {
        //received the complete chunk
        //update the buffer size and initiate the next request.
        /* NS_LOG_UNCOND("Client " << GetNode() << " received complete chunk at " <<
                      Simulator::Now().GetMilliSeconds()); */
        m_chunkCount++;
        m_requestState = false;
 
        // Estimating
        m_downloadDuration = Simulator::Now().GetMilliSeconds() - m_requestTime.GetMilliSeconds();

        m_bpsLastChunk = (m_commulativeSize * 8) / m_downloadDuration;
        m_bpsLastChunk = m_bpsLastChunk * 1000;

        if (m_bpsAvg == 0)
            m_bpsAvg = m_bpsLastChunk;

        // Smoothing
        m_bpsAvg = (m_bpsAvg * 80) / 100 + (m_bpsLastChunk * 20) / 100;

        m_bufferSize += m_chunkSize * 1000;
        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

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

    m_requestTime = Simulator::Now();
    /* NS_LOG_UNCOND ("Client " << GetNode() << " request bitrate " << nextRate <<
               " at " << Simulator::Now().GetMilliSeconds()) ; */
}

uint32_t DashClientApp::GetNextBitrate()
{
    uint32_t bandwidth, requestBitrate;

    // Stop Event
    if (m_chunkCount >= m_numChunks)
    {
        return 0;
    }
    bandwidth = m_bpsLastChunk;
    // bandwidth = m_bpsAvg;

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
    return requestBitrate;
}

void DashClientApp::ScheduleFetch(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_chunkSize));
        m_fetchEvent = Simulator::Schedule(tNext, &DashClientApp::ScheduleFetch, this);
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

        if (m_bufferPercent < 100 && m_requestState == false)
            SendRequest();

        Time tNext("1ms");
        m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::ClientBufferModel, this);
    }
}

void DashClientApp::GetStatistics()
{
    NS_LOG_UNCOND ("=======START=========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
                   " ChunkCount: " << m_chunkCount <<
                   " bpsLastChunk: " << m_bpsLastChunk / 1000 <<
                   " LastBitrate: " << (m_lastRequestedSize * 8) / m_chunkSize <<
                   " DownloadTime: " << m_downloadDuration);
}

void DashClientApp::GetBufferState()
{
    NS_LOG_UNCOND ("=======BUFFER========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetSeconds () <<
                   " VideoLevel: " << (m_lastRequestedSize * 8) / m_chunkSize / 1000 <<
                   " BufferSize: " << m_bufferSize / 1000);
    
    Time tNext("1s");
    m_statisticsEvent = Simulator::Schedule(tNext,
                                            &DashClientApp::GetBufferState, this);
}

//================================================================
// CLIENT2 APPLICATION
//================================================================

class DashClientApp2: public Application
{
public:

    DashClientApp2();
    virtual ~DashClientApp2();
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
    int32_t m_bufferSize;
    uint32_t m_bufferPercent;
    uint32_t m_bpsAvg;
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
};

DashClientApp2::DashClientApp2() :
    m_socket(0), 
    m_peer(), 
    m_chunkSize(0), 
    m_numChunks(0), 
    m_chunkCount(0),
    m_bufferSize(0), 
    m_bufferPercent(0), 
    m_bpsAvg(0), 
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
    m_requestTime()
{

}

DashClientApp2::~DashClientApp2()
{
    m_socket = 0;
}

void DashClientApp2::Setup(Address address, uint32_t chunkSize,
                          uint32_t numChunks)
{
    m_peer = address;
    m_chunkSize = chunkSize;
    m_numChunks = numChunks;

    //bitrate profile of the content
    m_bitrate_array.push_back(500000);
    m_bitrate_array.push_back(1000000);
    m_bitrate_array.push_back(1500000);
    m_bitrate_array.push_back(2000000);
    m_bitrate_array.push_back(2500000);
    m_bitrate_array.push_back(3000000);
    m_bitrate_array.push_back(3500000);
}

void DashClientApp2::RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void DashClientApp2::StartApplication(void)
{
    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&DashClientApp2::RxDrop, this));

    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    m_socket->SetRecvCallback(MakeCallback(&DashClientApp2::RxCallback, this));

    // Scheduling
    Time tNext(Seconds(0));
    m_fetchEvent = Simulator::Schedule(tNext, &DashClientApp2::ScheduleFetch, this);
    SendRequest();

    m_bufferStateEvent = Simulator::ScheduleNow(&DashClientApp2::GetBufferState, this);
}

void DashClientApp2::RxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet = socket->Recv();
    m_commulativeSize += packet->GetSize();
    m_packetCount++;

    if (m_commulativeSize >= m_lastRequestedSize)
    {
        //received the complete chunk
        //update the buffer size and initiate the next request.
        /* NS_LOG_UNCOND("Client " << GetNode() << " received complete chunk at " <<
                      Simulator::Now().GetMilliSeconds()); */
        m_chunkCount++;
        m_requestState = false;
 
        // Estimating
        m_downloadDuration = Simulator::Now().GetMilliSeconds() - m_requestTime.GetMilliSeconds();

        m_bpsLastChunk = (m_commulativeSize * 8) / m_downloadDuration;
        m_bpsLastChunk = m_bpsLastChunk * 1000;

        if (m_bpsAvg == 0)
            m_bpsAvg = m_bpsLastChunk;

        // Smoothing
        m_bpsAvg = (m_bpsAvg * 80) / 100 + (m_bpsLastChunk * 20) / 100;

        m_bufferSize += m_chunkSize * 1000;
        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        m_statisticsEvent = Simulator::ScheduleNow(&DashClientApp2::GetStatistics, this);

        // Start ClientBufferModel
        if (m_chunkCount == 1)
        {
            Time tNext("1ms");
            m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp2::ClientBufferModel, this);
        }

        m_commulativeSize = 0;
        m_packetCount = 0;
    }
}

void DashClientApp2::StopApplication(void)
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

void DashClientApp2::SendRequest(void)
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

    m_requestTime = Simulator::Now();
    /* NS_LOG_UNCOND ("Client " << GetNode() << " request bitrate " << nextRate <<
               " at " << Simulator::Now().GetMilliSeconds()) ; */
}

uint32_t DashClientApp2::GetNextBitrate()
{
    uint32_t bandwidth, requestBitrate;

    // Stop Event
    if (m_chunkCount >= m_numChunks)
    {
        return 0;
    }
    bandwidth = m_bpsLastChunk;
    // bandwidth = m_bpsAvg;

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
    return requestBitrate;
}

void DashClientApp2::ScheduleFetch(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_chunkSize));
        m_fetchEvent = Simulator::Schedule(tNext, &DashClientApp2::ScheduleFetch, this);
    }
}

void DashClientApp2::ClientBufferModel(void)
{
    if (m_running)
    {
        m_bufferSize -= 1;
        if (m_bufferSize < 0)
            m_bufferSize = 0;

        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        if (m_bufferPercent < 100 && m_requestState == false)
            SendRequest();

        Time tNext("1ms");
        m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp2::ClientBufferModel, this);
    }
}

void DashClientApp2::GetStatistics()
{
    NS_LOG_UNCOND ("=======START2=========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time2: " << Simulator::Now ().GetMilliSeconds () <<
                   " ChunkCount2: " << m_chunkCount <<
                   " bpsLastChunk2: " << m_bpsLastChunk / 1000 <<
                   " LastBitrate2: " << (m_lastRequestedSize * 8) / m_chunkSize <<
                   " DownloadTime2: " << m_downloadDuration);
}

void DashClientApp2::GetBufferState()
{
    NS_LOG_UNCOND ("=======BUFFER2========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time2: " << Simulator::Now ().GetSeconds () <<
                   " VideoLevel2: " << (m_lastRequestedSize * 8) / m_chunkSize / 1000 <<
                   " BufferSize2: " << m_bufferSize / 1000);
    
    Time tNext("1s");
    m_statisticsEvent = Simulator::Schedule(tNext,
                                            &DashClientApp2::GetBufferState, this);
}

//================================================================
// SIMULATION
//================================================================

int main(int argc, char *argv[])
{
	LogComponentEnable("DashApplication", LOG_LEVEL_ALL);
	
	PointToPointHelper bottleNeck;
	bottleNeck.SetDeviceAttribute("DataRate", StringValue("4Mbps"));
	bottleNeck.SetChannelAttribute("Delay", StringValue("20ms"));
	bottleNeck.SetQueue("ns3::DropTailQueue", "Mode", StringValue("QUEUE_MODE_BYTES"));

	PointToPointHelper pointToPointLeaf;
	pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	pointToPointLeaf.SetChannelAttribute("Delay", StringValue("2ms"));

	PointToPointDumbbellHelper dB(10, pointToPointLeaf, 10, pointToPointLeaf, bottleNeck);

	InternetStackHelper stack;
	dB.InstallStack(stack);

	dB.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
	Ipv4AddressHelper("10.2.1.0", "255.255.255.0"), 
	Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

	uint16_t serverPort = 8080;

	// Cross Traffic CBR

	//OnOffHelper onoff1("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(3), serverPort));
	//onoff1.SetConstantRate(DataRate("2000kb/s"));

	//ApplicationContainer cbrApp1 = onoff1.Install(dB.GetLeft(3));
	//cbrApp1.Start(Seconds(150.0));
	//cbrApp1.Stop(Seconds(300.0));

	//PacketSinkHelper cbrSink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
	//cbrApp1 = cbrSink1.Install(dB.GetRight(3));
	//cbrApp1.Start(Seconds(150.0));
	//cbrApp1.Stop(Seconds(300.0));

	//OnOffHelper onoff2("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(4), serverPort));
	//onoff2.SetConstantRate(DataRate("1000kb/s"));

	//ApplicationContainer cbrApp2 = onoff2.Install(dB.GetLeft(4));
	//cbrApp2.Start(Seconds(300.0));
	//cbrApp2.Stop(Seconds(400.0));

	//PacketSinkHelper cbrSink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
	//cbrApp2 = cbrSink1.Install(dB.GetRight(4));
	//cbrApp2.Start(Seconds(300.0));
	//cbrApp2.Stop(Seconds(400.0));


	// HTTP Adaptive Streaming Server

	Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
	Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
	serverApp1->Setup(bindAddress1, 512);
	dB.GetLeft(1)->AddApplication(serverApp1);
	serverApp1->SetStartTime(Seconds(0.0));
	serverApp1->SetStopTime(Seconds(400.0));

	Address bindAddress2(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
	Ptr<DashServerApp> serverApp2 = CreateObject<DashServerApp>();
	serverApp2->Setup(bindAddress2, 512);
	dB.GetLeft(2)->AddApplication(serverApp2);
	serverApp2->SetStartTime(Seconds(0.0));
	serverApp2->SetStopTime(Seconds(400.0));

	// HTTP Adaptive Streaming Client

	Address serverAddress1(InetSocketAddress(dB.GetLeftIpv4Address(1), serverPort));
	Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
	clientApp1->Setup(serverAddress1, 2, 512);
	dB.GetRight(1)->AddApplication(clientApp1);
	clientApp1->SetStartTime(Seconds(0.0));
	clientApp1->SetStopTime(Seconds(400.0));

	Address serverAddress2(InetSocketAddress(dB.GetLeftIpv4Address(2), serverPort));
	Ptr<DashClientApp2> clientApp2 = CreateObject<DashClientApp2>();
	clientApp2->Setup(serverAddress2, 2, 512);
	dB.GetRight(2)->AddApplication(clientApp2);
	clientApp2->SetStartTime(Seconds(50.0));
	clientApp2->SetStopTime(Seconds(400.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	Simulator::Stop(Seconds(400.0));
	
	Simulator::Run();
	Simulator::Destroy();

    return 0;
}
