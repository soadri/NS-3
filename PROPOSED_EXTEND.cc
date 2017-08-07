#include <fstream>
#include <vector>
#include <math.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/netanim-module.h"

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
// CLIENT 1 APPLICATION (Test_Multiple)
//================================================================

class Test_Multiple: public Application
{
public:

    Test_Multiple();
    virtual ~Test_Multiple();
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
    uint32_t m_preDownloadDuration;
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
    double preRequestInterval;
    double actualInterRequestInterval;
    int bufMin;
    uint32_t initialDataRate;
    uint32_t currentBitrate;

    // Test_Multiple
    double last_target;
    double gamma;
    uint32_t preQuality;
    bool qualityMaintenance;
    uint32_t m_preLastRequestedSize;
    uint32_t m_predicDownloadDuration;
    double predicRequestInterval;
    uint32_t upQuality;
    
    
};

Test_Multiple::Test_Multiple() :
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
    m_preDownloadDuration(0),
    m_requestTime(),

    m_throughput(0.0),

    target(0.0),
    kappa(0.56),
    omega(300000),
    epsilon(0.15),
    alpha(0.2),
    beta(0.2),
    requestInterval(0.0),
    preRequestInterval(0.0),
    actualInterRequestInterval(0.0),
    bufMin(26000),
    initialDataRate(0),
    currentBitrate(0),
    last_target(0.0),
    gamma(0.35),
    preQuality(0),
    qualityMaintenance(false),
    m_preLastRequestedSize(0),
    m_predicDownloadDuration(0),
    predicRequestInterval(0.0),
    upQuality(0)

{

}

Test_Multiple::~Test_Multiple()
{
    m_socket = 0;
}

void Test_Multiple::Setup(Address address, uint32_t chunkSize,
                          uint32_t numChunks)
{
    m_peer = address;
    m_chunkSize = chunkSize;
    m_numChunks = numChunks;

    //bitrate profile of the content
   m_bitrate_array.push_back(450000);
    m_bitrate_array.push_back(700000);
    m_bitrate_array.push_back(1200000);
    m_bitrate_array.push_back(1950000);
    m_bitrate_array.push_back(3050000);
    m_bitrate_array.push_back(4250000);
    m_bitrate_array.push_back(6050000);

    currentBitrate = m_bitrate_array[0];
}

void Test_Multiple::RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void Test_Multiple::StartApplication(void)
{
    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&Test_Multiple::RxDrop, this));

    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    m_socket->SetRecvCallback(MakeCallback(&Test_Multiple::RxCallback, this));

    SendRequest();

    // Monitoring
    m_bufferStateEvent = Simulator::ScheduleNow(&Test_Multiple::GetBufferState, this);
}

void Test_Multiple::RxCallback(Ptr<Socket> socket)
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
            
	    else if (m_bpsLastChunk > target + omega) {

		if (preRequestInterval > ((double)m_preDownloadDuration / 1000) && preRequestInterval != 0){

			if ((double)m_predicDownloadDuration / 1000 >= predicRequestInterval){
				target = target + omega * actualInterRequestInterval;
			}

			else
				target = (1 + kappa * (preRequestInterval / ((double)m_preDownloadDuration / 1000))) * target;
		}

		else {
			target = target + kappa * omega * actualInterRequestInterval;
		}
	    }
            
	    else {
		
		if (preRequestInterval != 0){

			target = target + kappa * (((double)m_preDownloadDuration / 1000) / preRequestInterval) * actualInterRequestInterval * (m_bpsLastChunk - target);

		}

		else

			target = target + kappa * actualInterRequestInterval * (m_bpsLastChunk - target);

		last_target = target;
		
                if (target < initialDataRate)
                    target = initialDataRate;
            }

            // Smoothing
            m_bpsAvg = m_bpsAvg * (1 - alpha) + target * alpha;
        }

        m_bufferSize += m_chunkSize * 1000;
        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        // Scheduling
        Simulator::ScheduleNow(&Test_Multiple::ScheduleFetch, this);

        // Monitoring
        m_statisticsEvent = Simulator::ScheduleNow(&Test_Multiple::GetStatistics, this);

        // Start ClientBufferModel
        if (m_chunkCount == 1)
        {
            Time tNext("1ms");
            m_bufferEvent = Simulator::Schedule(tNext, &Test_Multiple::ClientBufferModel, this);
        }

        m_commulativeSize = 0;
        m_packetCount = 0;
    }
}

