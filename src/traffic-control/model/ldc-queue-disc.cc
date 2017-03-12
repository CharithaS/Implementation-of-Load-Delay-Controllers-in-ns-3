// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright © 2011 Marcos Talau
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
 * Author: Marcos Talau (talau@users.sourceforge.net)
 *
 * Thanks to: Duy Nguyen<duy@soe.ucsc.edu> by RED efforts in NS3
 *
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PORT NOTE: This code was ported from ns-2 (queue/ldc.cc).  Almost all 
 * comments have also been ported from NS-2
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ldc-queue-disc.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LdcQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (LdcQueueDisc);

TypeId LdcQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LdcQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName("TrafficControl")
    .AddConstructor<LdcQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (Queue::QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&LdcQueueDisc::SetMode),
                   MakeEnumChecker (Queue::QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    Queue::QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (500),
                   MakeUintegerAccessor (&LdcQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&LdcQueueDisc::SetQueueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QW",
                   "Queue weight related to the exponential weighted moving average (EWMA)",
                   DoubleValue (0.002),
                   MakeDoubleAccessor (&LdcQueueDisc::m_qW),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("LinkBandwidth", 
                   "The LDC link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&LdcQueueDisc::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The LDC link delay",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&LdcQueueDisc::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("Time Interval", 
                   "The time interval after which LDC calculates queueing delay",
                   TimeValue (Seconds (0.002)),
                   MakeTimeAccessor (&LdcQueueDisc::m_oInterval),
                   MakeTimeChecker ())
    .AddAttribute ("RW", 
                   "Input rate weight related to the exponential weighted moving average (EWMA)",
                   DoubleValue (0.998),
                   MakeDoubleAccessor (&LdcQueueDisc::m_wt1),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("WQ", 
                   "Weight for the delay component in drop probability calculation",
                   DoubleValue (0.75),
                   MakeDoubleAccessor (&LdcQueueDisc::m_wQ),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("Target Queueing Delay", 
                   "The LDC target queueing delay",
                   TimeValue (Seconds (4.0)),
                   MakeTimeAccessor (&LdcQueueDisc:: m_dTarget),
                   MakeTimeChecker ())
    .AddAttribute ("Target Load Factor Ratio", 
                   "The LDC target load factor ratio",
                   DoubleValue (0.95),
                   MakeDoubleAccessor (&LdcQueueDisc::m_rTarget),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("LDC Exponent",
                   "Exponent value used in drop probability calculation",
                   UintegerValue (3),
                   MakeUintegerAccessor (&LdcQueueDisc::m_lExp),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

LdcQueueDisc::LdcQueueDisc () :
  QueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
  m_rtrsEvent = Simulator::Schedule (m_oInterval, &LdcQueueDisc::Timeout, this);
}

LdcQueueDisc::~LdcQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
LdcQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  Simulator::Remove (m_rtrsEvent);
  QueueDisc::DoDispose ();
}

void
LdcQueueDisc::SetMode (Queue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

Queue::QueueMode
LdcQueueDisc::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}


void
LdcQueueDisc::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_queueLimit = lim;
}

LdcQueueDisc::Stats
LdcQueueDisc::GetStats ()
{
  NS_LOG_FUNCTION (this);
  return m_stats;
}

int64_t 
LdcQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

void 
LdcQueueDisc::Timeout ()
{
  NS_LOG_FUNCTION (this);

  uint32_t nQueued = GetQueueSize ();

  uint32_t m = 0;

  if (m_idle == 1)
    {
      NS_LOG_DEBUG ("LDC Queue Disc is idle.");
      Time now = Simulator::Now ();

      m_idle = 0;     
      m = uint32_t (m_ptc * (now - m_idleTime).GetSeconds ());
    }

  // overload factor
  m_nPkt = ((1.0 - m_wt1) * m_nPkt) + (m_wt1 * m_nIncome);
  m_nIncome = 0;
  double mm = m_oInterval.GetSeconds () * m_ptc;
  m_ratio = m_nPkt / mm;
 
  m_qAvg = Estimator (nQueued, m + 1, m_qAvg, m_qW);

  /* qlen + load */
  double qRatio = pow ((m_qAvg / m_qTarget), m_lExp);
  double rRatio = pow ((m_ratio / m_rTarget), m_lExp);
  
  if (qRatio > 1)
    {
      qRatio = 1;
    }

   if (rRatio > 1)
    {
      rRatio = 1;
    }

  m_vProb = (m_wQ * qRatio) + ((1-m_wQ) * rRatio);

  m_rtrsEvent = Simulator::Schedule (m_oInterval, &LdcQueueDisc::Timeout, this);
}

bool
LdcQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
   
  m_nIncome++;

  uint32_t nQueued = GetQueueSize ();

  NS_LOG_DEBUG ("\t bytesInQueue  " << GetInternalQueue (0)->GetNBytes () << "\tQavg " << m_qAvg);
  NS_LOG_DEBUG ("\t packetsInQueue  " << GetInternalQueue (0)->GetNPackets () << "\tQavg " << m_qAvg);

  uint32_t dropType = DTYPE_NONE;
  
  if (DropEarly (item, nQueued))
    {
      NS_LOG_LOGIC ("DropEarly returns 1");
      dropType = DTYPE_UNFORCED;
    }
  if ((GetMode () == Queue::QUEUE_MODE_PACKETS && nQueued >= m_queueLimit) ||
      (GetMode () == Queue::QUEUE_MODE_BYTES && nQueued + item->GetPacketSize() > m_queueLimit))
    {
      NS_LOG_DEBUG ("\t Dropping due to Queue Full " << nQueued);
      dropType = DTYPE_FORCED;
      m_stats.qLimDrop++;
    }

  if (dropType == DTYPE_UNFORCED)
    {
      NS_LOG_DEBUG ("\t Dropping due to Prob Mark " << m_qAvg);
      m_stats.unforcedDrop++;
      Drop (item);
      return false;
    }

  else if (dropType == DTYPE_FORCED)
    {
      NS_LOG_DEBUG ("\t Dropping due to Hard Mark " << m_qAvg);
      m_stats.forcedDrop++;
      Drop (item);
      return false;
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
  // because QueueDisc::AddInternalQueue sets the drop callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval;
}

/*
 * Note: if the link bandwidth changes in the course of the
 * simulation, the bandwidth-dependent LDC parameters do not change.
 * This should be fixed, but it would require some extra parameters,
 * and didn't seem worth the trouble...
 */
void
LdcQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Initializing LDC params.");

  m_ptc = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize);

  m_stats.forcedDrop = 0;
  m_stats.unforcedDrop = 0;
  m_stats.qLimDrop = 0;

  m_qAvg = 0.0;
  m_idle = 1;

  m_bCount = 0;
  m_nPkt = 0.0;
  m_ratio = 0.0;
  m_qTarget = m_dTarget.GetSeconds() * m_ptc; 
  m_nIncome = 0;

  m_idleTime = NanoSeconds (0);

  NS_LOG_DEBUG ("\tm_delay " << m_linkDelay.GetSeconds () << "; m_qW " << m_qW << "; m_ptc " << m_ptc);
}

// Compute the average queue size
double
LdcQueueDisc::Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW)
{
  NS_LOG_FUNCTION (this << nQueued << m << qAvg << qW);

  double newAve = qAvg * pow(1.0-qW, m);
  newAve += qW * nQueued;

  return newAve;
}

// Check if packet p needs to be dropped due to probability mark
uint32_t
LdcQueueDisc::DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
  NS_LOG_FUNCTION (this << item << qSize);

  double u = m_uv->GetValue ();

  if (u <= m_vProb)
    {
      NS_LOG_LOGIC ("u <= m_vProb; u " << u << "; m_vProb " << m_vProb);

      return 1; // drop
    }

  return 0; // no drop/mark
}

uint32_t
LdcQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown LDC mode.");
    }
}

Ptr<QueueDiscItem>
LdcQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      m_idle = 1;
      m_idleTime = Simulator::Now ();

      return 0;
    }
  else
    {
      m_idle = 0;
      Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
      m_bCount -= item->GetPacketSize ();

      NS_LOG_LOGIC ("Popped " << item);

      NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
      NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

      return item;
    }
}

Ptr<const QueueDiscItem>
LdcQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
LdcQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("LdcQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("LdcQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<Queue> queue = CreateObjectWithAttributes<DropTailQueue> ("Mode", EnumValue (m_mode));
      if (m_mode == Queue::QUEUE_MODE_PACKETS)
        {
          queue->SetMaxPackets (m_queueLimit);
        }
      else
        {
          queue->SetMaxBytes (m_queueLimit);
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("LdcQueueDisc needs 1 internal queue");
      return false;
    }

  if (GetInternalQueue (0)->GetMode () != m_mode)
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the LdcQueueDisc");
      return false;
    }

  if ((m_mode ==  Queue::QUEUE_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () < m_queueLimit) ||
      (m_mode ==  Queue::QUEUE_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () < m_queueLimit))
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

} // namespace ns3
