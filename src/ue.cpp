//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include <iostream>
#include <memory>
#include <stdexcept>

#include <unistd.h>

#include <lib/app/base_app.hpp>
#include <lib/app/cli_base.hpp>
#include <lib/app/cli_cmd.hpp>
#include <ue/task.hpp>
#include <ue/types.hpp>
#include <utils/common.hpp>
#include <utils/constants.hpp>
#include <utils/options.hpp>
#include <utils/yaml_utils.hpp>
#include <yaml-cpp/yaml.h>

static nr::ue::UeConfig *g_refConfig = nullptr;

static struct Options
{
    std::string configFile{};
    bool noRoutingConfigs{};
    bool disableCmd{};
    std::string imsi{};
    int count{};
} g_options{};

static nr::ue::UeConfig *ReadConfigYaml()
{
    auto *result = new nr::ue::UeConfig();
    auto config = YAML::LoadFile(g_options.configFile);

    result->hplmn.mcc = yaml::GetInt32(config, "mcc", 1, 999);
    yaml::GetString(config, "mcc", 3, 3);
    result->hplmn.mnc = yaml::GetInt32(config, "mnc", 0, 999);
    result->hplmn.isLongMnc = yaml::GetString(config, "mnc", 2, 3).size() == 3;

    for (auto &gnbSearchItem : yaml::GetSequence(config, "gnbSearchList"))
        result->gnbSearchList.push_back(gnbSearchItem.as<std::string>());

    if (yaml::HasField(config, "default-nssai"))
    {
        for (auto &sNssai : yaml::GetSequence(config, "default-nssai"))
        {
            SingleSlice s{};
            s.sst = yaml::GetInt32(sNssai, "sst", 0, 0xFF);
            if (yaml::HasField(sNssai, "sd"))
                s.sd = octet3{yaml::GetInt32(sNssai, "sd", 0, 0xFFFFFF)};
            result->defaultConfiguredNssai.slices.push_back(s);
        }
    }

    if (yaml::HasField(config, "configured-nssai"))
    {
        for (auto &sNssai : yaml::GetSequence(config, "configured-nssai"))
        {
            SingleSlice s{};
            s.sst = yaml::GetInt32(sNssai, "sst", 0, 0xFF);
            if (yaml::HasField(sNssai, "sd"))
                s.sd = octet3{yaml::GetInt32(sNssai, "sd", 0, 0xFFFFFF)};
            result->configuredNssai.slices.push_back(s);
        }
    }

    result->key = OctetString::FromHex(yaml::GetString(config, "key", 32, 32));
    result->opC = OctetString::FromHex(yaml::GetString(config, "op", 32, 32));
    result->amf = OctetString::FromHex(yaml::GetString(config, "amf", 4, 4));

    result->configureRouting = !g_options.noRoutingConfigs;

    // If we have multiple UEs in the same process, then log names should be separated.
    result->prefixLogger = g_options.count > 1;

    result->disableCmd = g_options.disableCmd;

    if (yaml::HasField(config, "supi"))
        result->supi = Supi::Parse(yaml::GetString(config, "supi"));
    if (yaml::HasField(config, "imei"))
        result->imei = yaml::GetString(config, "imei", 15, 15);
    if (yaml::HasField(config, "imeiSv"))
        result->imeiSv = yaml::GetString(config, "imeiSv", 16, 16);

    yaml::AssertHasField(config, "integrity");
    yaml::AssertHasField(config, "ciphering");

    result->supportedAlgs.nia1 = yaml::GetBool(config["integrity"], "IA1");
    result->supportedAlgs.nia2 = yaml::GetBool(config["integrity"], "IA2");
    result->supportedAlgs.nia3 = yaml::GetBool(config["integrity"], "IA3");
    result->supportedAlgs.nea1 = yaml::GetBool(config["ciphering"], "EA1");
    result->supportedAlgs.nea2 = yaml::GetBool(config["ciphering"], "EA2");
    result->supportedAlgs.nea3 = yaml::GetBool(config["ciphering"], "EA3");

    std::string opType = yaml::GetString(config, "opType");
    if (opType == "OP")
        result->opType = nr::ue::OpType::OP;
    else if (opType == "OPC")
        result->opType = nr::ue::OpType::OPC;
    else
        throw std::runtime_error("Invalid OP type: " + opType);

    if (yaml::HasField(config, "sessions"))
    {
        for (auto &sess : yaml::GetSequence(config, "sessions"))
        {
            nr::ue::SessionConfig s{};

            if (yaml::HasField(sess, "apn"))
                s.apn = yaml::GetString(sess, "apn");
            if (yaml::HasField(sess, "slice"))
            {
                auto slice = sess["slice"];
                s.sNssai = SingleSlice{};
                s.sNssai->sst = yaml::GetInt32(slice, "sst", 0, 0xFF);
                if (yaml::HasField(slice, "sd"))
                    s.sNssai->sd = octet3{yaml::GetInt32(slice, "sd", 0, 0xFFFFFF)};
            }

            std::string type = yaml::GetString(sess, "type");
            if (type == "IPv4")
                s.type = nas::EPduSessionType::IPV4;
            else if (type == "IPv6")
                s.type = nas::EPduSessionType::IPV6;
            else if (type == "IPv4v6")
                s.type = nas::EPduSessionType::IPV4V6;
            else if (type == "Ethernet")
                s.type = nas::EPduSessionType::ETHERNET;
            else if (type == "Unstructured")
                s.type = nas::EPduSessionType::UNSTRUCTURED;
            else
                throw std::runtime_error("Invalid PDU session type: " + type);

            s.isEmergency = false;

            result->defaultSessions.push_back(s);
        }
    }

    yaml::AssertHasField(config, "integrityMaxRate");
    {
        auto uplink = yaml::GetString(config["integrityMaxRate"], "uplink");
        auto downlink = yaml::GetString(config["integrityMaxRate"], "downlink");
        if (uplink != "full" && uplink != "64kbps")
            throw std::runtime_error("Invalid integrity protection maximum uplink data rate: " + uplink);
        if (downlink != "full" && downlink != "64kbps")
            throw std::runtime_error("Invalid integrity protection maximum downlink data rate: " + downlink);
        result->integrityMaxRate.uplinkFull = uplink == "full";
        result->integrityMaxRate.downlinkFull = downlink == "full";
    }

    yaml::AssertHasField(config, "uacAic");
    {
        result->uacAic.mps = yaml::GetBool(config["uacAic"], "mps");
        result->uacAic.mcs = yaml::GetBool(config["uacAic"], "mcs");
    }

    yaml::AssertHasField(config, "uacAcc");
    {
        result->uacAcc.normalCls = yaml::GetInt32(config["uacAcc"], "normalClass", 0, 9);
        result->uacAcc.cls11 = yaml::GetBool(config["uacAcc"], "class11");
        result->uacAcc.cls12 = yaml::GetBool(config["uacAcc"], "class12");
        result->uacAcc.cls13 = yaml::GetBool(config["uacAcc"], "class13");
        result->uacAcc.cls14 = yaml::GetBool(config["uacAcc"], "class14");
        result->uacAcc.cls15 = yaml::GetBool(config["uacAcc"], "class15");
    }

    return result;
}

