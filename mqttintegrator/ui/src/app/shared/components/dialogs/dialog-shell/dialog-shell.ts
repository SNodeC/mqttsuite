import { NgComponentOutlet } from '@angular/common';
import { Component, Inject } from '@angular/core';
import { MAT_DIALOG_DATA } from '@angular/material/dialog';

export interface DialogShellData {
  component: any;
  inputs?: Record<string, unknown>;
  title?: string;
}

@Component({
  selector: 'app-dialog-shell',
  imports: [NgComponentOutlet],
  template: `
    <ng-container
      *ngComponentOutlet="data.component; inputs: data.inputs">
    </ng-container>
  `,
  styleUrl: './dialog-shell.scss'
})
export class DialogShell {
  constructor(@Inject(MAT_DIALOG_DATA) public data: DialogShellData) { }
}
