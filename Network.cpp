#include "include/Network.h"

// ===================
// Peer implementation
// ===================

Peer::Peer(const IpEndpointName& endpoint) :
  m_location(endpoint),
  m_socket(new UdpTransmitSocket(m_location)),
  m_mousePosition(Point2D(-100, -100)),
  m_mousePositionText(new Text(Point2D(-100, -100), "")),
  m_mouseDown(false),
  m_circle(new Spiral(m_mousePosition, 360, 5, 0, 5))
{
  m_circle->setColor(Color(1, 0, 1));
  m_mousePositionText->setColor(Color(1, 0, 1));
}

Peer::Peer(const char* host, const int port) :
  m_location(IpEndpointName(host, port)),
  m_socket(new UdpTransmitSocket(m_location)),
  m_mousePosition(Point2D(-100, -100)),
  m_mousePositionText(new Text(Point2D(-100, -100), "")),
  m_mouseDown(false),
  m_circle(new Spiral(m_mousePosition, 360, 5, 0, 5))
{
  m_circle->setColor(Color(1, 0, 1));
  m_mousePositionText->setColor(Color(1, 0, 1));
}

Peer::~Peer()
{
  delete m_socket;
  delete m_circle;
  delete m_mousePositionText;
}

void Peer::sendMessage(const osc::OutboundPacketStream& msg)
{
  m_socket->Send(msg.Data(), msg.Size());
}

const IpEndpointName& Peer::getLocation()
{
  return m_location;
}

void Peer::setMousePosition(float x, float y)
{
  m_mousePosition.x = x;
  m_mousePosition.y = y;
}

const Point2D& Peer::getMousePosition()
{
  return m_mousePosition;
}

void Peer::setDisplayName(std::string displayName)
{
  m_mousePositionText->setText(displayName);
}

void Peer::setMouseDown(bool down)
{
  m_mouseDown = down;
}

const bool Peer::getMouseDown()
{
  return m_mouseDown;
}

void Peer::draw()
{
  m_circle->setCenter(m_mousePosition);
  m_circle->setFilled(false);
  m_circle->draw();
  if (m_mouseDown) {
    m_circle->setFilled(true);
    m_circle->draw();
  }
  if (m_mousePositionText)
  {
    m_mousePositionText->setPos(Point2D(m_mousePosition.x + 10, m_mousePosition.y + 5));
    m_mousePositionText->draw();
  }
}

// ======================
// Network implementation
// ======================

Network::Network() : m_engine(NULL)
{
  m_handlers.insert(HandlerData("/network/peer/up",     &Network::handlePeerUpMessage));
  m_handlers.insert(HandlerData("/network/peer/down",   &Network::handlePeerDownMessage));
  m_handlers.insert(HandlerData("/network/peer/text",   &Network::handlePeerTextMessage));

  m_handlers.insert(HandlerData("/mouse/position",      &Network::handleMousePositionMessage));

  m_handlers.insert(HandlerData("/object/plucker",      &Network::handlePluckerMessage));
  m_handlers.insert(HandlerData("/object/pad",          &Network::handleObjectPadMessage));
  m_handlers.insert(HandlerData("/object/pad/text",     &Network::handleObjectPadTextMessage));
  m_handlers.insert(HandlerData("/object/track/spiral", &Network::handleObjectSpiralMessage));
  m_handlers.insert(HandlerData("/object/track/line",   &Network::handleObjectLineMessage));
  m_handlers.insert(HandlerData("/object/track/string", &Network::handleObjectStringMessage));
  m_handlers.insert(HandlerData("/object/delete",       &Network::handleObjectDeleteMessage));
  m_handlers.insert(HandlerData("/object/query",        &Network::handleObjectQueryMessage));
}

Network::~Network()
{
  sendPeerDownMessage();
}

int Network::getPort()
{
  return m_port;
}

void Network::rescueOrphans()
{
  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit, oit;
  for (oit = m_orphans.begin(); oit != m_orphans.end(); oit++) {
    if ((wit = widgets->find(oit->first)) != widgets->end()) { // parent found?!!
      // De-orphan it
      wit->second->addChild(oit->second);
      if (dynamic_cast<SpiralTrack*>(oit->second))
        ((SpiralTrack*)oit->second)->setCenter(((RoundPad*)wit->second)->getCenter());
      else if (dynamic_cast<String*>(oit->second))
        ((String*)oit->second)->setPadRadius(((RoundPad*)wit->second)->getRadius());
      wit = widgets->insert(WidgetData(oit->second->getUuid(), oit->second)).first;
      m_orphans.erase(oit);

      // Tell the world about the now ex-orphan
      char buffer[1024];
      osc::OutboundPacketStream ps(buffer, 1024);
      wit->second->toOutboundPacketStream(ps);
      broadcast(ps);
    }
  }
}

