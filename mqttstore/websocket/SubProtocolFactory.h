#ifndef MQTTSTORE_WEBSOCKET_SUBPROTOCOLFACTORY_H
#define MQTTSTORE_WEBSOCKET_SUBPROTOCOLFACTORY_H

#include <iot/mqtt/client/SubProtocolFactory.h>

namespace mqtt::mqttstore::websocket {

    class SubProtocolFactory : public iot::mqtt::client::SubProtocolFactory {
    public:
        SubProtocolFactory();

    private:
        iot::mqtt::client::SubProtocol* create(web::websocket::SubProtocolContext* subProtocolContext) final;
    };

} // namespace mqtt::mqttstore::websocket

#endif // MQTTSTORE_WEBSOCKET_SUBPROTOCOLFACTORY_H
