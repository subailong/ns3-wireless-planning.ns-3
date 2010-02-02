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

#include "bs-scheduler-simple.h"
#include "ns3/simulator.h"
#include "bs-net-device.h"
#include "packet-burst.h"
#include "cid.h"
#include "wimax-mac-header.h"
#include "ss-record.h"
#include "wimax-mac-queue.h"
#include "ns3/log.h"
#include "burst-profile-manager.h"
#include "wimax-connection.h"
#include "connection-manager.h"
#include "ss-manager.h"
#include "service-flow.h"
#include "service-flow-record.h"
#include "service-flow-manager.h"

NS_LOG_COMPONENT_DEFINE ("BSSchedulerSimple");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED ( BSSchedulerSimple);

TypeId
BSSchedulerSimple::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BSSchedulerSimple").SetParent<Object> ().AddConstructor<BSSchedulerSimple> ();
  return tid;
}

BSSchedulerSimple::BSSchedulerSimple ()
  : m_downlinkBursts (new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst> > > ())
{
  SetBs (0);
}

BSSchedulerSimple::BSSchedulerSimple (Ptr<BaseStationNetDevice> bs)
  : m_downlinkBursts (new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst> > > ())
{
  // m_downlinkBursts is filled by AddDownlinkBurst and emptied by
  // wimax-bs-net-device::sendBurst and wimax-ss-net-device::sendBurst
  SetBs (bs);
}

BSSchedulerSimple::~BSSchedulerSimple (void)
{
  std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst> > > *downlinkBursts = m_downlinkBursts;
  std::pair<OfdmDlMapIe*, Ptr<PacketBurst> > pair;
  while (downlinkBursts->size ())
    {
      pair = downlinkBursts->front ();
      pair.second = 0;
      delete pair.first;
    }
  SetBs (0);
  delete m_downlinkBursts;
  m_downlinkBursts = 0;
}

std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst> > >*
BSSchedulerSimple::GetDownlinkBursts (void) const
{
  return m_downlinkBursts;
}

void
BSSchedulerSimple::AddDownlinkBurst (Ptr<const WimaxConnection> connection,
                                     uint8_t diuc,
                                     WimaxPhy::ModulationType modulationType,
                                     Ptr<PacketBurst> burst)
{
  OfdmDlMapIe *dlMapIe = new OfdmDlMapIe ();
  dlMapIe->SetCid (connection->GetCid ());
  dlMapIe->SetDiuc (diuc);

  NS_LOG_INFO ("BS scheduler, burst size: " << burst->GetSize () << " bytes" << ", pkts: " << burst->GetNPackets ()
                                            << ", connection: " << connection->GetTypeStr () << ", CID: " << connection->GetCid ());
  if (connection->GetType () == Cid::TRANSPORT)
    {
      NS_LOG_INFO (", SFID: " << connection->GetServiceFlow ()->GetSfid () << ", service: "
                              << connection->GetServiceFlow ()->GetSchedulingTypeStr ());
    }
  NS_LOG_INFO (", modulation: " << modulationType << ", DIUC: " << (uint32_t) diuc);

  m_downlinkBursts->push_back (std::make_pair (dlMapIe, burst));
}

