import { inject, Injectable } from '@angular/core';
import { createTwoFilesPatch } from 'diff';
import { MappingService } from './mapping.service';
import { BehaviorSubject, combineLatest, Observable } from 'rxjs';

@Injectable({ providedIn: 'root' })
export class DiffService {
  mappingService = inject(MappingService);

  private _combinedConfig$ = new BehaviorSubject<string | null>(null);
  readonly combindedConfig$: Observable<string | null> = this._combinedConfig$.asObservable();

  constructor() {
    this.mappingService.integratorLive$.subscribe(result => console.log(result));
    combineLatest([this.mappingService.jsonExportStrict$, this.mappingService.integratorLive$]).subscribe({
      next: next  => {
        this._combinedConfig$.next(this.unifiedDiff(next[0], next[1]));
      },
      error: error => {
        console.log("Couldnt create diff with error: ", error);
      }
    })
  }

  toPrettyJson(obj: unknown): string {
    return JSON.stringify(obj, null, 2) + '\n';
  }

  unifiedDiff(
    oldObj: unknown,
    newObj: unknown,
    oldName = 'server',
    newName = 'local'
  ): string {
    const oldText = this.toPrettyJson(oldObj);
    const newText = this.toPrettyJson(newObj);

    if(oldObj === newObj) return 'No changes';

    return createTwoFilesPatch(
      oldName,
      newName,
      oldText,
      newText,
      '', // old header
      '', // new header
      { context: 3 } // lines of context around changes (like git)
    );
  }
}
