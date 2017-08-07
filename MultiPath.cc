#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
//#include "ns3/epc-helper.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("DashApplication");

// Thesis
// Multi-path & Multi-connection based transmission technique in mobile network

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
    m_connected(false), m_socket(0), m_peer_socket(0), 
    ads(), m_peer_address(), m_remainingData(0), 
    m_sendEvent(), m_packetSize(0), m_packetCount(0)
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

    // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
    if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
      m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
    {
      NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                      "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                      "In other words, use TCP instead of UDP.");
    }

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
    NS_LOG_UNCOND("Server: connection callback ");

    m_connected = true;
    return true;
}

void DashServerApp::AcceptCallback(Ptr<Socket> socket, const Address &ads)
{
    NS_LOG_UNCOND("Server: accept callback ");

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
        uint32_t toSend = min (m_packetSize, m_remainingData);
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

    void Setup(Address address1, Address address2, uint32_t chunkSize, uint32_t numChunks, string algorithm);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    // Request
    void SendRequest(void);
    void RequestNextChunk(void);
    void GetStatistics(void);
    void RxCallback(Ptr<Socket> socket);

    // Buffer Model
    void ClientBufferModel(void);
    void GetBufferState(void);
    void RxDrop (Ptr<const Packet> p);

    // Rate Adaptation Algorithm
    void DASH(void);
    void Conventional(void);
    void PANDA(void);
    void FESTIVE(void);
    void BBA(void);
    void Proposed(void);

    // Another
    uint32_t GetQualityDuration(uint32_t bitrate);
    uint32_t GetIndexByBitrate(uint32_t bitrate);
    void UpdateNumberOfSwitching(void);

    Ptr<Socket> m_socket;
    Ptr<Socket> m_socket2;
    Address m_peer;
    Address m_peer2;
    uint32_t m_chunkSize;
    uint32_t m_numChunks;
    uint32_t m_chunkCount;
    int32_t m_bufferSize;
    uint32_t m_bufferPercent;
    uint32_t m_bpsAvg;
    uint32_t m_bpsLastChunk;
    vector<uint32_t> m_bitrate_array;
    EventId m_fetchEvent;
    EventId m_statisticsEvent;
    bool m_running;
    uint32_t m_comulativeSize;
    uint32_t m_lastRequestedSize;
    Time m_requestTime;
    uint32_t m_sessionData;
    uint32_t m_sessionTime;

    EventId m_bufferEvent;
    EventId m_bufferStateEvent;
    uint32_t m_downloadDuration;
    double m_throughput;
    uint32_t m_prevBitrate;
    uint32_t m_nextBitrate;
    uint32_t m_algorithm;
    uint32_t m_numOfSwitching;

    // PANDA
    double bandwidth_panda;

    // FESTIVE
    uint32_t chunkNum;
    uint32_t currentBitrate;
    uint32_t referenceBitrate;
    uint32_t bitrateLevel;
    uint32_t numberOfSwitching;
    std::vector<double> k_bandwidth_array;

    // Proposed
    double bitrateRatio;
    double bufferOccupancy;
    double qualityDurationFactor;
    double futureBuffer;
    double bufferVariation;
    uint32_t qualityDuration;
};

DashClientApp::DashClientApp() :
    m_socket(0), m_socket2(0), m_peer(), m_peer2(), m_chunkSize(0), m_numChunks(0), m_chunkCount(0),
    m_bufferSize(0), m_bufferPercent(0), m_bpsAvg(0), m_bpsLastChunk(0),
    m_fetchEvent(), m_statisticsEvent(), m_running(false),
    m_comulativeSize(0), m_lastRequestedSize(0), m_requestTime(), m_sessionData(0), m_sessionTime(0),
    m_bufferEvent(), m_bufferStateEvent(), m_downloadDuration(0), m_throughput(0.0), 
    m_prevBitrate(0), m_nextBitrate(0), m_algorithm(0), m_numOfSwitching(0),
    bandwidth_panda(0.0),
    chunkNum(0), currentBitrate(0), referenceBitrate(0), bitrateLevel(0), numberOfSwitching(0),
    bitrateRatio(0.0), bufferOccupancy(0.0), qualityDurationFactor(0.0), futureBuffer(0.0), bufferVariation(0.0), qualityDuration(0)
{

}