void Test_Multiple::StopApplication(void)
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

void Test_Multiple::SendRequest(void)
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

uint32_t Test_Multiple::GetNextBitrate()
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
    } 

    else
        requestBitrate = currentBitrate;

    // Up-Quality Prediction

    for (uint32_t j = 0; j < m_bitrate_array.size(); j++){

	if (preQuality == m_bitrate_array[j]){

		if (j < m_bitrate_array.size()-1){
			upQuality = m_bitrate_array[j+1];
		}

		else
			upQuality = preQuality;
	}

    }

    return requestBitrate;
}

void Test_Multiple::ScheduleFetch(void)
{
    if (m_running)
    {
        // Scheduling
        if (m_requestState == false) {

            requestInterval = (GetNextBitrate() * m_chunkSize) / m_bpsAvg + beta * ((m_bufferSize - bufMin) / 1000);

            if (requestInterval < 0) requestInterval = 0;
	    
	    actualInterRequestInterval = std::max(requestInterval, ((double)m_downloadDuration / 1000));

	    preRequestInterval = requestInterval;

	    m_preDownloadDuration = m_downloadDuration;

	    predicRequestInterval = (upQuality * m_chunkSize) / m_bpsAvg + beta * ((m_bufferSize - bufMin) / 1000);

	    m_predicDownloadDuration = (upQuality * m_chunkSize) / m_bpsLastChunk;

            Time tNext(Seconds(requestInterval));

	    preQuality = ((m_lastRequestedSize * 8) / m_chunkSize / 1000);

            m_fetchEvent = Simulator::Schedule(tNext, &Test_Multiple::SendRequest, this);
        }
    }
}

void Test_Multiple::ClientBufferModel(void)
{
    if (m_running)
    {
        m_bufferSize -= 1;
        if (m_bufferSize < 0)
            m_bufferSize = 0;

        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        Time tNext("1ms");
        m_bufferEvent = Simulator::Schedule(tNext, &Test_Multiple::ClientBufferModel, this);
    }
}

void Test_Multiple::GetStatistics()
{
    NS_LOG_UNCOND ("=====START===== " << GetNode() << " =====");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<                  
                   " BufferSize: " << m_bufferSize / 1000 <<                 
                   " ChunkCount: " << m_chunkCount <<
		   " VideoLevel: " << ((m_lastRequestedSize * 8) / m_chunkSize / 1000) <<                  
                   " DownloadTime: " << ((double)m_downloadDuration / 1000) << 
                   " RequestInterval: " << requestInterval <<
		   " BufferVariation: " << (m_chunkSize - ((double)m_downloadDuration / 1000) - requestInterval));		   
}

void Test_Multiple::GetBufferState()
{
    //m_throughput = m_commulativeSize * 8 / 1 / 1000; 

    NS_LOG_UNCOND ("=====BUFFER===== " << GetNode() << " =====");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<                  
                   " BufferSize: " << m_bufferSize / 1000 <<
		   " VideoLevel: " << ((m_lastRequestedSize * 8) / m_chunkSize / 1000) <<                       
                   " DownloadTime: " << ((double)m_downloadDuration / 1000) << 
                   " RequestInterval: " << requestInterval <<
		   " BufferVariation: " << m_chunkSize - ((double)m_downloadDuration / 1000) - requestInterval <<
                   " PreviousQuality: " << preQuality);
    
    Time tNext("1s");
    m_statisticsEvent = Simulator::Schedule(tNext,&Test_Multiple::GetBufferState, this);
}

//================================================================
// CLIENT 2 APPLICATION (Test_Multiple2)
//================================================================

class Test_Multiple2: public Application
{
public:

