//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <lib/rls/rls_pdu.hpp>
#include <lib/udp/server.hpp>
#include <lib/udp/server_task.hpp>
#include <ue/types.hpp>
#include <utils/nts.hpp>
#include <utils/compound_buffer.hpp>

namespace nr::ue
{

class RlsUdpLayer
{
  private:
    struct CellInfo
    {
        InetAddress address;
        int64_t lastSeen{};
        int dbm{};
        int cellId{};
    };

  private:
    UeTask *m_ue;
    std::unique_ptr<Logger> m_logger;
    CompoundBuffer m_cBuffer;
    std::vector<InetAddress> m_searchSpace;
    std::unordered_map<uint64_t, CellInfo> m_cells;
    std::unordered_map<int, uint64_t> m_cellIdToSti;
    int64_t m_lastLoop;
    Vector3 m_simPos;
    int m_cellIdCounter;

    friend class UeCmdHandler;

  public:
    explicit RlsUdpLayer(UeTask *ue);
    ~RlsUdpLayer();

  private:
    void sendRlsPdu(const InetAddress &address, CompoundBuffer &buffer);
    void onSignalChangeOrLost(int cellId);
    void heartbeatCycle(uint64_t time, const Vector3 &simPos);

  public:
    void checkHeartbeat();
    void send(int cellId, CompoundBuffer &buffer);
    void receiveRlsPdu(const InetAddress &address, uint8_t *buffer, size_t size);
};

} // namespace nr::ue
