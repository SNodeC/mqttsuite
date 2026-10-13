import { Component, inject, signal, ViewChild } from '@angular/core';
import { MatSidenav, MatSidenavModule } from '@angular/material/sidenav';
import { RouterOutlet } from '@angular/router';
import { SidenavService } from './shared/services/sidenav.service';
import { BreakpointObserver, Breakpoints } from '@angular/cdk/layout';
import { map, shareReplay } from 'rxjs';
import { AsyncPipe } from '@angular/common';
import { HomePage } from './components/home-page/home-page';
import { MatIconModule, MatIconRegistry } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { DomSanitizer } from '@angular/platform-browser';

const ICONS = [
  { name: 'chevron-down', url: 'assets/icons/chevron-down.svg' },
  { name: 'chevron-right', url: 'assets/icons/chevron-right.svg' },
  { name: 'cloud-done', url: 'assets/icons/cloud-done.svg' },
  { name: 'cloud-error', url: 'assets/icons/cloud-error.svg' },
  { name: 'create-topic', url: 'assets/icons/create-new-folder.svg' },
  { name: 'database-off', url: 'assets/icons/database-off.svg' },
  { name: 'database-upload', url: 'assets/icons/database-upload.svg' },
  { name: 'database', url: 'assets/icons/database.svg' },
  { name: 'delete', url: 'assets/icons/delete.svg' },
  { name: 'save', url: 'assets/icons/save.svg' },
  { name: 'settings', url: 'assets/icons/settings.svg' },
  { name: 'sync', url: 'assets/icons/sync.svg' },
  { name: 'tree', url: 'assets/icons/tree.svg' },
];

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, MatSidenavModule, MatIconModule, MatButtonModule, //HomePage, AsyncPipe,
  ],
  templateUrl: './app.html',
  styleUrl: './app.scss'
})
export class App {
  private bp = inject(BreakpointObserver);
  readonly isDesktop$ = this.bp.observe([Breakpoints.Medium, Breakpoints.Large, Breakpoints.XLarge]).pipe(
    map(r => r.matches),
    shareReplay({ bufferSize: 1, refCount: true })
  );
  // private readonly sidenav = inject(SidenavService); //not in use

  @ViewChild('drawer') drawer!: MatSidenav;
  showFiller = false;
  protected readonly title = signal('mqtt-integrator-ui');


  constructor(
    iconRegistry: MatIconRegistry,
    sanitizer: DomSanitizer
  ) {
    ICONS.forEach(icon =>
      iconRegistry.addSvgIcon(
        icon.name,
        sanitizer.bypassSecurityTrustResourceUrl(icon.url)
      )
    );
  }

  // ngAfterViewInit() {
  //   this.sidenav.register(this.drawer); //not in use
  // }

}
