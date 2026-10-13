/* ============================================================
   MQTT Integrator – UI model (camelCase, normalized)
   ============================================================ */

export type QoS = 0 | 1 | 2;

export type SubscriptionKind = "static" | "value" | "json";

/* ----------------------------- Root ----------------------------- */

export interface IntegratorUI {
  discoverPrefix: string;
  connection: ConnectionUI;
  mapping: MappingUI;
}

/* -------------------------- Connection -------------------------- */

export interface ConnectionUI {
  keepAlive: number;
  clientId: string;
  cleanSession: boolean;

  willTopic: string;
  willMessage: string;
  willQos: QoS;
  willRetain: boolean;

  username: string;
  password: string;
}

/* ---------------------------- Mapping --------------------------- */

export interface MappingUI {
  plugins: string[];
  topicLevels: TopicLevelUI[];
}

/* --------------------------- Topic Tree ------------------------- */

/**
 * UI-normalized topic node
 * - children always an array
 * - subscription only for leaf nodes
 */
export interface TopicLevelUI {
  /** UI-only identifier (not in schema) */
  id: string;

  name: string;

  /** child topic levels */
  children: TopicLevelUI[];

  /** leaf subscription; null for branches */
  subscription: SubscriptionUI | null;
}

/* -------------------------- Subscription ------------------------ */

export type SubscriptionUI =
  | StaticSubscriptionUI
  | ValueSubscriptionUI
  | JsonSubscriptionUI;

export interface SubscriptionBaseUI {
  kind: SubscriptionKind;
  qos: QoS;
  type: string;
}

export interface StaticSubscriptionUI extends SubscriptionBaseUI {
  kind: "static";
  mappings: StaticMappingUI[];
}

export interface ValueSubscriptionUI extends SubscriptionBaseUI {
  kind: "value";
  mappings: ValueTemplateMappingUI[];
}

export interface JsonSubscriptionUI extends SubscriptionBaseUI {
  kind: "json";
  mappings: ValueTemplateMappingUI[];
}

/* ---------------------------- Mappings -------------------------- */

export interface MappingCommonsUI {
  mappedTopic: string;
  retain?: boolean;
  qos?: QoS | undefined;
  delay?: number | null;
}

export interface StaticMappingUI extends MappingCommonsUI {
  messageMappings: MessageMappingUI[];
}

export interface MessageMappingUI {
  message: string;
  mappedMessage: string;
}

export interface ValueTemplateMappingUI extends MappingCommonsUI {
  mappingTemplate: string;
  suppressions?: string[];
}
