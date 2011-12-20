#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <math.h>
#include <arpa/inet.h>

#include "stk/Thread.h"
#include "osc/OscPacketListener.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#include "Engine.h"
#include "Widget.h"

/**
* Represents the remote host information 
*/ 
class Peer
{
  public:
    Peer(const IpEndpointName&);
    Peer(const char*, const int);
    ~Peer();

    const IpEndpointName& getLocation();

    void setMousePosition(float, float);
    const Point2D& getMousePosition();
    void setMouseDown(bool);
    void setDisplayName(std::string displayName);
    const bool getMouseDown();
    void draw();

    void sendMessage(const osc::OutboundPacketStream&);

  private:
    IpEndpointName m_location;
    UdpTransmitSocket* m_socket;

    Point2D m_mousePosition;
    Text *m_mousePositionText;
    bool m_mouseDown;
    Spiral *m_circle;
};

/**
* The class deals with all the network related message passing and processing
*/
class Network : public osc::OscPacketListener
{
  public:
    typedef std::map<IpEndpointName, Peer*> PeerMap;
    typedef std::pair<IpEndpointName, Peer*> PeerData;

    typedef void(Network::*HandlerFunction)(const osc::ReceivedMessage&, const IpEndpointName&);
    typedef std::map<std::string, HandlerFunction> HandlerMap;
    typedef std::pair<std::string, HandlerFunction> HandlerData;

    Network();
    ~Network();
    void listen(int);

    void addPeer(Peer&);
    PeerMap* getPeers();

    int getPort();

    void setEngine(Engine*);

    void sendPeerUpMessage();
    void sendPeerDownMessage();
    void sendPeerMessage(const std::string);
    void sendPeerTextMessage(const std::string);
    void sendMousePosition(const float x, const float y, const bool down);
    void sendPlucker(const Track*);

    void sendObjectMessage(const Widget*, bool = false);

    void sendRoundPadTextMessage(const RoundPad*);

  protected:
    static void *listen(void*);

    virtual void ProcessMessage(const osc::ReceivedMessage&, const IpEndpointName&);

    void broadcast(const osc::OutboundPacketStream&);

    void handlePeerUpMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handlePeerDownMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handlePeerTextMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectPadMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectPadTextMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handlePluckerMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectSpiralMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectLineMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectStringMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectDeleteMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleObjectQueryMessage(const osc::ReceivedMessage&, const IpEndpointName&);
    void handleMousePositionMessage(const osc::ReceivedMessage&, const IpEndpointName&);

    void rescueOrphans();

  private:
    PeerMap m_peers;
    HandlerMap m_handlers;

    stk::Thread m_thread;
    int m_port;

    WidgetMap m_orphans;

    Engine* m_engine;
};

#endif
