#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DashApplication");

// New Feature
// Multi-Socket Server-Based Distribution HAS Player

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
    uint32_t AddressCheck(std::vector<Address> address_list, Address address);
    void MultiConnectionSendData(std::vector<Address> address_list, Address address, Ptr<Packet> packet);
    bool ConnectionCallback(Ptr<Socket> s, const Address &ad);
    void AcceptCallback(Ptr<Socket> s, const Address &ad);
    void SendData();

    void CwndChange(uint32_t oldCwnd, uint32_t newCwnd);
    void RttTrace(Time oldRtt, Time newRtt);
    void SsthreshTrace(uint32_t oldSsthresh, uint32_t newSsthresh);
    void EstimatedBWTrace(double oldBW, double newBW);

    bool m_connected;
    bool m_connection_callback_validated;
    Ptr<Socket> m_socket;
    Ptr<Socket> m_peer_socket;
    std::vector<Ptr<Socket>> m_peer_socket_list;
    std::vector<bool> m_connected_list;
    uint32_t m_userCount;

    uint32_t m_cwnd;
    Time m_rtt;
    uint32_t m_ssthresh;
    Ptr<TcpSocketBase> m_tcpSocketBase;

    Address ads;
    Address m_peer_address;
    std::vector<Address> m_peer_address_list;
    uint32_t m_remainingData;
    EventId m_sendEvent;
    uint32_t m_packetSize;
    uint32_t m_packetCount;
    uint32_t m_peer_address_count;
    uint32_t m_peer_socket_count;
    int actual;
    uint32_t m_client;
};

DashServerApp::DashServerApp() :
    m_connected(false), m_connection_callback_validated(false), m_socket(0), m_peer_socket(0), 
    m_userCount(0), m_cwnd(0), m_rtt(0), m_ssthresh(0), m_tcpSocketBase(0), ads(), m_peer_address(), 
    m_remainingData(0), m_sendEvent(), m_packetSize(0), m_packetCount(0), m_peer_address_count(0),
    m_peer_socket_count(0), actual(0), m_client(0)
{

}

DashServerApp::~DashServerApp()
{
    m_socket = 0;

    // Multi-Socket List Reset
    if (!m_peer_socket_list.empty() && !m_peer_address_list.empty()){
    	m_peer_socket_list.clear();
    	m_peer_address_list.clear();
    }
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
    if (!m_connected_list.empty())
    	m_connected_list.clear();

    if (m_socket)
        m_socket->Close();

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }
}

bool DashServerApp::ConnectionCallback(Ptr<Socket> socket, const Address &ads)
{
    // Multi-Socket Connect
    NS_LOG_UNCOND("Server : connection callback");

    m_connected = true;
    m_connected_list.push_back(m_connected);
    m_userCount++;

    if (!m_connected_list.empty())
    	m_connection_callback_validated = true;

    return m_connection_callback_validated;
}

void DashServerApp::AcceptCallback(Ptr<Socket> socket, const Address &ads)
{
    // Multi-Socket Accept

    // NS_LOG_UNCOND("Server : accept callback");
    m_peer_address = ads;
    m_peer_socket = socket;

    m_peer_address_list.push_back(m_peer_address);
    m_peer_socket_list.push_back(m_peer_socket);

    m_peer_address_count++;

    Ptr<Socket> m_accepted_socket = m_peer_socket_list[m_peer_socket_count];

   /* m_accepted_socket->TraceConnectWithoutContext("CongestionWindow",
    	MakeCallback(&DashServerApp::CwndChange, this));
    m_accepted_socket->TraceConnectWithoutContext("SlowStartThreshold",
    	MakeCallback(&DashServerApp::SsthreshTrace, this));*/

    m_accepted_socket->SetRecvCallback(MakeCallback(&DashServerApp::RxCallback, this));
    m_accepted_socket->SetSendCallback(MakeCallback(&DashServerApp::TxCallback, this));

    m_peer_socket_count++;

    NS_LOG_UNCOND(GetNode() << " Server Peer Socket Count : " << m_peer_socket_count);
    NS_LOG_UNCOND(GetNode() << " Server Peer Address Count : " << m_peer_address_count);
}

void DashServerApp::CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    //g_hasCwnd = newCwnd;
    //NS_LOG_UNCOND ("cwnd\t" << GetNode() << "\t" << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    //m_tcpSocketBase->GetBytesInFlight();
}

void DashServerApp::SsthreshTrace(uint32_t oldSsthresh, uint32_t newSsthresh)
{
    //m_ssthresh = newSsthresh;
    // NS_LOG_UNCOND ("ssthresh at " << GetNode() << "\t" << Simulator::Now ().GetSeconds () << "\t" <<
    //                newSsthresh);
}

void DashServerApp::RxCallback(Ptr<Socket> socket)
{
    Address ads;
    Ptr<Packet> pckt = socket->RecvFrom(ads);

    // Multi-Connection Send Data
    MultiConnectionSendData(m_peer_address_list, ads, pckt);
}

