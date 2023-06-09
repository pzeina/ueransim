//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "layer.hpp"

#include <algorithm>

#include <lib/rrc/encode.hpp>
#include <ue/task.hpp>

namespace nr::ue
{

void UeRrcLayer::performCellSelection()
{
    if (m_state == ERrcState::RRC_CONNECTED)
        return;

    int64_t currentTime = utils::CurrentTimeMillis();

    if (currentTime - m_startedTime <= 1000LL && m_cellDesc.empty())
        return;
    if (currentTime - m_startedTime <= 4000LL && !m_ue->shCtx.selectedPlmn.hasValue())
        return;

    auto lastCell = m_ue->shCtx.currentCell;

    bool shouldLogErrors = lastCell.cellId != 0 || (currentTime - m_lastTimePlmnSearchFailureLogged >= 30'000LL);

    ActiveCellInfo cellInfo;
    CellSelectionReport report;

    bool cellFound = false;
    if (m_ue->shCtx.selectedPlmn.hasValue())
    {
        cellFound = lookForSuitableCell(cellInfo, report);
        if (!cellFound)
        {
            if (shouldLogErrors)
            {
                if (!m_cellDesc.empty())
                {
                    m_logger->warn(
                        "Suitable cell selection failed in [%d] cells. [%d] out of PLMN, [%d] no SI, [%d] reserved, "
                        "[%d] barred, ftai [%d]",
                        static_cast<int>(m_cellDesc.size()), report.outOfPlmnCells, report.siMissingCells,
                        report.reservedCells, report.barredCells, report.forbiddenTaiCells);
                }
                else
                {
                    m_logger->warn("Suitable cell selection failed, no cell is in coverage");
                }

                m_lastTimePlmnSearchFailureLogged = currentTime;
            }
        }
    }

    if (!cellFound)
    {
        report = {};

        cellFound = lookForAcceptableCell(cellInfo, report);

        if (!cellFound)
        {
            if (shouldLogErrors)
            {
                if (!m_cellDesc.empty())
                {
                    m_logger->warn("Acceptable cell selection failed in [%d] cells. [%d] no SI, [%d] reserved, [%d] "
                                   "barred, ftai [%d]",
                                   static_cast<int>(m_cellDesc.size()), report.siMissingCells, report.reservedCells,
                                   report.barredCells, report.forbiddenTaiCells);
                }
                else
                {
                    m_logger->warn("Acceptable cell selection failed, no cell is in coverage");
                }

                m_logger->err("Cell selection failure, no suitable or acceptable cell found");

                m_lastTimePlmnSearchFailureLogged = currentTime;
            }
        }
    }

    int selectedCell = cellInfo.cellId;
    m_ue->shCtx.currentCell = cellInfo;

    if (selectedCell != 0 && selectedCell != lastCell.cellId)
        m_logger->info("Selected cell plmn[%s] tac[%d] category[%s]", ToJson(cellInfo.plmn).str().c_str(), cellInfo.tac,
                       ToJson(cellInfo.category).str().c_str());

    if (selectedCell != lastCell.cellId)
    {
        m_ue->rlsCtl->assignCurrentCell(selectedCell);
        m_ue->nas->handleActiveCellChange(Tai{lastCell.plmn, lastCell.tac});
    }
}

bool UeRrcLayer::lookForSuitableCell(ActiveCellInfo &cellInfo, CellSelectionReport &report)
{
    Plmn selectedPlmn = m_ue->shCtx.selectedPlmn;
    if (!selectedPlmn.hasValue())
        return false;

    std::vector<int> candidates;

    for (auto &item : m_cellDesc)
    {
        auto &cell = item.second;

        if (!cell.sib1.hasSib1)
        {
            report.siMissingCells++;
            continue;
        }

        if (!cell.mib.hasMib)
        {
            report.siMissingCells++;
            continue;
        }

        if (cell.sib1.plmn != selectedPlmn)
        {
            report.outOfPlmnCells++;
            continue;
        }

        if (cell.mib.isBarred)
        {
            report.barredCells++;
            continue;
        }

        if (cell.sib1.isReserved)
        {
            report.reservedCells++;
            continue;
        }

        Tai tai{cell.sib1.plmn, cell.sib1.tac};

        if (std::any_of(m_ue->shCtx.forbiddenTaiRoaming.begin(), m_ue->shCtx.forbiddenTaiRoaming.end(),
                        [&tai](auto &element) { return element == tai; }))
        {
            report.forbiddenTaiCells++;
            continue;
        }
        if (std::any_of(m_ue->shCtx.forbiddenTaiRps.begin(), m_ue->shCtx.forbiddenTaiRps.end(),
                        [&tai](auto &element) { return element == tai; }))
        {
            report.forbiddenTaiCells++;
            continue;
        }

        // It seems suitable
        candidates.push_back(item.first);
    }

    if (candidates.empty())
        return false;

    // Order candidates by signal strength
    std::sort(candidates.begin(), candidates.end(), [this](int a, int b) {
        auto &cellA = m_cellDesc[a];
        auto &cellB = m_cellDesc[b];
        return cellB.dbm < cellA.dbm;
    });

    auto &selectedId = candidates[0];
    auto &selectedCell = m_cellDesc[selectedId];

    cellInfo = {};
    cellInfo.cellId = selectedId;
    cellInfo.plmn = selectedCell.sib1.plmn;
    cellInfo.tac = selectedCell.sib1.tac;
    cellInfo.category = ECellCategory::SUITABLE_CELL;

    return true;
}

bool UeRrcLayer::lookForAcceptableCell(ActiveCellInfo &cellInfo, CellSelectionReport &report)
{
    std::vector<int> candidates;

    for (auto &item : m_cellDesc)
    {
        auto &cell = item.second;

        if (!cell.sib1.hasSib1)
        {
            report.siMissingCells++;
            continue;
        }

        if (!cell.mib.hasMib)
        {
            report.siMissingCells++;
            continue;
        }

        if (cell.mib.isBarred)
        {
            report.barredCells++;
            continue;
        }

        if (cell.sib1.isReserved)
        {
            report.reservedCells++;
            continue;
        }

        Tai tai{cell.sib1.plmn, cell.sib1.tac};

        if (std::any_of(m_ue->shCtx.forbiddenTaiRoaming.begin(), m_ue->shCtx.forbiddenTaiRoaming.end(),
                        [&tai](auto &element) { return element == tai; }))
        {
            report.forbiddenTaiCells++;
            continue;
        }
        if (std::any_of(m_ue->shCtx.forbiddenTaiRps.begin(), m_ue->shCtx.forbiddenTaiRps.end(),
                        [&tai](auto &element) { return element == tai; }))
        {
            report.forbiddenTaiCells++;
            continue;
        }

        // It seems acceptable
        candidates.push_back(item.first);
    }

    if (candidates.empty())
        return false;

    // Order candidates by signal strength first
    std::sort(candidates.begin(), candidates.end(), [this](int a, int b) {
        auto &cellA = m_cellDesc[a];
        auto &cellB = m_cellDesc[b];
        return cellB.dbm < cellA.dbm;
    });

    // Then order candidates by PLMN priority if we have a selected PLMN
    Plmn& selectedPlmn = m_ue->shCtx.selectedPlmn;
    if (selectedPlmn.hasValue())
    {
        // Using stable-sort here
        std::stable_sort(candidates.begin(), candidates.end(), [this, &selectedPlmn](int a, int b) {
            auto &cellA = m_cellDesc[a];
            auto &cellB = m_cellDesc[b];

            bool matchesA = cellA.sib1.hasSib1 && cellA.sib1.plmn == selectedPlmn;
            bool matchesB = cellB.sib1.hasSib1 && cellB.sib1.plmn == selectedPlmn;

            return matchesB < matchesA;
        });
    }

    auto &selectedId = candidates[0];
    auto &selectedCell = m_cellDesc[selectedId];

    cellInfo = {};
    cellInfo.cellId = selectedId;
    cellInfo.plmn = selectedCell.sib1.plmn;
    cellInfo.tac = selectedCell.sib1.tac;
    cellInfo.category = ECellCategory::ACCEPTABLE_CELL;

    return true;
}

} // namespace nr::ue