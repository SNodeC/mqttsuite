import { Injectable } from '@angular/core';
import { MatSidenav } from '@angular/material/sidenav';

@Injectable({ providedIn: 'root' })
export class SidenavService {
  private drawer?: MatSidenav;

  register(drawer: MatSidenav) {
    this.drawer = drawer;
  }

  toggle() {
    console.log(this.drawer)
    return this.drawer?.toggle();
  }
  open()   { return this.drawer?.open(); }
  close()  { return this.drawer?.close(); }
}