DashClientApp::~DashClientApp()
{
    m_socket = 0;
}

void DashClientApp::Setup(Address address1, Address address2, uint32_t chunkSize,
                          uint32_t numChunks, string algorithm)
{
    m_peer = address1;
    m_peer2 = address2;
    m_chunkSize = chunkSize;
    m_numChunks = numChunks;

    //bitrate profile of the content
    m_bitrate_array.push_back(700000);
    m_bitrate_array.push_back(1400000);
    m_bitrate_array.push_back(2100000);
    m_bitrate_array.push_back(2800000);
    m_bitrate_array.push_back(3500000);
    m_bitrate_array.push_back(4200000);

    if (algorithm.compare("DASH") == 0)
        m_algorithm = 0;
    else if (algorithm.compare("Conventional") == 0)
        m_algorithm = 1;
    else if (algorithm.compare("PANDA") == 0)
        m_algorithm = 2;
    else if (algorithm.compare("FESTIVE") == 0)
        m_algorithm = 3;
    else if (algorithm.compare("BBA") == 0)
        m_algorithm = 4;
    else if (algorithm.compare("Proposed") == 0)
        m_algorithm = 5;
    else {
        cout << "Unknown algorithm" << endl;
        exit(1);
    }
}

void DashClientApp::RxDrop(Ptr<const Packet> p)
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

    //

	m_socket2 = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket2->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&DashClientApp::RxDrop, this));

    m_running = true;
    m_socket2->Bind();
    m_socket2->Connect(m_peer2);
    m_socket2->SetRecvCallback(MakeCallback(&DashClientApp::RxCallback, this));    

    SendRequest();

    Time tNext("1s");
    m_bufferStateEvent = Simulator::Schedule(tNext, &DashClientApp::GetBufferState, this);
}

void DashClientApp::RxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv())) {
        if (packet->GetSize() == 0) {   // EOF
            break;
        }

        // For calculate throughput
        m_comulativeSize += packet->GetSize();

        if (m_comulativeSize >= m_lastRequestedSize)
        {
            // Received the complete chunk
            // Update the buffer size and initiate the next request

            m_chunkCount++;
            
            // Estimating
            m_downloadDuration = Simulator::Now().GetMilliSeconds() - m_requestTime.GetMilliSeconds();
            m_bpsLastChunk = (m_comulativeSize * 8) / (m_downloadDuration / 1000.0);

            // Update buffer
            m_bufferSize += m_chunkSize * 1000;
            m_bufferPercent = (uint32_t) (m_bufferSize * 100) / MAX_BUFFER_SIZE;

            // Scheduling
            Simulator::ScheduleNow(&DashClientApp::RequestNextChunk, this);

            // Start BufferModel
            if (m_chunkCount == 1) {
                Time tNext("100ms");
                m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::ClientBufferModel, this);
            }

            // Monitoring
            m_statisticsEvent = Simulator::ScheduleNow(&DashClientApp::GetStatistics, this);

            m_comulativeSize = 0;
        }
    }
}

void DashClientApp::RequestNextChunk(void)
{
    switch (m_algorithm) {
        case 0:
            DASH();
            break;
        case 1:
            Conventional();
            break;
        case 2:
            PANDA();
            break;
        case 3:
            FESTIVE();
            break;
        case 4:
            BBA();
            break;
        case 5:
            Proposed();
            break;
    }
}

void DashClientApp::DASH(void)
{
    // Estimating
    uint32_t bandwidth = m_bpsLastChunk;

    // Quantizing
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++) {
        if (bandwidth <= m_bitrate_array[i])
            break;
        m_nextBitrate = m_bitrate_array[i];
    }

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else {
        Time tNext(Seconds(m_chunkSize));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }
}

void DashClientApp::Conventional(void)
{
    // Estimating
    uint32_t bandwidth = m_bpsLastChunk;

    // Smoothing
    if (m_bpsAvg == 0)
        m_bpsAvg = bandwidth;
    else
        m_bpsAvg = 0.8 * m_bpsAvg + 0.2 * bandwidth;

    // Quantizing
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++) {
        if (m_bpsAvg <= m_bitrate_array[i])
            break;
        m_nextBitrate = m_bitrate_array[i];
    }

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else {
        Time tNext(Seconds(m_chunkSize));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }
}