uint32_t DashServerApp::AddressCheck(std::vector<Address> address_list, Address address)
{
    // Return Number of Current Connection
	uint32_t m_whose_address = 0;
	Address temp_address;

	for (unsigned int i = 0; i < address_list.size(); i++){
		temp_address = address_list[i];

		if (address == temp_address)
			m_whose_address = i;
	}

	return m_whose_address;
}

void DashServerApp::MultiConnectionSendData(std::vector<Address> address_list, Address address, Ptr<Packet> packet)
{

    // Should be Multi-Threaded
    // Get Number of Current Connection
	m_client = AddressCheck(address_list, address);

    // Multi-Socket Address Compare
	for (unsigned int i = 0; i < address_list.size(); i++){
		Address temp_address = address_list[i];

		if (address == temp_address){

            uint32_t data = 0;          
            packet->CopyData((uint8_t *) &data, 4);
        	   m_remainingData = data;
            m_packetCount = 0;

            //NS_LOG_UNCOND(i << "'s Remaining Data : " << m_remainingData);

 		     SendData();
		}
	}
}


void DashServerApp::TxCallback(Ptr<Socket> socket, uint32_t txSpace)
{
    if (!m_connected_list.empty())
        Simulator::ScheduleNow (&DashServerApp::SendData, this);
}

