import { Component, DestroyRef, HostListener, inject } from '@angular/core';
import { MatToolbarModule } from '@angular/material/toolbar';
import { MatIconModule } from '@angular/material/icon';
import { ActivatedRoute, NavigationEnd, Router, RouterOutlet } from '@angular/router';
import { CommonModule } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { combineLatest, filter, map, startWith, switchMap, take } from 'rxjs';
import { takeUntilDestroyed } from '@angular/core/rxjs-interop';
import { TopicTree } from '../topic-tree/topic-tree';
import { MappingService } from '../../shared/services/mapping.service';
import { TopicLevelUI } from '../../shared/models/mapping-model';
import { BreakpointObserver, Breakpoints } from '@angular/cdk/layout';
import { JsonPreview } from '../json-preview/json-preview';
import { MatTooltipModule } from '@angular/material/tooltip';
// import { SidenavService } from '../../shared/services/sidenav.service';

@Component({
  selector: 'app-editor-shell',
  imports: [CommonModule,
    RouterOutlet,
    TopicTree,
    JsonPreview,
    MatToolbarModule,
    MatIconModule,
    MatButtonModule,
    MatTooltipModule
  ],
  templateUrl: './editor-shell.html',
  styleUrl: './editor-shell.scss'
})
export class EditorShell {
  private readonly router = inject(Router);
  private readonly route = inject(ActivatedRoute);
  private readonly destroyRef = inject(DestroyRef);
  private readonly mappingService = inject(MappingService);
  // private readonly sideNavService = inject(SidenavService); //not in use
  private readonly bp = inject(BreakpointObserver);

  dirty$ = this.mappingService.dirty$;
  isDeploying$ = this.mappingService.isDeploying$;

  // resizing
  sideviewPx = 0;
  previewPx = 0;


  private dragging = false;
  private startY = 0;
  private startSide = 0;
  private total = 0;

  title = 'MQTT Integrator';
  showTreeButton = false;

  isDesktop$ = this.bp.observe([Breakpoints.Medium, Breakpoints.Large, Breakpoints.XLarge]).pipe(
    map(r => r.matches)
  );
  isRootIntegratorRoute$ = this.router.events.pipe(
    filter(e => e instanceof NavigationEnd),
    startWith(null),
    map(() => this.router.url === '/')
  );
  showOutlet$ = combineLatest([this.isDesktop$, this.isRootIntegratorRoute$]).pipe(
    map(([isDesktop, isRoot]) => !(isDesktop && isRoot)) // hide only on desktop + root route
  );

  showBack$ = this.router.events.pipe(
    filter(e => e instanceof NavigationEnd),
    startWith(null),
    map(() => {
      const path = this.router.url.split('?')[0].split('#')[0];
      return path !== '/'; // root = topic tree
    })
  );

  currentTopicNode: TopicLevelUI | null = null;

  title$ = combineLatest([
  this.mappingService.integrator$,
  this.mappingService.activeTopicId$,
  this.isDesktop$
]).pipe(
  map(([integrator, activeTopicId, isDesktop]) => {
    // Desktop: always show integrator title
    if (isDesktop) {
      return integrator?.discoverPrefix ?? 'No active integrator';
    }

    // Mobile + edit-topic/:id → show topic name
    if (activeTopicId && integrator) {
      const topic = this.mappingService.getTopicLevelById(
        activeTopicId
      );
      return topic?.name ?? 'Topic';
    }

    // Mobile + root/config → fallback to integrator
    return integrator?.discoverPrefix ?? 'No active integrator';
  }),
  startWith('No active integrator')
);


  ngOnInit(): void {
    const total = this.getTotalHeight();
    this.sideviewPx = Math.round(total * 0.7);
    this.previewPx = total - this.sideviewPx;
  }

  goToTopicTree(): void {
    this.router.navigate(['']);
  }


  openConfig(): void {
    this.router.navigate(['config/']);
  }

  back(): void {
    window.history.back();
  }

  // openSidenav(): void {
  //   this.sideNavService.toggle();      //not in use
  // }

  openConfigDiffView() {
    this.router.navigate(['integrator', 'config-diff'])
  }

  syncWithServer() {
    this.mappingService.saveConfigToServer();
  }

  deploy() {
    this.mappingService.deployIntegrator();
  }



  // resizing

  startResize(ev: PointerEvent): void {
    ev.preventDefault();
    this.dragging = true;

    this.total = this.getTotalHeight();
    this.startY = ev.clientY;
    this.startSide = this.sideviewPx;

    // capture pointer so movement keeps working even if cursor leaves splitter
    (ev.target as HTMLElement).setPointerCapture(ev.pointerId);
  }

  @HostListener('window:pointermove', ['$event'])
  onPointerMove(ev: PointerEvent): void {
    if (!this.dragging) return;

    const dy = ev.clientY - this.startY;

    let nextSide = this.startSide + dy;

    // clamp
    const minSide = 140;
    const minPrev = 100;

    if (nextSide < minSide) nextSide = minSide;
    if (this.total - nextSide < minPrev) nextSide = this.total - minPrev;

    this.sideviewPx = nextSide;
    this.previewPx = this.total - nextSide;
  }

  @HostListener('window:pointerup')
  onPointerUp(): void {
    this.dragging = false;
  }

  @HostListener('window:resize')
  onResize(): void {
    // keep proportions-ish on resize
    const newTotal = this.getTotalHeight();
    const ratio = this.total ? this.sideviewPx / this.total : 0.7;

    this.total = newTotal;
    this.sideviewPx = Math.round(newTotal * ratio);
    this.previewPx = newTotal - this.sideviewPx;
  }

  private getTotalHeight(): number {
    // content height minus splitter height (8px)
    return Math.max(0, Math.round(window.innerHeight - 64 - 8));
  }
}
