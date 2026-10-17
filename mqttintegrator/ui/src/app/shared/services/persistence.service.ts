import { Injectable } from "@angular/core";

type StorageMode = 'memory' | 'session' | 'local';

@Injectable({ providedIn: 'root' })
export class PersistenceService {
  private memory = new Map<string, string>();

  private defaultMode: StorageMode = 'memory';

  set<T>(key: string, value: T, mode: StorageMode = this.defaultMode): void {
    console.log('PersistenceService.set', { key, value, mode });
    const payload = JSON.stringify(value);

    if (mode === 'memory') {
      this.memory.set(key, payload);
      return;
    }

    const storage = mode === 'session' ? sessionStorage : localStorage;
    storage.setItem(key, JSON.stringify(value));
  }

  get<T>(key: string, mode: StorageMode = this.defaultMode): T | null {
    const raw =
      mode === 'memory'
        ? this.memory.get(key) ?? null
        : (mode === 'session' ? sessionStorage : localStorage).getItem(key);

    if (!raw) return null;
    try { return JSON.parse(raw) as T; } catch { return null; }
  }

  has<T>(key: string, mode: StorageMode = this.defaultMode): boolean {
    if (mode === 'memory') {
      return this.memory.has(key);
    }
    const storage = mode === 'session' ? sessionStorage : localStorage;
    return storage.getItem(key) !== null;
  }

  remove(key: string, mode: StorageMode = this.defaultMode): void {
    if (mode === 'memory') {
      this.memory.delete(key);
      return;
    }
    const storage = mode === 'session' ? sessionStorage : localStorage;
    storage.removeItem(key);
  }

  clearMemory(): void {
    this.memory.clear();
  }
}