Network::PeerMap* Network::getPeers()
{
  return &m_peers;
}

void Network::setEngine(Engine* engine)
{
  m_engine = engine;
}

void Network::listen(int port)
{
  m_port = port;
  if (!m_thread.start(listen, this)) {
    std::cerr << "Error when creating listener thread!" << std::endl;
    exit(1);
  }

  usleep(1000000); // listener thread grace period
  sendPeerUpMessage();
}

void* Network::listen(void* network)
{
  int port = ((Network*)network)->getPort();
  std::cerr << "Starting to listen on port: " << port << std::endl;
  UdpListeningReceiveSocket s(IpEndpointName(IpEndpointName::ANY_ADDRESS, port), (Network*)network);
  s.Run();
  std::cerr << "Exiting listening thread!" << std::endl;
}

void Network::broadcast(const osc::OutboundPacketStream& stream)
{
  for (PeerMap::iterator pit = m_peers.begin(); pit != m_peers.end(); pit++)
    pit->second->sendMessage(stream);
}

void Network::addPeer(Peer& peer)
{
  if (m_peers.find(peer.getLocation()) == m_peers.end())
    m_peers.insert(PeerData(peer.getLocation(), &peer));
}

void Network::ProcessMessage(const osc::ReceivedMessage& m,
                             const IpEndpointName& remoteEndpoint)
{
  try {
    std::string address = m.AddressPattern();
    if (address != "/mouse/position") // too noisy!
      std::cerr << "Port " << m_port << " >>> Received '" << address << "' message with arguments: ";
    (this->*(m_handlers[address]))(m, remoteEndpoint);
  } catch(osc::Exception& e) {
    std::cerr << "error while parsing message: "
              << m.AddressPattern() << ": " << e.what() << std::endl;
  }

  rescueOrphans();
}

void Network::sendPeerUpMessage()
{
  sendPeerMessage("up");
}

void Network::sendPeerDownMessage()
{
  sendPeerMessage("down");
}

void Network::sendPeerMessage(const std::string type)
{
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);
  ps << osc::BeginMessage(("/network/peer/" + type).c_str()) << (osc::int64)0 << (osc::int32)m_port << osc::EndMessage;
  broadcast(ps);
}

void Network::sendPeerTextMessage(const std::string msg)
{
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);
  ps << osc::BeginMessage("/network/peer/text")
     << (osc::int64)0
     << (osc::int32)m_port
     << osc::Symbol(msg.c_str())
     << osc::EndMessage;
  broadcast(ps);
}

void Network::sendMousePosition(const float x, const float y, const bool down)
{
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);
  ps << osc::BeginMessage("/mouse/position") << m_port << x << y << down << osc::EndMessage;
  //std::cerr << m_port << ", " << x << ", " << y << std::endl;
  broadcast(ps);
}

void Network::handlePeerUpMessage(const osc::ReceivedMessage& m,
                                  const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::int32 port;
  osc::int64 address;
  m.ArgumentStream() >> address >> port >> osc::EndMessage;
  if ((long)address == 0)
    address = remoteEndpoint.address;
  char ipStr[INET_ADDRSTRLEN];
  uint32_t endian = htonl(address);
  std::cerr << inet_ntop(AF_INET, &(endian), ipStr, INET_ADDRSTRLEN) << ", " << (int)port << std::endl;

  // Check for known peer, add & broadcast if unknown
  IpEndpointName peerEndpoint((unsigned long)address, (int)port);
  PeerMap::iterator peer = m_peers.find(peerEndpoint);
  if (peer == m_peers.end()) {
    PeerData peerData(peerEndpoint, new Peer(peerEndpoint));

    // Tell the world
    for (PeerMap::iterator pit = m_peers.begin(); pit != m_peers.end(); pit++) {
      std::cerr << "sending " << (int)port << " to " << pit->first.port << std::endl;
      ps.Clear();
      ps << osc::BeginMessage("/network/peer/up") << address << port << osc::EndMessage;
      pit->second->sendMessage(ps);

      ps.Clear();
      ps << osc::BeginMessage("/network/peer/up")
         << (osc::int64)(pit->first.address)
         << (osc::int32)(pit->first.port)
         << osc::EndMessage;
      peerData.second->sendMessage(ps);
    }

    peer = m_peers.insert(peerData).first;
    sendPeerUpMessage();

    // Tell our new friend everything we know
    /*
     *  This doesn't work terribly well, so it's disabled for now
     *
    WidgetMap* widgets = Widget::getAll();
    for (WidgetMap::iterator wit = widgets->begin(); wit != widgets->end(); wit++) {
      wit->second->toOutboundPacketStream(ps);
      peer->second->sendMessage(ps);
    }
    */
  }
}

