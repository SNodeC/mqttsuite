import { Network, NetIn, NetIn6, NetRc, NetL2, NetUn, Protocol } from "../types/mqtt-bridge";
import { Input } from "./ui/input";
import { Label } from "./ui/label";

interface NetworkAddressFormProps {
  network: Network;
  onChange: (network: Network) => void;
  onSave: () => void;
}

export function NetworkAddressForm({ network, onChange, onSave }: NetworkAddressFormProps) {
  const updateProtocolField = (protocol: Protocol, field: string, value: string | number, shouldSave: boolean = false) => {
    const updatedNetwork = { ...network };

    // Update the specific protocol field
    switch (protocol) {
      case "in":
        updatedNetwork.in = {
          ...(network.in || { host: "", port: 1883 }),
          [field]: value,
        } as NetIn;
        break;
      case "in6":
        updatedNetwork.in6 = {
          ...(network.in6 || { host: "", port: 1883 }),
          [field]: value,
        } as NetIn6;
        break;
      case "rc":
        updatedNetwork.rc = {
          ...(network.rc || { host: "", channel: 1 }),
          [field]: value,
        } as NetRc;
        break;
      case "l2":
        updatedNetwork.l2 = {
          ...(network.l2 || { host: "", psm: 1 }),
          [field]: value,
        } as NetL2;
        break;
      case "un":
        updatedNetwork.un = {
          ...(network.un || { path: "" }),
          [field]: value,
        } as NetUn;
        break;
    }

    onChange(updatedNetwork);
    if (shouldSave) {
      onSave();
    }
  };

  const renderAddressFields = () => {
    switch (network.protocol) {
      case "in":
        return (
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="ipv4-host">Host</Label>
              <Input
              onGreyBg
                id="ipv4-host"
                value={network.in?.host || ""}
                onChange={(e) => updateProtocolField("in", "host", e.target.value)}
                onBlur={onSave}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                placeholder="e.g., 192.168.1.100"
              />
            </div>
            <div className="space-y-2">
              <Label htmlFor="ipv4-port">Port</Label>
              <Input
              onGreyBg
                id="ipv4-port"
                type="number"
                min="1"
                max="65535"
                value={network.in?.port || 1883}
                onChange={(e) => updateProtocolField("in", "port", parseInt(e.target.value) || 1883, true)}
                onBlur={onSave}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                placeholder="1883"
              />
            </div>
          </div>
        );

      case "in6":
        return (
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="ipv6-host">Host</Label>
              <Input
              onGreyBg
                id="ipv6-host"
                value={network.in6?.host || ""}
                onChange={(e) => updateProtocolField("in6", "host", e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                onBlur={onSave}
                placeholder="e.g., ::1 or fe80::1"
              />
            </div>
            <div className="space-y-2">
              <Label htmlFor="ipv6-port">Port</Label>
              <Input
              onGreyBg
                id="ipv6-port"
                type="number"
                min="1"
                max="65535"
                value={network.in6?.port || 1883}
                onChange={(e) => updateProtocolField("in6", "port", parseInt(e.target.value) || 1883, true)}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                onBlur={onSave}
                placeholder="1883"
              />
            </div>
          </div>
        );

      case "rc":
        return (
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="rc-host">Bluetooth Address</Label>
              <Input
              onGreyBg
                id="rc-host"
                value={network.rc?.host || ""}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                onChange={(e) => updateProtocolField("rc", "host", e.target.value)}
                onBlur={onSave}
                placeholder="00:1A:7D:DA:71:13"
              />
              <p className="text-xs text-muted-foreground">Format: XX:XX:XX:XX:XX:XX</p>
            </div>
            <div className="space-y-2">
              <Label htmlFor="rc-channel">Channel</Label>
              <Input
              onGreyBg
                id="rc-channel"
                type="number"
                min="1"
                max="30"
                value={network.rc?.channel || 1}
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                onChange={(e) => updateProtocolField("rc", "channel", parseInt(e.target.value) || 1, true)}
                onBlur={onSave}
                placeholder="1"
              />
            </div>
          </div>
        );

      case "l2":
        return (
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <Label htmlFor="l2-host">Bluetooth Address</Label>
              <Input
              onGreyBg
                id="l2-host"
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                value={network.l2?.host || ""}
                onChange={(e) => updateProtocolField("l2", "host", e.target.value)}
                onBlur={onSave}
                placeholder="AA:BB:CC:DD:EE:FF"
              />
              <p className="text-xs text-muted-foreground">Format: XX:XX:XX:XX:XX:XX</p>
            </div>
            <div className="space-y-2">
              <Label htmlFor="l2-psm">PSM</Label>
              <Input
              onGreyBg
                id="l2-psm"
                type="number"
                min="1"
                max="65535"
                onKeyDown={(e) => e.key === 'Enter' && onSave()}
                value={network.l2?.psm || 1}
                onChange={(e) => updateProtocolField("l2", "psm", parseInt(e.target.value) || 1, true)}
                onBlur={onSave}
                placeholder="1"
              />
            </div>
          </div>
        );

      case "un":
        return (
          <div className="space-y-2">
            <Label htmlFor="un-path">Socket Path</Label>
            <Input
            onGreyBg
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              id="un-path"
              value={network.un?.path || ""}
              onChange={(e) => updateProtocolField("un", "path", e.target.value)}
              onBlur={onSave}
              placeholder="/var/run/mqttbridge.sock"
              maxLength={107}
            />
            <p className="text-xs text-muted-foreground">Maximum 107 characters</p>
          </div>
        );

      default:
        return null;
    }
  };

  return (
    <div className="space-y-4 p-4 border border-border rounded-lg bg-muted/50">
      <h4>Network Address</h4>
      {renderAddressFields()}
    </div>
  );
}