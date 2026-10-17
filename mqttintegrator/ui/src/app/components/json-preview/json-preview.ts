import { Component, DestroyRef, inject } from '@angular/core';
import { MappingService } from '../../shared/services/mapping.service';
import { take, takeUntil } from 'rxjs';
import { takeUntilDestroyed } from '@angular/core/rxjs-interop';
import { AsyncPipe } from '@angular/common';
import { MatButtonModule } from "@angular/material/button";

@Component({
  selector: 'app-json-preview',
  imports: [AsyncPipe, MatButtonModule],
  templateUrl: './json-preview.html',
  styleUrl: './json-preview.scss'
})
export class JsonPreview {
  private readonly destroyRef = inject(DestroyRef);
  private readonly mappingService = inject(MappingService);

  jsonPreview$ = this.mappingService.jsonPrettyStringExport$;
}
