import { Routes } from '@angular/router';
import { EditorShell } from './components/editor-shell/editor-shell';
import { EditTopic } from './components/edit-topic/edit-topic';
import { TopicTree } from './components/topic-tree/topic-tree';
import { IntegratorConfig } from './components/integrator-config/integrator-config';

export const routes: Routes = [
  {
    path: '',
    component: EditorShell,
    children: [
      { path: '', component: TopicTree },
      { path: 'config', component: IntegratorConfig },
      { path: 'edit-topic/:id', component: EditTopic },
      // optional: wildcard fallback
      { path: '**', redirectTo: '' },
    ],
  },
];