void DashClientApp::PANDA(void)
{
    uint32_t bandwidth = m_bpsLastChunk;

    // double safetyMargin;
    double requestInterval;
    double k = 0.56;
    double w = 300000;
    double T = (double) max(m_chunkSize * 1000, m_downloadDuration) / 1000.0;
    // double e = 0.15;
    double b = 0.2;
    int bufMin = 26000;

    // Estimating
    if (bandwidth_panda + w < bandwidth)
        bandwidth_panda = bandwidth_panda + k * w * T;
    else {
        bandwidth_panda = bandwidth_panda + k * ((double) bandwidth - bandwidth_panda) * T;
        if (bandwidth_panda < 0)
            bandwidth_panda = 0;
    }

    // Smoothing
    if (m_bpsAvg == 0)
        m_bpsAvg = bandwidth_panda;

    m_bpsAvg = 0.8 * m_bpsAvg + 0.2 * bandwidth_panda;

    // Quantizing
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++) {
        if (m_bpsAvg <= m_bitrate_array[i]) 
            break;
        m_nextBitrate = m_bitrate_array[i];
    }

    // // Dead-zone quantizer
    // if (currentBitrate < m_nextBitrate)
    //     safetyMargin = e * m_bpsAvg;    
    // else if (currentBitrate > m_nextBitrate)
    //     safetyMargin = 0;
        
    // if (currentBitrate != m_nextBitrate) {
    //     if (m_nextBitrate <= m_bpsAvg - safetyMargin) {
    //         // do nothing
    //     } else {
    //         m_nextBitrate = currentBitrate;
    //     }
    // }

    requestInterval = (m_nextBitrate * m_chunkSize) / m_bpsAvg + b * ((m_bufferSize - bufMin) / 1000.0);
    if (requestInterval < 0) requestInterval = 0;

    Time tNext(Seconds(requestInterval));
    Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
}

