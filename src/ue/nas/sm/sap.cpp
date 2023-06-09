//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "sm.hpp"

#include <algorithm>

#include <lib/nas/proto_conf.hpp>
#include <ue/nas/mm/mm.hpp>
#include <ue/task.hpp>

namespace nr::ue
{

void NasSm::onTimerTick()
{
    if (m_mm->m_mmState == EMmState::MM_NULL)
        return;

    int pti = 0;
    for (auto &pt : m_procedureTransactions)
    {
        if (pt.timer && pt.timer->performTick())
            onTransactionTimerExpire(pti);
        pti++;
    }
}

void NasSm::handleUplinkDataRequest(int psi, CompoundBuffer &buffer)
{
    auto state = m_mm->m_mmSubState;
    if (state != EMmSubState::MM_REGISTERED_INITIATED_PS && state != EMmSubState::MM_REGISTERED_NORMAL_SERVICE &&
        state != EMmSubState::MM_REGISTERED_NON_ALLOWED_SERVICE &&
        state != EMmSubState::MM_REGISTERED_LIMITED_SERVICE && state != EMmSubState::MM_DEREGISTERED_INITIATED_PS &&
        state != EMmSubState::MM_SERVICE_REQUEST_INITIATED_PS)
        return;

    if (m_pduSessions[psi]->psState != EPsState::ACTIVE)
        return;

    if (m_mm->m_cmState == ECmState::CM_CONNECTED)
    {
        // TODO: We should also check if radio resources are established by RRC.
        //  Checking CM state is not sufficient

        if (m_pduSessions[psi]->uplinkPending)
        {
            m_pduSessions[psi]->uplinkPending = false;
            handleUplinkStatusChange(psi, false);
        }

        m_ue->rlsCtl->handleUplinkDataDelivery(psi, buffer);
    }
    else
    {
        if (!m_pduSessions[psi]->uplinkPending)
        {
            m_pduSessions[psi]->uplinkPending = true;
            handleUplinkStatusChange(psi, true);
        }
    }
}

void NasSm::handleDownlinkDataRequest(int psi, const uint8_t *buffer, size_t size)
{
    if (m_mm->m_cmState == ECmState::CM_IDLE)
        return;

    auto state = m_mm->m_mmSubState;
    if (state != EMmSubState::MM_REGISTERED_INITIATED_PS && state != EMmSubState::MM_REGISTERED_NORMAL_SERVICE &&
        state != EMmSubState::MM_REGISTERED_NON_ALLOWED_SERVICE &&
        state != EMmSubState::MM_REGISTERED_LIMITED_SERVICE && state != EMmSubState::MM_DEREGISTERED_INITIATED_PS &&
        state != EMmSubState::MM_SERVICE_REQUEST_INITIATED_PS)
        return;

    m_ue->fdBase->write(FdBase::PS_START + psi, buffer, size);
}

} // namespace nr::ue
