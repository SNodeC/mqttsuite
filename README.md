# MQTT-Suite: A very lightweight MQTT-Integration System

The MQTT-Suite project consist of five applications *mqttbroker*, *mqttintegrator* and *mqttbridge* powered by *[SNode.C](https://github.com/VolkerChristian/snode.c)*, a single threaded, single tasking framework for networking applications written entirely in C++. Due to it's little resource usage it is especially usable on resource limited systems.

- **mqttbroker**: Is a full featured MQTT-broker utilizing version [3.1.1](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) of the MQTT protocol standard. It accepts incoming plain and WebSockets MQTT connections via unencrypted IPv4 and SSL/TLS-encrypted IPv4.
  In addition it provides a rudimentary Web-Interface showing all currently connected MQTT-clients.
- **mqttintegrator**: Is a IoT-integration application controlled by a JSON-mapping description. It connects via SSL/TLS to an MQTT-broker.
- **mqttbridge**: A purely client side bridge. It can establish multiple bridges each connecting to multiple mqttbrokers and bridge configurable topics.



