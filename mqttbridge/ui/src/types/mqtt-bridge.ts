// Type definitions based on the JSON schema

export type Protocol = "in" | "in6" | "l2" | "rc" | "un";
export type Encryption = "legacy" | "tls";
export type Transport = "stream" | "websocket";

export interface Topic {
  topic: string;
  qos: 0 | 1 | 2;
}

export interface Mqtt {
  client_id: string;
  keep_alive: number;
  clean_session: boolean;
  will_topic: string;
  will_message: string;
  will_qos: 0 | 1 | 2;
  will_retain: boolean;
  username: string;
  password: string;
  loop_prevention: boolean;
}

// Network address types
export interface NetIn {
  host: string;
  port: number;
}

export interface NetIn6 {
  host: string;
  port: number;
}

export interface NetRc {
  host: string;
  channel: number;
}

export interface NetL2 {
  host: string;
  psm: number;
}

export interface NetUn {
  path: string;
}

export interface Network {
  instance_name: string;
  protocol: Protocol;
  encryption: Encryption;
  transport: Transport;
  in?: NetIn;
  in6?: NetIn6;
  rc?: NetRc;
  l2?: NetL2;
  un?: NetUn;
}

export interface Broker {
  disabled?: boolean;
  session_store: string;
  mqtt?: Mqtt;
  network: Network;
  prefix?: string;
  topics: Topic[];
}

export interface Bridge {
  disabled?: boolean;
  name: string;
  prefix?: string;
  brokers: Broker[];
}

export interface BridgesConfig {
  bridges: Bridge[];
}

// Default values
export const defaultMqtt: Mqtt = {
  client_id: "",
  keep_alive: 60,
  clean_session: true,
  will_topic: "",
  will_message: "",
  will_qos: 0,
  will_retain: false,
  username: "",
  password: "",
  loop_prevention: false,
};

export const defaultTopic: Topic = {
  topic: "#",
  qos: 0,
};

export const defaultNetwork: Network = {
  instance_name: "",
  protocol: "in",
  encryption: "legacy",
  transport: "stream",
  in: {
    host: "",
    port: 1883,
  },
};

export const defaultBroker: Broker = {
  session_store: "",
  mqtt: { ...defaultMqtt },
  network: { ...defaultNetwork },
  topics: [{ ...defaultTopic }],
};

export const defaultBridge: Bridge = {
  name: "",
  brokers: [{ ...defaultBroker }, { ...defaultBroker }],
};