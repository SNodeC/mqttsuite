import { Mqtt } from "../types/mqtt-bridge";
import { Input } from "./ui/input";
import { Label } from "./ui/label";
import { Switch } from "./ui/switch";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "./ui/select";
import { Collapsible, CollapsibleContent, CollapsibleTrigger } from "./ui/collapsible";
import { ChevronDown } from "lucide-react";
import { useState } from "react";

interface ConnectionFormProps {
  connection: Mqtt;
  onChange: (connection: Mqtt) => void;
  onSave: () => void;
}

export function ConnectionForm({ connection, onChange, onSave }: ConnectionFormProps) {
  const [isOpen, setIsOpen] = useState(false);

  const updateField = <K extends keyof Mqtt>(field: K, value: Mqtt[K]) => {
    onChange({ ...connection, [field]: value });
  };

  return (
    <Collapsible open={isOpen} onOpenChange={setIsOpen} className={` rounded-lg bg-secondary`}>
      <CollapsibleTrigger className="flex items-center gap-2 w-full p-3 bg-muted/50 rounded-lg hover:bg-muted/80 transition-colors">
        <ChevronDown className={`h-4 w-4 transition-transform ${isOpen ? 'rotate-180' : ''}`} />
        <span>Connection Settings</span>
      </CollapsibleTrigger>

      <CollapsibleContent className="space-y-4 p-3">
        <div className="grid grid-cols-2 gap-4">
          <div className="space-y-2">
            <Label htmlFor="client_id">Client ID</Label>
            <Input
              id="client_id"
              onGreyBg
              value={connection.client_id}
              onChange={(e) => updateField("client_id", e.target.value)}
              onBlur={onSave}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              placeholder="Auto-generated if empty"
            />
          </div>

          <div className="space-y-2">
            <Label htmlFor="keep_alive">Keep Alive (seconds)</Label>
            <Input
              id="keep_alive"
              type="number"
              onGreyBg
              value={connection.keep_alive}
              onChange={(e) => updateField("keep_alive", parseInt(e.target.value) || 60)}
              onBlur={onSave}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
            />
          </div>
        </div>

        <div className="flex items-center justify-between p-3 rounded-lg">
          <Label htmlFor="clean_session">Clean Session</Label>
          <Switch
            id="clean_session"
            checked={connection.clean_session}
            onCheckedChange={(checked) => {
              updateField("clean_session", checked);
              setTimeout(onSave, 0);
            }}
          />
        </div>

        <div className="flex items-center justify-between p-3 rounded-lg">
          <Label htmlFor="loop_prevention">Loop Prevention</Label>
          <Switch
            id="loop_prevention"
            checked={connection.loop_prevention}
            onCheckedChange={(checked) => {
              updateField("loop_prevention", checked);
              setTimeout(onSave, 0);
            }}
          />
        </div>

        <div className="space-y-4 p-4 border border-border rounded-lg">
          <h4>Will Message</h4>

          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="will_topic">Will Topic</Label>
              <Input
                id="will_topic"
                onGreyBg
                value={connection.will_topic}
                onChange={(e) => updateField("will_topic", e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                onBlur={onSave}
                placeholder="Optional"
              />
            </div>

            <div className="space-y-2">
              <Label htmlFor="will_qos">Will QoS</Label>
              <Select
                value={connection.will_qos.toString()}
                onValueChange={(value) => {
                  updateField("will_qos", parseInt(value) as 0 | 1 | 2);
                  setTimeout(onSave, 0);
                }}
              >
                <SelectTrigger id="will_qos" onGreyBg>
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="0">0 - At most once</SelectItem>
                  <SelectItem value="1">1 - At least once</SelectItem>
                  <SelectItem value="2">2 - Exactly once</SelectItem>
                </SelectContent>
              </Select>
            </div>
          </div>

          <div className="space-y-2">
            <Label htmlFor="will_message">Will Message</Label>
            <Input
              onGreyBg
              id="will_message"
              value={connection.will_message}
              onChange={(e) => updateField("will_message", e.target.value)}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              onBlur={onSave}
              placeholder="Optional"
            />
          </div>

          <div className="flex items-center justify-between p-3 bg-secondary rounded-lg">
            <Label htmlFor="will_retain">Will Retain</Label>
            <Switch
              id="will_retain"
              checked={connection.will_retain}
              onCheckedChange={(checked) => {
                updateField("will_retain", checked);
                setTimeout(onSave, 0);
              }}
            />
          </div>
        </div>

        <div className="space-y-4 p-4 border border-border rounded-lg">
          <h4>Authentication</h4>

          <div className="space-y-2">
            <Label htmlFor="username">Username</Label>
            <Input
              onGreyBg
              id="username"
              value={connection.username}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              onChange={(e) => updateField("username", e.target.value)}
              onBlur={onSave}
              placeholder="Optional"
            />
          </div>

          <div className="space-y-2">
            <Label htmlFor="password">Password</Label>
            <Input
              onGreyBg
              id="password"
              type="password"
              value={connection.password}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              onChange={(e) => updateField("password", e.target.value)}
              onBlur={onSave}
              placeholder="Optional"
            />
          </div>
        </div>
      </CollapsibleContent>
    </Collapsible>
  );
}