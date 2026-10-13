import { CommonModule } from '@angular/common';
import { Component, inject } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatIconModule } from '@angular/material/icon';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { MappingService } from '../../shared/services/mapping.service';
import { Router } from '@angular/router';
import { JsonRootConfig } from '../../shared/models/mapping-json.dto';
import { MappingAdapter } from '../../shared/adapter/mapping-adapter';
import { MatDialog } from '@angular/material/dialog';
import { DialogShell } from '../../shared/components/dialogs/dialog-shell/dialog-shell';



@Component({
  selector: 'app-home-page',
  imports: [
    CommonModule,
    MatButtonModule,
    MatCardModule,
    MatIconModule,
    MatSnackBarModule,
  ],
  templateUrl: './home-page.html',
  styleUrl: './home-page.scss'
})
export class HomePage {
  dialog = inject(MatDialog);

  constructor(
    private readonly mappingService: MappingService,
    private readonly router: Router,
    private readonly snackBar: MatSnackBar
  ) { }

  createNew(): void {
    this.mappingService.createNewIntegrator('new_integrator_client');
    this.router.navigate(['/integrator']); // adjust route
  }

  async onFileSelected(evt: Event): Promise<void> {
    const input = evt.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;

    try {
      const text = await file.text();
      const parsed = JSON.parse(text) as unknown;

      // 🔁 Convert schema JSON → UI model
      const integratorUI = MappingAdapter.fromJson(
        parsed as JsonRootConfig
      );

      this.mappingService.setIntegrator(integratorUI);

      // allow selecting the same file again
      input.value = '';

      this.snackBar.open('Integrator loaded', 'OK', { duration: 2000 });
      this.router.navigate(['/integrator']);
    } catch (e) {
      input.value = '';
      const msg =
        e instanceof Error ? e.message : 'Failed to load JSON file.';
      this.snackBar.open(msg, 'Dismiss', { duration: 5000 });
    }
  }

}