void Network::handlePeerDownMessage(const osc::ReceivedMessage& m,
                                    const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::int32 port;
  osc::int64 address;
  m.ArgumentStream() >> address >> port >> osc::EndMessage;
  if (address == 0)
    address = remoteEndpoint.address;
  char ipStr[INET_ADDRSTRLEN];
  uint32_t endian = htonl(address);
  std::cerr << inet_ntop(AF_INET, &(endian), ipStr, INET_ADDRSTRLEN) << ", " << (int)port << std::endl;

  // Check for known peer, add & broadcast if unknown
  IpEndpointName peerEndpoint((unsigned long)address, (int)port);
  PeerMap::iterator peer = m_peers.find(peerEndpoint);
  if (peer != m_peers.end()) {
    m_peers.erase(peer);

    // Tell the world
    ps << osc::BeginMessage("/network/peer/down") << address << port << osc::EndMessage;
    broadcast(ps);
  }
}

void Network::handlePeerTextMessage(const osc::ReceivedMessage& m,
                                    const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol text;
  osc::int64 address;
  osc::int32 port;
  m.ArgumentStream() >> address >> port >> text >> osc::EndMessage;

  if (address == 0)
    address = remoteEndpoint.address;

  char ipStr[INET_ADDRSTRLEN];
  uint32_t endian = htonl(address);
  std::cerr << inet_ntop(AF_INET, &(endian), ipStr, INET_ADDRSTRLEN) << ": " << text << std::endl;

  // Check for known peer, add & broadcast if unknown
  IpEndpointName peerEndpoint((unsigned long)address, (int)port);
  PeerMap::iterator peer = m_peers.find(peerEndpoint);
  if (peer != m_peers.end())
    peer->second->setDisplayName(std::string(text));
}

void Network::sendObjectMessage(const Widget* object, bool remove)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  if (remove) {
    std::cerr << object->getUuid() << std::endl;
    ps << osc::BeginMessage("/object/delete")
       << osc::Symbol(object->getUuid())
       << osc::EndMessage;
  } else
    object->toOutboundPacketStream(ps);

  broadcast(ps);
}

void Network::sendRoundPadTextMessage(const RoundPad* pad)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  ps << osc::BeginMessage("/object/pad/text")
     << osc::Symbol(pad->getUuid())
     << osc::Symbol(pad->getCommentText()->getText().c_str())
     << osc::EndMessage;

  broadcast(ps);
}

void Network::sendPlucker(const Track* track)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  ps << osc::BeginMessage("/object/plucker")
     << osc::Symbol(track->getUuid())
     << osc::EndMessage;

  broadcast(ps);
}

void Network::handleObjectPadMessage(const osc::ReceivedMessage& m,
                                     const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  float x, y, radius;
  osc::Symbol uuid;
  m.ArgumentStream() >> uuid >> x >> y >> radius >> osc::EndMessage;
  std::cerr << uuid << ", " << x << ", " << y << ", " << radius << std::endl;

  // Check for known pad, add & broadcast if unknown
  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  if (wit == widgets->end()) {
    RoundPad* newPad = new RoundPad(Point2D(x, y), radius);
    newPad->setUuid(uuid);
    widgets->insert(WidgetData(std::string(uuid), newPad));
    m_engine->addChild(newPad);

    // Tell the world
    newPad->toOutboundPacketStream(ps);
    broadcast(ps);
  }
}

void Network::handleObjectPadTextMessage(const osc::ReceivedMessage& m,
                                         const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol uuid, text;
  m.ArgumentStream() >> uuid >> text >> osc::EndMessage;
  std::cerr << uuid << ", " << text << std::endl;

  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  if (wit != widgets->end() && dynamic_cast<RoundPad*>(wit->second))
    ((RoundPad*)wit->second)->setCommentText(std::string(text));
}

