import { inject, Injectable } from '@angular/core';
import { BehaviorSubject, catchError, combineLatest, filter, finalize, map, Observable, startWith, switchMap, throwError } from 'rxjs';
import { ConnectionUI, IntegratorUI, SubscriptionUI, TopicLevelUI } from '../models/mapping-model';
import { JsonRootConfig } from '../models/mapping-json.dto';
import { JsonRootConfigLoose, MappingAdapter } from '../adapter/mapping-adapter';
import { createTopicId } from '../utils/id.util';
import { MatSnackBar } from '@angular/material/snack-bar';
import { ActivatedRoute, NavigationEnd, Router } from '@angular/router';
import { ApiService } from './api.service';
import { SidenavService } from './sidenav.service';

@Injectable({ providedIn: 'root' })
export class MappingService {
  private readonly _router = inject(Router)
  private readonly _route = inject(ActivatedRoute);
  private readonly _apiService = inject(ApiService);
  private readonly _sideNavService = inject(SidenavService);
  private snackbar = inject(MatSnackBar)

  private readonly _integrator$ = new BehaviorSubject<IntegratorUI | null>(null);
  private readonly _integratorLive$ = new BehaviorSubject<JsonRootConfig | null>(null);
  private readonly _jsonPrettyStringExport$ = new BehaviorSubject<string | null>(null);
  private readonly _jsonExportStrict$ = new BehaviorSubject<JsonRootConfig | null>(null);
  private readonly _activeTopicId$ = new BehaviorSubject<string | null>(null);
  private readonly _dirty$ = new BehaviorSubject<boolean>(false);
  private readonly _isDeploying$ = new BehaviorSubject<boolean>(false);
  readonly integrator$: Observable<IntegratorUI | null> = this._integrator$.asObservable();
  readonly integratorLive$: Observable<JsonRootConfig | null> = this._integratorLive$.asObservable();
  readonly jsonPrettyStringExport$: Observable<string | null> = this._jsonPrettyStringExport$.asObservable();
  readonly jsonExportStrict$: Observable<JsonRootConfig | null> = this._jsonExportStrict$.asObservable();
  readonly activeTopicId$: Observable<string | null> = this._activeTopicId$.asObservable();
  readonly dirty$: Observable<boolean> = this._dirty$.asObservable();
  readonly isDeploying$: Observable<boolean> = this._isDeploying$.asObservable();



  constructor() {
    const routeId$ = this._router.events.pipe(
      filter(e => e instanceof NavigationEnd),
      startWith(null),
      map(() => this.getDeepestChild(this._route)),
      switchMap(r => r.paramMap),
      map(pm => pm.get('id')),
    );
    combineLatest([routeId$, this.integrator$]).subscribe(([id, integrator]) => {
      this._activeTopicId$.next(id);

      if (!id) return;          // /config, /, etc.
      if (!integrator) return;  // not loaded yet

      if (!this.getTopicLevelById(id)) {
        this._router.navigate(['']);
      }
    });

    //load config from server
    this._apiService.getConfig().subscribe({
      next: result => {
        if (result) {
          console.log(result)
          const integratorUI = MappingAdapter.fromJson(
            result as JsonRootConfig
          );

          console.log(integratorUI)
          this.setIntegrator(integratorUI);
          this._integratorLive$.next(result); // save current live config

          this.snackbar.open('Config loaded', 'OK', { duration: 2000 });
          //this._sideNavService.close();  <---- not in use right now
        }
      },
      error: error => {
        console.log("Error loading config from server: ", error);
        this.snackbar.open('There was an error loading the config from the server', 'OK', { duration: 2000 });
      }
    })
  }


  setIntegrator(integrator: IntegratorUI): void {
    this._integrator$.next(integrator);
    this._jsonPrettyStringExport$.next(this.exportAsPrettyJsonString());
  }

  reset(): void {
    this._integrator$.next(this.createEmptyIntegrator());
  }

  createNewIntegrator(client_id: string): void {
    this._integrator$.next(this.createNewIntegratorObject(client_id));
  }

  // -------------------------
  // QUERIES
  // -------------------------

  getTopicLevelById(id: string): TopicLevelUI | null {
    const integrator = this._integrator$.value;
    if (!integrator) return null;

    const find = (nodes: TopicLevelUI[]): TopicLevelUI | null => {
      for (const node of nodes) {
        if (node.id === id) return node;

        const foundInChildren = find(node.children);
        if (foundInChildren) return foundInChildren;
      }
      return null;
    };

    return find(integrator.mapping.topicLevels);
  }

