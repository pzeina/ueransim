//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "server_task.hpp"

#include <cstring>

#define BUFFER_SIZE 65536
#define TIMEOUT_MS 500

udp::UdpServerTask::UdpServerTask(NtsTask *targetTask) : server{}, targetTask(targetTask)
{
    server = new UdpServer();
}

udp::UdpServerTask::UdpServerTask(const std::string &address, uint16_t port, NtsTask *targetTask)
    : server{}, targetTask(targetTask)
{
    server = new UdpServer(address, port);
}

udp::UdpServerTask::~UdpServerTask() = default;

void udp::UdpServerTask::onStart()
{
}

void udp::UdpServerTask::onLoop()
{
    uint8_t buffer[BUFFER_SIZE];

    InetAddress peerAddress{};

    int size = server->Receive(buffer, BUFFER_SIZE, TIMEOUT_MS, peerAddress);
    if (size > 0)
    {
        targetTask->push(std::make_unique<NwUdpServerReceive>(OctetString::FromArray(buffer, static_cast<size_t>(size)),
                                                              peerAddress));
    }
}

void udp::UdpServerTask::onQuit()
{
    delete server;
}

void udp::UdpServerTask::send(const InetAddress &to, const OctetString &packet)
{
    send(to, packet.data(), packet.length());
}

void udp::UdpServerTask::send(const InetAddress &to, const uint8_t *buffer, size_t bufferSize)
{
    server->Send(to, buffer, bufferSize);
}
