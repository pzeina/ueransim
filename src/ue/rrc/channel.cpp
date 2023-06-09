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

#include <asn/rrc/ASN_RRC_RRCReject.h>
#include <asn/rrc/ASN_RRC_RRCSetup.h>
#include <asn/rrc/ASN_RRC_UL-CCCH-Message.h>
#include <asn/rrc/ASN_RRC_UL-DCCH-Message.h>

namespace nr::ue
{

void UeRrcLayer::handleDownlinkRrc(int cellId, rrc::RrcChannel channel, const uint8_t *buffer, size_t size)
{
    if (!hasSignalToCell(cellId))
        return;

    switch (channel)
    {
    case rrc::RrcChannel::BCCH_BCH: {
        auto *pdu = rrc::encode::Decode<ASN_RRC_BCCH_BCH_Message>(asn_DEF_ASN_RRC_BCCH_BCH_Message, buffer, size);
        if (pdu == nullptr)
            m_logger->err("RRC BCCH-BCH PDU decoding failed.");
        else
            receiveRrcMessage(cellId, pdu);
        asn::Free(asn_DEF_ASN_RRC_BCCH_BCH_Message, pdu);
        break;
    }
    case rrc::RrcChannel::BCCH_DL_SCH: {
        auto *pdu = rrc::encode::Decode<ASN_RRC_BCCH_DL_SCH_Message>(asn_DEF_ASN_RRC_BCCH_DL_SCH_Message, buffer, size);
        if (pdu == nullptr)
            m_logger->err("RRC BCCH-DL-SCH PDU decoding failed.");
        else
            receiveRrcMessage(cellId, pdu);
        asn::Free(asn_DEF_ASN_RRC_BCCH_DL_SCH_Message, pdu);
        break;
    }
    case rrc::RrcChannel::DL_CCCH: {
        auto *pdu = rrc::encode::Decode<ASN_RRC_DL_CCCH_Message>(asn_DEF_ASN_RRC_DL_CCCH_Message, buffer, size);
        if (pdu == nullptr)
            m_logger->err("RRC DL-CCCH PDU decoding failed.");
        else
            receiveRrcMessage(cellId, pdu);
        asn::Free(asn_DEF_ASN_RRC_DL_CCCH_Message, pdu);
        break;
    }
    case rrc::RrcChannel::DL_DCCH: {
        if (isActiveCell(cellId))
        {
            auto *pdu = rrc::encode::Decode<ASN_RRC_DL_DCCH_Message>(asn_DEF_ASN_RRC_DL_DCCH_Message, buffer, size);
            if (pdu == nullptr)
                m_logger->err("RRC DL-DCCH PDU decoding failed.");
            else
                receiveRrcMessage(pdu);
            asn::Free(asn_DEF_ASN_RRC_DL_DCCH_Message, pdu);
        }
        break;
    };
    case rrc::RrcChannel::PCCH: {
        if (isActiveCell(cellId))
        {
            auto *pdu = rrc::encode::Decode<ASN_RRC_PCCH_Message>(asn_DEF_ASN_RRC_PCCH_Message, buffer, size);
            if (pdu == nullptr)
                m_logger->err("RRC PCCH PDU decoding failed.");
            else
                receiveRrcMessage(pdu);
            asn::Free(asn_DEF_ASN_RRC_PCCH_Message, pdu);
        }
        break;
    }
    case rrc::RrcChannel::UL_CCCH:
    case rrc::RrcChannel::UL_CCCH1:
    case rrc::RrcChannel::UL_DCCH:
        break;
    }
}

void UeRrcLayer::sendRrcMessage(int cellId, ASN_RRC_UL_CCCH_Message *msg)
{
    OctetString pdu = rrc::encode::EncodeS(asn_DEF_ASN_RRC_UL_CCCH_Message, msg);
    if (pdu.length() == 0)
    {
        m_logger->err("RRC UL-CCCH encoding failed.");
        return;
    }

    m_ue->rlsCtl->handleUplinkRrcDelivery(cellId, 0, rrc::RrcChannel::UL_CCCH, std::move(pdu));
}

void UeRrcLayer::sendRrcMessage(int cellId, ASN_RRC_UL_CCCH1_Message *msg)
{
    OctetString pdu = rrc::encode::EncodeS(asn_DEF_ASN_RRC_UL_CCCH1_Message, msg);
    if (pdu.length() == 0)
    {
        m_logger->err("RRC UL-CCCH1 encoding failed.");
        return;
    }

    m_ue->rlsCtl->handleUplinkRrcDelivery(cellId, 0, rrc::RrcChannel::UL_CCCH1, std::move(pdu));
}

void UeRrcLayer::sendRrcMessage(ASN_RRC_UL_DCCH_Message *msg)
{
    OctetString pdu = rrc::encode::EncodeS(asn_DEF_ASN_RRC_UL_DCCH_Message, msg);
    if (pdu.length() == 0)
    {
        m_logger->err("RRC UL-DCCH encoding failed.");
        return;
    }

    m_ue->rlsCtl->handleUplinkRrcDelivery(m_ue->shCtx.currentCell.cellId, 0, rrc::RrcChannel::UL_DCCH, std::move(pdu));
}

void UeRrcLayer::receiveRrcMessage(int cellId, ASN_RRC_BCCH_BCH_Message *msg)
{
    if (msg->message.present == ASN_RRC_BCCH_BCH_MessageType_PR_mib)
        receiveMib(cellId, *msg->message.choice.mib);
}

void UeRrcLayer::receiveRrcMessage(int cellId, ASN_RRC_BCCH_DL_SCH_Message *msg)
{
    if (msg->message.present != ASN_RRC_BCCH_DL_SCH_MessageType_PR_c1)
        return;

    auto &c1 = msg->message.choice.c1;
    switch (c1->present)
    {
    case ASN_RRC_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        receiveSib1(cellId, *c1->choice.systemInformationBlockType1);
        break;
    default:
        break;
    }
}

void UeRrcLayer::receiveRrcMessage(int cellId, ASN_RRC_DL_CCCH_Message *msg)
{
    if (msg->message.present != ASN_RRC_DL_CCCH_MessageType_PR_c1)
        return;

    auto &c1 = msg->message.choice.c1;
    switch (c1->present)
    {
    case ASN_RRC_DL_CCCH_MessageType__c1_PR_rrcReject:
        receiveRrcReject(cellId, *c1->choice.rrcReject);
        break;
    case ASN_RRC_DL_CCCH_MessageType__c1_PR_rrcSetup:
        receiveRrcSetup(cellId, *c1->choice.rrcSetup);
        break;
    default:
        break;
    }
}

void UeRrcLayer::receiveRrcMessage(ASN_RRC_DL_DCCH_Message *msg)
{
    if (msg->message.present != ASN_RRC_DL_DCCH_MessageType_PR_c1)
        return;

    auto &c1 = msg->message.choice.c1;
    switch (c1->present)
    {
    case ASN_RRC_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
        receiveDownlinkInformationTransfer(*c1->choice.dlInformationTransfer);
        break;
    case ASN_RRC_DL_DCCH_MessageType__c1_PR_rrcRelease:
        receiveRrcRelease(*c1->choice.rrcRelease);
        break;
    default:
        break;
    }
}

void UeRrcLayer::receiveRrcMessage(ASN_RRC_PCCH_Message *msg)
{
    if (msg->message.present != ASN_RRC_PCCH_MessageType_PR_c1)
        return;

    auto &c1 = msg->message.choice.c1;
    switch (c1->present)
    {
    case ASN_RRC_PCCH_MessageType__c1_PR_paging:
        receivePaging(*c1->choice.paging);
        break;
    default:
        break;
    }
}

} // namespace nr::ue
