import { Component, Input, provideZonelessChangeDetection } from '@angular/core';
import { ComponentFixture, TestBed } from '@angular/core/testing';
import { MAT_DIALOG_DATA } from '@angular/material/dialog';
import { DialogShell } from './dialog-shell';

@Component({ template: '<p>{{ value }}</p>' })
class DialogContent {
  @Input() value = '';
}

describe('DialogShell', () => {
  let component: DialogShell;
  let fixture: ComponentFixture<DialogShell>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [DialogShell, DialogContent],
      providers: [provideZonelessChangeDetection(),
        { provide: MAT_DIALOG_DATA, useValue: { component: DialogContent, inputs: { value: 'Dialog input' } } }]
    }).compileComponents();
    fixture = TestBed.createComponent(DialogShell);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('should render the supplied component with its inputs', () => {
    expect(fixture.nativeElement.querySelector('p').textContent).toBe('Dialog input');
  });
});