// How to Schedule Transmission Data?
void DashServerApp::SendData()
{
    Ptr<Socket> m_send_socket;

    while (m_remainingData > 0)
    {
        uint32_t toSend = std::min (m_packetSize, m_remainingData);
        Ptr<Packet> packet = Create<Packet> (toSend);

        int actual = 0;

        // Multi-Socket Transmission
        for (int i = 0; (unsigned)i < m_peer_socket_list.size(); i++){
            m_send_socket = m_peer_socket_list[i];

            actual = m_send_socket->Send (packet);
        }

        // NS_LOG_UNCOND("Server send at " << Simulator::Now().GetMilliSeconds()  <<
        //               " remain: " << m_remainingData << " cwnd: " << m_cwnd <<
        //               " rtt: " << m_rtt.GetMilliSeconds() << " actual: " << actual);

        // if (toSend == m_remainingData)
        // {
        //     NS_LOG_UNCOND("Server send last packet to at " <<
        //                   Simulator::Now().GetMilliSeconds());
        // }

        if (actual > 0)
        {
            m_remainingData -= actual;
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

    void Setup(Address address, Address address2, uint32_t chunkSize, uint32_t numChunks, std::string algorithm);

    typedef enum
    {
        UP,     // 1
        HOLD,   // 2
        DOWN,   // 3
        LAST_STATE
    } HasStates_t;
    static const char *const HasStateName[LAST_STATE];

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void RequestNextChunk(void);
    void SendRequest(void);
    void GetStatistics(void);
    void RxCallback(Ptr<Socket> socket);

    // Jiwoo
    void BufferModel(void);
    void GetBufferState(void);
    void RxDrop (Ptr<const Packet> p);
    uint32_t GetIndexByBitrate(uint32_t bitrate);
    void TrafficMonitor(void);

    // Rate Adaptation
    void Conventional(void);
    void PANDA(void);
    void BBA(void);
    void Proposed(void);

    Ptr<Socket> m_socket;
    Ptr<Socket> m_socket2;
    Ptr<Socket> m_socket3;
    Address m_peer;
    Address m_peer2;
    uint32_t m_chunkSize;
    uint32_t m_numChunks;
    uint32_t m_chunkCount;
    uint32_t m_bufferSize;
    uint32_t m_bpsAvg;
    uint32_t m_bpsLastChunk;
    std::vector<uint32_t> m_bitrate_array;
    EventId m_fetchEvent;
    EventId m_statisticsEvent;
    bool m_running;
    uint32_t m_lastRequestedSize;
    Time m_requestStartTime;
    uint32_t m_sessionData;
    uint32_t m_sessionTime;

    // Jiwoo
    EventId m_bufferEvent;
    EventId m_bufferStateEvent;
    uint32_t m_downloadDuration;
    uint32_t m_oldBitrate;
    double m_throughput;
    uint32_t m_cumulativeSize;
    uint32_t m_nextBitrate;
    uint32_t m_bandwidthAvg;
    int m_nextRequestTime;
    uint32_t m_algorithm;
    // PANDA 
    double bandwidth_panda;
    // Proposed
    double aA, vA, cA;     // Smoothing Paramater
    double gamma;           // Smoothing Paramater
    double lambda, mu;      // arrival and service rate
    double rho, rhoAvg;     // Traffic intensity
    HasStates_t m_state;
    std::vector<uint32_t> m_bitrate_history;
};

const char *const DashClientApp::HasStateName[LAST_STATE] = { "UP", "HOLD", "DOWN"};

DashClientApp::DashClientApp() :
    m_socket(0), m_socket2(0), m_socket3(0), m_peer(), m_peer2(), m_chunkSize(0), m_numChunks(0), m_chunkCount(0),
    m_bufferSize(0), m_bpsAvg(0), m_bpsLastChunk(0),
    m_fetchEvent(), m_statisticsEvent(), m_running(false), m_lastRequestedSize(0),
    m_requestStartTime(), m_sessionData(0), m_sessionTime(0), m_bufferEvent(),
    m_bufferStateEvent(), m_downloadDuration(0), m_oldBitrate(0), m_throughput(0.0), m_cumulativeSize(0),
    m_nextBitrate(0), m_bandwidthAvg(0), m_nextRequestTime(0), m_algorithm(0), bandwidth_panda(0.0), aA(0.0), vA(0.0), cA(0.0), gamma(0.0), lambda(0.0), mu(0.0),
    rho(0.0), rhoAvg(0.0), m_state(HOLD)
{
}

DashClientApp::~DashClientApp()
{
    m_socket = 0;
    m_socket2 = 0;
    m_socket3 = 0;
}

void DashClientApp::Setup(Address address, Address address2, uint32_t chunkSize, uint32_t numChunks, std::string algorithm)
{
    m_peer = address;
    m_peer2 = address2;
    m_chunkSize = chunkSize;    // 2s
    m_numChunks = numChunks;
    //bitrate profile of the contnet
    m_bitrate_array.push_back(700000);  //240p
    m_bitrate_array.push_back(1400000); //360p
    m_bitrate_array.push_back(2100000); //480p
    m_bitrate_array.push_back(2800000); //720p
    m_bitrate_array.push_back(3500000); //1080p
    m_bitrate_array.push_back(4900000); 
   /* m_bitrate_array.push_back(6300000); 
    m_bitrate_array.push_back(7700000);*/

    if (algorithm.compare("Conventional") == 0)
        m_algorithm = 0;
    else if (algorithm.compare("PANDA") == 0)
        m_algorithm = 1;
    else if (algorithm.compare("BBA") == 0)
        m_algorithm = 2;
    else if (algorithm.compare("Proposed") == 0)
        m_algorithm = 3;
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

   /* m_socket2 = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket2->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&DashClientApp::RxDrop, this));
    m_socket2->Bind();
    m_socket2->Connect(m_peer);
    m_socket2->SetRecvCallback(MakeCallback(&DashClientApp::RxCallback, this));*/

   /* m_socket3 = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    m_socket3->TraceConnectWithoutContext("Drop",
                                         MakeCallback (&DashClientApp::RxDrop, this));
    m_socket3->Bind();
    m_socket3->Connect(m_peer);
    m_socket3->SetRecvCallback(MakeCallback(&DashClientApp::RxCallback, this));*/

    SendRequest();

    NS_LOG_UNCOND ("=======START=========== " << GetNode() << " =================");

    Simulator::ScheduleNow(&DashClientApp::TrafficMonitor, this);

    // Time tF(Seconds (1));
    // m_bufferStateEvent = Simulator::Schedule(tF, &DashClientApp::GetBufferState, this);
}

void DashClientApp::RxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv()))
    {
        if (packet->GetSize () == 0) //EOF
        {
            break;
        }

        // For calculating throughput
        m_cumulativeSize += packet->GetSize();

        if (m_cumulativeSize >= m_lastRequestedSize)
        {
            //received the complete chunk
            //update the buffer size and initiate the next request.
            // NS_LOG_UNCOND("Client " << GetNode() << " received complete chunk at " <<
            //               Simulator::Now().GetMilliSeconds());
            m_chunkCount++;

            m_downloadDuration = Simulator::Now().GetMilliSeconds() -
                                 m_requestStartTime.GetMilliSeconds();
            m_bpsLastChunk = (m_cumulativeSize * 8) / (m_downloadDuration / 1000.0);

            // Update buffer
            m_bufferSize += m_chunkSize * 1000;

            // Scheduling
            Simulator::ScheduleNow(&DashClientApp::RequestNextChunk, this);
            
            // Start BufferModel
            if (m_chunkCount == 1)
            {
                Time tNext("100ms");
                m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::BufferModel, this);
            }

            // Monitoring
            m_statisticsEvent = Simulator::ScheduleNow(&DashClientApp::GetStatistics, this);

            m_cumulativeSize = 0;
        }
    }

}


void DashClientApp::RequestNextChunk(void)
{
    switch(m_algorithm) {
        case 0:
            Conventional();
            break;
        case 1:
            PANDA();
            break;
        case 2:
            BBA();
            break;
        case 3:
            Proposed();
            break;
    }
}

void DashClientApp::Conventional(void)
{
    // Estimating
    uint32_t bandwidth = m_bpsLastChunk;

    // Smoothing
    if (m_bandwidthAvg == 0)
        m_bandwidthAvg = bandwidth;
    m_bandwidthAvg = 0.8 * m_bandwidthAvg + 0.2 * bandwidth;

    // Quantizing
    uint32_t R_up = m_bitrate_array[0];
    uint32_t R_down = m_bitrate_array[0];

    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (m_bitrate_array[i] <= 1 * m_bandwidthAvg) // Safety margin
        {
            R_up = m_bitrate_array[i];
        }
        else
            break;
    }

    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (m_bitrate_array[i] <= 1 * m_bandwidthAvg) // Safety margin
        {
            R_down = m_bitrate_array[i];
        }
        else
            break;
    }

    if (m_oldBitrate < R_up)
        m_nextBitrate = R_up;
    else if (m_oldBitrate <= R_down)
        m_nextBitrate = m_oldBitrate;
    else
        m_nextBitrate = R_down;

    m_oldBitrate = m_nextBitrate;

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else
    {
        m_nextRequestTime = m_chunkSize * 1000 - m_downloadDuration;
        if (m_nextRequestTime < 0)
            m_nextRequestTime = 0;

        Time tNext(MilliSeconds(m_nextRequestTime));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }    
}

