import { Component, inject } from '@angular/core';
import { DiffService } from '../../shared/services/diff.service';
import { AsyncPipe } from '@angular/common';

@Component({
  selector: 'app-diff-view',
  imports: [AsyncPipe],
  templateUrl: './diff-view.html',
  styleUrl: './diff-view.scss'
})
export class DiffView {
  diffService = inject(DiffService);
  configDiff$ = this.diffService.combindedConfig$;
}