static void ReadOptions(int argc, char **argv)
{
    opt::OptionsDescription desc{
        cons::Project, cons::Tag, cons::DescriptionUe, cons::Owner, "nr-ue", {"-c <config-file> [option...]"}, {},
        true,          false};

    opt::OptionItem itemConfigFile = {'c', "config", "Use specified configuration file for UE", "config-file"};
    opt::OptionItem itemImsi = {'i', "imsi", "Use specified IMSI number instead of provided one", "imsi"};
    opt::OptionItem itemCount = {'n', "num-of-UE", "Generate specified number of UEs starting from the given IMSI",
                                 "num"};
    opt::OptionItem itemDisableCmd = {'l', "disable-cmd", "Disable command line functionality for this instance",
                                      std::nullopt};
    opt::OptionItem itemDisableRouting = {'r', "no-routing-config",
                                          "Do not auto configure routing for UE TUN interface", std::nullopt};

    desc.items.push_back(itemConfigFile);
    desc.items.push_back(itemImsi);
    desc.items.push_back(itemCount);
    desc.items.push_back(itemDisableCmd);
    desc.items.push_back(itemDisableRouting);

    opt::OptionsResult opt{argc, argv, desc, false, nullptr};

    g_options.configFile = opt.getOption(itemConfigFile);
    g_options.noRoutingConfigs = opt.hasFlag(itemDisableRouting);
    if (opt.hasFlag(itemCount))
    {
        g_options.count = utils::ParseInt(opt.getOption(itemCount));
        if (g_options.count <= 0)
            throw std::runtime_error("Invalid number of UEs");
        if (g_options.count > 512)
            throw std::runtime_error("Number of UEs is too big");
    }
    else
    {
        g_options.count = 1;
    }

    g_options.imsi = {};
    if (opt.hasFlag(itemImsi))
    {
        g_options.imsi = opt.getOption(itemImsi);
        if (g_options.imsi.length() > 5 && g_options.imsi[0] == 'i' && g_options.imsi[1] == 'm' &&
            g_options.imsi[2] == 's' && g_options.imsi[3] == 'i' && g_options.imsi[4] == '-')
        {
            g_options.imsi = g_options.imsi.substr(5);
        }

        Supi::Parse("imsi-" + g_options.imsi); // validate the string by parsing
    }

    g_options.disableCmd = opt.hasFlag(itemDisableCmd);
}

