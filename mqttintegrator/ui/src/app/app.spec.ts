import { BreakpointObserver } from '@angular/cdk/layout';
import { of } from 'rxjs';
import { provideZonelessChangeDetection } from '@angular/core';
import { provideHttpClient } from '@angular/common/http';
import { HttpTestingController, provideHttpClientTesting } from '@angular/common/http/testing';
import { TestBed } from '@angular/core/testing';
import { provideRouter, Router, withHashLocation } from '@angular/router';
import { App } from './app';
import { routes } from './app.routes';

// Exercise the real editor route and HTTP boundary, rather than Angular's removed starter page.
describe('App', () => {
  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [App],
      providers: [provideZonelessChangeDetection(), provideRouter(routes, withHashLocation()),
        provideHttpClient(), provideHttpClientTesting(),
        { provide: BreakpointObserver, useValue: { observe: () => of({ matches: true, breakpoints: {} }) } }]
    }).compileComponents();
  });

  afterEach(() => TestBed.inject(HttpTestingController).verify());

  it('should create the app', () => {
    const fixture = TestBed.createComponent(App);
    expect(fixture.componentInstance).toBeTruthy();
  });

  it('should render the editor title and mapping loaded from the server', async () => {
    const fixture = TestBed.createComponent(App);
    fixture.detectChanges();
    await TestBed.inject(Router).navigateByUrl('/');
    const http = TestBed.inject(HttpTestingController);
    const config = http.expectOne('/api/config');
    expect(config.request.method).toBe('GET');
    config.flush({
      discover_prefix: 'Smoke integrator',
      connection: { client_id: 'Smoke integrator', keep_alive: 60, clean_session: true,
        will_topic: '', will_message: '', will_qos: 0, will_retain: false },
      mapping: { plugins: [], topic_level: [{ name: 'smoke-topic' }] }
    });
    fixture.detectChanges();
    http.match(request => request.url.startsWith('assets/icons/')).forEach(request =>
      request.flush('<svg xmlns="http://www.w3.org/2000/svg"></svg>'));
    await fixture.whenStable();
    fixture.detectChanges();
    const compiled = fixture.nativeElement as HTMLElement;
    expect(compiled.querySelector('.title')?.textContent).toContain('Smoke integrator');
    expect(compiled.querySelector('app-topic-tree-sheet')?.textContent).toContain('smoke-topic');
    expect(compiled.querySelector('.json-preview')?.textContent).toContain('smoke-topic');
    expect(compiled.querySelector('button[aria-label="Integrator config"]')).toBeTruthy();
  });
});