void Network::handlePluckerMessage(const osc::ReceivedMessage& m,
                                   const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol uuid;
  m.ArgumentStream() >> uuid >> osc::EndMessage;
  std::cerr << uuid << std::endl;

  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  if (wit != widgets->end() && dynamic_cast<Track*>(wit->second))
    ((Track*)wit->second)->addPlucker();
}

void Network::handleObjectSpiralMessage(const osc::ReceivedMessage& m,
                                        const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol uuid, padUuid;
  float startAngle, startRadius, endAngle, endRadius;
  m.ArgumentStream() >> uuid >> padUuid >> startAngle >> startRadius
                                        >> endAngle >> endRadius >> osc::EndMessage;
  std::cerr << uuid << ", " << padUuid << ", "
            << startAngle << ", " << startRadius << ", "
            << endAngle << ", " << endRadius << std::endl;

  // Check for known arc, add & broadcast if unknown
  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  if (wit == widgets->end()) {
    wit = m_orphans.find(std::string(uuid));
    if (wit == m_orphans.end()) {
      SpiralTrack* newSpiral = NULL;

      // Search for pad
      wit = widgets->find(std::string(padUuid));
      if (wit != widgets->end()) {
        Point2D center = ((RoundPad*)wit->second)->getCenter();
        Point2D startPoint = Spiral::getPointFromRadius(center, startRadius, startAngle);
        Point2D endPoint = Spiral::getPointFromRadius(center, endRadius, endAngle);
        Joint *endJoint = NULL, *startJoint = NULL;
        std::vector<Widget*>* children = wit->second->getChildren();

        for (int i = children->size() - 1; i >= 0; i --) {
          Track *track = dynamic_cast<Track *>(children->at(i));
          if (track) {
            Joint *joint = track->getJoint1();
            if (!startJoint && joint && joint->hitTest(startPoint.x, startPoint.y))
              startJoint = joint;
            else if (!endJoint && joint && joint->hitTest(endPoint.x, endPoint.y))
              endJoint = joint;

            joint = track->getJoint2();
            if (!startJoint && joint && joint->hitTest(startPoint.x, startPoint.y))
              startJoint = joint;
            else if (!endJoint && joint && joint->hitTest(endPoint.x, endPoint.y))
              endJoint = joint;
          }
        }

        newSpiral = new SpiralTrack(center, startAngle, startRadius,
                                    endAngle, endRadius, startJoint, endJoint);
        newSpiral->setUuid(uuid);
        newSpiral->getSpiral()->setLineWidth(3);
        newSpiral->getJoint1()->setParentRoundPad(((RoundPad*)wit->second));
        newSpiral->getJoint2()->setParentRoundPad(((RoundPad*)wit->second));

        wit->second->addChild(newSpiral);
        widgets->insert(WidgetData(std::string(uuid), newSpiral));

        // Tell the world
        newSpiral->toOutboundPacketStream(ps);
        broadcast(ps);
      } else {// orphan if we don't know about its pad yet
        newSpiral = new SpiralTrack(Point2D(0, 0), startAngle, startRadius,
                                    endAngle, endRadius, NULL, NULL);
        newSpiral->setUuid(uuid);
        m_orphans.insert(WidgetData(std::string(padUuid), newSpiral));
      }
    }
  }
}

void Network::handleObjectLineMessage(const osc::ReceivedMessage& m,
                                      const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol uuid, padUuid;
  float startX, startY, endX, endY;
  m.ArgumentStream() >> uuid >> padUuid >> startX >> startY
                                        >> endX >> endY >> osc::EndMessage;
  std::cerr << uuid << ", " << padUuid << ", "
            << startX << ", " << startY << ", "
            << endX << ", " << endY << std::endl;

  // Check for known arc, add & broadcast if unknown
  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  if (wit == widgets->end()) {
    wit = m_orphans.find(std::string(uuid));
    if (wit == m_orphans.end()) {
      LineTrack* newLine = NULL;

      // Search for pad
      wit = widgets->find(std::string(padUuid));
      if (wit != widgets->end()) {
        Joint *endJoint = NULL, *startJoint = NULL;
        for (WidgetMap::iterator ptr = widgets->begin(); ptr != widgets->end(); ptr++) {
          Track *track = dynamic_cast<Track *>(ptr->second);
          if (track) {
            Joint *joint = track->getJoint1();
            if (!startJoint && joint && joint->hitTest(startX, startY))
              startJoint = joint;
            else if (!endJoint && joint && joint->hitTest(endX, endY))
              endJoint = joint;

            joint = track->getJoint2();
            if (!startJoint && joint && joint->hitTest(startX, startY))
              startJoint = joint;
            else if (!endJoint && joint && joint->hitTest(endX, endY))
              endJoint = joint;
          }
        }

        LineTrack* newLine = new LineTrack(Point2D(startX, startY), Point2D(endX, endY), startJoint, endJoint);
        newLine->setUuid(uuid);
        newLine->getLine()->setLineWidth(3);

        wit->second->addChild(newLine);
        widgets->insert(WidgetData(std::string(uuid), newLine));

        // Tell the world
        newLine->toOutboundPacketStream(ps);
        broadcast(ps);
      } else {// orphan if we don't know about its pad yet
        newLine = new LineTrack(Point2D(startX, startY), Point2D(endX, endY), NULL, NULL);
        newLine->setUuid(uuid);
        m_orphans.insert(WidgetData(std::string(padUuid), newLine));
      }
    }
  }
}