    Test_Multiple2();
    virtual ~Test_Multiple2();
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
    uint32_t m_preDownloadDuration;
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
    double preRequestInterval;
    double actualInterRequestInterval;
    int bufMin;
    uint32_t initialDataRate;
    uint32_t currentBitrate;

    // Test_Multiple2
    double last_target;
    double gamma;
    uint32_t preQuality;
    bool qualityMaintenance;
    uint32_t m_preLastRequestedSize;
    uint32_t m_predicDownloadDuration;
    double predicRequestInterval;
    uint32_t upQuality;
    
};

Test_Multiple2::Test_Multiple2() :
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
    m_preDownloadDuration(0),
    m_requestTime(),

    m_throughput(0.0),

    target(0.0),
    kappa(0.56),
    omega(300000),
    epsilon(0.15),
    alpha(0.2),
    beta(0.2),
    requestInterval(0.0),
    preRequestInterval(0.0),
    actualInterRequestInterval(0.0),
    bufMin(26000),
    initialDataRate(0),
    currentBitrate(0),
    last_target(0.0),
    gamma(0.35),
    preQuality(0),
    qualityMaintenance(false),
    m_preLastRequestedSize(0),
    m_predicDownloadDuration(0),
    predicRequestInterval(0.0),
    upQuality(0)

{

}

Test_Multiple2::~Test_Multiple2()
{
    m_socket = 0;
}

void Test_Multiple2::Setup(Address address, uint32_t chunkSize,
                          uint32_t numChunks)
{
    m_peer = address;
    m_chunkSize = chunkSize;
    m_numChunks = numChunks;

    //bitrate profile of the content
   m_bitrate_array.push_back(450000);
    m_bitrate_array.push_back(700000);
    m_bitrate_array.push_back(1200000);
    m_bitrate_array.push_back(1950000);
    m_bitrate_array.push_back(3050000);
    m_bitrate_array.push_back(4250000);
    m_bitrate_array.push_back(6050000);

    currentBitrate = m_bitrate_array[0];
}

void Test_Multiple2::RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void Test_Multiple2::StartApplication(void)
{
    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&Test_Multiple2::RxDrop, this));

    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    m_socket->SetRecvCallback(MakeCallback(&Test_Multiple2::RxCallback, this));

    SendRequest();

    // Monitoring
    m_bufferStateEvent = Simulator::ScheduleNow(&Test_Multiple2::GetBufferState, this);
}

void Test_Multiple2::RxCallback(Ptr<Socket> socket)
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
            
	    else if (m_bpsLastChunk > target + omega) {

		if (preRequestInterval > ((double)m_preDownloadDuration / 1000) && preRequestInterval != 0){

			if ((double)m_predicDownloadDuration / 1000 >= predicRequestInterval){
				target = target + omega * actualInterRequestInterval;
			}

			else
				target = (1 + kappa * (preRequestInterval / ((double)m_preDownloadDuration / 1000))) * target;
		}

		else {
			target = target + kappa * omega * actualInterRequestInterval;
		}
	    }
            
	    else {
		
		
		if (preRequestInterval != 0){

			target = target + kappa * (((double)m_preDownloadDuration / 1000) / preRequestInterval) * actualInterRequestInterval * (m_bpsLastChunk - target);

		}

		else

			target = target + kappa * actualInterRequestInterval * (m_bpsLastChunk - target);

		last_target = target;
		
                if (target < initialDataRate)
                    target = initialDataRate;
            }

            // Smoothing
            m_bpsAvg = m_bpsAvg * (1 - alpha) + target * alpha;
        }

        m_bufferSize += m_chunkSize * 1000;
        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        // Scheduling
        Simulator::ScheduleNow(&Test_Multiple2::ScheduleFetch, this);

        // Monitoring
        m_statisticsEvent = Simulator::ScheduleNow(&Test_Multiple2::GetStatistics, this);

        // Start ClientBufferModel
        if (m_chunkCount == 1)
        {
            Time tNext("1ms");
            m_bufferEvent = Simulator::Schedule(tNext, &Test_Multiple2::ClientBufferModel, this);
        }

        m_commulativeSize = 0;
        m_packetCount = 0;
    }
}

