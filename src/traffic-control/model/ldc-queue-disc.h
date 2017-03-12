/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * PORT NOTE: This code was ported from ns-2 (queue/red.h).  Almost all
 * comments also been ported from NS-2.
 * This implementation aims to be close to the results cited in [0]
 * [0] S.Floyd, K.Fall http://icir.org/floyd/papers/redsims.ps
 */

#ifndef LDC_QUEUE_DISC_H
#define LDC_QUEUE_DISC_H

#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup traffic-control
 *
 * \brief A RED packet queue disc
 */
class LdcQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief LdcQueueDisc Constructor
   *
   * Create an Ldc queue disc
   */
  LdcQueueDisc ();

  /**
   * \brief LdcQueueDisc Destructor
   *
   */ 
  virtual ~LdcQueueDisc ();

  /**
   * \brief Stats
   */
  typedef struct
  {   
    uint32_t unforcedDrop;  //!< Early probability drops
    uint32_t forcedDrop;    //!< Forced drops, qavg > max threshold
    uint32_t qLimDrop;      //!< Drops due to queue limits
  } Stats;

  /** 
   * \brief Drop types
   */
  enum
  {
    DTYPE_NONE,        //!< Ok, no drop
    DTYPE_FORCED,      //!< A "forced" drop
    DTYPE_UNFORCED,    //!< An "unforced" (random) drop
  };

  /**
   * \brief Set the operating mode of this queue.
   *  Set operating mode
   *
   * \param mode The operating mode of this queue.
   */
  void SetMode (Queue::QueueMode mode);

  /**
   * \brief Get the encapsulation mode of this queue.
   * Get the encapsulation mode of this queue
   *
   * \returns The encapsulation mode of this queue.
   */
  Queue::QueueMode GetMode (void);

  /**
   * \brief Get the current value of the queue in bytes or packets.
   *
   * \returns The queue size in bytes or packets.
   */
  uint32_t GetQueueSize (void);

  /**
   * \brief Set the limit of the queue in bytes or packets.
   *
   * \param lim The limit in bytes or packets.
   */
  void SetQueueLimit (uint32_t lim);

  /**
   * \brief Get the LDC statistics after running.
   *
   * \returns The drop statistics.
   */
  Stats GetStats ();

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);

  /**
   * \brief Initialize the queue parameters.
   *
   * Note: if the link bandwidth changes in the course of the
   * simulation, the bandwidth-dependent LDC parameters do not change.
   * This should be fixed, but it would require some extra parameters,
   * and didn't seem worth the trouble...
   */
  virtual void InitializeParams (void);
  /**
   * \brief Compute the average queue size
   * \param nQueued number of queued packets
   * \param m simulated number of packets arrival during idle period
   * \param qAvg average queue size
   * \param qW queue weight given to cur q size sample
   * \returns new average queue size
   */
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);
  /**
   * \brief Check if a packet needs to be dropped due to probability mark
   * \param item queue item
   * \param qSize queue size
   * \returns 0 for no drop/mark, 1 for drop
   */
  uint32_t DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize);
  
  void Timeout ();

  Stats m_stats; //!< LDC statistics

  // ** Variables supplied by user
  Queue::QueueMode m_mode;  //!< Mode (Bytes or packets)
  uint32_t m_meanPktSize;   //!< Avg pkt size
  uint32_t m_queueLimit;    //!< Queue limit in bytes / packets
  double m_qW;              //!< Queue weight given to cur queue size sample
  DataRate m_linkBandwidth; //!< Link bandwidth
  Time m_linkDelay;         //!< Link delay
  Time m_oInterval;         //!< The time interval after which LDC calculates queueing delay
  double m_wt1;             //!< Input rate weight given to current input rate
  double m_wQ;              //!< Weight for the delay component in drop probability calculation
  Time m_dTarget;           //!< Target queueing delay
  double m_rTarget;         //!< Target load factor ratio
  uint32_t m_lExp;          //!< Exponent value used in drop probability calculation

  // ** Variables maintained by LDC
  double m_vProb;           //!< Prob. of packet drop before "count"
  uint32_t m_idle;          //!< 0/1 idle status
  double m_ptc;             //!< packet time constant in packets/second
  double m_qAvg;            //!< Average queue length
  uint32_t m_bCount;
  double m_nPkt;
  double m_ratio;
  double m_qTarget; 
  uint32_t m_nIncome;
  Time m_idleTime;          //!< Start of current idle period

  EventId m_rtrsEvent;
  Ptr<UniformRandomVariable> m_uv;  //!< rng stream
};

}; // namespace ns3

#endif // LDC_QUEUE_DISC_H