void
BSSchedulerSimple::Schedule (void)
{
  Ptr<WimaxConnection> connection, prevConnection;
  WimaxPhy::ModulationType modulationType = WimaxPhy::MODULATION_TYPE_BPSK_12;
  uint8_t diuc = OfdmDlBurstProfile::DIUC_BURST_PROFILE_1;
  uint32_t nrSymbolsRequired = 0;
  GenericMacHeader hdr;
  Ptr<Packet> packet;
  Ptr<PacketBurst> burst = Create<PacketBurst> ();
  ServiceFlow::SchedulingType schedulingType = ServiceFlow::SF_TYPE_NONE;
  uint32_t availableSymbols = GetBs ()->GetNrDlSymbols ();

  while (SelectConnection (connection))
    {
      prevConnection = connection;

      if (connection != GetBs ()->GetInitialRangingConnection () && connection != GetBs ()->GetBroadcastConnection ())
        {
          /* determines modulation/DIUC only once per burst as it is always same for a particular CID */
          if (connection->GetType () == Cid::MULTICAST)
            {
              modulationType = connection->GetServiceFlow ()->GetModulation ();
            }
          else
            {
              modulationType = GetBs ()->GetSSManager ()->GetSSRecord (connection->GetCid ())->GetModulationType ();
            }
          diuc = GetBs ()->GetBurstProfileManager ()->GetBurstProfile (modulationType,
                                                                       WimaxNetDevice::DIRECTION_DOWNLINK);
        }
      else if (connection == GetBs ()->GetInitialRangingConnection () || connection
               == GetBs ()->GetBroadcastConnection ())
        {

          modulationType = WimaxPhy::MODULATION_TYPE_BPSK_12;
          diuc = OfdmDlBurstProfile::DIUC_BURST_PROFILE_1;
        }

      if (connection->GetType () == Cid::TRANSPORT || connection->GetType () == Cid::MULTICAST)
        {
          schedulingType = (ServiceFlow::SchedulingType) connection->GetSchedulingType ();
        }

      if (schedulingType == ServiceFlow::SF_TYPE_UGS)
        {
          nrSymbolsRequired = connection->GetServiceFlow ()->GetRecord ()->GetGrantSize ();

          burst = CreateUgsBurst (connection->GetServiceFlow (), modulationType, nrSymbolsRequired);
        }
      else
        {
          packet = connection->GetQueue ()->Peek (hdr);
          nrSymbolsRequired = GetBs ()->GetPhy ()->GetNrSymbols (packet->GetSize (), modulationType);
        }

      if (availableSymbols < nrSymbolsRequired)
        {

          break;
        }

      if (schedulingType != ServiceFlow::SF_TYPE_UGS)
        {
          packet = connection->Dequeue ();
          NS_ASSERT_MSG (hdr.GetCid () == connection->GetCid (),
                         "Base station: Error while scheduling connection: header CID != connection CID");

          /* create a new burst if CID for this packet is different, add previous burst to the list */
          if (burst->GetNPackets () != 0)
            {
              AddDownlinkBurst (connection, diuc, modulationType, burst);
              burst = Create<PacketBurst> ();
            }

          burst->AddPacket (packet);
        }

      availableSymbols -= nrSymbolsRequired;
      schedulingType = ServiceFlow::SF_TYPE_NONE;
    }

  if (burst->GetNPackets ())
    {
      AddDownlinkBurst (prevConnection, diuc, modulationType, burst);
    }

  if (m_downlinkBursts->size ())
    {
      NS_LOG_DEBUG ("BS scheduler, number of bursts: " << m_downlinkBursts->size () << ", symbols left: "
                                                       << availableSymbols << std::endl << "BS scheduler, queues:"
                                                       << " IR "
                                                       << GetBs ()->GetInitialRangingConnection ()->GetQueue ()->GetSize ()
                                                       << " broadcast "
                                                       << GetBs ()->GetBroadcastConnection ()->GetQueue ()->GetSize ()
                                                       << " basic "
                                                       << GetBs ()->GetConnectionManager ()->GetNPackets (Cid::BASIC, ServiceFlow::SF_TYPE_NONE)
                                                       << " primary "
                                                       << GetBs ()->GetConnectionManager ()->GetNPackets (Cid::PRIMARY, ServiceFlow::SF_TYPE_NONE)
                                                       << " transport "
                                                       << GetBs ()->GetConnectionManager ()->GetNPackets (Cid::TRANSPORT, ServiceFlow::SF_TYPE_ALL));
    }
}

