import {
    JsonRootConfig,
    JsonTopicNode,
    JsonTopicLevelUnion,
    JsonSubscription,
    JsonMappingUnion,
    JsonStaticMapping,
    JsonMessageMappingUnion,
    JsonMappingEntry,
    JsonMessageMapping,
} from '../models/mapping-json.dto';

import {
    IntegratorUI,
    ConnectionUI,
    MappingUI,
    TopicLevelUI,
    SubscriptionUI,
    StaticSubscriptionUI,
    ValueSubscriptionUI,
    JsonSubscriptionUI,
    StaticMappingUI,
    ValueTemplateMappingUI,
    MessageMappingUI,
    QoS,
} from '../models/mapping-model';
import { createTopicId } from '../utils/id.util';


export interface JsonTopicNodeLoose {
    name: string;
    subscription?: JsonSubscription;
    topic_level?: JsonTopicLevelUnion;
}

export type JsonTopicLevelUnionLoose = JsonTopicNodeLoose | JsonTopicNodeLoose[];

export type JsonMappingLoose =
    Omit<JsonRootConfig["mapping"], "topic_level"> & {
        topic_level: JsonTopicLevelUnionLoose;
    };


export type JsonRootConfigLoose =
  Omit<JsonRootConfig, "mapping"> & {
    mapping: JsonMappingLoose;
  };

export class MappingAdapter {

    /* ============================================================
       ROOT
       ============================================================ */

    static fromJson(json: JsonRootConfig): IntegratorUI {
        return {
            discoverPrefix: json.discover_prefix,
            connection: this.connectionFromJson(json.connection),
            mapping: {
                plugins: json.mapping.plugins ?? [],
                topicLevels: this.topicUnionToArray(json.mapping.topic_level)
                    .map((node) => this.topicNodeFromJson(node)),
            }
        };
    }

    static toJsonStrict(model: IntegratorUI): JsonRootConfig {
        return {
            discover_prefix: model.discoverPrefix ?? "",
            connection: this.connectionToJson(model.connection),
            mapping: {
                plugins: model.mapping?.plugins ?? [],
                topic_level: (model.mapping?.topicLevels ?? []).map(n => this.topicNodeToJson(n, true)),
            },
        };
    }

    static toJsonLoose(model: IntegratorUI): JsonRootConfigLoose {
        return {
            discover_prefix: model.discoverPrefix ?? "",
            connection: this.connectionToJson(model.connection),
            mapping: {
                plugins: model.mapping?.plugins ?? [],
                topic_level: (model.mapping?.topicLevels ?? []).map(n => this.topicNodeToJson(n, false)),
            },
        };
    }



    /* ============================================================
       CONNECTION
       ============================================================ */

    private static connectionFromJson(json: JsonRootConfig['connection']): ConnectionUI {
        return {
            keepAlive: json.keep_alive,
            clientId: json.client_id,
            cleanSession: json.clean_session,
            willTopic: json.will_topic,
            willMessage: json.will_message,
            willQos: json.will_qos as QoS,
            willRetain: json.will_retain,
            username: json.username ?? '',
            password: json.password ?? '',
        };
    }

    static connectionToJson(
        conn: IntegratorUI["connection"]
    ): JsonRootConfig["connection"] {
        const base: JsonRootConfig["connection"] = {
            keep_alive: conn.keepAlive ?? 60,
            client_id: conn.clientId ?? "",
            clean_session: conn.cleanSession ?? false,
            will_topic: conn.willTopic ?? "",
            will_message: conn.willMessage ?? "",
            will_qos: conn.willQos ?? 0,
            will_retain: conn.willRetain ?? false,
        };

        // username/password are optional in JSON dto
        if (conn.username?.trim()) base.username = conn.username;
        if (conn.password?.trim()) base.password = conn.password;

        return base;
    }

    /* ============================================================
       TOPIC TREE
       ============================================================ */

    private static topicNodeFromJson(node: JsonTopicNode): TopicLevelUI {

        const children = this.topicUnionToArray(node.topic_level);

        return {
            id: createTopicId(),               // ✅ random stable ID
            name: node.name,
            children: children.map(child => this.topicNodeFromJson(child)),
            subscription: node.subscription ? this.subscriptionFromJson(node.subscription) : null,
        };
    }


    private static topicNodeToJson(node: TopicLevelUI, strict: true): JsonTopicNode;
    private static topicNodeToJson(node: TopicLevelUI, strict: false): JsonTopicNodeLoose;
    private static topicNodeToJson(node: TopicLevelUI, strict: boolean): JsonTopicNode | JsonTopicNodeLoose;
    private static topicNodeToJson(node: TopicLevelUI, strict: boolean): JsonTopicNode | JsonTopicNodeLoose {
        const name = node.name ?? "";

        const hasChildren = !!node.children?.length;
        const hasSub = !!node.subscription;

        // Build children first if present
        const topic_level = hasChildren
            ? (() => {
                const mapped = node.children.map(c => this.topicNodeToJson(c, strict as any));
                return (mapped.length === 1 ? mapped[0] : mapped) as JsonTopicLevelUnion;
            })()
            : undefined;

        const subscription = hasSub ? this.subscriptionToJson(node.subscription!) : undefined;

        if (strict) {
            if (!topic_level && !subscription) {
                throw new Error(`Invalid topic node "${name}": must have children or subscription.`);
            }
            // return the strict union members
            if (topic_level && subscription) return { name, topic_level, subscription };
            if (topic_level) return { name, topic_level };
            return { name, subscription: subscription! };
        }

        // non-strict: allow empty node
        const out: JsonTopicNodeLoose = { name };
        if (topic_level) out.topic_level = topic_level;
        if (subscription) out.subscription = subscription;
        return out;
    }


