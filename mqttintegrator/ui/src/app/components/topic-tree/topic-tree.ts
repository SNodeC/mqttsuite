import { CommonModule } from '@angular/common';
import { Component, DestroyRef, inject } from '@angular/core';
import { MatBottomSheetModule } from '@angular/material/bottom-sheet';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MappingService } from '../../shared/services/mapping.service';
import { Router } from '@angular/router';
import { TopicLevelUI } from '../../shared/models/mapping-model';
import { takeUntilDestroyed } from '@angular/core/rxjs-interop';
import { MatDividerModule } from '@angular/material/divider';
import { PersistenceService } from '../../shared/services/persistence.service';
import { Observable } from 'rxjs';
import { MatTooltipModule } from '@angular/material/tooltip';

type TopicRow = { node: TopicLevelUI; depth: number, hasChildren: boolean, expanded?: boolean };
const EXPANDED_TREE_IDS = 'expanded_tree_ids';

@Component({
  selector: 'app-topic-tree-sheet',
  imports: [
    CommonModule,
    MatBottomSheetModule,
    MatDividerModule,
    MatIconModule,
    MatButtonModule,
    MatTooltipModule
  ],
  templateUrl: './topic-tree.html',
  styleUrl: './topic-tree.scss'
})
export class TopicTree {

  private readonly mappingService = inject(MappingService);
  private readonly persistenceService = inject(PersistenceService);
  private readonly router = inject(Router);
  // readonly sheetRef = inject(MatBottomSheetRef<TopicTreeSheet>);
  private readonly destroyRef = inject(DestroyRef);


  integratorDiscoverPrefix: string | undefined;
  nodes: TopicLevelUI[] | undefined;
  rows: TopicRow[] = [];
  expandedIds = new Set<string>();
  activeTopicId$: Observable<string | null>;
  integratorLoaded = false;


  constructor() {
    this.activeTopicId$ = this.mappingService.activeTopicId$;
    this.mappingService.integrator$
      .pipe(
        takeUntilDestroyed(this.destroyRef))
      .subscribe(integrator => {
        if(integrator) this.integratorLoaded = true;
        this.integratorDiscoverPrefix = integrator?.discoverPrefix;
        this.nodes = integrator?.mapping.topicLevels;
        if (this.nodes) {
          if (this.persistenceService.has<string[]>(EXPANDED_TREE_IDS + ':' + integrator?.discoverPrefix)) {
            this.expandedIds = new Set(this.persistenceService.get<string[]>(EXPANDED_TREE_IDS + ':' + integrator?.discoverPrefix) || []);
            console.log(this.expandedIds)
          }
          this.buildRows();
        }

      });
  }

  goTo(node: TopicLevelUI): void {
    this.router.navigate(['edit-topic', node.id]);
    // this.sheetRef.dismiss();
  }

  isExpanded(id: string): boolean {
    return this.expandedIds.has(id);
  }

  toggle(id: string): void {
    if (this.expandedIds.has(id)) {
      this.expandedIds.delete(id);
    } else {
      this.expandedIds.add(id);
    }
    this.persistenceService.set<string[]>(EXPANDED_TREE_IDS + ':' + this.integratorDiscoverPrefix, Array.from(this.expandedIds));
    this.buildRows();
  }

  private buildRows(): void {
    this.rows = [];

    const walk = (nodes: TopicLevelUI[] | undefined, depth: number) => {
      for (const node of nodes || []) {
        const hasChildren = !!node.children?.length;
        const expanded = this.isExpanded(node.id);

        this.rows.push({
          node,
          depth,
          hasChildren,
          expanded,
        });

        if (hasChildren && expanded) {
          walk(node.children, depth + 1);
        }
      }
    };

    walk(this.nodes, 0);
  }

  createNewRootTopic(): void {
    const newNode = this.mappingService.addRootTopicLevel('new_topic');
    this.router.navigate(['edit-topic', newNode.id]);
  }

  addChildTopic(node: TopicLevelUI): void {
    const newNode = this.mappingService.addChildTopicLevel(node.id, `${node.name}_child_topic`);
    this.router.navigate(['edit-topic', newNode.id]);
  }

  removeTopic(node: TopicLevelUI): void {
    this.mappingService.removeTopicLevel(node.id);
  }

  exportAsJsonString(): void {
    const json = this.mappingService.exportAsPrettyJsonString(true);
    console.log(json);
  }
}