void DashClientApp::PANDA(void)
{
    double k = 0.56;
    double w = 300000;
    double T = std::max (m_chunkSize*1000, m_downloadDuration)/1000.0;

    // Estimating
    if (bandwidth_panda + w < m_bpsLastChunk)
        bandwidth_panda = bandwidth_panda + k * w * T;
    else
    {
        bandwidth_panda = bandwidth_panda + k * ((double)m_bpsLastChunk - bandwidth_panda) * T;
        if (bandwidth_panda < 0)
            bandwidth_panda = 0;
        NS_LOG_UNCOND("k * w * T: " << k * ((double)m_bpsLastChunk - bandwidth_panda) * T);
    }    

    // Smoothing
    if (m_bandwidthAvg == 0)
        m_bandwidthAvg = bandwidth_panda;
    m_bandwidthAvg = 0.8 * m_bandwidthAvg + 0.2 * bandwidth_panda;

    // Quantizing
    uint32_t R_up = m_bitrate_array[0];
    uint32_t R_down = m_bitrate_array[0];

    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (m_bitrate_array[i] <= 1 * m_bandwidthAvg) // Safety margin
        {
            R_up = m_bitrate_array[i];
        }
        else
            break;
    }

    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (m_bitrate_array[i] <= 1 * m_bandwidthAvg) // Safety margin
        {
            R_down = m_bitrate_array[i];
        }
        else
            break;
    }

    if (m_oldBitrate < R_up)
        m_nextBitrate = R_up;
    else if (m_oldBitrate <= R_down)
        m_nextBitrate = m_oldBitrate;
    else
        m_nextBitrate = R_down;

    m_oldBitrate = m_nextBitrate;

    // Scheduling
    if (m_bufferSize < 26000)
        m_nextRequestTime = 0;
    else
        m_nextRequestTime = 0.2 * (m_bufferSize - 26000);

    Time tNext(MilliSeconds(m_nextRequestTime));
    Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);

    NS_LOG_UNCOND("bandwidth_panda: " << bandwidth_panda << " m_nextRequestTime: " << m_nextRequestTime);
}

void DashClientApp::BBA(void)
{
    // Buffer-based Adaptation
    if (m_bufferSize < 10000)
        m_nextBitrate = m_bitrate_array[0];
    else if (m_bufferSize < 15000)
        m_nextBitrate = m_bitrate_array[1];
    else if (m_bufferSize < 20000)
        m_nextBitrate = m_bitrate_array[2];
    else if (m_bufferSize < 25000)
        m_nextBitrate = m_bitrate_array[3];
    else 
        m_nextBitrate = m_bitrate_array[4];

    m_oldBitrate = m_nextBitrate;

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE)
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else
    {
        m_nextRequestTime = m_chunkSize * 1000 - m_downloadDuration;
        if (m_nextRequestTime < 0)
            m_nextRequestTime = 0;

        Time tNext(MilliSeconds(m_nextRequestTime));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }    
}

void DashClientApp::Proposed(void)
{
    // Estimation
    int interArrivalTime = m_downloadDuration + m_nextRequestTime;

    if (aA == 0){
        vA = 0;
        aA = interArrivalTime;
    }        
    else{
        vA = vA + 0.8 * (abs(interArrivalTime - aA) - vA);
        aA = aA + 0.8 * (interArrivalTime - aA);
    }

    cA = 4*vA*vA/aA/aA;
    rho = m_chunkSize*1000/aA;

    // Expectation
    int K = ceil(MAX_BUFFER_SIZE / 1000.0 / m_chunkSize);
    gamma = cA * (K - 1) * m_chunkSize * 1000 / 4;

    // Adaptation
    if (m_oldBitrate == 0)
        m_oldBitrate = m_bitrate_array[0];

    uint32_t index = GetIndexByBitrate(m_oldBitrate);
    uint32_t lowerBitrate = (index == 0) ? m_bitrate_array[index] : m_bitrate_array[index - 1];
    uint32_t upperBitrate = (index >= m_bitrate_array.size() - 1) ? m_bitrate_array[index] : m_bitrate_array[index + 1];
    double lowerRatio = (double)m_oldBitrate / lowerBitrate;
    double upperRatio = (double)m_oldBitrate / upperBitrate;

    if (m_state == HOLD)
    {
        if ((m_bufferSize < 14000 && rho < 1) || m_bufferSize < gamma)
            m_state = DOWN;
        else if (m_bufferSize > 25000 && rho > 1 && m_bufferSize > gamma)
            m_state = UP;
    }
    else if (m_state == UP)
    {
        if (rho * upperRatio < 1 || m_bufferSize < gamma)
            m_state = HOLD;
    }
    else if (m_state == DOWN)
    {
        if (rho * lowerRatio > 1)
            m_state = HOLD;
    }

    if (m_state == HOLD)
    {
        m_nextBitrate = m_oldBitrate;
    }
    else if (m_state == UP)
    {
        m_nextBitrate = upperBitrate;
    }
    else if (m_state == DOWN)
    {
        m_nextBitrate = lowerBitrate;
    }

    m_oldBitrate = m_nextBitrate;

    // Scheduling
    if (m_bufferSize < MAX_BUFFER_SIZE || m_oldBitrate != m_bitrate_array[4])
        Simulator::ScheduleNow(&DashClientApp::SendRequest, this);
    else
    {
        m_nextRequestTime = m_chunkSize * 1000 - m_downloadDuration;
        if (m_nextRequestTime < 0)
            m_nextRequestTime = 0;

        Time tNext(MilliSeconds(m_nextRequestTime));
        Simulator::Schedule(tNext, &DashClientApp::SendRequest, this);
    }    

    NS_LOG_UNCOND("aA: " << aA << " vA: " << vA << " cA: " << cA << " gamma: " << gamma << " rho: " << rho << 
        " lowerRatio: " << lowerRatio << " upperRatio: " << upperRatio << " state: " << HasStateName[m_state]);
}

