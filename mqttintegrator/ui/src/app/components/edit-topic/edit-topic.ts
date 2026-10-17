import { Component, DestroyRef, inject } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { AbstractControl, FormArray, FormBuilder, FormControl, FormGroup, ReactiveFormsModule, ValidationErrors, ValidatorFn, Validators } from '@angular/forms';
import { takeUntilDestroyed } from '@angular/core/rxjs-interop';
import { combineLatest, map, distinctUntilChanged } from 'rxjs';

import { MappingService } from '../../shared/services/mapping.service';
import { QoS, SubscriptionKind, ValueTemplateMappingUI, TopicLevelUI, StaticMappingUI, SubscriptionUI } from '../../shared/models/mapping-model';

import { MatToolbarModule } from '@angular/material/toolbar';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatCardModule } from '@angular/material/card';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatDividerModule } from '@angular/material/divider';
import { AsyncPipe } from '@angular/common';
import { MatOptionModule } from '@angular/material/core';
import { MatSelectModule } from '@angular/material/select';
import { MatButtonToggleModule } from '@angular/material/button-toggle';
import { MatSnackBar } from '@angular/material/snack-bar';

const TOPIC_LEVEL_NAME_PATTERN = /^(?:[^#+]{2,}|.)$/;
const MAPPED_TOPIC_PATTERN =
  /(\{\{\s*[^\{\}]+?\s*\}\}|\{\{\s*#\s*[^\{\}]+?\s*\}\}|\{\{\s*\/\s*[^\{\}]+?\s*\}\})|^[^#+]*$/;


@Component({
  selector: 'app-edit-topic',
  standalone: true,
  imports: [
    AsyncPipe,
    ReactiveFormsModule,
    MatToolbarModule,
    MatButtonModule,
    MatIconModule,
    MatCardModule,
    MatFormFieldModule,
    MatInputModule,
    MatSlideToggleModule,
    MatDividerModule,
    MatOptionModule,
    MatSelectModule,
    MatButtonToggleModule
  ],
  templateUrl: './edit-topic.html',
  styleUrl: './edit-topic.scss',
})
export class EditTopic {
  private readonly mappingService = inject(MappingService);
  private readonly route = inject(ActivatedRoute);
  private readonly router = inject(Router);
  private readonly formBuilder = inject(FormBuilder).nonNullable;
  private readonly destroyRef = inject(DestroyRef);


  readonly mappedTopicPatternHint = 'Cannot contain + or # (template blocks {{...}} are allowed).';

  get mappings(): FormArray {
    return this.form.get('subscription.mappings') as FormArray;
  }

  suppressionsArray(mappingIndex: number): FormArray<FormControl<string>> {
    return this.mappings.at(mappingIndex).get('suppressions') as FormArray<FormControl<string>>;
  }



  readonly form = this.formBuilder.group({
    name: ['', [Validators.required, Validators.minLength(1), Validators.pattern(TOPIC_LEVEL_NAME_PATTERN)]],
    hasSubscription: [false],
    subscription: this.formBuilder.group({
      kind: ['static' as SubscriptionKind, Validators.required],
      qos: [0 as QoS],
      type: [''],
      mappings: this.formBuilder.array([]),
    }),
  });


  /** current route id as an observable */
  readonly topicId$ = this.route.paramMap.pipe(
    map(p => p.get('id')),
    distinctUntilChanged()
  );

  /** the selected topic level, derived from route id + integrator$ */
  readonly topicLevel$ = combineLatest([
    this.topicId$,
    this.mappingService.integrator$,
  ]).pipe(
    map(([id, _integrator]) => {
      if (!id) return null;
      // uses your existing query method; it reads the latest snapshot from BehaviorSubject
      return this.mappingService.getTopicLevelById(id);
    }),
    distinctUntilChanged((a, b) => a?.id === b?.id)
  );

  /** keep an imperative reference too (for button handlers) */
  topicLevel: TopicLevelUI | null = null;
  private topicId: string | null = null;

  constructor(private readonly snackBar: MatSnackBar) {
    // Keep local fields + form in sync with observable state
    this.topicLevel$
      .pipe(takeUntilDestroyed(this.destroyRef))
      .subscribe(topic => {
        this.topicLevel = topic;
        this.form.reset(
          { name: '', hasSubscription: false, subscription: { kind: 'static', qos: 0, type: '', mappings: [] } },
          { emitEvent: false }
        );

        if (!topic) return;

        // Patch name
        this.form.patchValue(
          { name: topic?.name ?? '' },
          { emitEvent: false }
        );
        const sub = this.form.get('subscription') as FormGroup;

        if (topic?.subscription) {
          this.form.patchValue(
            {
              hasSubscription: true,
              subscription: {
                kind: topic.subscription.kind,
                qos: topic.subscription.qos,
                type: topic.subscription.type,
              },
            },
            { emitEvent: false }
          );

          if (topic.subscription.kind === 'static') {
            const arr = this.buildStaticMappingsArray(topic.subscription.mappings as StaticMappingUI[]);
            sub.setControl('mappings', arr);
          } else {
            const arr = this.buildTemplateMappingsArray(topic.subscription.mappings as ValueTemplateMappingUI[]);
            sub.setControl('mappings', arr);
          }
        } else {
          // no subscription -> empty mapping
          sub.setControl('mappings', this.formBuilder.array([]));
        }
        sub.updateValueAndValidity({ emitEvent: false });
        if (this.form.get('hasSubscription')?.value) {
          this.ensureAtLeastOneMapping();
        }
      });

    // Keep latest route id for handlers
    this.topicId$
      .pipe(takeUntilDestroyed(this.destroyRef))
      .subscribe(id => (this.topicId = id));

    this.form.get('subscription.kind')?.valueChanges
      .pipe(takeUntilDestroyed(this.destroyRef))
      .subscribe(kind => {
        if (!this.form.get('hasSubscription')?.value) return; //no subscription active

        const sub = this.form.get('subscription') as FormGroup;

        switch (kind) {
          case 'static':
            sub.setControl('mappings', this.buildStaticMappingsArray([]));
            break;
          case 'value':
          case 'json':
            sub.setControl('mappings', this.buildTemplateMappingsArray([]));
            break;
        }

        sub.updateValueAndValidity({ emitEvent: false });
      });

    this.form.get('hasSubscription')?.valueChanges
      .pipe(takeUntilDestroyed(this.destroyRef))
      .subscribe(on => {
        if (on) {
          this.ensureAtLeastOneMapping();
        } else {
          this.clearMappings();
        }
      });
  }
  addTopicLevel(): void {
    if (!this.topicId) return;

    const child = this.mappingService.addChildTopicLevel(this.topicId, `${this.topicLevel?.name}_child_topic`);
    this.router.navigate(['edit-topic', child.id]);
  }

  goToTopic(id: string): void {
    this.router.navigate(['edit-topic', id]);
  }


  addStaticMapping(): void {
    this.mappings.push(
      this.formBuilder.group({
        mappedTopic: ['', [Validators.required, Validators.minLength(1), Validators.pattern(MAPPED_TOPIC_PATTERN)]],
        messageMappings: this.formBuilder.array([
          this.formBuilder.group({
            message: ['', [Validators.required, Validators.minLength(1)]],
            mappedMessage: ['', [Validators.required, Validators.minLength(1)]],
          })]
        ),

        retain: [false],
        qos: [0 as QoS],
        hasDelay: [false],
        delay: [0]
      })
    );
  }

  removeMapping(index: number): void {
    if (this.mappings.length <= 1) return;
    this.mappings.removeAt(index);
  }

  addMessageMapping(staticIndex: number): void {
    this.staticMessageMappings(staticIndex).push(
      this.formBuilder.group({
        message: ['', [Validators.required, Validators.minLength(1)]],
        mappedMessage: ['', [Validators.required, Validators.minLength(1)]],
      })
    );
  }

  removeMessageMapping(staticIndex: number, messageIndex: number): void {
    const arr = this.staticMessageMappings(staticIndex);
    if (arr.length <= 1) return;
    arr.removeAt(messageIndex);
  }


  addTemplateMapping(): void {
    this.mappings.push(
      this.formBuilder.group({
        mappedTopic: ['', [Validators.required, Validators.minLength(1), Validators.pattern(MAPPED_TOPIC_PATTERN)]],
        mappingTemplate: ['', [Validators.required, Validators.minLength(1)]],
        retain: [false as boolean],
        qos: [0 as QoS],
        hasSuppressions: [false],
        suppressions: this.buildSuppressionsArray([]),
        hasDelay: [false],
        delay: [0],
      })
    );
  }

  private buildStaticMappingsArray(mappings: StaticMappingUI[] = []): FormArray {
    return this.formBuilder.array(
      (mappings.length ? mappings : [{} as StaticMappingUI]).map(mapping =>
        this.formBuilder.group({
          mappedTopic: [mapping.mappedTopic ?? '', [Validators.required, Validators.minLength(1), Validators.pattern(MAPPED_TOPIC_PATTERN)]],
          messageMappings: this.formBuilder.array(
            (mapping.messageMappings?.length ? mapping.messageMappings : [{} as any]).map(mm =>
              this.formBuilder.group({
                message: [mm?.message ?? '', [Validators.required, Validators.minLength(1)]],
                mappedMessage: [mm?.mappedMessage ?? '', [Validators.required, Validators.minLength(1)]],
              })
            )
          ),
          retain: [mapping.retain ?? false],
          qos: [mapping.qos ?? (0 as QoS)],
          hasDelay: [mapping.delay != null],
          delay: [mapping.delay ?? 0],
        })
      )
    );
  }

  private buildTemplateMappingsArray(mappings: ValueTemplateMappingUI[] = []): FormArray {
    return this.formBuilder.array(
      (mappings.length ? mappings : [{} as ValueTemplateMappingUI]).map(m =>
        this.formBuilder.group({
          mappedTopic: [m.mappedTopic ?? '', [Validators.required, Validators.minLength(1), Validators.pattern(MAPPED_TOPIC_PATTERN)]],
          mappingTemplate: [m.mappingTemplate ?? '', [Validators.required, Validators.minLength(1)]],
          retain: [m.retain ?? false],
          qos: [m.qos ?? (0 as QoS)],

          hasSuppressions: [(m.suppressions?.length ?? 0) > 0],
          suppressions: this.buildSuppressionsArray(m.suppressions ?? []),

          hasDelay: [m.delay != null],
          delay: [m.delay ?? 0],
        })
      )
    );
  }

  staticMessageMappings(staticIndex: number): FormArray {
    return (this.mappings.at(staticIndex) as FormGroup).get('messageMappings') as FormArray;
  }

  saveChanges(): void {
    if (!this.topicId) return;

    this.form.updateValueAndValidity({ emitEvent: false });

    if (this.form.invalid) {
      this.form.markAllAsTouched();
      this.snackBar.open('Form contains errors!', 'Ok', { duration: 5000, panelClass: ['snackbar-error'] });
      return;
    }

    const raw = this.form.getRawValue();
    const rawName = raw.name ?? '';

    const subscription: SubscriptionUI | null = raw.hasSubscription
      ? {
        kind: raw.subscription.kind,
        qos: raw.subscription.qos,
        type: raw.subscription.type ?? '',
        mappings: (raw.subscription.mappings ?? []).map((m: any) => {
          // remove UI-only flags by default
          const {
            hasDelay,
            hasSuppressions,
            ...rest
          } = m;

          // Delay: only keep if enabled
          if (!hasDelay) {
            delete rest.delay;
          }

          // Suppressions: only keep if enabled (and remove empties)
          if (hasSuppressions) {
            rest.suppressions = (rest.suppressions ?? [])
              .map((s: string) => (s ?? '').trim())
              .filter((s: string) => s.length > 0);
          } else {
            delete rest.suppressions;
          }

          return rest;
        }),
      }
    : null;

    const topicLevelPatch = {
      name: rawName,
      subscription: subscription
    }

    this.mappingService.patchTopicLevel(this.topicId, topicLevelPatch);
    this.snackBar.open('Changes saved.', 'Ok', { duration: 2500 });
  }


  //helpers for ensuring at least one mapping exists
  private ensureAtLeastOneMapping(): void {
    if (this.mappings.length > 0) return;

    const kind = this.form.get('subscription.kind')?.value as SubscriptionKind;
    if (kind === 'static') {
      this.addStaticMapping();
    } else {
      // value or json -> your current UI uses the value mapping group
      this.addTemplateMapping();
    }
  }

  private buildSuppressionsArray(values: string[] = []): FormArray {
    return this.formBuilder.array(
      values.length
        ? values.map(v => this.formBuilder.control(v))
        : [this.formBuilder.control('')]
    );
  }

  addSuppression(mappingIndex: number): void {
    const arr = this.mappings
      .at(mappingIndex)
      .get('suppressions') as FormArray;

    arr.push(this.formBuilder.control(''));
  }

  private clearMappings(): void {
    this.mappings.clear();
  }

  removeTopicLevel(): void {
    if (!this.topicId) return;
    this.mappingService.removeTopicLevel(this.topicId);
  }

}
