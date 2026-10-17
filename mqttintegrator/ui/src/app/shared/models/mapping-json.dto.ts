export interface JsonRootConfig {
  discover_prefix: string;
  connection: {
    keep_alive: number;
    client_id: string;
    clean_session: boolean;
    will_topic: string;
    will_message: string;
    will_qos: number;
    will_retain: boolean;
    username?: string;
    password?: string;
  };
  mapping: {
    plugins: string[];
    topic_level: JsonTopicNode[]; // top is array in your example
  };
}

type JsonTopicNodeBase = { name: string };

type JsonTopicNodeWithChildren = JsonTopicNodeBase & {
  topic_level: JsonTopicLevelUnion;
  subscription?: JsonSubscription;
};

type JsonTopicNodeWithSubscription = JsonTopicNodeBase & {
  subscription: JsonSubscription;
  topic_level?: JsonTopicLevelUnion;
};

export type JsonTopicNode = JsonTopicNodeWithChildren | JsonTopicNodeWithSubscription;

export type JsonTopicLevelUnion = JsonTopicNode | JsonTopicNode[];

export interface JsonSubscription {
  qos?: number;
  type?: string;

  static?: JsonStaticMapping[];
  value?: JsonMappingUnion;
  json?: JsonMappingUnion;
}

export interface JsonStaticMapping {
  mapped_topic: string;
  message_mapping: JsonMessageMappingUnion;
  qos?: number;
  retain?: boolean;
  delay?: number;
}

export type JsonMessageMappingUnion = JsonMessageMapping | JsonMessageMapping[];
export interface JsonMessageMapping {
  message: string;
  mapped_message: string;
}

export type JsonMappingUnion = JsonMappingEntry | JsonMappingEntry[];
export interface JsonMappingEntry {
  mapped_topic: string;
  mapping_template?: string;
  retain?: boolean;
  qos?: number;
  suppressions?: string[];
  mapping_template_also_works?: string;
  delay?: number;
}
