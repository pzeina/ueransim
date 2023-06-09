//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"
#include "cmd.hpp"

#include <utils/random.hpp>

struct TimerPeriod
{
    static constexpr const int L3_MACHINE_CYCLE = 2500;
    static constexpr const int L3_TIMER = 1000;
    static constexpr const int RLS_ACK_CONTROL = 1500;
    static constexpr const int RLS_ACK_SEND = 2250;
    static constexpr const int SWITCH_OFF = 500;
};

#define BUFFER_SIZE 32768ull

namespace nr::ue
{

ue::UeTask::UeTask(std::unique_ptr<UeConfig> &&config) : m_cBuffer(BUFFER_SIZE)
{
    this->logBase = std::make_unique<LogBase>("logs/ue-" + config->getNodeName() + ".log");
    this->config = std::move(config);
    this->fdBase = std::make_unique<FdBase>();
    this->m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[BUFFER_SIZE]);
    this->m_cmdHandler = std::make_unique<UeCmdHandler>(this);

    this->shCtx.sti = Random::Mixed(this->config->getNodeName()).nextL();

    this->rlsUdp = std::make_unique<RlsUdpLayer>(this);
    this->rlsCtl = std::make_unique<RlsCtlLayer>(this);
    this->rrc = std::make_unique<UeRrcLayer>(this);
    this->nas = std::make_unique<NasLayer>(this);
    this->tun = std::make_unique<TunLayer>(this);

    this->m_timerL3MachineCycle = -1;
    this->m_timerL3Timer = -1;
    this->m_timerRlsAckControl = -1;
    this->m_timerRlsAckSend = -1;
    this->m_timerSwitchOff = -1;

    this->m_immediateCycle = true;
}

UeTask::~UeTask() = default;

void UeTask::onStart()
{
    rrc->onStart();
    nas->onStart();
    m_cmdHandler->onStart();

    auto current = utils::CurrentTimeMillis();
    m_timerL3MachineCycle = current + TimerPeriod::L3_MACHINE_CYCLE;
    m_timerL3Timer = current + TimerPeriod::L3_TIMER;
    m_timerRlsAckControl = current + TimerPeriod::RLS_ACK_CONTROL;
    m_timerRlsAckSend = current + TimerPeriod::RLS_ACK_SEND;
}

bool UeTask::onLoop()
{
    rlsUdp->checkHeartbeat();

    if (checkTimers())
        return true;

    if (m_immediateCycle)
    {
        m_immediateCycle = false;
        rrc->performCycle();
        nas->performCycle();
        return false;
    }

    int fdId = fdBase->performSelect();
    if (fdId >= 0)
    {
        if (fdId >= FdBase::PS_START && fdId <= FdBase::PS_END)
        {
            size_t n = fdBase->read(fdId, m_cBuffer.cmAddress(), m_cBuffer.cmCapacity());
            m_cBuffer.reset();
            m_cBuffer.setCmSize(n);
            nas->handleUplinkDataRequest(fdId - FdBase::PS_START, m_cBuffer);
        }
        else if (fdId == FdBase::RLS_IP4 || fdId == FdBase::RLS_IP6)
        {
            InetAddress peer;
            size_t n = fdBase->receive(fdId, m_buffer.get(), BUFFER_SIZE, peer);
            rlsUdp->receiveRlsPdu(peer, m_buffer.get(), n);
        }
        else if (fdId == FdBase::CLI)
        {
            InetAddress peer;
            size_t n = fdBase->receive(fdId, m_buffer.get(), BUFFER_SIZE, peer);
            m_cmdHandler->receiveCmd(peer, m_buffer.get(), n);
        }
        return false;
    }

    return false;
}

void UeTask::onQuit()
{
    rrc->onQuit();
    nas->onQuit();
}

bool UeTask::checkTimers()
{
    auto current = utils::CurrentTimeMillis();

    if (m_timerL3MachineCycle != -1 && m_timerL3MachineCycle <= current)
    {
        m_timerL3MachineCycle = current + TimerPeriod::L3_MACHINE_CYCLE;
        rrc->performCycle();
        nas->performCycle();
    }
    else if (m_timerL3Timer != -1 && m_timerL3Timer <= current)
    {
        m_timerL3Timer = current + TimerPeriod::L3_TIMER;
        rrc->performCycle();
        nas->performCycle();
    }
    else if (m_timerRlsAckControl != -1 && m_timerRlsAckControl <= current)
    {
        m_timerRlsAckControl = current + TimerPeriod::RLS_ACK_CONTROL;
        rlsCtl->onAckControlTimerExpired();
    }
    else if (m_timerRlsAckSend != -1 && m_timerRlsAckSend <= current)
    {
        m_timerRlsAckSend = current + TimerPeriod::RLS_ACK_SEND;
        rlsCtl->onAckSendTimerExpired();
    }
    else if (m_timerSwitchOff != -1 && m_timerSwitchOff <= current)
    {
        return true;
    }

    return false;
}

void UeTask::triggerCycle()
{
    m_immediateCycle = true;
}

void UeTask::triggerSwitchOff()
{
    m_timerSwitchOff = utils::CurrentTimeMillis() + TimerPeriod::SWITCH_OFF;
}

} // namespace nr::ue