uint32_t DashClientApp::GetIndexByBitrate(uint32_t bitrate)
{
    for (uint32_t i = 0; i < m_bitrate_array.size(); i++)
    {
        if (bitrate == m_bitrate_array[i])
            return i;
    }

    return -1;
}



void DashClientApp::SendRequest(void)
{
    if (m_nextBitrate == 0)
        m_nextBitrate = m_bitrate_array[0];

    uint32_t bytesReq = (m_nextBitrate * m_chunkSize) / 8;
    // uint32_t bytesReq2 = (m_nextBitrate * m_chunkSize) / 8 / 3;
    // uint32_t bytesReq3 = (m_nextBitrate * m_chunkSize) / 8 / 3;

    Ptr<Packet> packet = Create<Packet>((uint8_t *) &bytesReq, 4);
    // Ptr<Packet> packet2 = Create<Packet>((uint8_t *) &bytesReq2, 4);
    // Ptr<Packet> packet3 = Create<Packet>((uint8_t *) &bytesReq3, 4);

    m_lastRequestedSize = bytesReq;
    m_socket->Send(packet);
    // m_socket2->Send(packet2);
    // m_socket3->Send(packet3);

    // Monitoring
    m_requestStartTime = Simulator::Now();
    // NS_LOG_UNCOND ("Client " << GetNode() << " request bitrate " << m_nextBitrate <<
    //                " at " <<
    //                Simulator::Now().GetMilliSeconds()) ;
}

void DashClientApp::BufferModel(void)
{
    if (m_running)
    {        
        if (m_bufferSize < 100)
        {
            m_bufferSize = 0;
            m_chunkCount = 0;
            Simulator::Cancel(m_bufferEvent);

            return;
        }

        m_bufferSize -= 100;
        
        Time tNext("100ms");
        m_bufferEvent = Simulator::Schedule(tNext, &DashClientApp::BufferModel,
                                            this);
    }
}

void DashClientApp::GetStatistics()
{
    NS_LOG_UNCOND ("=======START=========== " << GetNode() << " =================");
    NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
                   " Estimated: " << m_bpsLastChunk <<
                   " Averaged: " << m_bandwidthAvg <<
                   " VideoRate: " << m_nextBitrate <<
                   " Buffer: " << m_bufferSize <<
                   " DownloadDuration: " << m_downloadDuration);
    //NS_LOG_UNCOND ("=================== " << GetNode() << " ========END=========");
}

void DashClientApp::TrafficMonitor()
{
    // Calculate TCP Throughput
    // g_throughput = (g_throughput * 8) / (500 / 1000.0);
    // NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
    //            " Throughput: " << g_throughput);
    // g_throughput = 0;

    // Time tNext("500ms");
    // Simulator::Schedule(tNext, &DashClientApp::TrafficMonitor, this);
}

