/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
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
 * Author: Kirill Andreev <andreev@iitp.ru>
 */


#ifndef WIFI_PREP_INFORMATION_ELEMENT_H
#define WIFI_PREP_INFORMATION_ELEMENT_H


#include "ns3/mac48-address.h"
#include "ns3/dot11s-codes.h"
#include "ns3/wifi-information-element.h"

namespace ns3 {
namespace dot11s {
/**
 * \ingroup dot11s
 * \brief See 7.3.2.97 of 802.11s draft 2.07
 */
class IeDot11sPrep : public WifiInformationElement
{
public:
  IeDot11sPrep ();
  ~IeDot11sPrep ();
  static TypeId GetTypeId ();
  virtual TypeId GetInstanceTypeId () const;
  virtual void Print (std::ostream &os) const;
  void SetFlags (uint8_t flags);
  void SetHopcount (uint8_t hopcount);
  void SetTTL (uint8_t ttl);
  void SetDestinationAddress (Mac48Address dest_address);
  void SetDestinationSeqNumber (uint32_t dest_seq_number);
  void SetLifetime (uint32_t lifetime);
  void SetMetric (uint32_t metric);
  void SetOriginatorAddress (Mac48Address originator_address);
  void SetOriginatorSeqNumber (uint32_t originator_seg_number);
 
  uint8_t GetFlags () const;
  uint8_t GetHopcount () const;
  uint32_t GetTTL () const;
  Mac48Address GetDestinationAddress () const;
  uint32_t GetDestinationSeqNumber () const;
  uint32_t GetLifetime () const;
  uint32_t GetMetric () const;
  Mac48Address GetOriginatorAddress () const;
  uint32_t GetOriginatorSeqNumber ()const ;

  void  DecrementTtl ();
  void  IncrementMetric (uint32_t metric);
private:
  WifiElementId ElementId () const{
    return IE11S_PREP;
  }
  void SerializeInformation (Buffer::Iterator i) const;
  uint8_t DeserializeInformation (Buffer::Iterator start, uint8_t length);
  uint8_t GetInformationSize () const;
  void PrintInformation (std::ostream& os) const;
  uint8_t  m_flags;
  uint8_t  m_hopcount;
  uint8_t  m_ttl;
  Mac48Address m_destinationAddress;
  uint32_t m_destSeqNumber;
  uint32_t m_lifetime;
  uint32_t m_metric;
  Mac48Address m_originatorAddress;
  uint32_t m_originatorSeqNumber;
};
  
} // namespace dot11s
} //namespace ns3
#endif