void DashClientApp::FESTIVE(void)
{
    // Estimating
    uint32_t bandwidth = m_bpsLastChunk;

    uint32_t alpha = 12;
    uint32_t k = 20;
    uint32_t tmpLevel;
    // double safetyMargin = 0.85;
    double requestInterval;
    
    // Bandwidth sampling
    k_bandwidth_array.push_back(bandwidth);
    if (k_bandwidth_array.size() > k)
        k_bandwidth_array.erase(k_bandwidth_array.begin());

    // Smoothing
    if (k_bandwidth_array.size() == k) {
        // Harmonic bandwidth estimator
        double sum = 0;

        for (uint32_t i = 0; i < k_bandwidth_array.size(); i++)
            sum += 1.0 / k_bandwidth_array[i];

        m_bpsAvg = (double) k_bandwidth_array.size() / sum;
    } else  {
        m_bpsAvg = 0;
    }

    // Initialize
    if (currentBitrate == 0) {
        currentBitrate = m_bitrate_array[0];
        bitrateLevel = 0;
    }

    // Stateful bitrate selection
    if (currentBitrate == m_bitrate_array[m_bitrate_array.size() - 1]) {
        // Most highest bitrate
        if (m_bpsAvg < currentBitrate) {
            // Bitrate decrease
            referenceBitrate = m_bitrate_array[bitrateLevel - 1];
            tmpLevel = bitrateLevel - 1;
        } else {
            // Bitrate maintain
            m_nextBitrate = referenceBitrate = currentBitrate;
        }
    } else if (currentBitrate == m_bitrate_array[0]) {
        // Most lowest bitrate
        if (m_bitrate_array[bitrateLevel + 1] < m_bpsAvg) {
            // Bitrate increase
            referenceBitrate = m_bitrate_array[bitrateLevel + 1];
            tmpLevel = bitrateLevel + 1;
        } else {
            // Bitrate maintain
            m_nextBitrate = referenceBitrate = currentBitrate;
        }
    } else {
        // Another bitrate
        if (m_bitrate_array[bitrateLevel + 1] < m_bpsAvg) {
            // Bitrate increase
            chunkNum++;
            if (chunkNum >= bitrateLevel) {
                referenceBitrate = m_bitrate_array[bitrateLevel + 1];
                tmpLevel = bitrateLevel + 1;
            } else {
                // Bitrate maintain
                m_nextBitrate = referenceBitrate = currentBitrate;
            }
        } else if (m_bpsAvg < currentBitrate) {
            // Bitrate decrease
            referenceBitrate = m_bitrate_array[bitrateLevel - 1];
            tmpLevel = bitrateLevel - 1;
        } else {
            // Bitrate maintain
            m_nextBitrate = referenceBitrate = currentBitrate;
        }
    }

    // Delayed update
    if (currentBitrate != referenceBitrate) {
        double efficiency = 0;
        double stability = 0;
        double currentScore = 0;
        double referenceScore = 0;

        // Current score
        efficiency = ((double) currentBitrate / fmin(m_bpsAvg, referenceBitrate)) -1;
        if (efficiency < 0) efficiency = -efficiency;
        stability = pow(2.0, (double)numberOfSwitching);
        currentScore = stability + alpha * efficiency;

        // Reference score
        efficiency = ((double) referenceBitrate / fmin(m_bpsAvg, referenceBitrate)) - 1;
        if (efficiency < 0) efficiency = -efficiency;
        stability = pow(2.0, (double)numberOfSwitching) + 1;
        referenceScore = stability + alpha * efficiency;

        if (referenceScore == fmin(currentScore, referenceScore))
            m_nextBitrate = referenceBitrate;
        else
            m_nextBitrate = currentBitrate;
    }

    if (currentBitrate != m_nextBitrate) {
        numberOfSwitching++;
        bitrateLevel = tmpLevel;
        chunkNum = 0;

        Time tNext(Seconds(k));
        Simulator::Schedule(tNext, &DashClientApp::UpdateNumberOfSwitching, this);
    }

     // Randomized scheduler
    int randBuf = MAX_BUFFER_SIZE - m_chunkSize + rand() % (2 * m_chunkSize) + 1;

    if (m_bufferSize < randBuf)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else {
        requestInterval = (double)(m_bufferSize - randBuf) / 1000.0;

        Time tNext(Seconds(requestInterval));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }
}

void DashClientApp::UpdateNumberOfSwitching(void)
{
    if (numberOfSwitching > 0)
        numberOfSwitching--;
}

void DashClientApp::BBA(void)
{
    // Buffer-based Adaptation
    if (m_bufferSize < 5000)
        m_nextBitrate = m_bitrate_array[0];
    else if (m_bufferSize < 10000)
        m_nextBitrate = m_bitrate_array[1];
    else if (m_bufferSize < 15000)
        m_nextBitrate = m_bitrate_array[2];
    else if (m_bufferSize < 20000)
        m_nextBitrate = m_bitrate_array[3];
    else if (m_bufferSize < 25000)
        m_nextBitrate = m_bitrate_array[4];
    else
        m_nextBitrate = m_bitrate_array[5];

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else {
        Time tNext(Seconds(m_chunkSize));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }
}