void DashClientApp::GetBufferState()
{
    // Calculate Throughput (interval: 100ms)
    // m_throughput = m_cumulativeSize * 8 / 1 / 1000;
    // m_cumulativeSize = 0;

    // NS_LOG_UNCOND ("=======BUFFER========== " << GetNode() <<
    //                " =================");
    // NS_LOG_UNCOND ("Time: " << Simulator::Now ().GetMilliSeconds () <<
    //                " BufferSize: " << m_bufferLength <<
    //                " HASThroughput: " << m_throughput <<
    //                " EstimatedBW: " << m_bpsAvg / 1000.0 <<
    //                " VideoLevel: " << (m_lastRequestedSize * 8) / m_chunkSize / 1000 <<
    //                " State: " << HasStateName[m_state] <<
    //                " lambda: " << lambda <<
    //                " rhoAvg: " << rho );
    //NS_LOG_UNCOND ("=================== " << GetNode() << " ========END=========");


    Time tNext (Seconds (1));
    m_bufferStateEvent = Simulator::Schedule (tNext, &DashClientApp::GetBufferState, this);
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
    LogComponentEnable("DashApplication", LOG_LEVEL_ALL);
    //LogComponentEnable("TcpNewReno", LOG_LEVEL_LOGIC);
    //LogComponentEnable("TcpSocketBase", LOG_LEVEL_LOGIC);
    //LogComponentEnable("Queue", LOG_LEVEL_LOGIC);
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpHighSpeed"));

    NodeContainer node;
    node.Create(8);

    InternetStackHelper internet;
    internet.Install(node);

    PointToPointHelper link;
    link.SetDeviceAttribute("DataRate", DataRateValue(DataRate("5Mb/s")));
    link.SetDeviceAttribute("Mtu", UintegerValue(1500));
    link.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

    PointToPointHelper bottleNeck;
    bottleNeck.SetDeviceAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    bottleNeck.SetDeviceAttribute("Mtu", UintegerValue(1500));
    bottleNeck.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

    NetDeviceContainer d0d5 = link.Install(node.Get(0), node.Get(5));
    NetDeviceContainer d5d6 = link.Install(node.Get(5), node.Get(6));
    NetDeviceContainer d6d1 = link.Install(node.Get(6), node.Get(1));
    NetDeviceContainer d6d2 = link.Install(node.Get(6), node.Get(2));
    NetDeviceContainer d2d3 = bottleNeck.Install(node.Get(2), node.Get(3));
    NetDeviceContainer d3d4 = link.Install(node.Get(3), node.Get(4));
    NetDeviceContainer d7d5 = link.Install(node.Get(7), node.Get(5));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i5 = address.Assign(d0d5);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i7i5 = address.Assign(d7d5);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i5i6 = address.Assign(d5d6);
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i6i1 = address.Assign(d6d1);
    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i6i2 = address.Assign(d6d2);
    address.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i3 = address.Assign(d2d3);
    address.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i4 = address.Assign(d3d4);

    setPos(node.Get(0), 30, 80, 0);
    setPos(node.Get(7), 50, 80, 0);
    setPos(node.Get(5), 40, 70, 0);
    setPos(node.Get(6), 40, 60, 0);
    setPos(node.Get(1), 30, 50, 0);
    setPos(node.Get(2), 50, 50, 0);
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

    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue(10));

    // Install mobility model
    MobilityHelper mobility;
    
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(20.0, 50.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator", 
                                "X", StringValue("10.0"),
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
    address.SetBase("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer i1ap = address.Assign(d1ap);

   /* NetDeviceContainer d4ap = link.Install(node.Get(4), wifiStaNode.Get(0));
    address.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer i4ap = address.Assign(d4ap);*/

   /* // create point to point link helpers
    PointToPointHelper bottleNeck;
    bottleNeck.SetDeviceAttribute("DataRate", StringValue("4Mbps"));
    bottleNeck.SetChannelAttribute("Delay", StringValue("8ms"));
    //bottleNeck.SetQueue("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"));
    //bottleNeck.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue (1));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointDumbbellHelper dB(1, pointToPointLeaf, 4, pointToPointLeaf,
                                  bottleNeck);

    // install stack
    InternetStackHelper stack;
    dB.InstallStack(stack);

    // assign IP addresses
    dB.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    setPos(dB.GetLeft(0), 50, 70, 0);
    setPos(dB.GetRight(0), 70, 90, 0);
    setPos(dB.GetRight(1), 70, 80, 0);
    setPos(dB.GetRight(2), 70, 60, 0);
    setPos(dB.GetRight(3), 70, 50, 0);*/

    uint16_t serverPort = 8080;

    // Scenario 1
    // OnOffHelper crossTrafficSrc1("ns3::UdpSocketFactory", InetSocketAddress (dB.GetRightIpv4Address(0), serverPort));
    // crossTrafficSrc1.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
    // crossTrafficSrc1.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("1000kb/s")));
    // crossTrafficSrc1.SetAttribute("PacketSize", UintegerValue (512));
    // ApplicationContainer srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(100.0));
    // srcApp1.Stop(Seconds(400.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(400.0));
    // srcApp1.Stop(Seconds(600.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("3000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(600.0));
    // srcApp1.Stop(Seconds(700.0));

    // OnOffHelper crossTrafficSrc2("ns3::UdpSocketFactory", InetSocketAddress (dB.GetRightIpv4Address(1), serverPort));
    // crossTrafficSrc2.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    // crossTrafficSrc2.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // crossTrafficSrc2.SetAttribute("DataRate", DataRateValue (DataRate("1000kb/s")));
    // crossTrafficSrc2.SetAttribute("PacketSize", UintegerValue (512));
    // ApplicationContainer srcApp2 = crossTrafficSrc2.Install(dB.GetLeft(1));
    // srcApp2.Start(Seconds(200.0));
    // srcApp2.Stop(Seconds(300.0));
    // crossTrafficSrc2.SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s"))); // 2Mbps
    // srcApp2 = crossTrafficSrc2.Install(dB.GetLeft(1));
    // srcApp2.Start(Seconds(300.0));
    // srcApp2.Stop(Seconds(400.0));
    // crossTrafficSrc2.SetAttribute("DataRate", DataRateValue (DataRate("1000kb/s"))); // 2Mbps
    // srcApp2 = crossTrafficSrc2.Install(dB.GetLeft(1));
    // srcApp2.Start(Seconds(400.0));
    // srcApp2.Stop(Seconds(500.0));  

    // Scenario 2
    // OnOffHelper crossTrafficSrc1("ns3::UdpSocketFactory", InetSocketAddress (dB.GetRightIpv4Address(0), serverPort));
    // crossTrafficSrc1.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    // crossTrafficSrc1.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("1000kb/s")));
    // crossTrafficSrc1.SetAttribute("PacketSize", UintegerValue (512));
    // ApplicationContainer srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(100.0));
    // srcApp1.Stop(Seconds(200.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(200.0));
    // srcApp1.Stop(Seconds(300.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("3000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(300.0));
    // srcApp1.Stop(Seconds(400.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(400.0));
    // srcApp1.Stop(Seconds(500.0));
    // crossTrafficSrc1.SetAttribute("DataRate", DataRateValue (DataRate("1000kb/s")));
    // srcApp1 = crossTrafficSrc1.Install(dB.GetLeft(0));
    // srcApp1.Start(Seconds(500.0));
    // srcApp1.Stop(Seconds(600.0));

    // Scenario 4 - FTP
    // Ptr<BulkSendApplication> crossTrafficSrc = CreateObject<BulkSendApplication> ();
    // AddressValue remoteAddress(
    //     InetSocketAddress(dB.GetRightIpv4Address(0), serverPort));
    // crossTrafficSrc->SetAttribute("Remote", remoteAddress);

    // ApplicationContainer srcApp;
    // dB.GetLeft(0)->AddApplication(crossTrafficSrc);
    // srcApp.Add(ApplicationContainer(crossTrafficSrc));
    // srcApp.Start(Seconds(0));
    // srcApp.Stop(Seconds(700.0));

    // Address sinkLocalAddress(
    //     InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    // PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    // ApplicationContainer sinkApps;
    // sinkApps.Add(packetSinkHelper.Install(dB.GetRight(0)));
    // //sinkApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback (&TrafficTrace));
    // sinkApps.Start(Seconds(0));
    // sinkApps.Stop(Seconds(700.0));

    // Scenario 4 - Exponential On/Off
    // Ptr<OnOffApplication> crossTrafficSrc = CreateObject<OnOffApplication> ();
    // AddressValue remoteAddress(
    //     InetSocketAddress(dB.GetRightIpv4Address(0), serverPort));
    // crossTrafficSrc->SetAttribute("Remote", remoteAddress);
    // crossTrafficSrc->SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s")));
    // crossTrafficSrc->SetAttribute("OnTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.8]"));
    // crossTrafficSrc->SetAttribute("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.2]"));

    // Scenario 4 - Pareto On/Off
   /* Ptr<OnOffApplication> crossTrafficSrc = CreateObject<OnOffApplication> ();
    AddressValue remoteAddress(
        InetSocketAddress(dB.GetRightIpv4Address(0), serverPort));
    crossTrafficSrc->SetAttribute("Remote", remoteAddress);
    crossTrafficSrc->SetAttribute("DataRate", DataRateValue (DataRate("2000kb/s")));
    crossTrafficSrc->SetAttribute("OnTime", StringValue ("ns3::ParetoRandomVariable[Mean=0.8]"));
    crossTrafficSrc->SetAttribute("OffTime", StringValue ("ns3::ParetoRandomVariable[Mean=0.2]"));

    ApplicationContainer srcApp;
    dB.GetLeft(0)->AddApplication(crossTrafficSrc);
    srcApp.Add(ApplicationContainer(crossTrafficSrc));
    srcApp.Start(Seconds(0));
    srcApp.Stop(Seconds(700.0));

    Address sinkLocalAddress(
        InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApps;
    sinkApps.Add(packetSinkHelper.Install(dB.GetRight(0)));
    //sinkApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback (&TrafficTrace));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(Seconds(700.0));*/

    /*// DashApp
    Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    serverApp1->Setup(bindAddress1, 512);
    dB.GetLeft(0)->AddApplication(serverApp1);
    serverApp1->SetStartTime(Seconds(0.0));
    serverApp1->SetStopTime(Seconds(50.0));

    Address serverAddress1(
        InetSocketAddress(dB.GetLeftIpv4Address(0), serverPort));
    Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
    clientApp1->Setup(serverAddress1, 2, 512, "Conventional");
    dB.GetRight(0)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.0));
    clientApp1->SetStopTime(Seconds(50.0));

    Ptr<DashClientApp> clientApp2 = CreateObject<DashClientApp>();
    clientApp2->Setup(serverAddress1, 2, 512, "Conventional");
    dB.GetRight(1)->AddApplication(clientApp2);
    clientApp2->SetStartTime(Seconds(0.0));
    clientApp2->SetStopTime(Seconds(50.0));*/

   /* Ptr<DashClientApp> clientApp3 = CreateObject<DashClientApp>();
    clientApp3->Setup(serverAddress1, 2, 512, "Conventional");
    dB.GetRight(2)->AddApplication(clientApp3);
    clientApp3->SetStartTime(Seconds(0.0));
    clientApp3->SetStopTime(Seconds(50.0));*/

    /*Ptr<DashClientApp> clientApp4 = CreateObject<DashClientApp>();
    clientApp4->Setup(serverAddress1, 2, 512, "Conventional");
    dB.GetRight(3)->AddApplication(clientApp4);
    clientApp4->SetStartTime(Seconds(0.0));
    clientApp4->SetStopTime(Seconds(50.0));*/
    //clientApp1->SetStopTime(Seconds(400.0));    // Scenario 3

    // Scenario 3
    // Address bindAddress2(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    // Ptr<DashServerApp> serverApp2 = CreateObject<DashServerApp>();
    // serverApp2->Setup(bindAddress2, 512);
    // dB.GetLeft(3)->AddApplication(serverApp2);
    // serverApp2->SetStartTime(Seconds(0.0));
    // serverApp2->SetStopTime(Seconds(700.0));

    // Address bindAddress3(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    // Ptr<DashServerApp> serverApp3 = CreateObject<DashServerApp>();
    // serverApp3->Setup(bindAddress3, 512);
    // dB.GetLeft(4)->AddApplication(serverApp3);
    // serverApp3->SetStartTime(Seconds(0.0));
    // serverApp3->SetStopTime(Seconds(700.0));

    // Address bindAddress4(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    // Ptr<DashServerApp> serverApp4 = CreateObject<DashServerApp>();
    // serverApp4->Setup(bindAddress4, 512);
    // dB.GetLeft(5)->AddApplication(serverApp4);
    // serverApp4->SetStartTime(Seconds(0.0));
    // serverApp4->SetStopTime(Seconds(700.0));

    // Address serverAddress2(
    //     InetSocketAddress(dB.GetLeftIpv4Address(3), serverPort));
    // Ptr<DashClientApp> clientApp2 = CreateObject<DashClientApp>();
    // clientApp2->Setup(serverAddress2, 2, 512, argv[1]);
    // dB.GetRight(3)->AddApplication(clientApp2);
    // clientApp2->SetStartTime(Seconds(100.0));
    // clientApp2->SetStopTime(Seconds(500.0));

    // Address serverAddress3(
    //     InetSocketAddress(dB.GetLeftIpv4Address(4), serverPort));
    // Ptr<DashClientApp> clientApp3 = CreateObject<DashClientApp>();
    // clientApp3->Setup(serverAddress3, 2, 512, argv[1]);
    // dB.GetRight(4)->AddApplication(clientApp3);
    // clientApp3->SetStartTime(Seconds(200.0));
    // clientApp3->SetStopTime(Seconds(600.0));    

    // Address serverAddress4(
    //     InetSocketAddress(dB.GetLeftIpv4Address(5), serverPort));
    // Ptr<DashClientApp> clientApp4 = CreateObject<DashClientApp>();
    // clientApp4->Setup(serverAddress4, 2, 512, argv[1]);
    // dB.GetRight(5)->AddApplication(clientApp4);
    // clientApp4->SetStartTime(Seconds(300.0));
    // clientApp4->SetStopTime(Seconds(700.0));

    // DASH server
    Address bindAddress1(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp1 = CreateObject<DashServerApp>();
    serverApp1->Setup(bindAddress1, 512);
    node.Get(0)->AddApplication(serverApp1);
    serverApp1->SetStartTime(Seconds(0.0));
    serverApp1->SetStopTime(Seconds(20.0));

    Address bindAddress2(InetSocketAddress(Ipv4Address::GetAny(), serverPort));
    Ptr<DashServerApp> serverApp2 = CreateObject<DashServerApp>();
    serverApp2->Setup(bindAddress2, 512);
    node.Get(7)->AddApplication(serverApp2);
    serverApp2->SetStartTime(Seconds(0.0));
    serverApp2->SetStopTime(Seconds(20.0));
    
    // DASH client
    Address serverAddress1(InetSocketAddress(i0i5.GetAddress(0), serverPort));
    Address serverAddress2(InetSocketAddress(i7i5.GetAddress(0), serverPort));
    Ptr<DashClientApp> clientApp1 = CreateObject<DashClientApp>();
    clientApp1->Setup(serverAddress1, serverAddress2, 2, 512, "Conventional");
    wifiStaNode.Get(0)->AddApplication(clientApp1);
    clientApp1->SetStartTime(Seconds(0.0));
    clientApp1->SetStopTime(Seconds(20.0));

    /*Ptr<DashClientApp> clientApp2 = CreateObject<DashClientApp>();
    clientApp2->Setup(serverAddress1, serverAddress2, 2, 512, "Conventional");
    node.Get(4)->AddApplication(clientApp2);
    clientApp2->SetStartTime(Seconds(0.0));
    clientApp2->SetStopTime(Seconds(20.0));*/

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(20.0));

    AnimationInterface anim("Multiconnection.xml");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