void Network::handleObjectStringMessage(const osc::ReceivedMessage& m,
                                        const IpEndpointName& remoteEndpoint)
{
  // Utility data structures
  char buffer[1024];
  osc::OutboundPacketStream ps(buffer, 1024);

  // Parse OSC message
  osc::Symbol uuid, padUuid;
  float startX, startY, endX, endY;
  m.ArgumentStream() >> uuid >> padUuid >> startX >> startY
                                        >> endX >> endY >> osc::EndMessage;
  std::cerr << uuid << ", " << padUuid << ", "
            << startX << ", " << startY << ", "
            << endX << ", " << endY << std::endl;

  // Check for known arc, add & broadcast if unknown
  bool unlocked = false;
  SoundSource::lockGlobals();

  SoundSourceMap* soundSources = SoundSource::getAllForEngine();
  WidgetMap* widgets = Widget::getAll();
  if (soundSources->find(std::string(uuid)) == soundSources->end()) {
    WidgetMap::iterator wit = m_orphans.find(std::string(uuid));
    if (wit == m_orphans.end()) {
      // Search for pad
      wit = widgets->find(std::string(padUuid));
      if (wit != widgets->end()) {
        String* newString = new String(Point2D(startX, startY), Point2D(endX, endY), 1);
        newString->setUuid(uuid);
        // Add string to soundsource
        soundSources->insert(SoundSourceData(newString->getUuid(), newString));

        wit->second->addChild(newString);
        newString->setPadRadius(((RoundPad*)wit->second)->getRadius());

        // Tell the world
        newString->toOutboundPacketStream(ps);
        broadcast(ps);
      } else {// orphan if we don't know about its pad yet
        String* newString = new String(Point2D(startX, startY), Point2D(endX, endY), 1);
        newString->setUuid(uuid);
        m_orphans.insert(WidgetData(std::string(padUuid), newString));
      }

      SoundSource::unlockGlobals();
      unlocked = true;
    }
  }

  if (!unlocked)
    SoundSource::unlockGlobals();
}

void Network::handleMousePositionMessage(const osc::ReceivedMessage& m,
                                         const IpEndpointName& remoteEndpoint)
{
  // Parse OSC message
  osc::int32 port;
  float x, y;
  bool down;
  m.ArgumentStream() >> port >> x >> y >> down >> osc::EndMessage;

  PeerMap::iterator pit = m_peers.find(IpEndpointName(remoteEndpoint.address, (int)port));
  if (pit != m_peers.end()) {
    pit->second->setMousePosition(x, y);
    pit->second->setMouseDown(down);
  }
}

void Network::handleObjectDeleteMessage(const osc::ReceivedMessage& m,
                                        const IpEndpointName& remoteEndpoint)
{
  // Parse OSC message
  osc::Symbol uuid;
  m.ArgumentStream() >> uuid >> osc::EndMessage;

  std::cerr << uuid << std::endl;

  WidgetMap* widgets = Widget::getAll();
  WidgetMap::iterator wit = widgets->find(std::string(uuid));
  Widget* widget;
  if (wit != widgets->end() &&
      (widget = wit->second->getParent()->removeChild(wit->second))) {
    sendObjectMessage(widget, true);
    delete widget;
  }
}

void Network::handleObjectQueryMessage(const osc::ReceivedMessage& m,
                                       const IpEndpointName& remoteEndpoint)
{
  // TODO: well, maybe...
}