    private static topicUnionToArray(union?: JsonTopicLevelUnion): JsonTopicNode[] {
        if (!union) return [];
        return Array.isArray(union) ? union : [union];
    }

    /* ============================================================
       SUBSCRIPTIONS
       ============================================================ */

    private static subscriptionFromJson(json: JsonSubscription): SubscriptionUI {
        const qos = (json.qos ?? 0) as QoS;
        const type = json.type ?? '';

        if (json.static) {
            const mappings = this.normalizeArray(json.static)
                .map(m => this.staticMappingFromJson(m));

            const sub: StaticSubscriptionUI = {
                kind: 'static',
                qos,
                type,
                mappings,
            };
            return sub;
        }

        if (json.value) {
            const mappings = this.normalizeArray(json.value)
                .map(m => this.templateMappingFromJson(m));

            const sub: ValueSubscriptionUI = {
                kind: 'value',
                qos,
                type,
                mappings,
            };
            return sub;
        }

        if (json.json) {
            const mappings = this.normalizeArray(json.json)
                .map(m => this.templateMappingFromJson(m));

            const sub: JsonSubscriptionUI = {
                kind: 'json',
                qos,
                type,
                mappings,
            };
            return sub;
        }

        throw new Error('Invalid subscription: no mapping type found');
    }

    private static subscriptionToJson(sub: SubscriptionUI): JsonSubscription {
        const out: JsonSubscription = {};

        // only include if not default-ish
        if (sub.qos !== undefined && sub.qos !== 0) out.qos = sub.qos;
        if (sub.type?.trim()) out.type = sub.type;

        switch (sub.kind) {
            case "static": {
                const s = sub as StaticSubscriptionUI;
                out.static = s.mappings.map((m) => this.staticMappingToJson(m));
                // static is always an array in your dto, keep it that way
                return out;
            }

            case "value": {
                const v = sub as ValueSubscriptionUI;
                out.value = this.mappingEntriesToUnion(
                    v.mappings.map((m) => this.templateMappingToJson(m))
                );
                return out;
            }

            case "json": {
                const j = sub as JsonSubscriptionUI;
                out.json = this.mappingEntriesToUnion(
                    j.mappings.map((m) => this.templateMappingToJson(m))
                );
                return out;
            }
        }
    }

    /* ============================================================
       MAPPINGS
       ============================================================ */

    private static staticMappingFromJson(json: JsonStaticMapping): StaticMappingUI {
        return {
            mappedTopic: json.mapped_topic,
            retain: json.retain ?? false,
            qos: (json.qos ?? 0) as QoS,
            delay: json.delay ?? null,
            messageMappings: this.messageMappingUnionToArray(json.message_mapping)
                .map(m => this.messageMappingFromJson(m)),
        };
    }

    private static templateMappingFromJson(json: any): ValueTemplateMappingUI {
        return {
            mappedTopic: json.mapped_topic,
            mappingTemplate: json.mapping_template ?? '',
            retain: json.retain ?? false,
            qos: (json.qos ?? 0) as QoS,
            suppressions: json.suppressions ?? [],
            delay: json.delay ?? null,
        };
    }

    private static messageMappingFromJson(json: any): MessageMappingUI {
        return {
            message: json.message,
            mappedMessage: json.mapped_message,
        };
    }

    private static staticMappingToJson(m: StaticMappingUI): JsonStaticMapping {
        const msgs = (m.messageMappings ?? []).map(mm => this.messageMappingToJson(mm));

        if (msgs.length === 0) {
            throw new Error(`Invalid static mapping for "${m.mappedTopic ?? ''}": messageMappings must not be empty.`);
        }

        const out: JsonStaticMapping = {
            mapped_topic: m.mappedTopic ?? "",
            message_mapping: (msgs.length === 1 ? msgs[0] : msgs) as any,
        };

        // optional fields (if your JsonStaticMapping allows them)
        if (m.retain) (out as any).retain = true;
        if (m.qos !== undefined && m.qos !== 0) (out as any).qos = m.qos;
        if (m.delay !== null) (out as any).delay = m.delay;

        return out;
    }

    private static messageMappingToJson(m: MessageMappingUI): JsonMessageMapping {
        return {
            message: m.message ?? "",
            mapped_message: m.mappedMessage ?? "",
        };
    }

    private static templateMappingToJson(m: ValueTemplateMappingUI): JsonMappingEntry {
        const out: JsonMappingEntry = {
            mapped_topic: m.mappedTopic ?? "",
        };

        // Only include optional fields if they actually carry meaning
        if (m.mappingTemplate) out.mapping_template = m.mappingTemplate;
        if (m.retain) out.retain = true;
        if (m.qos !== undefined && m.qos !== 0) out.qos = m.qos;
        if (m.suppressions?.length) out.suppressions = m.suppressions;
        if (m.delay !== null) out.delay = m.delay;

        return out;
    }

    /* ============================================================
       HELPERS
       ============================================================ */

    private static normalizeArray<T>(value: T | T[]): T[] {
        return Array.isArray(value) ? value : [value];
    }

    private static messageMappingUnionToArray(union?: JsonMessageMappingUnion): any[] {
        if (!union) return [];
        return Array.isArray(union) ? union : [union];
    }

    private static mappingEntriesToUnion(entries: JsonMappingEntry[]): JsonMappingUnion {
        // union: single object if length === 1, else array
        if (entries.length === 1) return entries[0];
        return entries;
    }
}
