/*
Tencent is pleased to support the open source community by making 
PhxPaxos available.
Copyright (C) 2016 THL A29 Limited, a Tencent company. 
All rights reserved.

Licensed under the BSD 3-Clause License (the "License"); you may 
not use this file except in compliance with the License. You may 
obtain a copy of the License at

https://opensource.org/licenses/BSD-3-Clause

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
implied. See the License for the specific language governing 
permissions and limitations under the License.

See the AUTHORS file for names of contributors. 
*/

#pragma once

#include "base.h"
#include <string>
#include "ioloop.h"
#include "msg_counter.h"

namespace phxpaxos
{

class ProposerState
{
public:
    ProposerState(const Config * poConfig);
    ~ProposerState();

    void Init();

    void SetStartProposalID(const uint64_t llProposalID);

    void NewPrepare();

    void AddPreAcceptValue(const BallotNumber & oOtherPreAcceptBallot, const std::string & sOtherPreAcceptValue);

    /////////////////////////

    const uint64_t GetProposalID();

    const std::string & GetValue();

    void SetValue(const std::string & sValue);

    void SetOtherProposalID(const uint64_t llOtherProposalID);

    void ResetHighestOtherPreAcceptBallot();

public:
    uint64_t m_llProposalID;
    uint64_t m_llHighestOtherProposalID;
    std::string m_sValue;

    BallotNumber m_oHighestOtherPreAcceptBallot;

    Config * m_poConfig;
};

//////////////////////////////////////////////////

class Learner;

class Proposer : public Base
{
public:
    Proposer(
            const Config * poConfig, 
            const MsgTransport * poMsgTransport,
            const Instance * poInstance,
            const Learner * poLearner,
            const IOLoop * poIOLoop);
    ~Proposer();

    void SetStartProposalID(const uint64_t llProposalID);

    virtual void InitForNewPaxosInstance();

    int NewValue(const std::string & sValue);
    
    bool IsWorking();

    /////////////////////////////

    void Prepare(const bool bNeedNewBallot = true);

    void OnPrepareReply(const PaxosMsg & oPaxosMsg);

    void OnExpiredPrepareReply(const PaxosMsg & oPaxosMsg);

    void Accept();

    void OnAcceptReply(const PaxosMsg & oPaxosMsg);

    void OnExpiredAcceptReply(const PaxosMsg & oPaxosMsg);

    void OnPrepareTimeout();

    void OnAcceptTimeout();

    void ExitPrepare();

    void ExitAccept();

    void CancelSkipPrepare();

    /////////////////////////////

    void AddPrepareTimer(const int iTimeoutMs = 0);
    
    void AddAcceptTimer(const int iTimeoutMs = 0);

public:
    ProposerState m_oProposerState;
    MsgCounter m_oMsgCounter;
    Learner * m_poLearner;

    bool m_bIsPreparing;
    bool m_bIsAccepting;

    IOLoop * m_poIOLoop;

    uint32_t m_iPrepareTimerID;
    int m_iLastPrepareTimeoutMs;
    uint32_t m_iAcceptTimerID;
    int m_iLastAcceptTimeoutMs;
    uint64_t m_llTimeoutInstanceID;

    bool m_bCanSkipPrepare;

    bool m_bWasRejectBySomeone;

    TimeStat m_oTimeStat;
};
    
}



/*
已经accept的提议 {n,v} 
对所有 k < n {k,v} 
对所有 p > n {p,v}, 因为p prepare阶段读到已经accept的 value == v , 所有 p > n 的提议 value == v



 p arg1 = break num 
 p arg2 = propose num

 a arg1 = break num
 a arg2 = propose num
 a arg3 = propose val

case 1:
 p1(1,n) a1(1,n,v)
 p1(2,n) a2(2,n,v)
 p1(3,n) a3(3,n,v)
 议案 (n,v)

case 2:
 p1(1,n) p4(2,m)  a1(1,n,v) a1(2,m,a)
 p2(1,n) p4(2,m)  a2(1,n,v) a2(2,m,a)
 p3(1,n) p4(2,m)            a3(2,m,a)
 议案 (m,a) 第二轮accept确定
*/