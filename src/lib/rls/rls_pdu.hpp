//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <utils/common_types.hpp>
#include <utils/compound_buffer.hpp>
#include <utils/octet_string.hpp>
#include <utils/octet_view.hpp>

namespace rls
{

enum class EMessageType : uint8_t
{
    RESERVED = 0,

    DEPRECATED1 [[maybe_unused]] = 1,
    DEPRECATED2 [[maybe_unused]] = 2,
    DEPRECATED3 [[maybe_unused]] = 3,

    HEARTBEAT = 4,
    HEARTBEAT_ACK = 5,
    PDU_TRANSMISSION = 6,
    PDU_TRANSMISSION_ACK = 7,
};

enum class EPduType : uint8_t
{
    RESERVED = 0,
    RRC,
    DATA
};

struct RlsMessage // TODO: remove
{
    const EMessageType msgType;
    const uint64_t sti{};

    explicit RlsMessage(EMessageType msgType, uint64_t sti) : msgType(msgType), sti(sti)
    {
    }

    virtual ~RlsMessage() = default;
};

struct RlsHeartBeat : RlsMessage
{
    Vector3 simPos;

    explicit RlsHeartBeat(uint64_t sti) : RlsMessage(EMessageType::HEARTBEAT, sti)
    {
    }
};

struct RlsHeartBeatAck : RlsMessage
{
    int dbm{};

    explicit RlsHeartBeatAck(uint64_t sti) : RlsMessage(EMessageType::HEARTBEAT_ACK, sti)
    {
    }
};

struct RlsPduTransmission : RlsMessage
{
    EPduType pduType{};
    uint32_t pduId{};
    uint32_t payload{};
    OctetString pdu{};

    explicit RlsPduTransmission(uint64_t sti) : RlsMessage(EMessageType::PDU_TRANSMISSION, sti)
    {
    }
};

struct RlsPduTransmissionAck : RlsMessage
{
    std::vector<uint32_t> pduIds;

    explicit RlsPduTransmissionAck(uint64_t sti) : RlsMessage(EMessageType::PDU_TRANSMISSION_ACK, sti)
    {
    }
};

int EncodeRlsMessage(const RlsMessage &msg, uint8_t *buffer);          // todo: remove
std::unique_ptr<RlsMessage> DecodeRlsMessage(const OctetView &stream); // todo: remove

void EncodeHeartbeat(CompoundBuffer &buffer, uint64_t sti, const Vector3 &simPos);
void EncodePduTransmissionAck(CompoundBuffer &buffer, uint64_t sti, const std::vector<uint32_t> &pduIds);
void EncodePduTransmission(CompoundBuffer &buffer, uint64_t sti, rls::EPduType pduType, uint32_t payload,
                           uint32_t pduId);

bool DecodeRlsHeader(const uint8_t *buffer, size_t size, EMessageType &msgType, uint64_t &sti);
void DecodeHeartbeatAck(const uint8_t *buffer, size_t size, int &dbm);
OctetView DecodePduTransmissionAck(const uint8_t *buffer, size_t size);
void DecodePduTransmission(const uint8_t *buffer, size_t size, EPduType &pduType, uint32_t &pduId, uint32_t &payload,
                           const uint8_t *&pduData, size_t &pduLength);

} // namespace rls