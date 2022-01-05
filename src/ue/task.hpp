//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#pragma once

#include "types.hpp"

#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include <lib/rls/rls_pdu.hpp>
#include <lib/rrc/rrc.hpp>
#include <lib/udp/server_task.hpp>
#include <ue/nas/layer.hpp>
#include <ue/rls/ctl_layer.hpp>
#include <ue/rls/udp_layer.hpp>
#include <ue/rrc/layer.hpp>
#include <ue/tun/layer.hpp>
#include <utils/common_types.hpp>
#include <utils/logger.hpp>
#include <utils/nts.hpp>

namespace nr::ue
{

class RlsUdpLayer;
class RlsCtlLayer;
class UeRrcLayer;
class NasLayer;
class TunLayer;

class UeTask : public NtsTask
{
  private:
    std::unique_ptr<Logger> m_logger;

  private:
    std::unique_ptr<RlsUdpLayer> m_rlsUdp;
    std::unique_ptr<RlsCtlLayer> m_rlsCtl;
    std::unique_ptr<UeRrcLayer> m_rrc;
    std::unique_ptr<NasLayer> m_nas;
    std::unique_ptr<TunLayer> m_tun;

  public:
    UserEquipment *ue{};
    std::unique_ptr<UeConfig> config{};
    std::unique_ptr<LogBase> logBase{};
    app::IUeController *ueController{};
    app::INodeListener *nodeListener{};
    NtsTask *cliCallbackTask{};
    UeSharedContext shCtx{};

    friend class UeCmdHandler;

  public:
    explicit UeTask(std::unique_ptr<UeConfig> &&config, app::IUeController *ueController,
                    app::INodeListener *nodeListener, NtsTask *cliCallbackTask);
    ~UeTask() override;

  protected:
    void onStart() override;
    void onLoop() override;
    void onQuit() override;

  public:
    inline RlsUdpLayer &rlsUdp()
    {
        return *m_rlsUdp;
    }

    inline RlsCtlLayer &rlsCtl()
    {
        return *m_rlsCtl;
    }

    inline UeRrcLayer &rrc()
    {
        return *m_rrc;
    }

    inline NasLayer &nas()
    {
        return *m_nas;
    }

    inline TunLayer &tun()
    {
        return *m_tun;
    }
};

} // namespace nr::ue