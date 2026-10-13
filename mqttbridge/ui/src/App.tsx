import { useState, useEffect, useRef } from "react";
import { BridgesConfig, defaultBridge } from "./types/mqtt-bridge";
import { BridgeForm } from "./components/BridgeForm";
import { Button } from "./components/ui/button";
import { Plus, AlertCircle, CheckCircle2, Circle, Loader2 } from "lucide-react";
import { Alert, AlertDescription, AlertTitle } from "./components/ui/alert";
import { toast } from "sonner@2.0.3";
import { Toaster } from "./components/ui/sonner";
import { Tooltip, TooltipTrigger, TooltipContent } from "./components/ui/tooltip";
import logo from "figma:asset/0edd3e9239d6ff6f8f9c5985d785e309773e3d03.png";



// Precise JSON patch comparison function
function generateJSONPatch(oldObj: any, newObj: any, path = ""): any[] {
  const patches: any[] = [];

  // Handle array changes - compare element by element
  if (Array.isArray(oldObj) && Array.isArray(newObj)) {
    // Handle array length changes and element-wise comparison
    const maxLength = Math.max(oldObj.length, newObj.length);

    for (let i = 0; i < maxLength; i++) {
      const elementPath = `${path}/${i}`;
      const oldElement = oldObj[i];
      const newElement = newObj[i];

      if (i >= newObj.length) {
        // Element was removed
        patches.push({ op: "remove", path: elementPath });
      } else if (i >= oldObj.length) {
        // Element was added
        patches.push({ op: "add", path: elementPath, value: newElement });
      } else if (JSON.stringify(oldElement) !== JSON.stringify(newElement)) {
        // Element changed
        if (typeof oldElement === "object" && typeof newElement === "object" && oldElement !== null && newElement !== null) {
          // Recursively compare array element objects
          const nestedPatches = generateJSONPatch(oldElement, newElement, elementPath);
          patches.push(...nestedPatches);
        } else {
          patches.push({ op: "replace", path: elementPath, value: newElement });
        }
      }
    }
    return patches;
  }

  // Handle object changes
  if (typeof oldObj === "object" && typeof newObj === "object" && oldObj !== null && newObj !== null) {
    const allKeys = new Set([...Object.keys(oldObj), ...Object.keys(newObj)]);

    for (const key of allKeys) {
      const newPath = path ? `${path}/${key}` : `/${key}`;
      const oldValue = oldObj[key];
      const newValue = newObj[key];

      if (!(key in newObj)) {
        // Key was removed
        patches.push({ op: "remove", path: newPath });
      } else if (!(key in oldObj)) {
        // Key was added
        patches.push({ op: "add", path: newPath, value: newValue });
      } else if (JSON.stringify(oldValue) !== JSON.stringify(newValue)) {
        // Value changed
        if (typeof oldValue === "object" && typeof newValue === "object" && oldValue !== null && newValue !== null) {
          // Recursively compare objects and arrays
          const nestedPatches = generateJSONPatch(oldValue, newValue, newPath);
          patches.push(...nestedPatches);
        } else {
          patches.push({ op: "replace", path: newPath, value: newValue });
        }
      }
    }
  } else if (oldObj !== newObj) {
    // Primitive value changed
    patches.push({ op: "replace", path: path || "/", value: newObj });
  }

  return patches;
}

type BridgeState = "disabled" | "starting" | "started" | "stopping" | "stopped";
type BrokerState = "disabled" | "connecting" | "connected" | "disconnecting" | "disconnected";
type BridgesSystemState = "starting" | "started" | "stopping" | "stopped";

interface BridgeStatusMap {
  [bridgeName: string]: BridgeState;
}

interface BrokerStatusMap {
  [key: string]: BrokerState; // key format: "bridgeName/instanceName"
}

