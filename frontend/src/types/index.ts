export type Difficulty = 'Easy' | 'Medium' | 'Hard' | 'Unknown';

export type Status = 'Unsolved' | 'InProgress' | 'Solved' | 'Mastered';

export type Topic =
  | 'Arrays'
  | 'Strings'
  | 'LinkedLists'
  | 'StacksQueues'
  | 'Trees'
  | 'Graphs'
  | 'DynamicProgramming'
  | 'BinarySearch'
  | 'RecursionBacktracking'
  | 'Greedy'
  | 'Heaps'
  | 'BitManipulation'
  | 'MathGeometry'
  | 'Other';

export type Platform =
  | 'LeetCode'
  | 'HackerRank'
  | 'Codeforces'
  | 'GeeksforGeeks'
  | 'CodeStudio'
  | 'Custom';

export interface Question {
  id: string;
  title: string;
  description: string;
  topic: Topic;
  difficulty: Difficulty;
  company: string;
  platform: Platform;
  source_url: string;
  status: Status;
  is_favorite: boolean;
  notes: string;
  created_at: number;
  updated_at: number;
  last_practiced_at: number;
  next_revision_at: number;
  revision_priority: number;
  tags: string[];
  owner_id: string;
}

export interface DifficultyStats {
  easyCount: number;
  mediumCount: number;
  hardCount: number;
  easyPercentage: number;
  mediumPercentage: number;
  hardPercentage: number;
  totalQuestions: number;
}

export interface TopicCount {
  topic: Topic;
  topicName: string;
  count: number;
  percentage: number;
}

export interface TopicStats {
  totalQuestions: number;
  distinctTopicsCount: number;
  topicCounts: TopicCount[];
}

export interface StatusStats {
  unsolvedCount: number;
  inProgressCount: number;
  solvedCount: number;
  masteredCount: number;
  unsolvedPercentage: number;
  inProgressPercentage: number;
  solvedPercentage: number;
  masteredPercentage: number;
}

export interface RevisionStats {
  dueCount: number;
  upcomingCount: number;
  scheduledCount: number;
  unscheduledCount: number;
  duePercentageOfScheduled: number;
  scheduledPercentageOfTotal: number;
  priorityCounts: number[];
  levelCounts: number[];
}

export interface PracticeStats {
  practicedCount: number;
  unpracticedCount: number;
  practicedPercentage: number;
  lastPracticedTimestamp: number;
}

export interface OverallStats {
  totalQuestions: number;
  solvedCount: number;
  inProgressCount: number;
  unsolvedCount: number;
  masteredCount: number;
  favoriteCount: number;
  dueForRevisionCount: number;
  upcomingRevisionsCount: number;
  totalScheduledCount: number;
  completionPercentage: number;
  solvedPercentage: number;
  masteredPercentage: number;
  inProgressPercentage: number;
  unsolvedPercentage: number;
  favoritePercentage: number;
}

export interface DashboardSnapshot {
  generatedAt: number;
  overall: OverallStats;
  difficulty: DifficultyStats;
  topic: TopicStats;
  status: StatusStats;
  revision: RevisionStats;
  practice: PracticeStats;
}

export interface RevisionItem {
  questionId: string;
  nextRevisionAt: number;
  priority: number;
}

export interface SessionProgress {
  total: number;
  completed: number;
  remaining: number;
  skipped: number;
}

export interface RevisionScheduleResult {
  nextLevel: number;
  nextRevisionAt: number;
  revisionPriority: number;
  lastPracticedAt: number;
  newStatus: Status;
  intervalSeconds: number;
}

export interface Diagnostics {
  version: string;
  runtime: string;
  storagePath: string;
  invariantsPassed: number;
  totalInvariants: number;
  prefixTrieActive: boolean;
  minHeapActive: boolean;
  practiceQueueActive: boolean;
  historyStackActive: boolean;
  sortingEngineActive: boolean;
  questionCount?: number;
  indexedTitles?: number;
  scheduledRevisions?: number;
  authEnabled?: boolean;
}

export interface User {
  id: string;
  username: string;
  displayName: string;
  email: string;
  createdAt: number;
  updatedAt: number;
  active: boolean;
}

export type ConflictStrategy = 'skip' | 'overwrite' | 'generate_new_id';

export interface ImportResult {
  success: boolean;
  totalProcessed: number;
  importedCount: number;
  updatedCount: number;
  skippedCount: number;
  errors: string[];
}

// Stage 10: Advanced Practice Workflow Types
export interface PracticeNextCriteria {
  topic?: Topic | 'All';
  difficulty?: Difficulty | 'All';
  includeDueRevisions?: boolean;
  preferUnsolved?: boolean;
}

export interface PracticeNextResult {
  hasQuestion: boolean;
  question?: Question | null;
  recommendationReason: string;
}

export interface PracticeQueueResponse {
  queue: Question[];
  count: number;
}

export interface PracticeQueueRemoveResult {
  success: boolean;
  removed: boolean;
  questionId: string;
}

export interface PracticeSessionFilterPayload {
  topic?: Topic | 'All';
  difficulty?: Difficulty | 'All';
  status?: Status | 'All';
  company?: string;
  isFavorite?: boolean;
  dueOnly?: boolean;
}

export interface SinglePracticeResultResponse {
  success: boolean;
  questionId: string;
  verdict: string;
  question: Question;
  schedule: RevisionScheduleResult | null;
}

