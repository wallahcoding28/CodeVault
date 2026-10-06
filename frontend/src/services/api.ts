import {
  Question,
  DashboardSnapshot,
  RevisionItem,
  SessionProgress,
  RevisionScheduleResult,
  Diagnostics,
  Topic,
  Difficulty,
  Status,
  User,
  ConflictStrategy,
  ImportResult,
  PracticeNextCriteria,
  PracticeNextResult,
  PracticeQueueResponse,
  PracticeQueueRemoveResult,
  PracticeSessionFilterPayload,
  SinglePracticeResultResponse,
} from '../types';

const API_BASE = '/api';

export interface QuestionQueryParams {
  prefix?: string;
  keyword?: string;
  topic?: Topic | 'All';
  difficulty?: Difficulty | 'All';
  status?: Status | 'All';
  company?: string;
  favorite?: boolean;
  sortBy?: string;
  sortDir?: 'asc' | 'desc';
  sortAlgo?: 'mergeSort' | 'quickSort';
}

export interface RegisterPayload {
  username: string;
  displayName?: string;
  email?: string;
  password: string;
}

export interface LoginPayload {
  username: string;
  password: string;
}

async function request<T>(path: string, options: RequestInit = {}): Promise<T> {
  const url = path.startsWith('http') ? path : `${API_BASE}${path}`;
  const headers: Record<string, string> = {
    ...(options.headers as Record<string, string>),
  };

  if (options.body && typeof options.body === 'string' && !headers['Content-Type']) {
    headers['Content-Type'] = 'application/json';
  }

  const res = await fetch(url, {
    ...options,
    headers,
    credentials: 'include', // Ensure cookies are sent with requests
  });

  if (res.status === 401) {
    window.dispatchEvent(new CustomEvent('codevault:unauthorized'));
    const data = await res.json().catch(() => ({}));
    throw new Error(data.error || 'Authentication required');
  }

  if (!res.ok) {
    const data = await res.json().catch(() => ({}));
    throw new Error(data.error || `Request failed with status ${res.status}`);
  }

  return res.json();
}

async function downloadFile(path: string, fallbackFilename: string): Promise<void> {
  const url = path.startsWith('http') ? path : `${API_BASE}${path}`;
  const res = await fetch(url, {
    credentials: 'include',
  });

  if (res.status === 401) {
    window.dispatchEvent(new CustomEvent('codevault:unauthorized'));
    const data = await res.json().catch(() => ({}));
    throw new Error(data.error || 'Authentication required');
  }

  if (!res.ok) {
    const data = await res.json().catch(() => ({}));
    throw new Error(data.error || `Export failed with status ${res.status}`);
  }

  let filename = fallbackFilename;
  const disposition = res.headers.get('Content-Disposition');
  if (disposition && disposition.includes('filename=')) {
    const match = disposition.match(/filename=["']?([^"';]+)["']?/);
    if (match && match[1]) {
      filename = match[1].trim();
    }
  }

  const blob = await res.blob();
  const blobUrl = window.URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = blobUrl;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  window.URL.revokeObjectURL(blobUrl);
  document.body.removeChild(a);
}