function SystemStatusText({ state, startTime }: { state?: BridgesSystemState | null; startTime: number | null }) {
  const [uptime, setUptime] = useState<number>(0);

  useEffect(() => {
    if (state === "started" && startTime) {
      // Set initial uptime
      setUptime(Math.floor((Date.now() - startTime) / 1000)+1);
      // Then update every second
      const interval = setInterval(() => {
        setUptime(Math.floor((Date.now() - startTime) / 1000)+1);
      }, 1000);
      return () => clearInterval(interval);
    }
  }, [state, startTime]);

  const formatUptime = (seconds: number): string => {
    if (seconds < 60) {
      return `${seconds} sec`;
    } else if (seconds < 3600) {
      const minutes = Math.floor(seconds / 60);
      return `${minutes} min`;
    } else {
      const hours = Math.floor(seconds / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      return `${hours} h ${minutes} min`;
    }
  };

  if (!state) return null;

  let statusText = "";

  switch (state) {
    case "starting":
      statusText = "system pending...";
      break;
    case "started":
      statusText = `running - uptime: ${formatUptime(uptime)}`;
      break;
    case "stopping":
      statusText = "shutting down";
      break;
    case "stopped":
      statusText = "system down";
      break;
    default:
      return null;
  }

  return (
    <div className={`text-xs font-medium text-muted-foreground`}>
      {statusText}
    </div>
  );
}

export default function App() {
  const [config, setConfig] = useState<BridgesConfig>({
    bridges: [{ ...defaultBridge }],
  });
  const [isLoading, setIsLoading] = useState(true);
  const [validationErrors, setValidationErrors] = useState<string[]>([]);
  const [hasAttemptedValidation, setHasAttemptedValidation] = useState(false);
  const [bridgeStates, setBridgeStates] = useState<BridgeStatusMap>({});
  const [brokerStates, setBrokerStates] = useState<BrokerStatusMap>({});
  const [bridgesSystemState, setBridgesSystemState] = useState<BridgesSystemState | null>(null);
  const [systemStartTime, setSystemStartTime] = useState<number | null>(null);

  // Keep track of the last saved config for generating patches
  const lastSavedConfigRef = useRef<BridgesConfig | null>(null);
  // Keep a synchronous reference to the latest in-memory config so
  // immediate saves (e.g. checkbox toggles) can use it before state commits
  const latestConfigRef = useRef<BridgesConfig>(config);
  // Store EventSource for cleanup
  const eventSourceRef = useRef<EventSource | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);

  // Load configuration on mount
  useEffect(() => {
    loadConfig();
    connectToSSE();

    // Cleanup function
    return () => {
      if (eventSourceRef.current) {
        eventSourceRef.current.close();
        eventSourceRef.current = null;
      }
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
        reconnectTimeoutRef.current = null;
      }
    };
  }, []);

  const connectToSSE = () => {
    // Close existing connection if any
    if (eventSourceRef.current) {
      eventSourceRef.current.close();
    }

    // Clear any pending reconnect timeout
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
      reconnectTimeoutRef.current = null;
    }

    try {
      const eventSource = new EventSource("/api/bridge/sse");
      eventSourceRef.current = eventSource;

      // Handle connection opened
      eventSource.onopen = () => {
        console.log("SSE connection established");
      };

      // Handle all bridge and broker events
      const events = [
        "bridges_starting",
        "bridges_started",
        "bridges_stopping",
        "bridges_stopped",
        "bridge_disabled",
        "bridge_starting",
        "bridge_started",
        "bridge_stopping",
        "bridge_stopped",
        "broker_disabled",
        "broker_connecting",
        "broker_connected",
        "broker_disconnecting",
        "broker_disconnected",
      ];

      events.forEach((eventName) => {
        eventSource.addEventListener(eventName, (event) => {
          try {
            const data = JSON.parse(event.data);
            // console.log(`SSE Event [${eventName}]:`, data);

            // Update bridge states
            if (eventName === "bridge_disabled" && data.name) {
              setBridgeStates(prev => ({ ...prev, [data.name]: "disabled" }));
            } else if (eventName === "bridge_starting" && data.name) {
              setBridgeStates(prev => ({ ...prev, [data.name]: "starting" }));
            } else if (eventName === "bridge_started" && data.name) {
              setBridgeStates(prev => ({ ...prev, [data.name]: "started" }));
            } else if (eventName === "bridge_stopping" && data.name) {
              setBridgeStates(prev => ({ ...prev, [data.name]: "stopping" }));
            } else if (eventName === "bridge_stopped" && data.name) {
              setBridgeStates(prev => ({ ...prev, [data.name]: "stopped" }));
            }

            // Update broker states
            if (data.bridge && data.instance) {
              const instanceName =
                typeof data.instance === "string" && data.instance.includes("+")
                  ? data.instance.split("+").pop()
                  : data.instance;
              const brokerKey = `${data.bridge}/${instanceName}`;
              if (eventName === "broker_disabled") {
                setBrokerStates(prev => ({ ...prev, [brokerKey]: "disabled" }));
              } else if (eventName === "broker_connecting") {
                setBrokerStates(prev => ({ ...prev, [brokerKey]: "connecting" }));
              } else if (eventName === "broker_connected") {
                setBrokerStates(prev => ({ ...prev, [brokerKey]: "connected" }));
              } else if (eventName === "broker_disconnecting") {
                setBrokerStates(prev => ({ ...prev, [brokerKey]: "disconnecting" }));
              } else if (eventName === "broker_disconnected") {
                setBrokerStates(prev => ({ ...prev, [brokerKey]: "disconnected" }));
              }
            }

            // Update system state
            if (eventName === "bridges_starting") {
              setBridgesSystemState("starting");
              setSystemStartTime(null);
            } else if (eventName === "bridges_started") {
              setBridgesSystemState("started");
              // Use the timestamp from the event data if available, otherwise use current time
              const startTime = data.at ? new Date(data.at).getTime() : Date.now();
              setSystemStartTime(startTime);
            } else if (eventName === "bridges_stopping") {
              setBridgesSystemState("stopping");
            } else if (eventName === "bridges_stopped") {
              setBridgesSystemState("stopped");
              setSystemStartTime(null);
            }
          } catch (error) {
            console.error(`Error handling SSE event [${eventName}]:`, error, event);
            toast.error(`Error processing ${eventName} event`);
          }
        });
      });

      eventSource.onerror = (error) => {
        console.error("SSE connection error:", error);

        // Close the failed connection
        if (eventSourceRef.current) {
          eventSourceRef.current.close();
          eventSourceRef.current = null;
        }

        // Attempt to reconnect after 5 seconds
        console.log("Attempting to reconnect in 5 seconds...");
        reconnectTimeoutRef.current = window.setTimeout(() => {
          connectToSSE();
        }, 5000);
      };
    } catch (error) {
      console.error("Failed to create SSE connection:", error);
      // Retry after 5 seconds
      reconnectTimeoutRef.current = window.setTimeout(() => {
        connectToSSE();
      }, 5000);
    }
  };

  const loadConfig = async () => {
    try {
      setIsLoading(true);
      const response = await fetch('/api/bridge/config');

      if (!response.ok) {
        throw new Error(`Failed to load configuration: ${response.statusText}`);
      }

      const data = await response.json();
      if (data === null || typeof data !== 'object' || !Array.isArray(data.bridges)) {
        throw new Error("Invalid configuration format received from server");
      }
      setConfig(data);
      lastSavedConfigRef.current = JSON.parse(JSON.stringify(data)); // Deep clone
      // Keep latest reference in sync immediately
      latestConfigRef.current = data;
      toast.success("Configuration loaded successfully");
    } catch (error) {
      toast.error("Failed to load configuration", {
        description: error instanceof Error ? error.message : "Unknown error",
      });
      // Keep the default config if loading fails
      lastSavedConfigRef.current = JSON.parse(JSON.stringify(config));
      latestConfigRef.current = config;
    } finally {
      setIsLoading(false);
    }
  };

  const cleanConfig = (cfg: BridgesConfig) => {
    // Clean up the config to match the JSON schema
    // Keep all required fields even if empty to avoid server errors
    return {
      bridges: cfg.bridges.map(bridge => {
        const cleanBridge: any = {
          name: bridge.name,
          brokers: bridge.brokers.map(broker => {
            const cleanBroker: any = {
              network: {
                instance_name: broker.network.instance_name,
                protocol: broker.network.protocol,
                encryption: broker.network.encryption,
                transport: broker.network.transport,
              },
              mqtt: broker.mqtt || {}, // Always include mqtt object, even if empty
              topics: broker.topics,
            };

            // Add disabled if true
            if (broker.disabled) {
              cleanBroker.disabled = broker.disabled;
            }

            // Add session_store only if it's not empty
            if (broker.session_store) {
              cleanBroker.session_store = broker.session_store;
            }

            // Add prefix if it's not empty
            if (broker.prefix) {
              cleanBroker.prefix = broker.prefix;
            }

            // Add the protocol-specific address fields
            switch (broker.network.protocol) {
              case "in":
                if (broker.network.in) cleanBroker.network.in = broker.network.in;
                break;
              case "in6":
                if (broker.network.in6) cleanBroker.network.in6 = broker.network.in6;
                break;
              case "rc":
                if (broker.network.rc) cleanBroker.network.rc = broker.network.rc;
                break;
              case "l2":
                if (broker.network.l2) cleanBroker.network.l2 = broker.network.l2;
                break;
              case "un":
                if (broker.network.un) cleanBroker.network.un = broker.network.un;
                break;
            }

            return cleanBroker;
          }),
        };

        // Add disabled if true
        if (bridge.disabled) {
          cleanBridge.disabled = bridge.disabled;
        }

        // Add prefix if it's not empty
        if (bridge.prefix) {
          cleanBridge.prefix = bridge.prefix;
        }

        return cleanBridge;
      }),
    };
  };

  const sendPatchToServer = async (newConfig: BridgesConfig) => {
    if (!lastSavedConfigRef.current) {
      // If we don't have a reference config yet, skip sending patch
      return;
    }

    const cleanedOld = cleanConfig(lastSavedConfigRef.current);
    const cleanedNew = cleanConfig(newConfig);

    // Generate JSON patch
    const patch = generateJSONPatch(cleanedOld, cleanedNew);

    if (patch.length === 0) {
      // No changes detected
      return;
    }

    try {
      const response = await fetch('/api/bridge/config', {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json-patch+json',
        },
        body: JSON.stringify(patch),
      });

      if (!response.ok) {
        throw new Error(`Failed to save configuration: ${response.statusText}`);
      }

      // Update the last saved config
      lastSavedConfigRef.current = JSON.parse(JSON.stringify(newConfig));

    } catch (error) {
      toast.error("Failed to save configuration", {
        description: error instanceof Error ? error.message : "Unknown error",
      });
    }
  };

  const addBridge = () => {
    const newConfig = {
      bridges: [...config.bridges, { ...defaultBridge }],
    };
    setConfig(newConfig);
    // Update latest ref synchronously so immediate saves use correct data
    latestConfigRef.current = newConfig;
    sendPatchToServer(newConfig);
  };

  const removeBridge = (index: number) => {
    const newConfig = {
      bridges: config.bridges.filter((_, i) => i !== index),
    };
    setConfig(newConfig);
    // Update latest ref synchronously
    latestConfigRef.current = newConfig;
    sendPatchToServer(newConfig);
  };

  const duplicateBridge = (index: number) => {
    const bridgeToDuplicate = config.bridges[index];
    const duplicatedBridge = JSON.parse(JSON.stringify(bridgeToDuplicate));
    // update name to indicate it's a duplicate
    if (duplicatedBridge.name && duplicatedBridge.name !== "") {
      duplicatedBridge.name = `${bridgeToDuplicate.name}-copy`;
    } else {
      duplicatedBridge.name = "copy";
    }
    const newConfig = {
      bridges: [...config.bridges, duplicatedBridge],
    };
    setConfig(newConfig);
    // Update latest ref synchronously
    latestConfigRef.current = newConfig;
    sendPatchToServer(newConfig);
  };

  const updateBridge = (index: number, bridge: any) => {
    const newBridges = [...config.bridges];
    newBridges[index] = bridge;
    const newConfig = { bridges: newBridges };
    setConfig(newConfig);
    // Keep latest ref in sync immediately so children can call onSave()
    // and the save will operate on the most recent config.
    latestConfigRef.current = newConfig;
    // Don't send patch immediately - wait for onBlurSave callback
  };

  const handleSaveToServer = () => {
    // Use the synchronous latest config ref so saves triggered immediately
    // after setState see the updated data.
    sendPatchToServer(latestConfigRef.current);
  };

  return (
    <div className="min-h-screen bg-background">
      <Toaster />

      {/* Header */}
      <div className="border-b border-border bg-card">
        <div className="max-w-7xl mx-auto px-6 py-6">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-4">
              <img src={logo} alt="Logo" className="h-12 w-12" />
              <div>
                <h1>MQTT Bridge Configuration</h1>
                <p className="text-muted-foreground mt-2">
                  Configure MQTT bridges to connect and synchronize multiple brokers
                </p>
              </div>
            </div>
            {isLoading && (
              <div className="text-sm text-muted-foreground">Loading...</div>
            )}
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="max-w-7xl mx-auto px-6 py-8">
        <div className="space-y-6">
          <div className="flex items-end justify-between">
            <div>
              <h2>Bridges</h2>
              <p className="text-muted-foreground mt-1">
                {config.bridges.length} bridge{config.bridges.length !== 1 ? "s" : ""} configured
              </p>
            </div>
            {bridgesSystemState && (
              // Status indicator for overall bridge system
              <div className="flex items-end gap-4">
                <SystemStatusText state={bridgesSystemState} startTime={systemStartTime} />
              </div>
            )}
          </div>

          {config.bridges.length === 0 ? (
            <Alert>
              <AlertCircle className="h-4 w-4" />
              <AlertTitle>No bridges configured</AlertTitle>
              <AlertDescription>
                Click "Add Bridge" to create your first MQTT bridge configuration
              </AlertDescription>
            </Alert>
          ) : (
            <div className="space-y-6">
              {config.bridges.map((bridge, index) => (
                <BridgeForm
                  key={index}
                  bridge={bridge}
                  index={index}
                  bridgeState={bridgeStates[bridge.name]}
                  brokerStates={brokerStates}
                  onChange={(updatedBridge) => updateBridge(index, updatedBridge)}
                  onSave={handleSaveToServer}
                  onDelete={() => removeBridge(index)}
                  onDuplicate={() => duplicateBridge(index)}
                />
              ))}
            </div>
          )}

          <Button onClick={addBridge}>
            <Plus className="h-4 w-4 mr-2" />
            Add Bridge
          </Button>
        </div>

        {/* Info Section */}
        <div className="mt-12 p-6 bg-muted/50 rounded-lg border border-border">
          <h3 className="mb-3">Configuration Guide</h3>
          <div className="space-y-2 text-sm text-muted-foreground">
            <p>• Each bridge connects 2 or more MQTT brokers together</p>
            <p>• Messages on selected Topics will be forwarded to all other brokers of a bridge</p>
            <p>• Topics support MQTT wildcard patterns like # (multi-level) and + (single-level) to bridge all messages</p>
            <p>• Any changes to the Configuration are saved automatically and cause a restart of the bridge</p>
          </div>
        </div>
      </div>
    </div>
  );
}