import { Bridge, defaultBroker } from "../types/mqtt-bridge";
import { Input } from "./ui/input";
import { Label } from "./ui/label";
import { Button } from "./ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "./ui/card";
import { Plus, Trash2, ChevronDown, MoreVertical, Copy, Circle, Loader2 } from "lucide-react";
import { BrokerForm } from "./BrokerForm";
import { Alert, AlertDescription } from "./ui/alert";
import { AlertCircle } from "lucide-react";
import { Collapsible, CollapsibleContent, CollapsibleTrigger } from "./ui/collapsible";
import { Checkbox } from "./ui/checkbox";
import { Tooltip, TooltipTrigger, TooltipContent } from "./ui/tooltip";
import { useState } from "react";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "./ui/dropdown-menu";

type BridgeState = "disabled" | "starting" | "started" | "stopping" | "stopped";
type BrokerState = "disabled" | "connecting" | "connected" | "disconnecting" | "disconnected";

interface BrokerStatusMap {
  [key: string]: BrokerState;
}

interface BridgeFormProps {
  bridge: Bridge;
  index: number;
  bridgeState?: BridgeState;
  brokerStates: BrokerStatusMap;
  onChange: (bridge: Bridge) => void;
  onSave: () => void;
  onDelete: () => void;
  onDuplicate: () => void;
}

function BridgeStatusIcon({ state }: { state?: BridgeState }) {
  if (!state) return null;

  const statusLabels: Record<BridgeState, string> = {
    started: "Bridge Running",
    starting: "Bridge Starting",
    stopping: "Bridge Stopping",
    stopped: "Bridge Stopped",
    disabled: "Bridge Disabled",
  };

  switch (state) {
    case "started":
      return (
        <Tooltip>
          <TooltipTrigger asChild>
            <Circle className="h-4 w-4 fill-green-500 text-green-500" />
          </TooltipTrigger>
          <TooltipContent>{statusLabels.started}</TooltipContent>
        </Tooltip>
      );
    case "starting":
      return (
        <Tooltip>
          <TooltipTrigger asChild>
            <Loader2 className="h-4 w-4 text-yellow-500 animate-spin" />
          </TooltipTrigger>
          <TooltipContent>{statusLabels.starting}</TooltipContent>
        </Tooltip>
      );
    case "stopping":
      return (
        <Tooltip>
          <TooltipTrigger asChild>
            <Loader2 className="h-4 w-4 text-orange-500 animate-spin" />
          </TooltipTrigger>
          <TooltipContent>{statusLabels.stopping}</TooltipContent>
        </Tooltip>
      );
    case "stopped":
      return (
        <Tooltip>
          <TooltipTrigger asChild>
            <Circle className="h-4 w-4 fill-red-500 text-red-500" />
          </TooltipTrigger>
          <TooltipContent>{statusLabels.stopped}</TooltipContent>
        </Tooltip>
      );
    case "disabled":
      return (
        <Tooltip>
          <TooltipTrigger asChild>
            <Circle className="h-4 w-4 fill-gray-400 text-gray-400" />
          </TooltipTrigger>
          <TooltipContent>{statusLabels.disabled}</TooltipContent>
        </Tooltip>
      );
    default:
      return null;
  }
}