bool
BSSchedulerSimple::SelectConnection (Ptr<WimaxConnection> &connection)
{
  connection = 0;
  Time currentTime = Simulator::Now ();
  std::vector<Ptr<WimaxConnection> >::const_iterator iter1;
  std::vector<ServiceFlow*>::iterator iter2;
  ServiceFlowRecord *serviceFlowRecord;
  NS_LOG_INFO ("BS Scheduler: Selecting connection...");
  if (GetBs ()->GetBroadcastConnection ()->HasPackets ())
    {
      NS_LOG_INFO ("Return GetBroadcastConnection");
      connection = GetBs ()->GetBroadcastConnection ();
      return true;
    }
  else if (GetBs ()->GetInitialRangingConnection ()->HasPackets ())
    {
      NS_LOG_INFO ("Return GetInitialRangingConnection");
      connection = GetBs ()->GetInitialRangingConnection ();
      return true;
    }
  else
    {
      std::vector<Ptr<WimaxConnection> > connections;
      std::vector<ServiceFlow*> serviceFlows;

      connections = GetBs ()->GetConnectionManager ()->GetConnections (Cid::BASIC);
      for (iter1 = connections.begin (); iter1 != connections.end (); ++iter1)
        {
          if ((*iter1)->HasPackets ())
            {
              NS_LOG_INFO ("Return Basic");
              connection = *iter1;
              return true;
            }
        }

      connections = GetBs ()->GetConnectionManager ()->GetConnections (Cid::PRIMARY);
      for (iter1 = connections.begin (); iter1 != connections.end (); ++iter1)
        {
          if ((*iter1)->HasPackets ())
            {
              NS_LOG_INFO ("Return Primary");
              connection = *iter1;
              return true;
            }
        }

      serviceFlows = GetBs ()->GetServiceFlowManager ()->GetServiceFlows (ServiceFlow::SF_TYPE_UGS);
      for (iter2 = serviceFlows.begin (); iter2 != serviceFlows.end (); ++iter2)
        {
          serviceFlowRecord = (*iter2)->GetRecord ();
          NS_LOG_INFO ("processing UGS: HAS PACKET=" << (*iter2)->HasPackets () << "max Latency = "
                                                     << MilliSeconds ((*iter2)->GetMaximumLatency ()) << "Delay = " << ((currentTime
                                                                                                                         - serviceFlowRecord->GetDlTimeStamp ()) + GetBs ()->GetPhy ()->GetFrameDuration ()));
          // if latency would exceed in case grant is allocated in next frame then allocate in current frame
          if ((*iter2)->HasPackets () && ((currentTime - serviceFlowRecord->GetDlTimeStamp ())
                                          + GetBs ()->GetPhy ()->GetFrameDuration ()) > MilliSeconds ((*iter2)->GetMaximumLatency ()))
            {
              serviceFlowRecord->SetDlTimeStamp (currentTime);
              connection = (*iter2)->GetConnection ();
              NS_LOG_INFO ("Return UGS SF: CID = " << (*iter2)->GetCid () << "SFID = " << (*iter2)->GetSfid ());
              return true;
            }
        }

      serviceFlows = GetBs ()->GetServiceFlowManager ()->GetServiceFlows (ServiceFlow::SF_TYPE_RTPS);
      for (iter2 = serviceFlows.begin (); iter2 != serviceFlows.end (); ++iter2)
        {
          serviceFlowRecord = (*iter2)->GetRecord ();
          // if latency would exceed in case poll is allocated in next frame then allocate in current frame
          if ((*iter2)->HasPackets () && ((currentTime - serviceFlowRecord->GetDlTimeStamp ())
                                          + GetBs ()->GetPhy ()->GetFrameDuration ()) > MilliSeconds ((*iter2)->GetMaximumLatency ()))
            {
              serviceFlowRecord->SetDlTimeStamp (currentTime);
              connection = (*iter2)->GetConnection ();
              NS_LOG_INFO ("Return RTPS SF: CID = " << (*iter2)->GetCid () << "SFID = " << (*iter2)->GetSfid ());
              return true;
            }
        }

      serviceFlows = GetBs ()->GetServiceFlowManager ()->GetServiceFlows (ServiceFlow::SF_TYPE_NRTPS);
      for (iter2 = serviceFlows.begin (); iter2 != serviceFlows.end (); ++iter2)
        {
          serviceFlowRecord = (*iter2)->GetRecord ();
          if ((*iter2)->HasPackets ())
            {
              NS_LOG_INFO ("Return NRTPS SF: CID = " << (*iter2)->GetCid () << "SFID = " << (*iter2)->GetSfid ());
              connection = (*iter2)->GetConnection ();
              return true;
            }
        }

      serviceFlows = GetBs ()->GetServiceFlowManager ()->GetServiceFlows (ServiceFlow::SF_TYPE_BE);
      for (iter2 = serviceFlows.begin (); iter2 != serviceFlows.end (); ++iter2)
        {
          serviceFlowRecord = (*iter2)->GetRecord ();
          if ((*iter2)->HasPackets ())
            {
              NS_LOG_INFO ("Return BE SF: CID = " << (*iter2)->GetCid () << "SFID = " << (*iter2)->GetSfid ());
              connection = (*iter2)->GetConnection ();
              return true;
            }
        }
    }
  NS_LOG_INFO ("NO connection is selected!");
  return false;
}

Ptr<PacketBurst>
BSSchedulerSimple::CreateUgsBurst (ServiceFlow *serviceFlow,
                                   WimaxPhy::ModulationType modulationType,
                                   uint32_t availableSymbols)
{
  Time timeStamp;
  GenericMacHeader hdr;
  Ptr<Packet> packet;
  Ptr<PacketBurst> burst = Create<PacketBurst> ();
  uint32_t nrSymbolsRequired = 0;

  serviceFlow->CleanUpQueue ();

  while (serviceFlow->HasPackets ())
    {
      packet = serviceFlow->GetQueue ()->Peek (hdr, timeStamp);
      nrSymbolsRequired = GetBs ()->GetPhy ()->GetNrSymbols (packet->GetSize (), modulationType);

      if (nrSymbolsRequired > availableSymbols)
        {

          break;
        }

      packet = serviceFlow->GetConnection ()->Dequeue ();
      burst->AddPacket (packet);
    }

  NS_ASSERT_MSG (burst->GetSize (), "Base station: Error while creating UGS burst: burst size = 0");
  return burst;
}

}
