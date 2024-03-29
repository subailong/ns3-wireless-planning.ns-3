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

#include <stdint.h>
#include <list>
#include "ns3/packet.h"
#include "packet-burst.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("PacketBurst");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED ( PacketBurst);

TypeId
PacketBurst::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketBurst")
    .SetParent<Object> ()
    .AddConstructor<PacketBurst> ()
  ;
  return tid;
}

PacketBurst::PacketBurst (void)
{
}

PacketBurst::~PacketBurst (void)
{
  for (std::list<Ptr<Packet> >::const_iterator iter = m_packets.begin (); iter
       != m_packets.end (); ++iter)
    {
      (*iter)->Unref ();

    }
}

void
PacketBurst::DoDispose (void)
{
  m_packets.clear ();
}

Ptr<PacketBurst> PacketBurst::Copy (void) const
{
  Ptr<PacketBurst> burst = Create<PacketBurst> ();

  for (std::list<Ptr<Packet> >::const_iterator iter = m_packets.begin (); iter
       != m_packets.end (); ++iter)
    {
      Ptr<Packet> packet = (*iter)->Copy ();
      burst->AddPacket (packet);
    }
  return burst;
}

void
PacketBurst::AddPacket (Ptr<Packet> packet)
{
  if (packet)
    {
      m_packets.push_back (packet);
    }
}

std::list<Ptr<Packet> >
PacketBurst::GetPackets (void) const
{
  return m_packets;
}

uint32_t
PacketBurst::GetNPackets (void) const
{
  return m_packets.size ();
}

uint32_t
PacketBurst::GetSize (void) const
{
  uint32_t size = 0;
  for (std::list<Ptr<Packet> >::const_iterator iter = m_packets.begin (); iter
       != m_packets.end (); ++iter)
    {
      Ptr<Packet> packet = *iter;
      size += packet->GetSize ();
    }
  return size;
}

std::list<Ptr<Packet> >::const_iterator
PacketBurst::Begin (void) const
{
  return m_packets.begin ();
}

std::list<Ptr<Packet> >::const_iterator
PacketBurst::End (void) const
{
  return m_packets.end ();
}


} // namespace ns3