export function BridgeForm({ bridge, index, bridgeState, brokerStates, onChange, onSave, onDelete, onDuplicate }: BridgeFormProps) {
  const [isOpen, setIsOpen] = useState(true);
  const updateField = <K extends keyof Bridge>(field: K, value: Bridge[K]) => {
    onChange({ ...bridge, [field]: value });
  };

  const addBroker = () => {
    updateField("brokers", [...bridge.brokers, { ...defaultBroker }]);
    // Send changes immediately when adding a broker (button click)
    setTimeout(onSave, 0);
  };

  const removeBroker = (brokerIndex: number) => {
    updateField("brokers", bridge.brokers.filter((_, i) => i !== brokerIndex));
    // Send changes immediately when removing a broker (button click)
    setTimeout(onSave, 0);
  };

  const updateBroker = (brokerIndex: number, broker: any) => {
    const newBrokers = [...bridge.brokers];
    newBrokers[brokerIndex] = broker;
    updateField("brokers", newBrokers);
  };

  return (
    <Collapsible open={isOpen} onOpenChange={setIsOpen}>
      <Card className={`border-2 bg-secondary ${bridge.disabled ? 'opacity-50' : ''}`}>
        <CardHeader className={`flex flex-row items-center gap-3 space-y-0 pb-3 pt-3 bg-primary ${isOpen ? 'rounded-t-lg' : 'rounded-lg'}`}>
          <CollapsibleTrigger className="flex items-center gap-2 hover:opacity-70 transition-opacity flex-shrink-0">
            <ChevronDown className={`h-4 w-4 transition-transform text-primary-foreground ${isOpen ? 'rotate-180' : ''}`} />
            <span className="text-sm font-semibold text-primary-foreground">Bridge {index + 1}</span>
          </CollapsibleTrigger>
          <div className="flex-1">
            <Input
              id={`bridge-name-${index}`}
              onGreyBg
              value={bridge.name}
              onChange={(e) => updateField("name", e.target.value)}
              onBlur={onSave}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              placeholder="Bridge name (required)"
              required
              className="h-9 bg-primary-foreground text-primary-foreground placeholder:text-primary-foreground/70"
            />
          </div>
          <div className="flex items-center gap-2 shrink-0">
            <BridgeStatusIcon state={bridgeState} />
            <Checkbox
              id={`bridge-enabled-${index}`}
              checked={!bridge.disabled}
              onCheckedChange={(checked: boolean) => {
                updateField("disabled", !checked);
                setTimeout(onSave, 0);
              }}
              title={bridge.disabled ? "Enable bridge" : "Disable bridge"}
              className="border-white bg-primary-foreground"
            />
          </div>
          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <div>
                <Button size="icon" variant="default" className="shrink-0 text-primary-foreground ">
                  <MoreVertical className="h-4 w-4" />
                </Button>
              </div>
            </DropdownMenuTrigger>
            <DropdownMenuContent align="end">
              <DropdownMenuItem onClick={onDuplicate}>
                <Copy className="h-4 w-4" />
                Duplicate Bridge
              </DropdownMenuItem>
              <DropdownMenuItem onClick={onDelete}>
                <Trash2 className="h-4 w-4" />
                Delete Bridge
              </DropdownMenuItem>
            </DropdownMenuContent>
          </DropdownMenu>
        </CardHeader>
        <CollapsibleContent>
          <CardContent className="space-y-6 pt-0">
            <div className="space-y-4">

              {bridge.brokers.length < 2 && (
                <Alert variant="destructive">
                  <AlertCircle className="h-4 w-4" />
                  <AlertDescription>
                    A bridge requires at least 2 brokers to function properly.
                  </AlertDescription>
                </Alert>
              )}

              <div className="space-y-4">
                {bridge.brokers.map((broker, brokerIndex) => (
                  <BrokerForm
                    key={brokerIndex}
                    broker={broker}
                    bridgeName={bridge.name}
                    bridgeDisabled={bridge.disabled}
                    index={brokerIndex}
                    brokerState={brokerStates[`${bridge.name}/${broker.network.instance_name}`]}
                    canDelete={bridge.brokers.length > 2}
                    onChange={(updatedBroker) => updateBroker(brokerIndex, updatedBroker)}
                    onSave={onSave}
                    onDelete={() => removeBroker(brokerIndex)}
                  />
                ))}
              </div>

              <Button onClick={addBroker}  variant="outline">
                <Plus className=" w-full" />
                Add Broker
              </Button>

            </div>
          </CardContent>
        </CollapsibleContent>
      </Card>
    </Collapsible>
  );
}