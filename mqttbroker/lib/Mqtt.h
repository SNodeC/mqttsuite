#ifndef MQTTBROKER_LIB_MQTT_H
#define MQTTBROKER_LIB_MQTT_H

#include "lib/MqttMapper.h"

namespace iot::mqtt::server::broker {
    class Broker;
}

#include <core/timer/Timer.h>
#include <iot/mqtt/server/Mqtt.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::mqttbroker::lib {

    class Mqtt : public iot::mqtt::server::Mqtt {
    public:
        explicit Mqtt(const std::string& connectionName,
                      const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                      mqtt::lib::MqttMapper* mqttMapper);

        void subscribe(const std::string& topic, uint8_t qoS);
        void unsubscribe(const std::string& topic);

    private:
        struct ScheduledPublish {
            utils::Timeval when;
            std::size_t seq;
            iot::mqtt::packets::Publish publish;
            utils::Timeval delay;
        };

        struct EarlierFirst {
            bool operator()(const ScheduledPublish& a, const ScheduledPublish& b) const;
        };

        class DelayedQueue {
        public:
            explicit DelayedQueue(Mqtt* mqtt);
            ~DelayedQueue();

            void delayPublish(const utils::Timeval& delay, const iot::mqtt::packets::Publish& publish);

            bool empty() const;
            ScheduledPublish const& top() const;
            void pop();

        private:
            Mqtt* mqtt;
            std::size_t nextSeq = 0;
            std::priority_queue<ScheduledPublish, std::vector<ScheduledPublish>, EarlierFirst> minHeap;

            core::timer::Timer delayTimer;
            void processDue();
            void armDelayTimer();
        };

        void onConnect(const iot::mqtt::packets::Connect& connect) final;
        void onPublish(const iot::mqtt::packets::Publish& publish) final;
        void onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) final;
        void onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) final;
        void onDisconnected() final;

        mqtt::lib::MqttMapper* mqttMapper;
        DelayedQueue delayedQueue;
    };

} // namespace mqtt::mqttbroker::lib

#endif // MQTTBROKER_LIB_MQTT_H
