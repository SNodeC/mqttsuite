import { inject, Injectable } from '@angular/core';
import { HttpClient, HttpContext, HttpResponse } from '@angular/common/http';
import { Observable } from 'rxjs';
import { JsonRootConfig } from '../models/mapping-json.dto';
import { environment } from '../../../environments/environment';

interface JSONOpsParameters {
  value: unknown;
  path: string;
}

@Injectable({
  providedIn: 'root'
})

export class ApiService {
  private http = inject(HttpClient);
  baseUrl = environment.apiBaseUrl;

  getConfig(): Observable<JsonRootConfig> {
    return this.http.get<JsonRootConfig>(`${this.baseUrl}/config`)
  }

  patchConfig(jsonOpsParams: JSONOpsParameters[]): Observable<any> {
    jsonOpsParams = jsonOpsParams.map(ops => {
      return {...ops, op: 'replace'}
    });

    console.log(jsonOpsParams)

    return this.http.patch(`${this.baseUrl}/config`, jsonOpsParams, {
      headers: {
        'Content-Type': 'application/json-patch+json'
      }
    });
  }

  validate(integrator: JsonRootConfig): Observable<any> {
    return this.http.post(`${this.baseUrl}/config/validate`, integrator)
  }

  deploy(): Observable<any> {
    return this.http.post(`${this.baseUrl}/config/deploy`, {});
  }
}
