import { Topic } from "../types/mqtt-bridge";
import { Input } from "./ui/input";
import { Label } from "./ui/label";
import { Button } from "./ui/button";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "./ui/select";
import { Plus, X, Trash2 } from "lucide-react";

interface TopicsFormProps {
  topics: Topic[];
  onChange: (topics: Topic[]) => void;
  onSave: () => void;
}

export function TopicsForm({ topics, onChange, onSave }: TopicsFormProps) {
  const addTopic = () => {
    onChange([...topics, { topic: "", qos: 0 }]);
    setTimeout(onSave, 0);
  };

  const removeTopic = (index: number) => {
    onChange(topics.filter((_, i) => i !== index));
    setTimeout(onSave, 0);
  };

  const updateTopic = (index: number, field: keyof Topic, value: any) => {
    const newTopics = [...topics];
    newTopics[index] = { ...newTopics[index], [field]: value };
    onChange(newTopics);
  };

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <div>
        <h4>Topics</h4>
          <Label className="text-s text-muted-foreground mt-1">
            if this broker publishes messages on these topics, they will be bridged
          </Label>
        </div>
      </div>

      {/* <div className="space-y-3">
        {topics.map((topic, index) => (
          <div key={index} className="flex gap-3 items-end p-3 bg-secondary rounded-lg">
            <div className="flex-1 space-y-2">
              <Label htmlFor={`topic-${index}`}>Topic Pattern</Label>
              <Input
                id={`topic-${index}`}
                value={topic.topic}
                onChange={(e) => updateTopic(index, "topic", e.target.value)}
                onBlur={onSave}
                onGreyBg
                placeholder="e.g., #, sensor/+/temperature"
              />
            </div>

            <div className="w-48 space-y-2">
              <Label htmlFor={`qos-${index}`}>QoS</Label>
              <Select
                value={topic.qos.toString()}
                onValueChange={(value) => {
                  updateTopic(index, "qos", parseInt(value));
                  setTimeout(onSave, 0);
                }}
              >
                <SelectTrigger id={`qos-${index}`} onGreyBg>
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="0">0 - At most once</SelectItem>
                  <SelectItem value="1">1 - At least once</SelectItem>
                  <SelectItem value="2">2 - Exactly once</SelectItem>
                </SelectContent>
              </Select>
            </div>

            <Button
              onClick={() => removeTopic(index)}
              size="icon"
              variant="destructive"
              disabled={topics.length === 1}
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        ))}
      </div> */}

      {/* Topic tags */}
      <div className="flex flex-wrap gap-3">
        {topics.map((topic, index) => (
          <div
            key={index}
            className="inline-flex items-center gap-1 px-1 py-2 border-2 border-muted-foreground rounded-full bg-muted/30 hover:bg-primary/15 transition-colors"

          >
            <Input
              value={topic.topic}
              onChange={(e) => updateTopic(index, "topic", e.target.value)}
              onBlur={onSave}
              onKeyDown={(e) => e.key === 'Enter' && onSave()}
              placeholder="topic/pattern"
              className="h-6 px-2 py-0 text-sm text-center bg-background focus:bg-primary-foreground focus:border-0 focus-visible:ring-1 focus-visible:ring-offset-0 whitespace-nowrap"
              style={{ width: Math.max(100, Math.ceil(topic.topic.length * 8)) + "px" }}
            />

            <div className="h-4 w-px bg-primary opacity-30" />

            <Select
              value={topic.qos.toString()}
              onValueChange={(value) => {
                updateTopic(index, "qos", parseInt(value));
                setTimeout(onSave, 0);
              }}
            >
              <SelectTrigger className="h-6 w-18 px-1 py-0 text-sm bg-transparent hover:opacity-80 transition-opacity focus:ring-0 focus:ring-offset-0">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="0">QoS 0 - At most once</SelectItem>
                <SelectItem value="1">QoS 1 - At least once</SelectItem>
                <SelectItem value="2">QoS 2 - Exactly once</SelectItem>
              </SelectContent>
            </Select>

            <Button
              onClick={() => removeTopic(index)}
              size="icon"
              variant="ghost"
              className="h-4 w-4 rounded-full opacity-50 hover:text-red-800 hover:opacity-100"
            >
              <X className="h-3 w-3" />
            </Button>
          </div>
        ))}
        <div className="flex items-center px-4">
          <Button onClick={addTopic} size="sm" variant="outline" className="rounded-full">
            <Plus className="h-4 w-4 mr-2" />
            Add Topic
          </Button>
        </div>
      </div>

    </div>
  );
}