void Test_Multiple2::StopApplication(void)
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

void Test_Multiple2::SendRequest(void)
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

uint32_t Test_Multiple2::GetNextBitrate()
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
    } 

    else
        requestBitrate = currentBitrate;

    // Up-Quality Prediction

    for (uint32_t j = 0; j < m_bitrate_array.size(); j++){

	if (preQuality == m_bitrate_array[j]){

		if (j < m_bitrate_array.size()-1){
			upQuality = m_bitrate_array[j+1];
		}

		else
			upQuality = preQuality;
	}

    }

    return requestBitrate;
}

void Test_Multiple2::ScheduleFetch(void)
{
    if (m_running)
    {
        // Scheduling
        if (m_requestState == false) {

            requestInterval = (GetNextBitrate() * m_chunkSize) / m_bpsAvg + beta * ((m_bufferSize - bufMin) / 1000);

            if (requestInterval < 0) requestInterval = 0;
	    
	    actualInterRequestInterval = std::max(requestInterval, ((double)m_downloadDuration / 1000));

	    preRequestInterval = requestInterval;

	    m_preDownloadDuration = m_downloadDuration;

	    predicRequestInterval = (upQuality * m_chunkSize) / m_bpsAvg + beta * ((m_bufferSize - bufMin) / 1000);

	    m_predicDownloadDuration = (upQuality * m_chunkSize) / m_bpsLastChunk;

            Time tNext(Seconds(requestInterval));

            preQuality = ((m_lastRequestedSize * 8) / m_chunkSize / 1000);

            m_fetchEvent = Simulator::Schedule(tNext, &Test_Multiple2::SendRequest, this);
        }
    }
}

void Test_Multiple2::ClientBufferModel(void)
{
    if (m_running)
    {
        m_bufferSize -= 1;
        if (m_bufferSize < 0)
            m_bufferSize = 0;

        m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

        Time tNext("1ms");
        m_bufferEvent = Simulator::Schedule(tNext, &Test_Multiple2::ClientBufferModel, this);
    }
}

void Test_Multiple2::GetStatistics()
{
    NS_LOG_UNCOND ("=====START2===== " << GetNode() << " =====");
    NS_LOG_UNCOND ("Time2: " << Simulator::Now ().GetMilliSeconds () <<                  
                   " BufferSize2: " << m_bufferSize / 1000 <<                 
                   " ChunkCount2: " << m_chunkCount <<
		   " VideoLevel2: " << ((m_lastRequestedSize * 8) / m_chunkSize / 1000) <<                  
                   " DownloadTime2: " << ((double)m_downloadDuration / 1000) << 
                   " RequestInterval2: " << requestInterval <<
		   " BufferVariation2: " << (m_chunkSize - ((double)m_downloadDuration / 1000) - requestInterval));		   
}

