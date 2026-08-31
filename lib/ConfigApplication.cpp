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

#include "ConfigApplication.h"

#include "MqttMapper.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "lib/SemanticLog.h"

#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
//
#include <nlohmann/json_fwd.hpp>

#endif

namespace mqtt::lib {

    template <typename ConcretConfigApplication>
    ConfigApplication::ConfigApplication(utils::SubCommand* parent, ConcretConfigApplication* concretConfigApplication)
        : utils::SubCommand(parent, concretConfigApplication, "Applications")
        , mqttMapper(std::make_shared<MqttMapper>())
        , mappingFileOpt(addOptionFunction(
              "--mqtt-mapping-file",
              [this](const std::string& mappFilename) {
                  try {
                      loadMapping(mappFilename);
                  } catch (std::runtime_error& e) {
                      this->mapFilename.clear();

                      throw CLI::ValidationError(getName(),
                                                 std::string("Activating mapping description in '" + mappFilename +
                                                             "' failed\n"
                                                             "What: " +
                                                             e.what()));
                  }
              },
              "MQTT mapping file (json format) for integration",
              "filename",
              !CLI::ExistingDirectory))
        , sessionStoreOpt(addOption("--mqtt-session-store",
                                    "Path to file for the persistent session store",
                                    "filename",
                                    !CLI::ExistingDirectory))
        , adminUserOpt(addOption("--admin-user",
                                 "HTTP administration user",
                                 "username",
                                 CLI::TypeValidator<std::string>()))
        , adminPasswordFileOpt(addOption("--admin-password-file",
                                         "File containing the HTTP administration password",
                                         "filename",
                                         CLI::ExistingFile))
        , adminRealmOpt(addOption("--admin-realm",
                                  "HTTP administration authentication realm",
                                  "realm",
                                  std::string("mqttsuite-admin"),
                                  CLI::TypeValidator<std::string>())) {
    }

    ConfigApplication::~ConfigApplication() = default;

    ConfigApplication& ConfigApplication::setSessionStore(const std::string& sessionStore) {
        setDefaultValue(sessionStoreOpt, sessionStore);

        return *this;
    }

    std::string ConfigApplication::getSessionStore() const {
        return sessionStoreOpt->as<std::string>();
    }

    const std::shared_ptr<MqttMapper> ConfigApplication::getMqttMapper() const {
        return mqttMapper;
    }

    bool ConfigApplication::setMappingFile(const std::string& mapFilename) {
        setDefaultValue(mappingFileOpt, mapFilename);

        return loadMapping(mapFilename);
    }

    std::string ConfigApplication::getMappingFilename() const {
        return mapFilename;
    }

    bool ConfigApplication::setMapping(const std::string& mapping) const {
        return mqttMapper->setMapping(nlohmann::json::parse(mapping));
    }

    std::string ConfigApplication::getMapping(int indent) const {
        return mqttMapper->getMapping().dump(indent);
    }

    bool ConfigApplication::persistMapping() const {
        bool success = false;

        if (mapFilename.empty()) {
            mqttsuite::semantic::mappingLog().debug() << "Cannot persist mapping: no mapping file selected";
            return false;
        }

        std::ofstream mapFile(mapFilename, std::ios::trunc);
        if (mapFile.is_open()) {
            try {
                mapFile << getMapping();

                success = true;

                mqttsuite::semantic::mappingLog().debug() << "Write mapping file success";
            } catch (const std::exception& e) {
                mqttsuite::semantic::mappingLog().debug() << "Write mapping file failed: " << e.what();
            }

            mapFile.close();
        } else {
            mqttsuite::semantic::mappingLog().debug() << "Cannot open mapping file for writing: " << mapFilename;
        }

        return success;
    }

    std::string ConfigApplication::getAdminUser() const {
        return adminUserOpt->as<std::string>();
    }

    std::string ConfigApplication::getAdminPassword() const {
        return readAdminPassword();
    }

    std::string ConfigApplication::getAdminRealm() const {
        return adminRealmOpt->as<std::string>();
    }

    bool ConfigApplication::hasAdminAuthentication() const {
        return !getAdminUser().empty() && !adminPasswordFileOpt->as<std::string>().empty();
    }

    std::string ConfigApplication::readAdminPassword() const {
        const std::string passwordFileName = adminPasswordFileOpt->as<std::string>();
        if (passwordFileName.empty()) {
            return {};
        }

        std::ifstream passwordFile(passwordFileName);
        if (!passwordFile.is_open()) {
            throw std::runtime_error("Cannot open administration password file");
        }

        std::string password;
        std::getline(passwordFile, password);
        if (!password.empty() && password.back() == '\r') {
            password.pop_back();
        }
        if (password.empty()) {
            throw std::runtime_error("Administration password file is empty");
        }

        return password;
    }

    bool ConfigApplication::loadMapping(const std::string mapFilename) {
        mqttsuite::semantic::mappingLog().debug() << "Mapping file: " << mapFilename;

        this->mapFilename = mapFilename;

        bool mustReconnect = true;

        if (!mapFilename.empty()) {
            std::ifstream mapFile(mapFilename);

            if (mapFile.is_open()) {
                try {
                    mustReconnect = setMapping({std::istreambuf_iterator<char>(mapFile), std::istreambuf_iterator<char>()});

                    mapFile.close();

                    mqttsuite::semantic::mappingLog().debug() << "Load mapping file success";
                } catch (const std::exception& e) {
                    mapFile.close();

                    throw std::runtime_error("Loading mapping description from '" + mapFilename +
                                             "' failed\n"
                                             "What: " +
                                             e.what());
                }

            } else {
                throw std::runtime_error("Mapping file cannot be opened");
            }
        }

        return mustReconnect;
    }

    ConfigMqttBroker::ConfigMqttBroker(utils::SubCommand* parent)
        : ConfigApplication(parent, this)
        , htmlRootOpt(addOption("--html-root", "HTML root directory", "directory", CLI::ExistingDirectory)) {
        required(htmlRootOpt);
    }

    ConfigMqttBroker::~ConfigMqttBroker() = default;

    ConfigMqttBroker& ConfigMqttBroker::setHtmlRoot(const std::string& htmlRoot) {
        setDefaultValue(htmlRootOpt, htmlRoot);
        required(htmlRootOpt, false);

        return *this;
    }

    std::string ConfigMqttBroker::getHtmlRoot() {
        return htmlRootOpt->as<std::string>();
    }

    ConfigMqttIntegrator::ConfigMqttIntegrator(utils::SubCommand* parent)
        : ConfigApplication(parent, this) {
    }

    ConfigMqttIntegrator::~ConfigMqttIntegrator() = default;

} // namespace mqtt::lib