static std::string LargeSum(std::string a, std::string b)
{
    if (a.length() > b.length())
        std::swap(a, b);

    std::string str;
    size_t n1 = a.length(), n2 = b.length();

    reverse(a.begin(), a.end());
    reverse(b.begin(), b.end());

    int carry = 0;
    for (size_t i = 0; i < n1; i++)
    {
        int sum = ((a[i] - '0') + (b[i] - '0') + carry);
        str.push_back(static_cast<char>((sum % 10) + '0'));
        carry = sum / 10;
    }
    for (size_t i = n1; i < n2; i++)
    {
        int sum = ((b[i] - '0') + carry);
        str.push_back(static_cast<char>((sum % 10) + '0'));
        carry = sum / 10;
    }
    if (carry)
        throw std::runtime_error("UE serial number overflow");
    reverse(str.begin(), str.end());
    return str;
}

static void IncrementNumber(std::string &s, int delta)
{
    s = LargeSum(s, std::to_string(delta));
}

static std::unique_ptr<nr::ue::UeConfig> GetConfigByUe(int ueIndex)
{
    auto c = std::make_unique<nr::ue::UeConfig>();
    c->key = g_refConfig->key.copy();
    c->opC = g_refConfig->opC.copy();
    c->opType = g_refConfig->opType;
    c->amf = g_refConfig->amf.copy();
    c->imei = g_refConfig->imei;
    c->imeiSv = g_refConfig->imeiSv;
    c->supi = g_refConfig->supi;
    c->hplmn = g_refConfig->hplmn;
    c->configuredNssai = g_refConfig->configuredNssai;
    c->defaultConfiguredNssai = g_refConfig->defaultConfiguredNssai;
    c->supportedAlgs = g_refConfig->supportedAlgs;
    c->gnbSearchList = g_refConfig->gnbSearchList;
    c->defaultSessions = g_refConfig->defaultSessions;
    c->configureRouting = g_refConfig->configureRouting;
    c->prefixLogger = g_refConfig->prefixLogger;
    c->disableCmd = g_refConfig->disableCmd;
    c->integrityMaxRate = g_refConfig->integrityMaxRate;
    c->uacAic = g_refConfig->uacAic;
    c->uacAcc = g_refConfig->uacAcc;

    if (c->supi.has_value())
        IncrementNumber(c->supi->value, ueIndex);
    if (c->imei.has_value())
        IncrementNumber(*c->imei, ueIndex);
    if (c->imeiSv.has_value())
        IncrementNumber(*c->imeiSv, ueIndex);

    return c;
}

static void ExecuteUeTask(std::unique_ptr<nr::ue::UeTask> &ue)
{
    ue->onStart();
    while (true)
    {
        if (ue->onLoop())
            break;
    }
    ue->onQuit();
    ue = nullptr;
}

static void ExecuteUeTasks(std::vector<std::unique_ptr<nr::ue::UeTask>> &v)
{
    std::vector<std::thread> threads;

    for (size_t i = 1; i < v.size(); i++)
        threads.emplace_back([&v, i]() { ExecuteUeTask(v[i]); });

    ExecuteUeTask(v[0]);

    for (auto &thread : threads)
        thread.join();
}

int main(int argc, char **argv)
{
    app::Initialize();

    try
    {
        ReadOptions(argc, argv);
        g_refConfig = ReadConfigYaml();
        if (g_options.imsi.length() > 0)
            g_refConfig->supi = Supi::Parse("imsi-" + g_options.imsi);
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::cout << utils::CopyrightDeclarationUe() << std::endl;

    std::vector<std::unique_ptr<nr::ue::UeTask>> ueTasks;

    for (int i = 0; i < g_options.count; i++)
    {
        auto config = GetConfigByUe(i);
        auto nodeName = config->getNodeName();
        ueTasks.push_back(std::make_unique<nr::ue::UeTask>(std::move(config)));
    }

    ExecuteUeTasks(ueTasks);
    return 0;
}