void DashClientApp::Proposed(void)
{
    // Estimating
    uint32_t bandwidth = m_bpsLastChunk;
    double downTime;

    // Smoothing
    if (m_bpsAvg == 0)
        m_bpsAvg = bandwidth;
    else
        m_bpsAvg = 0.8 * m_bpsAvg + 0.2 * bandwidth;

    if (qualityDuration <= 0) {
        // Quantizing
        for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
        {
            if (m_bpsAvg <= m_bitrate_array[i])
            {
                break;
            }
            m_nextBitrate = m_bitrate_array[i];
        }

        if (m_prevBitrate <= m_bpsAvg) {
            qualityDuration = GetQualityDuration(m_nextBitrate);
            NS_LOG_UNCOND("Quality Duration : " << qualityDuration);
        } else {
            m_nextBitrate = m_prevBitrate;
            qualityDuration = GetQualityDuration(m_nextBitrate);
            
            // Quality adaptation algorithm
            downTime = (double)(qualityDuration * m_nextBitrate * m_chunkSize) / (double)m_bpsAvg;
            
            futureBuffer = ((double) m_bufferSize / 1000.0) + qualityDuration * m_chunkSize - downTime;
            bufferVariation = (futureBuffer - ((double)m_bufferSize / 1000.0)) / downTime;

            if (bufferVariation < -bufferOccupancy && m_nextBitrate != m_bitrate_array[0]) {
                m_nextBitrate = m_bitrate_array[GetIndexByBitrate(m_nextBitrate) - 1];
                qualityDuration = GetQualityDuration(m_nextBitrate);
            }

            NS_LOG_UNCOND("Quality Duration : " << qualityDuration);
            NS_LOG_UNCOND("Next Buffer Level : " << futureBuffer);
            NS_LOG_UNCOND("Buffer Variation : " << bufferVariation);
        }
    }

    qualityDuration--;

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else {
        Time tNext(Seconds(m_chunkSize));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }
}

uint32_t DashClientApp::GetQualityDuration(uint32_t bitrate)
{
    uint32_t QMD;

    // Calculate the quality maintain duration factor
    bitrateRatio = (double) bitrate / (double) m_bitrate_array[m_bitrate_array.size() - 1];
    bufferOccupancy = (double) m_bufferSize / (double) MAX_BUFFER_SIZE;

    if (bitrateRatio < -bufferOccupancy + 1)
        qualityDurationFactor = -bufferOccupancy - bitrateRatio + 1;
    else if (bitrateRatio > -bufferOccupancy + 1)
        qualityDurationFactor = bufferOccupancy + bitrateRatio - 1;
    else
        qualityDurationFactor = 0;

    // Calculate the quality maintain duration
    QMD = 1.5 + (((double) m_bufferSize / 1000.0) / (double) m_chunkSize) * qualityDurationFactor;

    return QMD;
}

uint32_t DashClientApp::GetIndexByBitrate(uint32_t bitrate)
{
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++) {
        if (bitrate == m_bitrate_array[i])
            return i;
    }

    return -1;
}

void DashClientApp::SendRequest(void)
{
    if (m_nextBitrate == 0)
        m_nextBitrate = m_bitrate_array[0];

    uint32_t bytesReq = (m_nextBitrate * m_chunkSize) / 8 / 2;
    uint32_t bytesReq2 = (m_nextBitrate * m_chunkSize) / 8 / 2;
    Ptr<Packet> packet = Create<Packet>((uint8_t *) &bytesReq, 4);
    Ptr<Packet> packet2 = Create<Packet>((uint8_t *) &bytesReq2, 4);

    m_lastRequestedSize = bytesReq + bytesReq2;
    m_socket->Send(packet);
    m_socket2->Send(packet2);

    if (m_prevBitrate != 0 && m_prevBitrate != m_nextBitrate) {
        m_numOfSwitching++;
        NS_LOG_UNCOND ("Number of Quality Switching : " << m_numOfSwitching);
    }

    m_prevBitrate = currentBitrate = m_nextBitrate;

    m_requestTime = Simulator::Now();
}

void DashClientApp::ClientBufferModel(void)
{
    if (m_running)
    {
        if (m_bufferSize < 100) {
            m_bufferSize = 0;
            m_bufferPercent = 0;
            m_chunkCount = 0;

            Simulator::Cancel(m_bufferEvent);
            return;
        }

        m_bufferSize -= 100;

        Time tNext("100ms");
        m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::ClientBufferModel, this);
    }
}

void DashClientApp::GetStatistics()
{
    NS_LOG_UNCOND ("=======START=========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
                   " bpsAverage: " << m_bpsAvg <<
                   " bpsLastChunk: " << m_bpsLastChunk / 1000 <<
                   " nextBitrate: " << m_nextBitrate <<
                   " chunkCount: " << m_chunkCount <<
                   " totalChunks: " << m_numChunks <<
                   " downloadDuration: " << m_downloadDuration);
}

