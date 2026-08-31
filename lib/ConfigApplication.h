/*
 * MQTTSuite - A lightweight MQTT Integration System
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H
#define APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H

namespace mqtt::lib {
    class MqttMapper;
}

#include <utils/SubCommand.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>
#include <string_view>

#endif

namespace mqtt::lib {

    class ConfigApplication : public utils::SubCommand {
    public:
        template <typename ConcretConfigApplicationT>
        ConfigApplication(utils::SubCommand* parent, ConcretConfigApplicationT* concretConfigApplication);
        ~ConfigApplication() override;

        ConfigApplication& setSessionStore(const std::string& sessionStore);
        std::string getSessionStore() const;

        const std::shared_ptr<MqttMapper> getMqttMapper() const;
        bool setMappingFile(const std::string& mapFilename); // can throw
        std::string getMappingFilename() const;
        bool setMapping(const std::string& mapping) const; // can throw
        std::string getMapping(int indent = 2) const;

        bool persistMapping() const;

        [[nodiscard]] std::string getAdminUser() const;
        [[nodiscard]] std::string getAdminPassword() const;
        [[nodiscard]] std::string getAdminRealm() const;
        [[nodiscard]] bool hasAdminAuthentication() const;

    private:
        bool loadMapping(const std::string mapFilename); // can throw
        [[nodiscard]] std::string readAdminPassword() const;

    protected:
        std::shared_ptr<MqttMapper> mqttMapper;

        CLI::Option* mappingFileOpt;
        CLI::Option* sessionStoreOpt;
        CLI::Option* adminUserOpt;
        CLI::Option* adminPasswordFileOpt;
        CLI::Option* adminRealmOpt;

    private:
        std::string mapFilename;
    };

    class ConfigMqttBroker : public ConfigApplication {
    public:
        constexpr static std::string_view NAME{"broker"};
        constexpr static std::string_view DESCRIPTION{"Configuration for Application mqttbroker"};

        ConfigMqttBroker(utils::SubCommand* parent);

        ~ConfigMqttBroker() override;

        ConfigMqttBroker& setHtmlRoot(const std::string& htmlRoot);
        std::string getHtmlRoot();

    private:
        CLI::Option* htmlRootOpt;
    };

    class ConfigMqttIntegrator : public ConfigApplication {
    public:
        constexpr static std::string_view NAME{"integrator"};
        constexpr static std::string_view DESCRIPTION{"Configuration for Application mqttintegrator"};

        ConfigMqttIntegrator(utils::SubCommand* parent);

        ~ConfigMqttIntegrator() override;
    };

} // namespace mqtt::lib

#endif // APPS_MQTTBROKER_MQTTBRIDGE_CONFIGBRIDGE_H