  // -------------------------
  // COMMANDS
  // -------------------------

  setDiscoverPrefix(value: string): void {
    this.updateIntegrator(curr => ({
      ...curr,
      discoverPrefix: value,
    }));
  }

  patchConnection(patch: ConnectionUI): void {
    this.updateIntegrator(curr => ({
      ...curr,
      connection: { ...curr.connection, ...patch },
    }),
    );
    // this._apiService.patchConfig([{ value: MappingAdapter.connectionToJson(patch), path: '/connection' }]).subscribe(result => console.log(result));
  }

  addRootTopicLevel(name = 'new-level'): TopicLevelUI {
    console.log("added root topic")
    const node = this.createTopicLevel(name);

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: [...curr.mapping.topicLevels, node],
      },
    }));

    return node;
  }

  addChildTopicLevel(parentId: string, name = 'new-level'): TopicLevelUI {
    const child = this.createTopicLevel(name);

    const addChild = (nodes: TopicLevelUI[]): TopicLevelUI[] =>
      nodes.map(node => {
        if (node.id === parentId) {
          return {
            ...node,
            children: [...node.children, child],
          };
        }
        return { ...node, children: addChild(node.children) };
      });

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: addChild(curr.mapping.topicLevels),
      },
    }));

    return child;
  }

  renameTopicLevel(id: string, newName: string): void {
    const name = newName.trim();
    console.log("rename: ", name, id)
    const rename = (nodes: TopicLevelUI[]): TopicLevelUI[] =>
      nodes.map(node => {
        if (node.id === id) {
          return { ...node, name };
        }
        return { ...node, children: rename(node.children) };
      });

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: rename(curr.mapping.topicLevels),
      },
    }));
  }

  removeTopicLevel(id: string): void {
    const remove = (nodes: TopicLevelUI[]): TopicLevelUI[] =>
      nodes
        .filter(node => node.id !== id)
        .map(node => ({
          ...node,
          children: remove(node.children),
        }));

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: remove(curr.mapping.topicLevels),
      },
    }));

    if (!this.topicIdExists(id)) this._router.navigate(['']);
  }

  patchTopicLevel(id: TopicLevelUI['id'], patchData: Partial<TopicLevelUI>): void {
    const patch = (nodes: TopicLevelUI[]): TopicLevelUI[] =>
      nodes.map(node => {
        if (node.id === id) {
          return {
            ...node,
            ...patchData,
            // prevent accidental child replacement unless explicitly passed
            children: patchData.children ?? node.children,
          };
        }

        return {
          ...node,
          children: patch(node.children),
        };
      });

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: patch(curr.mapping.topicLevels),
      },
    }));
  }

  setTopicSubscription(id: string, subscription: SubscriptionUI | null): void {
    console.log("sub: ", id, subscription);
    const setSub = (nodes: TopicLevelUI[]): TopicLevelUI[] =>
      nodes.map(node => {
        if (node.id === id) {
          return { ...node, subscription: subscription };
        }
        return { ...node, children: setSub(node.children) };
      });

    this.updateIntegrator(curr => ({
      ...curr,
      mapping: {
        ...curr.mapping,
        topicLevels: setSub(curr.mapping.topicLevels),
      },
    }));
  }

  clearTopicSubscription(id: string): void {
    this.setTopicSubscription(id, null);
  }


  // -------------------------
  // HELPERS
  // -------------------------


  private updateIntegrator(
    updater: (current: IntegratorUI) => IntegratorUI,
    options: { markDirty?: boolean } = { markDirty: true }
  ): void {
    const curr = this._integrator$.value;
    if (!curr) return;

    const next = updater(curr);

    // If nothing changed, don't spam streams or mark dirty
    if (next === curr) return;

    if (options.markDirty) {
      if (options.markDirty) this._dirty$.next(true);
    }
    this._integrator$.next(next);
    this._jsonPrettyStringExport$.next(this.exportAsPrettyJsonString());
    // this._jsonExportStrict$.next(this.exportAsJsonDto());
  }

  private createTopicLevel(name: string): TopicLevelUI {
    return { id: createTopicId(), name, children: [], subscription: null };
  }

  private createEmptyIntegrator(): IntegratorUI {
    return {
      discoverPrefix: '',
      connection: {
        keepAlive: 60,
        clientId: 'new_client',
        cleanSession: true,
        willTopic: '',
        willMessage: '',
        willQos: 0,
        willRetain: false,
        username: '',
        password: '',
      },
      mapping: { plugins: [], topicLevels: [] },
    };
  }

  private createNewIntegratorObject(client_id: string): IntegratorUI {
    return {
      discoverPrefix: '',
      connection: {
        keepAlive: 60,
        clientId: client_id,
        cleanSession: true,
        willTopic: '',
        willMessage: '',
        willQos: 0,
        willRetain: false,
        username: '',
        password: '',
      },
      mapping: { plugins: [], topicLevels: [] },
    };
  }

  private exportAsJsonDto(loose?: false): JsonRootConfig | null;
  private exportAsJsonDto(loose?: true): JsonRootConfigLoose | null;
  private exportAsJsonDto(loose = false): JsonRootConfig | JsonRootConfigLoose | null {
    const integrator = this._integrator$.value;
    if (!integrator) return null;
    return loose ? MappingAdapter.toJsonLoose(integrator) : MappingAdapter.toJsonStrict(integrator);
  }

  exportAsPrettyJsonString(pretty = true): string | null {
    const dto = this.exportAsJsonDto(true);
    if (!dto) return null;
    return JSON.stringify(dto, null, pretty ? 2 : 0);
  }

  saveConfigToServer() {
    if (this._integrator$.value) {
      const strictJsonIntegrator = MappingAdapter.toJsonStrict(this._integrator$.value);
      this._apiService.patchConfig([{ value: strictJsonIntegrator, path: "" }]).subscribe({
        next: (result) => {
          console.log(result);
          this.snackbar.open('Integrator saved to server', 'OK', {
            duration: 2000
          });
        },
        error: (error) => {
          console.error(error);
          this.snackbar.open(
            'There was an error trying to sync with server',
            'OK',
            { duration: 2000 }
          );
        }
      });
    }
  }

  deployIntegrator() {
    const integrator = this._integrator$.value;
    if (!integrator) return;

    let strict;
    try {
      strict = MappingAdapter.toJsonStrict(integrator);
    } catch (error: unknown) {
      const message =
        error instanceof Error
          ? error.message
          : 'Unknown error while creating strict JSON';

      this.snackbar.open(message, 'Close', {
        duration: 5000,
        panelClass: ['snackbar-error']
      });
      return;
    }

    this._isDeploying$.next(true); //set state for deploying

    this._apiService.patchConfig([{ value: strict, path: "" }]).pipe(

      catchError(err =>
        throwError(() =>
          new Error(`Patch failed: ${this.extractMessage(err)}`)
        )
      ),

      switchMap(() =>
        this._apiService.validate(strict).pipe(
          catchError(err =>
            throwError(() =>
              new Error(`Validation failed: ${this.extractMessage(err)}`)
            )
          )
        )
      ),

      switchMap(() =>
        this._apiService.deploy().pipe(
          catchError(err =>
            throwError(() =>
              new Error(`Deploy failed: ${this.extractMessage(err)}`)
            )
          )
        )
      ),

      finalize(() => this._isDeploying$.next(false))
    ).subscribe({
      next: (deployResult) => {
        console.log(deployResult);
        this._dirty$.next(false);
        this.snackbar.open('Deployed successfully', 'OK', { duration: 2000 });
      },
      error: (error: Error) => {
        this.snackbar.open(error.message, 'Close', {
          duration: 6000,
          panelClass: ['snackbar-error']
        });
        console.error(error);
      }
    });
  }

  private getDeepestChild(ar: ActivatedRoute): ActivatedRoute {
    let curr = ar;
    while (curr.firstChild) curr = curr.firstChild;
    return curr;
  }

  private extractMessage(err: unknown): string {
    if (err instanceof Error) return err.message;

    if (typeof err === 'object' && err && 'error' in err) {
      const e = err as any;
      return e.error?.message ?? e.message ?? 'Unknown backend error';
    }

    return 'Unknown error';
  }


  private topicIdExists(id: string | null): boolean {
    if (!id) return false;
    return this.getTopicLevelById(id) !== null;
  }
}