void Test_Multiple2::GetBufferState()
{
    //m_throughput = m_commulativeSize * 8 / 1 / 1000;

    NS_LOG_UNCOND ("=====BUFFER2===== " << GetNode() << " =====");
    NS_LOG_UNCOND ("Time2: " << Simulator::Now ().GetMilliSeconds () <<                  
                   " BufferSize2: " << m_bufferSize / 1000 <<
		   " VideoLevel2: " << ((m_lastRequestedSize * 8) / m_chunkSize / 1000) <<                       
                   " DownloadTime2: " << ((double)m_downloadDuration / 1000) << 
                   " RequestInterval2: " << requestInterval <<
		   " BufferVariation2: " << m_chunkSize - ((double)m_downloadDuration / 1000) - requestInterval <<
                   " PreviousQuality2: " << preQuality);
    
    Time tNext("1s");
    m_statisticsEvent = Simulator::Schedule(tNext,&Test_Multiple2::GetBufferState, this);
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
   //onoff1.SetConstantRate(DataRate("5000kb/s"));

   //ApplicationContainer cbrApp1 = onoff1.Install(dB.GetLeft(3));
   //cbrApp1.Start(Seconds(80.0));
   //cbrApp1.Stop(Seconds(100.0));

   //PacketSinkHelper cbrSink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   //cbrApp1 = cbrSink1.Install(dB.GetRight(3));
   //cbrApp1.Start(Seconds(80.0));
   //cbrApp1.Stop(Seconds(100.0));

   OnOffHelper onoff2("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(4), serverPort));
   onoff2.SetConstantRate(DataRate("5000kb/s"));

   ApplicationContainer cbrApp2 = onoff2.Install(dB.GetLeft(4));
   cbrApp2.Start(Seconds(120.0));
   cbrApp2.Stop(Seconds(140.0));

   PacketSinkHelper cbrSink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   cbrApp2 = cbrSink2.Install(dB.GetRight(4));
   cbrApp2.Start(Seconds(120.0));
   cbrApp2.Stop(Seconds(140.0));

   OnOffHelper onoff3("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(5), serverPort));
   onoff3.SetConstantRate(DataRate("5000kb/s"));

   ApplicationContainer cbrApp3 = onoff3.Install(dB.GetLeft(5));
   cbrApp3.Start(Seconds(160.0));
   cbrApp3.Stop(Seconds(180.0));

   PacketSinkHelper cbrSink3("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   cbrApp3 = cbrSink3.Install(dB.GetRight(5));
   cbrApp3.Start(Seconds(160.0));
   cbrApp3.Stop(Seconds(180.0));

   OnOffHelper onoff4("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(6), serverPort));
   onoff4.SetConstantRate(DataRate("5000kb/s"));
   ApplicationContainer cbrApp4 = onoff4.Install(dB.GetLeft(6));
   cbrApp4.Start(Seconds(200.0));
   cbrApp4.Stop(Seconds(220.0));

   PacketSinkHelper cbrSink4("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   cbrApp4 = cbrSink4.Install(dB.GetRight(6));
   cbrApp4.Start(Seconds(200.0));
   cbrApp4.Stop(Seconds(220.0));

   OnOffHelper onoff5("ns3::UdpSocketFactory", InetSocketAddress(dB.GetRightIpv4Address(7), serverPort));
   onoff5.SetConstantRate(DataRate("5000kb/s"));

   ApplicationContainer cbrApp5 = onoff5.Install(dB.GetLeft(7));
   cbrApp5.Start(Seconds(240.0));
   cbrApp5.Stop(Seconds(260.0));

   PacketSinkHelper cbrSink5("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),serverPort));
   cbrApp5 = cbrSink5.Install(dB.GetRight(7));
   cbrApp5.Start(Seconds(240.0));
   cbrApp5.Stop(Seconds(260.0));


    // DASH server
    Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    serverApp1->Setup(bindAddress1, 512);
    dB.GetLeft(1)->AddApplication(serverApp1);
    serverApp1->SetStartTime(Seconds(0.0));
    serverApp1->SetStopTime(Seconds(300.0));

    Address bindAddress2(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp2 = CreateObject<DashServerApp>();
    serverApp2->Setup(bindAddress2, 512);
    dB.GetLeft(2)->AddApplication(serverApp2);
    serverApp2->SetStartTime(Seconds(0.0));
    serverApp2->SetStopTime(Seconds(300.0));

    // DASH client
    Address serverAddress1(
        InetSocketAddress(dB.GetLeftIpv4Address(1), serverPort));
    Ptr<Test_Multiple> clientApp1 = CreateObject<Test_Multiple>();
    clientApp1->Setup(serverAddress1, 2, 512);
    dB.GetRight(1)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.0));
    clientApp1->SetStopTime(Seconds(300.0));

    Address serverAddress2(
        InetSocketAddress(dB.GetLeftIpv4Address(2), serverPort));
    Ptr<Test_Multiple2> clientApp2 = CreateObject<Test_Multiple2>();
    clientApp2->Setup(serverAddress2, 2, 512);
    dB.GetRight(2)->AddApplication(clientApp2);
    clientApp2->SetStartTime(Seconds(60.0));
    clientApp2->SetStopTime(Seconds(300.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(300.0));

    AnimationInterface anim("animation.xml");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
