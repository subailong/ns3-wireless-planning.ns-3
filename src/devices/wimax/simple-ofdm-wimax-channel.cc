/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#include "ns3/simulator.h"
#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/assert.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "wimax-phy.h"
#include "simple-ofdm-wimax-phy.h"
#include "simple-ofdm-wimax-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/cost231-propagation-loss-model.h"
#include "simple-ofdm-send-param.h"

NS_LOG_COMPONENT_DEFINE ("simpleOfdmWimaxChannel");

namespace ns3 {
// NS_OBJECT_ENSURE_REGISTERED (simpleOfdmWimaxChannel);


SimpleOfdmWimaxChannel::SimpleOfdmWimaxChannel (void)
{
  m_loss = 0;
}

SimpleOfdmWimaxChannel::~SimpleOfdmWimaxChannel (void)
{
  m_phyList.clear ();
}

SimpleOfdmWimaxChannel::SimpleOfdmWimaxChannel (PropModel propModel)
{
  switch (propModel)
    {
    case RANDOM_PROPAGATION:
      m_loss = CreateObject<RandomPropagationLossModel> ();
      break;

    case FRIIS_PROPAGATION:
      m_loss = CreateObject<FriisPropagationLossModel> ();
      break;
    case LOG_DISTANCE_PROPAGATION:
      m_loss = CreateObject<LogDistancePropagationLossModel> ();
      break;

    case COST231_PROPAGATION:
      m_loss = CreateObject<Cost231PropagationLossModel> ();
      break;

    default:
      m_loss = 0;
    }

}

void
SimpleOfdmWimaxChannel::SetPropagationModel (PropModel propModel)
{

  switch (propModel)
    {
    case RANDOM_PROPAGATION:
      m_loss = CreateObject<RandomPropagationLossModel> ();
      break;

    case FRIIS_PROPAGATION:
      m_loss = CreateObject<FriisPropagationLossModel> ();
      break;
    case LOG_DISTANCE_PROPAGATION:
      m_loss = CreateObject<LogDistancePropagationLossModel> ();
      break;

    case COST231_PROPAGATION:
      m_loss = CreateObject<Cost231PropagationLossModel> ();
      break;

    default:
      m_loss = 0;
    }

}

void
SimpleOfdmWimaxChannel::DoAttach (Ptr<WimaxPhy> phy)
{
  Ptr<SimpleOfdmWimaxPhy> o_phy = phy->GetObject<SimpleOfdmWimaxPhy> ();
  m_phyList.push_back (o_phy);
}

uint32_t
SimpleOfdmWimaxChannel::DoGetNDevices (void) const
{
  return m_phyList.size ();
}

Ptr<NetDevice>
SimpleOfdmWimaxChannel::DoGetDevice (uint32_t index) const
{
  uint32_t j = 0;
  for (std::list<Ptr<SimpleOfdmWimaxPhy> >::const_iterator iter = m_phyList.begin (); iter != m_phyList.end (); ++iter)
    {
      if (j == index)
        {
          return (*iter)->GetDevice ();
        }
      j++;
    }

  NS_FATAL_ERROR ("Unable to get device");
  return 0;
}

void
SimpleOfdmWimaxChannel::Send (Time BlockTime,
                              const bvec &fecBlock,
                              uint32_t burstSize,
                              Ptr<WimaxPhy> phy,
                              bool isFirstBlock,
                              uint64_t frequency,
                              WimaxPhy::ModulationType modulationType,
                              uint8_t direction,
                              uint32_t symbolIndex,
                              double txPowerDbm)
{
  double rxPowerDbm = 0;
  Ptr<MobilityModel> senderMobility = 0;
  Ptr<MobilityModel> receiverMobility = 0;
  if (phy->GetMobility ())
    {
      senderMobility = phy->GetMobility ()->GetObject<MobilityModel> ();
    }

  for (std::list<Ptr<SimpleOfdmWimaxPhy> >::iterator iter = m_phyList.begin (); iter != m_phyList.end (); ++iter)
    {
      double delay = 0;
      if (phy != *iter)
        {
          if ((*iter)->GetMobility ())
            {
              receiverMobility = (*iter)->GetMobility ()->GetObject<MobilityModel> ();
            }
          if (receiverMobility != 0 && senderMobility != 0 && m_loss != 0)
            {
              delay = (senderMobility->GetDistanceFrom (receiverMobility)) / 300000000LL;
              rxPowerDbm = m_loss->CalcRxPower (rxPowerDbm, senderMobility, receiverMobility);
            }
          // delay=0;
          simpleOfdmSendParam * param = new simpleOfdmSendParam (fecBlock,
                                                                 burstSize,
                                                                 isFirstBlock,
                                                                 frequency,
                                                                 modulationType,
                                                                 direction,
                                                                 symbolIndex,
                                                                 rxPowerDbm);

          Ptr<Object> dstNetDevice = (*iter)->GetDevice ();
          uint32_t dstNode;
          if (dstNetDevice == 0)
            {
              dstNode = 0xffffffff;
            }
          else
            {
              dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
            }
          Simulator::ScheduleWithContext (dstNode,
                                          Seconds (delay) + BlockTime,
                                          &SimpleOfdmWimaxChannel::EndSend,
                                          this,
                                          *iter,
                                          param);
        }
    }

}

void
SimpleOfdmWimaxChannel::EndSend (Ptr<SimpleOfdmWimaxPhy> rxphy, simpleOfdmSendParam * param)
{
  // std::cout << "END_SEND ";
  rxphy->StartReceive (param->GetFecBlock (),
                       param->GetBurstSize (),
                       param->GetIsFirstBlock (),
                       param->GetFrequency (),
                       param->GetModulationType (),
                       param->GetDirection (),
                       param->GetSymbolIndex (),
                       param->GetRxPowerDbm ());
  delete param;
}

}
// namespace ns3