void DashClientApp::GetBufferState()
{
    m_throughput = m_comulativeSize * 8 / 1 / 1000;

    NS_LOG_UNCOND ("=======BUFFER========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
                   " bufferSize: " << m_bufferSize / 1000 <<
                   " hasThroughput: " << m_throughput <<
                   " estimatedBW: " << m_bpsLastChunk / 1000.0 <<
                   " videoLevel: " << (m_lastRequestedSize * 8) / m_chunkSize / 1000);
    
    Time tNext("1s");
    m_bufferStateEvent = Simulator::Schedule(tNext, &DashClientApp::GetBufferState, this);
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

//=================================================================
// SIMULATION
//=================================================================

void setPos(Ptr<Node> n, int x, int y, int z)
{
	Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();
	n->AggregateObject(loc);
	Vector locVec(x,y,z);
	loc->SetPosition(locVec);
}

int main(int argc, char *argv[])
{
    string algorithm;

    cout << "Input the adaptation algorithm : ";
    cin >> algorithm;

    LogComponentEnable("DashApplication", LOG_LEVEL_ALL);

    //////////////////////////////////////////////////////////////////////////////////
    // WIRED NETWORK ENVIRONMENT /////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    NodeContainer node;
    node.Create(5);

    InternetStackHelper internet;
    internet.Install(node);

	PointToPointHelper link;
    link.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
   	link.SetDeviceAttribute("Mtu", UintegerValue(1500));
    link.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

    PointToPointHelper bottleNeck;
    bottleNeck.SetDeviceAttribute("DataRate", DataRateValue(DataRate("4Mbps")));
   	bottleNeck.SetDeviceAttribute("Mtu", UintegerValue(1500));
    bottleNeck.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

    NetDeviceContainer d0d1 = link.Install(node.Get(0), node.Get(1));
    NetDeviceContainer d2d3 = link.Install(node.Get(2), node.Get(3));
    NetDeviceContainer d3d4 = bottleNeck.Install(node.Get(3), node.Get(4));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = address.Assign(d0d1);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i3 = address.Assign(d2d3);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i4 = address.Assign(d3d4);

    setPos(node.Get(0), 50, 70, 0);
    setPos(node.Get(1), 60, 60, 0);
    setPos(node.Get(2), 50, 40, 0);
    setPos(node.Get(3), 60, 50, 0);
    setPos(node.Get(4), 70, 50, 0);

    //////////////////////////////////////////////////////////////////////////////////
    // WIFI NETWORK ENVIRONMENT ///////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    // Define the APs
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Define the STAs
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    // Create wifi channel
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(3.0), "ReferenceLoss", DoubleValue(40.0459));
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("ShortGuardEnabled", BooleanValue(true));

    // Set wifi mac
    WifiHelper wifi;
    // WIFI 802.11n
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("HtMcs0"), "ControlMode", StringValue("HtMcs0"));
    HtWifiMacHelper wifiMac = HtWifiMacHelper::Default();
    // WIFI 802.11ac
    // wifi.SetStandard(WIFI_PHY_STANDARD_80211ac);
    // wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("VhtMcs0"), "ControlMode", StringValue("VhtMcs0"));
    // VhtWifiMacHelper wifiMac = VhtWifiMacHelper::Default();

    // Install wifi device
    Ssid ssid = Ssid("ns3-wifi");
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, wifiStaNode);

    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, wifiApNode);

    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue(20));

    // Install mobility model
    MobilityHelper mobility;
	
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(70.0, 60.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator", 
    							"X", StringValue("80.0"),
    							"Y", StringValue("50.0"),
    							"Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
							"Mode", StringValue("Time"),
							"Time", StringValue("100ms"),
							"Speed", StringValue("ns3::ConstantRandomVariable[Constant=30.0]"),
							"Bounds", StringValue("0|1000|0|1000"));
    mobility.Install(wifiStaNode);

    // Install wifi node stack
    internet.Install(wifiStaNode);
    internet.Install(wifiApNode);

    // Set wifi address
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface;
    Ipv4InterfaceContainer apNodeInterface;
    staNodeInterface = address.Assign(staDevice);
    apNodeInterface = address.Assign(apDevice);

    NodeContainer n1ap = NodeContainer(node.Get(1), wifiApNode.Get(0));
	NetDeviceContainer d1ap = link.Install(node.Get(1), wifiApNode.Get(0));
	address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i1ap = address.Assign(d1ap);

	NetDeviceContainer d4ap = link.Install(node.Get(4), wifiStaNode.Get(0));
	address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i4ap = address.Assign(d4ap);

    //////////////////////////////////////////////////////////////////////////////////
    // LTE NETWORK ENVIRONMENT ///////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

 //    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
 //    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
	// lteHelper->SetEpcHelper(epcHelper);

	// Ptr<Node> pgw = epcHelper->GetPgwNode();
	// Ptr<Node> remoteHost = node.Get(0);

	// NetDeviceContainer d1d0 = link.Install(pgw, remoteHost);
	// address.SetBase("1.0.0.0", "255.0.0.0");
 //    Ipv4InterfaceContainer i1i0 = address.Assign(d1d0);
 //    Ipv4Address remoteHostAddr = i1i0.GetAddress(1);

 //    Ipv4StaticRoutingHelper ipv4RoutingHelper;
 //    Ptr<Ipv4StaticRouting> remoteHostStaticeRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
 //    remoteHostStaticeRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

 //    NodeContainer enbNode;
 //    NodeContainer ueNode;
 //    enbNode.Create(1);
 //    ueNode.Create(1);

	// // MobilityHelper mobility;
	// // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	// // mobility.Install(enbNode);
	// // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	// // mobility.Install(ueNode);
	// setPos(enbNode.Get(0), 70, 40, 0);
	// setPos(ueNode.Get(0), 80, 50, 0);
	// setPos(pgw, 60, 40, 0);

	// NetDeviceContainer enbDevice;
	// NetDeviceContainer ueDevice;
	
	// enbDevice = lteHelper->InstallEnbDevice(enbNode);
	// ueDevice = lteHelper->InstallUeDevice(ueNode);

	// internet.Install(ueNode);
	// Ipv4InterfaceContainer ueIpIface;
	// ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevice));

	// Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode.Get(0)->GetObject<Ipv4>());
	// ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

	// lteHelper->Attach(ueDevice.Get(0), enbDevice.Get(0));
 // 	lteHelper->ActivateDedicatedEpsBearer(ueDevice, EpsBearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default());

    //////////////////////////////////////////////////////////////////////////////////
    // APPLICATION ///////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    uint16_t port = 8080;

    // // DASH server
    // Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), port));
    // Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    // serverApp1->Setup(bindAddress1, 512);
    // node.Get(0)->AddApplication(serverApp1);
    // serverApp1->SetStartTime(Seconds(0.0));
    // serverApp1->SetStopTime(Seconds(50.0));
    
    // // DASH client
    // Address serverAddress1(InetSocketAddress(i0i2.GetAddress(0), port));
    // Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
    // clientApp1->Setup(serverAddress1, 2, 512, algorithm);
    // wifiStaNode.Get(0)->AddApplication(clientApp1);
    // clientApp1->SetStartTime(Seconds(0.0));
    // clientApp1->SetStopTime(Seconds(50.0));

    // DASH server1
    Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), port));
    Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    serverApp1->Setup(bindAddress1, 512);
    node.Get(0)->AddApplication(serverApp1);
    serverApp1->SetStartTime(Seconds(0.0));
    serverApp1->SetStopTime(Seconds(30.0));

    // DASH server2
    Address bindAddress2(InetSocketAddress(Ipv4Address::GetAny(), port));
    Ptr<DashServerApp> serverApp2 = CreateObject<DashServerApp>();
    serverApp2->Setup(bindAddress2, 512);
    node.Get(2)->AddApplication(serverApp2);
    serverApp2->SetStartTime(Seconds(0.0));
    serverApp2->SetStopTime(Seconds(30.0));
    
    // DASH client
    Address serverAddress1(InetSocketAddress(i0i1.GetAddress(0), port));
    Address serverAddress2(InetSocketAddress(i2i3.GetAddress(0), port));
    Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
    clientApp1->Setup(serverAddress1, serverAddress2, 2, 512, algorithm);
    wifiStaNode.Get(0)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.0));
    clientApp1->SetStopTime(Seconds(30.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(30.0));

    AnimationInterface anim("animation.xml");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
