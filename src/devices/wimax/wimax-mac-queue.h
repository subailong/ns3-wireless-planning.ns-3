/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#ifndef WIMAX_MAC_QUEUE_H
#define WIMAX_MAC_QUEUE_H

#include <queue>
#include <stdint.h>
#include "wimax-mac-header.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"

namespace ns3 {

class WimaxMacQueue : public Object
{
public:
  static TypeId GetTypeId (void);
  WimaxMacQueue (void);
  WimaxMacQueue (uint32_t maxSize);
  ~WimaxMacQueue (void);

  void SetMaxSize (uint32_t maxSize);
  uint32_t GetMaxSize (void) const;

  bool Enqueue (Ptr<Packet> packet, const MacHeaderType &hdrType,
                const GenericMacHeader &hdr);
  Ptr<Packet> Dequeue (MacHeaderType::HeaderType packetType =
                         MacHeaderType::HEADER_TYPE_GENERIC);

  /**
   * \brief Same as Dequeue but does not pop from queue
   */

  /*
   Exclusive for BS.
   Two versions of Peek function (with Generic MAC Header as parameter) only
   for BS, as BS's scheduler needs to know the CID in Generic MAC Header.
   */
  Ptr<Packet> Peek (GenericMacHeader &hdr) const;
  Ptr<Packet> Peek (GenericMacHeader &hdr, Time &timeStamp) const;

  /*
   Two versions for SS without Generic MAC Header parameter, as SS's
   scheduler does not require to know the CID in Generic MAC Header.
   */
  Ptr<Packet> Peek (MacHeaderType::HeaderType packetType) const;
  Ptr<Packet> Peek (MacHeaderType::HeaderType packetType, Time &timeStamp) const;

  bool IsEmpty (void) const;

  /*
   Exclusive for SS.
   */
  bool IsEmpty (MacHeaderType::HeaderType packetType) const;

  uint32_t GetSize (void) const;
  uint32_t GetNBytes (void) const;
private:
  struct QueueElement
  {
    QueueElement (void);
    QueueElement (Ptr<Packet> packet, const MacHeaderType &hdrType,
                  const GenericMacHeader &hdr, Time timeStamp);
    uint32_t GetSize (void) const;
    Ptr<Packet> m_packet;
    MacHeaderType m_hdrType;
    GenericMacHeader m_hdr;
    Time m_timeStamp;
  };

  /*
   In the case of non-UGS service flows at the SS side the queue will store both data packets
   and bandwidth request packets. The two are distinguished by their headers. The below two functions
   are for this purpose exclusively. The Front function returns the first packet of a specific packet
   type from the queue (which may not necessarily be at the front of the queue), and the Pop function
   pops that elements.
   */
  WimaxMacQueue::QueueElement Front (MacHeaderType::HeaderType packetType) const;
  void Pop (MacHeaderType::HeaderType packetType);

  typedef std::deque<QueueElement> PacketQueue;
  PacketQueue m_queue;
  uint32_t m_maxSize;
  uint32_t m_bytes;
  uint32_t m_nrDataPackets;
  uint32_t m_nrRequestPackets;

  TracedCallback<Ptr<const Packet> > m_traceEnqueue;
  TracedCallback<Ptr<const Packet> > m_traceDequeue;
  TracedCallback<Ptr<const Packet> > m_traceDrop;
public:
  const WimaxMacQueue::PacketQueue &
  GetPacketQueue (void) const;
};

} // namespace ns3

#endif /* WIMAX_MAC_QUEUE_H */