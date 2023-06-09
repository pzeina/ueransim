//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "layer.hpp"

#include <lib/rrc/encode.hpp>
#include <ue/task.hpp>

namespace nr::ue
{

void UeRrcLayer::handleCellSignalChange(int cellId, int dbm)
{
    bool considerLost = dbm < -120;

    if (!m_cellDesc.count(cellId))
    {
        if (!considerLost)
            notifyCellDetected(cellId, dbm);
    }
    else
    {
        if (considerLost)
            notifyCellLost(cellId);
        else
            m_cellDesc[cellId].dbm = dbm;
    }
}

void UeRrcLayer::notifyCellDetected(int cellId, int dbm)
{
    m_cellDesc[cellId] = {};
    m_cellDesc[cellId].dbm = dbm;

    m_logger->debug("New signal detected for cell[%d], total [%d] cells in coverage", cellId,
                    static_cast<int>(m_cellDesc.size()));

    updateAvailablePlmns();
}

void UeRrcLayer::notifyCellLost(int cellId)
{
    if (!m_cellDesc.count(cellId))
        return;

    bool isActiveCell = false;
    ActiveCellInfo lastActiveCell;

    if (m_ue->shCtx.currentCell.cellId == cellId)
    {
        lastActiveCell = m_ue->shCtx.currentCell;
        m_ue->shCtx.currentCell = {};
        isActiveCell = true;
    }

    m_cellDesc.erase(cellId);

    m_logger->debug("Signal lost for cell[%d], total [%d] cells in coverage", cellId,
                    static_cast<int>(m_cellDesc.size()));

    if (isActiveCell)
    {
        if (m_state != ERrcState::RRC_IDLE)
            declareRadioLinkFailure(rls::ERlfCause::SIGNAL_LOST_TO_CONNECTED_CELL);
        else
        {
            m_ue->nas->handleActiveCellChange(Tai{lastActiveCell.plmn, lastActiveCell.tac});
        }
    }

    updateAvailablePlmns();
}

bool UeRrcLayer::hasSignalToCell(int cellId)
{
    return m_cellDesc.count(cellId);
}

bool UeRrcLayer::isActiveCell(int cellId)
{
    return m_ue->shCtx.currentCell.cellId == cellId;
}

void UeRrcLayer::updateAvailablePlmns()
{
    m_ue->shCtx.availablePlmns.clear();
    for (auto &cellDesc : m_cellDesc)
        if (cellDesc.second.sib1.hasSib1)
            m_ue->shCtx.availablePlmns.insert(cellDesc.second.sib1.plmn);

    triggerCycle();
}

} // namespace nr::ue
