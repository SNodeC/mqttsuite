import { Component, DestroyRef, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Router } from '@angular/router';

import { FormBuilder, ReactiveFormsModule, Validators } from '@angular/forms';
import { takeUntilDestroyed } from '@angular/core/rxjs-interop';
import { distinctUntilChanged, map } from 'rxjs/operators';

import { QoS } from '../../shared/models/mapping-model';
import { MappingService } from '../../shared/services/mapping.service';

import { MatToolbarModule } from '@angular/material/toolbar';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatCardModule } from '@angular/material/card';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatDividerModule } from '@angular/material/divider';

@Component({
  selector: 'app-integrator-config',
  imports: [CommonModule,
    ReactiveFormsModule,

    MatToolbarModule,
    MatButtonModule,
    MatIconModule,
    MatCardModule,
    MatFormFieldModule,
    MatInputModule,
    MatSlideToggleModule,
    MatDividerModule,],
  templateUrl: './integrator-config.html',
  styleUrl: './integrator-config.scss'
})
export class IntegratorConfig {
  private readonly fb = inject(FormBuilder);
  private readonly destroyRef = inject(DestroyRef);
  private readonly router = inject(Router);
  private readonly mappingService = inject(MappingService);

  // Observable from service
  readonly integrator$ = this.mappingService.integrator$;

  // Simple flat form (keeps things easy on mobile)
  readonly form = this.fb.nonNullable.group({
    discoverPrefix: [''],
    clientId: [''],
    keepAlive: [60, [Validators.min(0)]],
    cleanSession: [true],

    willTopic: [''],
    willMessage: [''],
    willQos: [0 as QoS, [Validators.min(0), Validators.max(2)]],
    willRetain: [false],

    username: [''],
    password: [''],
  });

  constructor() {
    // 1) When store state changes (load/reset), update form without emitting events
    this.integrator$
      .pipe(
        takeUntilDestroyed(this.destroyRef),
        map(integrator => ({
          discoverPrefix: integrator?.discoverPrefix,
          clientId: integrator?.connection.clientId,
          keepAlive: integrator?.connection.keepAlive,
          cleanSession: integrator?.connection.cleanSession,

          willTopic: integrator?.connection.willTopic,
          willMessage: integrator?.connection.willMessage,
          willQos: integrator?.connection.willQos,
          willRetain: integrator?.connection.willRetain,

          username: integrator?.connection.username,
          password: integrator?.connection.password,
        })),
        // avoid patching if nothing changed
        distinctUntilChanged((a, b) => JSON.stringify(a) === JSON.stringify(b))
      )
      .subscribe(formValue => {
        this.form.patchValue(formValue, { emitEvent: false });
      });
  }

  saveChanges() {
    const formValue = this.form.getRawValue();
    const discoverPrefix = formValue.discoverPrefix;
    const connection = {
      clientId: formValue.clientId ?? '',
      keepAlive: Number(formValue.keepAlive ?? 60),
      cleanSession: !!formValue.cleanSession,

      willTopic: formValue.willTopic ?? '',
      willMessage: formValue.willMessage ?? '',
      willQos: Number(formValue.willQos ?? 0) as QoS,
      willRetain: !!formValue.willRetain,

      username: formValue.username ?? '',
      password: formValue.password ?? '',
    }
    this.mappingService.setDiscoverPrefix(discoverPrefix);
    this.mappingService.patchConnection(connection);
  }
}