export const api = {
  // Authentication
  async register(data: RegisterPayload): Promise<{ user: User }> {
    return request<{ user: User }>('/auth/register', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  },

  async login(data: LoginPayload): Promise<{ user: User }> {
    return request<{ user: User }>('/auth/login', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  },

  async logout(): Promise<{ message: string }> {
    return request<{ message: string }>('/auth/logout', {
      method: 'POST',
    });
  },

  async getMe(): Promise<{ user: User }> {
    return request<{ user: User }>('/auth/me', {
      method: 'GET',
    });
  },

  // Dashboard & Statistics
  async getDashboard(): Promise<DashboardSnapshot> {
    return request<DashboardSnapshot>('/dashboard');
  },

  // Questions CRUD & Queries
  async getQuestions(params: QuestionQueryParams = {}): Promise<Question[]> {
    const searchParams = new URLSearchParams();
    if (params.prefix) searchParams.set('prefix', params.prefix);
    if (params.keyword) searchParams.set('keyword', params.keyword);
    if (params.topic && params.topic !== 'All') searchParams.set('topic', params.topic);
    if (params.difficulty && params.difficulty !== 'All') searchParams.set('difficulty', params.difficulty);
    if (params.status && params.status !== 'All') searchParams.set('status', params.status);
    if (params.company) searchParams.set('company', params.company);
    if (params.favorite !== undefined) searchParams.set('favorite', params.favorite ? 'true' : 'false');
    if (params.sortBy) searchParams.set('sortBy', params.sortBy);
    if (params.sortDir) searchParams.set('sortDir', params.sortDir);
    if (params.sortAlgo) searchParams.set('sortAlgo', params.sortAlgo);

    const qs = searchParams.toString();
    const path = qs ? `/questions?${qs}` : '/questions';
    return request<Question[]>(path);
  },

  async getQuestionById(id: string): Promise<Question> {
    return request<Question>(`/questions/${encodeURIComponent(id)}`);
  },

  async createQuestion(data: Partial<Question>): Promise<Question> {
    return request<Question>('/questions', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  },

  async updateQuestion(id: string, data: Partial<Question>): Promise<Question> {
    return request<Question>(`/questions/${encodeURIComponent(id)}`, {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  },

  async deleteQuestion(id: string): Promise<boolean> {
    const data = await request<{ success: boolean }>(`/questions/${encodeURIComponent(id)}`, {
      method: 'DELETE',
    });
    return data.success;
  },

  // Spaced Revision
  async getDueRevisions(): Promise<RevisionItem[]> {
    return request<RevisionItem[]>('/revision/due');
  },

  async getUpcomingRevisions(): Promise<RevisionItem[]> {
    return request<RevisionItem[]>('/revision/upcoming');
  },

  async scheduleRevision(questionId: string, nextRevisionAt: number, priority: number = 2): Promise<boolean> {
    const data = await request<{ success: boolean }>('/revision/schedule', {
      method: 'POST',
      body: JSON.stringify({ questionId, nextRevisionAt, priority }),
    });
    return data.success;
  },

  // Practice Sessions (Queue FIFO)
  async startPracticeSession(options: { filter?: string; questionIds?: string[] }): Promise<SessionProgress> {
    return request<SessionProgress>('/practice/start', {
      method: 'POST',
      body: JSON.stringify(options),
    });
  },

  async getCurrentPracticeQuestion(): Promise<{ hasQuestion: boolean; question?: Question; progress?: SessionProgress }> {
    return request<{ hasQuestion: boolean; question?: Question; progress?: SessionProgress }>('/practice/current');
  },

  async submitPracticeVerdict(verdict: 'Solved' | 'NeedsReview' | 'Skipped'): Promise<{
    success: boolean;
    schedule: RevisionScheduleResult | null;
    progress: SessionProgress;
    hasNext: boolean;
    nextQuestion?: Question;
  }> {
    return request<{
      success: boolean;
      schedule: RevisionScheduleResult | null;
      progress: SessionProgress;
      hasNext: boolean;
      nextQuestion?: Question;
    }>('/practice/verdict', {
      method: 'POST',
      body: JSON.stringify({ verdict }),
    });
  },

  async skipPracticeQuestion(): Promise<{ skipped: boolean; progress: SessionProgress }> {
    return request<{ skipped: boolean; progress: SessionProgress }>('/practice/skip', {
      method: 'POST',
    });
  },

  async getPracticeProgress(): Promise<SessionProgress> {
    return request<SessionProgress>('/practice/progress');
  },

  async exitPracticeSession(): Promise<boolean> {
    const data = await request<{ success: boolean }>('/practice/exit', {
      method: 'POST',
    });
    return data.success;
  },

  // Stage 10: Advanced Practice Workflow
  async getPracticeNext(params: PracticeNextCriteria = {}): Promise<PracticeNextResult> {
    const searchParams = new URLSearchParams();
    if (params.topic && params.topic !== 'All') searchParams.set('topic', params.topic);
    if (params.difficulty && params.difficulty !== 'All') searchParams.set('difficulty', params.difficulty);
    if (params.includeDueRevisions !== undefined) searchParams.set('includeDueRevisions', params.includeDueRevisions ? 'true' : 'false');
    if (params.preferUnsolved !== undefined) searchParams.set('preferUnsolved', params.preferUnsolved ? 'true' : 'false');

    const qs = searchParams.toString();
    const path = qs ? `/practice/next?${qs}` : '/practice/next';
    return request<PracticeNextResult>(path);
  },

  async startFilteredPracticeSession(filter: PracticeSessionFilterPayload = {}): Promise<SessionProgress> {
    return request<SessionProgress>('/practice/session', {
      method: 'POST',
      body: JSON.stringify(filter),
    });
  },

  async getPracticeQueue(): Promise<PracticeQueueResponse> {
    return request<PracticeQueueResponse>('/practice/queue');
  },

  async removeFromPracticeQueue(questionId: string): Promise<PracticeQueueRemoveResult> {
    return request<PracticeQueueRemoveResult>(`/practice/queue/${encodeURIComponent(questionId)}`, {
      method: 'DELETE',
    });
  },

  async recordPracticeResultForQuestion(
    questionId: string,
    verdict: 'Solved' | 'NeedsReview' | 'Skipped'
  ): Promise<SinglePracticeResultResponse> {
    return request<SinglePracticeResultResponse>(`/practice/${encodeURIComponent(questionId)}/result`, {
      method: 'POST',
      body: JSON.stringify({ verdict }),
    });
  },

  // History & Diagnostics
  async getRecentHistory(): Promise<string[]> {
    return request<string[]>('/history');
  },

  async getDiagnostics(): Promise<Diagnostics> {
    return request<Diagnostics>('/settings/diagnostics');
  },

  // Stage 9.4: Profile, Security, Sessions & Export
  async updateProfile(data: { displayName?: string; email?: string }): Promise<{ user: User }> {
    return request<{ user: User }>('/auth/profile', {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  },

  async changePassword(data: { currentPassword: string; newPassword: string }): Promise<{ message: string }> {
    return request<{ message: string }>('/auth/change-password', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  },

  async getSessions(): Promise<{
    sessions: Array<{
      id: string;
      createdAt: number;
      expiresAt: number;
      lastSeenAt: number;
      isCurrentSession: boolean;
    }>;
  }> {
    return request('/auth/sessions');
  },

  async revokeSession(sessionId: string): Promise<{ message: string }> {
    return request<{ message: string }>(`/auth/sessions/${sessionId}/revoke`, {
      method: 'POST',
    });
  },

  async revokeOtherSessions(): Promise<{ message: string }> {
    return request<{ message: string }>('/auth/sessions/revoke-others', {
      method: 'POST',
    });
  },

  async downloadExport(format: 'json' | 'csv'): Promise<void> {
    return downloadFile(`/user/export?format=${format}`, `codevault_questions_export.${format}`);
  },

  async exportMarkdownCatalog(): Promise<void> {
    return downloadFile('/user/export/markdown', 'codevault-questions.md');
  },

  async exportAnkiCatalog(): Promise<void> {
    return downloadFile('/user/export/anki', 'codevault-questions-anki.tsv');
  },

  async importCatalog(file: File, conflictStrategy: ConflictStrategy = 'skip'): Promise<ImportResult> {
    const fileName = file.name.toLowerCase();
    const isJson = fileName.endsWith('.json');
    const isCsv = fileName.endsWith('.csv');

    if (!isJson && !isCsv) {
      throw new Error('Unsupported file type. Please select a .json or .csv catalog file.');
    }

    const contentType = isJson ? 'application/json' : 'text/csv';
    const content = await file.text();

    const url = `${API_BASE}/user/import?conflict_strategy=${encodeURIComponent(conflictStrategy)}`;
    const res = await fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': contentType,
      },
      body: content,
      credentials: 'include',
    });

    if (res.status === 401) {
      window.dispatchEvent(new CustomEvent('codevault:unauthorized'));
      const data = await res.json().catch(() => ({}));
      throw new Error(data.error || 'Authentication required');
    }

    const data = await res.json().catch(() => null);

    if (!data) {
      throw new Error(`Import failed with status ${res.status}`);
    }

    const result: ImportResult = {
      success: !!data.success,
      totalProcessed: data.totalProcessed ?? data.total_processed ?? 0,
      importedCount: data.importedCount ?? data.imported_count ?? 0,
      updatedCount: data.updatedCount ?? data.updated_count ?? 0,
      skippedCount: data.skippedCount ?? data.skipped_count ?? 0,
      errors: Array.isArray(data.errors) ? data.errors : (data.error ? [data.error] : []),
    };

    if (!res.ok && !result.errors.length) {
      result.errors.push(`Request failed with status ${res.status}`);
    }

    return result;
  },
};

