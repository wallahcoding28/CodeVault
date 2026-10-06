import React, { useEffect, useState, useMemo, useRef } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import {
  Search,
  Filter,
  ArrowUpDown,
  Plus,
  Star,
  ExternalLink,
  X,
  AlertCircle,
  PlaySquare,
  CheckCircle2,
  Circle,
  Clock,
  Shuffle,
  Edit3,
  Trash2,
  Zap,
  ListOrdered,
} from 'lucide-react';
import { api } from '../services/api';
import {
  Question,
  Difficulty,
  Status,
  Topic,
} from '../types';
import { DifficultyBadge, TopicBadge } from '../components/Badge';
import { EmptyState } from '../components/EmptyState';
import { ConfirmModal } from '../components/ConfirmModal';
import { PracticeQueueDrawer } from '../components/PracticeQueueDrawer';
import { StartSessionModal } from '../components/StartSessionModal';

const TOPICS: { key: Topic | ''; label: string }[] = [
  { key: '', label: 'All Topics' },
  { key: 'Arrays', label: 'Arrays' },
  { key: 'Strings', label: 'Strings' },
  { key: 'LinkedLists', label: 'Linked Lists' },
  { key: 'StacksQueues', label: 'Stacks & Queues' },
  { key: 'Trees', label: 'Trees' },
  { key: 'Graphs', label: 'Graphs' },
  { key: 'DynamicProgramming', label: 'Dynamic Programming' },
  { key: 'BinarySearch', label: 'Binary Search' },
  { key: 'RecursionBacktracking', label: 'Backtracking' },
  { key: 'Greedy', label: 'Greedy' },
  { key: 'Heaps', label: 'Heaps' },
  { key: 'BitManipulation', label: 'Bit Manipulation' },
  { key: 'MathGeometry', label: 'Math & Geometry' },
];

const DIFFICULTIES: Difficulty[] = ['Easy', 'Medium', 'Hard'];
const STATUSES: Status[] = ['Unsolved', 'InProgress', 'Solved', 'Mastered'];

export const QuestionsPage: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams, setSearchParams] = useSearchParams();

  // State
  const [questions, setQuestions] = useState<Question[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  // Filters & Search
  const [searchQuery, setSearchQuery] = useState(searchParams.get('keyword') || searchParams.get('prefix') || '');
  const [selectedTopic, setSelectedTopic] = useState<string>(searchParams.get('topic') || '');
  const [selectedDifficulty, setSelectedDifficulty] = useState<string>(searchParams.get('difficulty') || '');
  const [selectedStatus, setSelectedStatus] = useState<string>(searchParams.get('status') || '');
  const [onlyFavorites, setOnlyFavorites] = useState<boolean>(searchParams.get('favorite') === 'true');
  const [selectedCompany, setSelectedCompany] = useState<string>(searchParams.get('company') || '');

  // Autocomplete Suggestions (powered by Trie prefix query in C++)
  const [suggestions, setSuggestions] = useState<Question[]>([]);
  const [showSuggestions, setShowSuggestions] = useState(false);
  const searchContainerRef = useRef<HTMLDivElement>(null);

  // Sorting
  const [sortBy, setSortBy] = useState<string>('updatedAt');
  const [sortDir, setSortDir] = useState<'asc' | 'desc'>('desc');
  const [sortAlgo, setSortAlgo] = useState<'mergeSort' | 'quickSort'>('mergeSort');

  // Deletion modal
  const [deleteTarget, setDeleteTarget] = useState<Question | null>(null);
  const [isDeleting, setIsDeleting] = useState(false);

  // Pagination
  const [currentPage, setCurrentPage] = useState(1);
  const [pageSize, setPageSize] = useState(15);

  // Stage 10: Practice Queue & Next Recommendation State
  const [practiceQueue, setPracticeQueue] = useState<Question[]>([]);
  const [isQueueOpen, setIsQueueOpen] = useState(false);
  const [isStartModalOpen, setIsStartModalOpen] = useState(false);
  const [isNextLoading, setIsNextLoading] = useState(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);

  const fetchQueue = async () => {
    try {
      const qRes = await api.getPracticeQueue();
      setPracticeQueue(qRes.queue || []);
    } catch {
      // ignore
    }
  };

  const handlePracticeNext = async () => {
    try {
      setIsNextLoading(true);
      const res = await api.getPracticeNext({
        topic: (selectedTopic as Topic) || undefined,
        difficulty: (selectedDifficulty as Difficulty) || undefined,
      });
      if (res.hasQuestion && res.question) {
        navigate(`/problems/${res.question.id}?reason=${encodeURIComponent(res.recommendationReason)}`);
      } else {
        setToastMessage(res.recommendationReason || 'All caught up! No problems currently pending practice.');
        setTimeout(() => setToastMessage(null), 3500);
      }
    } catch (err: any) {
      alert(err.message || 'Failed to determine next problem recommendation');
    } finally {
      setIsNextLoading(false);
    }
  };

  const handleRemoveFromQueue = async (qid: string) => {
    try {
      await api.removeFromPracticeQueue(qid);
      setPracticeQueue((prev) => prev.filter((q) => q.id !== qid));
    } catch (err: any) {
      alert(err.message || 'Failed to remove from practice queue');
    }
  };

  // Load questions
  const fetchQuestions = async () => {
    try {
      setLoading(true);
      setError(null);

      const params: any = {};
      const trimmed = searchQuery.trim();
      if (trimmed.length > 0) {
        params.keyword = trimmed;
      }
      if (selectedTopic) params.topic = selectedTopic;
      if (selectedDifficulty) params.difficulty = selectedDifficulty;
      if (selectedStatus) params.status = selectedStatus;
      if (onlyFavorites) params.favorite = true;
      if (selectedCompany) params.company = selectedCompany;
      if (sortBy) params.sortBy = sortBy;
      if (sortDir) params.sortDir = sortDir;
      if (sortAlgo) params.sortAlgo = sortAlgo;

      const data = await api.getQuestions(params);
      setQuestions(data);
      setCurrentPage(1);
    } catch (err: any) {
      console.error(err);
      setError('Failed to load problems. Ensure CodeVault C++ REST gateway is running.');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchQuestions();
  }, [
    selectedTopic,
    selectedDifficulty,
    selectedStatus,
    onlyFavorites,
    selectedCompany,
    sortBy,
    sortDir,
    sortAlgo,
  ]);

  // Synchronize state when URL query parameters change (e.g. top bar search or dashboard topic click)
  useEffect(() => {
    const kw = searchParams.get('keyword') || searchParams.get('prefix') || '';
    const top = searchParams.get('topic') || '';
    const diff = searchParams.get('difficulty') || '';
    const stat = searchParams.get('status') || '';
    const comp = searchParams.get('company') || '';
    const fav = searchParams.get('favorite') === 'true';

    setSearchQuery(kw);
    setSelectedTopic(top);
    setSelectedDifficulty(diff);
    setSelectedStatus(stat);
    setSelectedCompany(comp);
    setOnlyFavorites(fav);
  }, [searchParams]);

  // Autocomplete search suggestions debounced
  useEffect(() => {
    const trimmed = searchQuery.trim();
    if (trimmed.length >= 2) {
      const handler = setTimeout(async () => {
        try {
          const matched = await api.getQuestions({ prefix: trimmed });
          setSuggestions(matched.slice(0, 6));
          setShowSuggestions(true);
        } catch {
          setSuggestions([]);
        }
      }, 150);
      return () => clearTimeout(handler);
    } else {
      setSuggestions([]);
      setShowSuggestions(false);
    }
  }, [searchQuery]);

  // Click outside to dismiss suggestions
  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      if (searchContainerRef.current && !searchContainerRef.current.contains(e.target as Node)) {
        setShowSuggestions(false);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const handleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    setShowSuggestions(false);
    fetchQuestions();
  };

  const handleSelectSuggestion = (q: Question) => {
    setShowSuggestions(false);
    navigate(`/problems/${q.id}`);
  };

  const toggleFavorite = async (e: React.MouseEvent, q: Question) => {
    e.stopPropagation();
    try {
      const updated = await api.updateQuestion(q.id, { is_favorite: !q.is_favorite });
      setQuestions((prev) =>
        prev.map((item) => (item.id === q.id ? updated : item))
      );
    } catch (err) {
      console.error('Failed to toggle favorite', err);
    }
  };

  const handlePickRandom = () => {
    if (questions.length === 0) return;
    const randomIndex = Math.floor(Math.random() * questions.length);
    const chosen = questions[randomIndex];
    navigate(`/problems/${chosen.id}`);
  };

  const handleDelete = async () => {
    if (!deleteTarget) return;
    try {
      setIsDeleting(true);
      await api.deleteQuestion(deleteTarget.id);
      setQuestions((prev) => prev.filter((item) => item.id !== deleteTarget.id));
      setDeleteTarget(null);
    } catch (err: any) {
      alert(err.message || 'Failed to delete problem');
    } finally {
      setIsDeleting(false);
    }
  };

  const clearFilters = () => {
    setSearchQuery('');
    setSelectedTopic('');
    setSelectedDifficulty('');
    setSelectedStatus('');
    setOnlyFavorites(false);
    setSelectedCompany('');
    setSearchParams({});
  };

  const hasActiveFilters =
    Boolean(searchQuery) ||
    Boolean(selectedTopic) ||
    Boolean(selectedDifficulty) ||
    Boolean(selectedStatus) ||
    onlyFavorites ||
    Boolean(selectedCompany);

  // Summary counts for current list
  const solvedCount = useMemo(() => questions.filter((q) => q.status === 'Solved' || q.status === 'Mastered').length, [questions]);
  const dueCount = useMemo(() => {
    const now = Math.floor(Date.now() / 1000);
    return questions.filter((q) => q.next_revision_at > 0 && q.next_revision_at <= now).length;
  }, [questions]);

  // Pagination calculation
  const totalPages = Math.ceil(questions.length / pageSize) || 1;
  const safeCurrentPage = Math.min(Math.max(1, currentPage), totalPages);
  const paginatedQuestions = useMemo(() => {
    const start = (safeCurrentPage - 1) * pageSize;
    return questions.slice(start, start + pageSize);
  }, [questions, safeCurrentPage, pageSize]);

  const handlePracticeQuestion = async (e: React.MouseEvent, q: Question) => {
    e.stopPropagation();
    try {
      await api.startPracticeSession({ questionIds: [q.id] });
      navigate('/practice');
    } catch {
      navigate('/practice');
    }
  };

  // Formatter for Revision State
  const renderRevisionBadge = (q: Question) => {
    if (!q.next_revision_at || q.next_revision_at === 0) {
      return (
        <span className="text-[11px] font-mono text-slate-300 dark:text-slate-600">
          —
        </span>
      );
    }
    const now = Math.floor(Date.now() / 1000);
    const diff = q.next_revision_at - now;

    if (diff <= 0) {
      return (
        <span className="inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[10px] font-mono font-bold bg-rose-50 dark:bg-rose-950/50 text-rose-700 dark:text-rose-300 border border-rose-200 dark:border-rose-800">
          <Clock className="w-2.5 h-2.5" />
          Due
        </span>
      );
    }
    if (diff <= 86400) {
      return (
        <span className="inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[10px] font-mono font-medium bg-amber-50 dark:bg-amber-950/50 text-amber-800 dark:text-amber-300 border border-amber-200 dark:border-amber-800">
          Tomorrow
        </span>
      );
    }
    const days = Math.ceil(diff / 86400);
    if (q.revision_priority === 5) {
      return (
        <span className="inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[10px] font-mono font-semibold bg-indigo-50 dark:bg-indigo-950/50 text-indigo-700 dark:text-indigo-300 border border-indigo-200 dark:border-indigo-800">
          Mastered
        </span>
      );
    }
    return (
      <span className="inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[10px] font-mono text-slate-600 dark:text-slate-300 bg-slate-100 dark:bg-slate-800 border border-slate-200 dark:border-slate-700">
        In {days}d
      </span>
    );
  };

  // Status icon indicator
  const renderStatusIcon = (status: Status) => {
    switch (status) {
      case 'Solved':
        return (
          <span title="Solved" className="inline-flex">
            <CheckCircle2 className="w-4 h-4 text-emerald-500 fill-emerald-50" />
          </span>
        );
      case 'Mastered':
        return (
          <span title="Mastered (30d+ retention cycle)" className="inline-flex">
            <Star className="w-4 h-4 text-indigo-500 fill-indigo-100" />
          </span>
        );
      case 'InProgress':
        return (
          <span title="Attempted / In Progress" className="inline-flex">
            <div className="w-4 h-4 rounded-full border-2 border-amber-500 border-t-transparent flex items-center justify-center" />
          </span>
        );
      case 'Unsolved':
      default:
        return (
          <span title="Todo / Unsolved" className="inline-flex">
            <Circle className="w-4 h-4 text-slate-300 stroke-1" />
          </span>
        );
    }
  };

  return (
    <div className="max-w-7xl mx-auto px-6 py-6">
      {/* Top Header: Title & Direct Practice CTAs */}
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 mb-4">
        <div>
          <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
            Problems
          </h1>
          <div className="flex items-center gap-3 text-xs font-mono text-slate-500 dark:text-slate-400 mt-0.5">
            <span>{questions.length} problems</span>
            <span>·</span>
            <span className="text-emerald-700 dark:text-emerald-400 font-semibold">{solvedCount} solved</span>
            {dueCount > 0 && (
              <>
                <span>·</span>
                <span className="text-rose-600 dark:text-rose-400 font-bold">{dueCount} due for revision</span>
              </>
            )}
          </div>
        </div>

        <div className="flex items-center gap-2">
          {/* Practice Next CTA */}
          <button
            onClick={handlePracticeNext}
            disabled={isNextLoading || questions.length === 0}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-white bg-indigo-600 rounded-md hover:bg-indigo-700 transition-colors shadow-2xs disabled:opacity-50 cursor-pointer"
            title="Open deterministic next recommended problem"
          >
            <Zap className="w-3.5 h-3.5 fill-current" />
            <span>{isNextLoading ? 'Recommending...' : 'Practice Next'}</span>
          </button>

          {/* Practice Queue Drawer Toggle */}
          <button
            onClick={() => setIsQueueOpen(true)}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-slate-800 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs cursor-pointer"
            title="Inspect active practice queue"
          >
            <ListOrdered className="w-3.5 h-3.5 text-slate-600 dark:text-slate-400" />
            <span>Queue</span>
            {practiceQueue.length > 0 && (
              <span className="ml-0.5 px-1.5 py-0.2 rounded-full text-[10px] font-mono bg-indigo-100 dark:bg-indigo-950/80 text-indigo-800 dark:text-indigo-300 font-bold">
                {practiceQueue.length}
              </span>
            )}
          </button>

          {/* Targeted Drill Modal */}
          <button
            onClick={() => setIsStartModalOpen(true)}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-slate-800 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs cursor-pointer"
            title="Start targeted practice drill"
          >
            <PlaySquare className="w-3.5 h-3.5 text-slate-600 dark:text-slate-400" />
            <span>Drill Session</span>
          </button>

          {/* Pick Random */}
          <button
            onClick={handlePickRandom}
            disabled={questions.length === 0}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-slate-700 dark:text-slate-300 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs disabled:opacity-50 cursor-pointer"
            title="Open a random problem from the current filter"
          >
            <Shuffle className="w-3.5 h-3.5 text-slate-500 dark:text-slate-400" />
            <span>Pick Random</span>
          </button>

          {/* New Problem */}
          <button
            onClick={() => navigate('/problems/new')}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 rounded-md hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs cursor-pointer"
          >
            <Plus className="w-3.5 h-3.5" />
            <span>New Problem</span>
          </button>
        </div>
      </div>

      {/* Toast Alert Banner */}
      {toastMessage && (
        <div className="mb-4 p-3 bg-indigo-50 dark:bg-indigo-950/40 border border-indigo-200 dark:border-indigo-800 text-indigo-900 dark:text-indigo-200 rounded-lg text-xs font-medium flex items-center justify-between shadow-2xs">
          <div className="flex items-center gap-2">
            <Zap className="w-4 h-4 text-indigo-600 dark:text-indigo-400 shrink-0" />
            <span>{toastMessage}</span>
          </div>
          <button onClick={() => setToastMessage(null)} className="text-indigo-400 dark:text-indigo-500 hover:text-indigo-700 dark:hover:text-indigo-300 cursor-pointer">
            <X className="w-3.5 h-3.5" />
          </button>
        </div>
      )}

      {/* Horizontal Topic Filtering Strip (Familiar Coding Platform Pattern) */}
      <div className="flex items-center gap-1.5 overflow-x-auto pb-2.5 mb-3 scrollbar-none text-xs">
        {TOPICS.map((t) => {
          const isSelected = selectedTopic === t.key;
          return (
            <button
              key={t.label}
              onClick={() => setSelectedTopic(t.key)}
              className={`px-3 py-1 rounded-full whitespace-nowrap text-xs font-medium transition-colors shrink-0 cursor-pointer ${
                isSelected
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-2xs'
                  : 'bg-white dark:bg-slate-800 border border-slate-200/90 dark:border-slate-700 text-slate-600 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-700 hover:text-slate-900 dark:hover:text-slate-100'
              }`}
            >
              {t.label}
            </button>
          );
        })}
      </div>

      {/* Search & Compact Filter Bar */}
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-3 mb-4 shadow-2xs">
        {/* Search Input with Autocomplete */}
        <div ref={searchContainerRef} className="relative mb-2.5">
          <form onSubmit={handleSearchSubmit} className="flex gap-2">
            <div className="relative flex-1">
              <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
              <input
                type="text"
                placeholder="Search problems by title, tag, or keywords... (Press Enter to query)"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                onFocus={() => {
                  if (suggestions.length > 0) setShowSuggestions(true);
                }}
                className="w-full pl-8 pr-8 py-1.5 text-xs border border-slate-300 dark:border-slate-700 rounded-md bg-slate-50/50 dark:bg-slate-800 text-slate-900 dark:text-slate-100 placeholder-slate-400 dark:placeholder-slate-500 focus:bg-white dark:focus:bg-slate-800 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 focus:border-slate-900 transition-all font-mono"
              />
              {searchQuery && (
                <button
                  type="button"
                  onClick={() => {
                    setSearchQuery('');
                    fetchQuestions();
                  }}
                  className="absolute right-2.5 top-1/2 -translate-y-1/2 text-slate-400 hover:text-slate-600"
                >
                  <X className="w-3.5 h-3.5" />
                </button>
              )}
            </div>
            <button
              type="submit"
              className="px-4 py-1.5 text-xs font-semibold text-white bg-slate-800 dark:bg-slate-700 hover:bg-slate-900 dark:hover:bg-slate-600 transition-colors shrink-0 cursor-pointer"
            >
              Search
            </button>
          </form>

          {/* Autocomplete Dropdown */}
          {showSuggestions && suggestions.length > 0 && (
            <div className="absolute left-0 right-0 top-full mt-1 bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-md shadow-lg z-30 overflow-hidden divide-y divide-slate-100 dark:divide-slate-800">
              <div className="px-3 py-1.5 bg-slate-50 dark:bg-slate-800/80 text-[10px] font-mono text-slate-400 dark:text-slate-400 uppercase tracking-wider">
                Prefix Matching ({suggestions.length})
              </div>
              {suggestions.map((sug) => (
                <div
                  key={sug.id}
                  onClick={() => handleSelectSuggestion(sug)}
                  className="px-3 py-2 hover:bg-slate-50 dark:hover:bg-slate-800 cursor-pointer flex items-center justify-between transition-colors"
                >
                  <div className="flex items-center gap-2 min-w-0">
                    <span className="font-mono text-[11px] text-slate-400 dark:text-slate-500">{sug.id}</span>
                    <span className="font-medium text-xs text-slate-900 dark:text-slate-100 truncate">{sug.title}</span>
                  </div>
                  <div className="flex items-center gap-2 shrink-0">
                    <span className="text-[10px] font-mono text-slate-500 dark:text-slate-400">{sug.topic}</span>
                    <DifficultyBadge difficulty={sug.difficulty} className="text-[10px] px-1.5 py-0" />
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Filter Dropdowns and Sorters */}
        <div className="flex flex-wrap items-center gap-2 pt-2 border-t border-slate-100 dark:border-slate-800 text-xs">
          <div className="flex items-center gap-1 text-slate-400 dark:text-slate-400 font-mono text-[11px] mr-1">
            <Filter className="w-3 h-3" />
            <span>Filters:</span>
          </div>

          {/* Difficulty Dropdown */}
          <select
            value={selectedDifficulty}
            onChange={(e) => setSelectedDifficulty(e.target.value)}
            className="px-2 py-1 border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 text-xs focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
          >
            <option value="">Difficulty: All</option>
            {DIFFICULTIES.map((d) => (
              <option key={d} value={d}>
                {d}
              </option>
            ))}
          </select>

          {/* Status Dropdown */}
          <select
            value={selectedStatus}
            onChange={(e) => setSelectedStatus(e.target.value)}
            className="px-2 py-1 border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 text-xs focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
          >
            <option value="">Status: All</option>
            {STATUSES.map((s) => (
              <option key={s} value={s}>
                {s}
              </option>
            ))}
          </select>

          {/* Company */}
          <input
            type="text"
            placeholder="Company..."
            value={selectedCompany}
            onChange={(e) => setSelectedCompany(e.target.value)}
            className="px-2 py-1 border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 text-xs w-28 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
          />

          {/* Favorites Filter */}
          <button
            type="button"
            onClick={() => setOnlyFavorites(!onlyFavorites)}
            className={`inline-flex items-center gap-1 px-2 py-1 border rounded text-xs transition-colors cursor-pointer ${
              onlyFavorites
                ? 'bg-amber-50 dark:bg-amber-950/40 border-amber-300 dark:border-amber-800 text-amber-900 dark:text-amber-200 font-medium'
                : 'bg-white dark:bg-slate-800 border-slate-200 dark:border-slate-700 text-slate-600 dark:text-slate-300 hover:border-slate-300 dark:hover:border-slate-600'
            }`}
          >
            <Star
              className={`w-3 h-3 ${
                onlyFavorites ? 'fill-amber-400 text-amber-500' : 'text-slate-400'
              }`}
            />
            <span>Favorites</span>
          </button>

          {hasActiveFilters && (
            <button
              onClick={clearFilters}
              className="inline-flex items-center gap-1 px-2 py-1 text-[11px] text-slate-500 dark:text-slate-400 hover:text-rose-600 dark:hover:text-rose-400 font-mono cursor-pointer"
            >
              <X className="w-3 h-3" />
              Reset Filters
            </button>
          )}

          {/* Sort Controller */}
          <div className="flex items-center gap-1.5 ml-auto border-l border-slate-200 dark:border-slate-800 pl-3">
            <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">Sort:</span>
            <select
              value={sortBy}
              onChange={(e) => setSortBy(e.target.value)}
              className="px-2 py-1 border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-[11px] text-slate-700 dark:text-slate-200 focus:outline-none"
            >
              <option value="id">ID #</option>
              <option value="title">Title</option>
              <option value="difficulty">Difficulty</option>
              <option value="topic">Topic</option>
              <option value="status">Status</option>
              <option value="revisionPriority">Leitner Box</option>
              <option value="updatedAt">Last Practiced</option>
            </select>

            <button
              onClick={() => setSortDir(sortDir === 'asc' ? 'desc' : 'asc')}
              className="p-1 border border-slate-200 dark:border-slate-700 rounded text-slate-600 dark:text-slate-300 hover:bg-slate-50 dark:hover:bg-slate-800 cursor-pointer"
              title={`Order: ${sortDir}`}
            >
              <ArrowUpDown className="w-3 h-3" />
            </button>

            <select
              value={sortAlgo}
              onChange={(e) => setSortAlgo(e.target.value as 'mergeSort' | 'quickSort')}
              className="px-1.5 py-1 border border-slate-200 dark:border-slate-700 rounded bg-slate-50 dark:bg-slate-800 text-[10px] font-mono text-slate-600 dark:text-slate-300 focus:outline-none"
              title="C++ DSA Sorting Algorithm"
            >
              <option value="mergeSort">MergeSort</option>
              <option value="quickSort">QuickSort</option>
            </select>
          </div>
        </div>
      </div>

      {/* Main Coding Problems Table */}
      {loading ? (
        <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-16 flex flex-col items-center justify-center gap-2.5">
          <div className="w-7 h-7 border-2 border-slate-300 dark:border-slate-700 border-t-slate-900 dark:border-t-indigo-400 rounded-full animate-spin"></div>
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">Querying C++ DSA Engine...</span>
        </div>
      ) : error ? (
        <div className="bg-rose-50 dark:bg-rose-950/40 border border-rose-200 dark:border-rose-900 rounded-lg p-6 text-center">
          <AlertCircle className="w-5 h-5 text-rose-600 dark:text-rose-400 mx-auto mb-2" />
          <p className="text-xs text-rose-800 dark:text-rose-300 font-medium">{error}</p>
        </div>
      ) : questions.length === 0 ? (
        <EmptyState
          title={hasActiveFilters ? 'No matching problems found' : 'Your coding practice workspace is ready.'}
          description={
            hasActiveFilters
              ? 'Try widening your filters or search query to find matching problems.'
              : 'You currently have 0 problems in your library. Record your first algorithm problem or import an existing catalog to get started.'
          }
          actionLabel={hasActiveFilters ? 'Clear all filters' : 'Add First Problem'}
          onAction={hasActiveFilters ? clearFilters : () => navigate('/problems/new')}
          secondaryActionLabel={!hasActiveFilters ? 'Import Problems' : undefined}
          onSecondaryAction={!hasActiveFilters ? () => navigate('/settings?tab=export') : undefined}
        />
      ) : (
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg shadow-2xs overflow-hidden">
          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="bg-slate-50/90 dark:bg-slate-800/80 border-b border-slate-200/80 dark:border-slate-800 font-mono text-[11px] text-slate-500 dark:text-slate-400 uppercase tracking-wider">
                  <th className="py-2.5 px-3 w-10 text-center">Status</th>
                  <th className="py-2.5 px-3 w-16">#</th>
                  <th className="py-2.5 px-3">Title</th>
                  <th className="py-2.5 px-3 w-28">Difficulty</th>
                  <th className="py-2.5 px-3 w-36">Topic</th>
                  <th className="py-2.5 px-3 w-28">Revision</th>
                  <th className="py-2.5 px-3 w-10 text-center">★</th>
                  <th className="py-2.5 px-3 w-24 text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100 dark:divide-slate-800/60">
                {paginatedQuestions.map((q, idx) => {
                  const sequentialNumber = (currentPage - 1) * pageSize + idx + 1;
                  return (
                    <tr
                      key={q.id}
                      onClick={() => navigate(`/problems/${q.id}`)}
                      className="hover:bg-slate-50/80 dark:hover:bg-slate-800/50 cursor-pointer transition-colors group"
                    >
                      {/* Status Icon */}
                      <td className="py-2 px-3 text-center" onClick={(e) => e.stopPropagation()}>
                        <div className="flex justify-center">
                          {renderStatusIcon(q.status)}
                        </div>
                      </td>

                      {/* Problem Sequential / ID */}
                      <td className="py-2 px-3 font-mono text-slate-400 dark:text-slate-500 text-[11px]">
                        {sequentialNumber}
                      </td>

                      {/* Title & Metadata */}
                      <td className="py-2 px-3">
                        <div className="flex items-center gap-2">
                          <span className="font-semibold text-slate-900 dark:text-slate-100 group-hover:text-indigo-600 dark:group-hover:text-indigo-400 transition-colors">
                            {q.title}
                          </span>
                          {practiceQueue.some((item) => item.id === q.id) && (
                            <span className="px-1.5 py-0.2 text-[10px] font-mono font-semibold bg-indigo-50 dark:bg-indigo-950/80 text-indigo-700 dark:text-indigo-300 border border-indigo-200 dark:border-indigo-800 rounded">
                              Queued
                            </span>
                          )}
                          {q.platform && (
                            <span className="px-1.5 py-0.2 text-[10px] font-mono bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 rounded">
                              {q.platform}
                            </span>
                          )}
                          {q.source_url && (
                            <a
                              href={q.source_url}
                              target="_blank"
                              rel="noopener noreferrer"
                              onClick={(e) => e.stopPropagation()}
                              className="text-slate-400 hover:text-slate-600 dark:hover:text-slate-300"
                              title="Open external problem link"
                            >
                              <ExternalLink className="w-3 h-3" />
                            </a>
                          )}
                        </div>
                        {/* Compact tags & company */}
                        <div className="flex items-center gap-2 mt-0.5">
                          <span className="font-mono text-[10px] text-slate-400 dark:text-slate-500">{q.id}</span>
                          {q.company && (
                            <span className="text-[10px] font-mono text-slate-500 dark:text-slate-400">
                              · {q.company}
                            </span>
                          )}
                          {q.tags && q.tags.length > 0 && (
                            <span className="text-[10px] font-mono text-slate-400 dark:text-slate-500 truncate max-w-xs">
                              · {q.tags.slice(0, 2).map((t) => `#${t}`).join(' ')}
                            </span>
                          )}
                        </div>
                      </td>

                      {/* Difficulty */}
                      <td className="py-2 px-3">
                        <DifficultyBadge difficulty={q.difficulty} />
                      </td>

                      {/* Topic */}
                      <td className="py-2 px-3">
                        <TopicBadge topic={q.topic} />
                      </td>

                      {/* Revision State */}
                      <td className="py-2 px-3">
                        {renderRevisionBadge(q)}
                      </td>

                      {/* Star Favorite */}
                      <td className="py-2 px-3 text-center" onClick={(e) => toggleFavorite(e, q)}>
                        <button type="button" className="p-1 hover:bg-slate-100 dark:hover:bg-slate-800 rounded cursor-pointer">
                          <Star
                            className={`w-3.5 h-3.5 mx-auto transition-transform active:scale-125 ${
                              q.is_favorite
                                ? 'fill-amber-400 text-amber-500'
                                : 'text-slate-300 dark:text-slate-600 hover:text-slate-400 dark:hover:text-slate-400'
                            }`}
                          />
                        </button>
                      </td>

                      {/* Actions */}
                      <td className="py-2 px-3 text-right" onClick={(e) => e.stopPropagation()}>
                        <div className="inline-flex items-center gap-1 opacity-70 group-hover:opacity-100 transition-opacity">
                          <button
                            onClick={(e) => handlePracticeQuestion(e, q)}
                            className="p-1 text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100 dark:hover:bg-slate-800 rounded cursor-pointer"
                            title={`Practice ${q.title}`}
                          >
                            <PlaySquare className="w-3.5 h-3.5" />
                          </button>
                          <button
                            onClick={() => navigate(`/problems/${q.id}/edit`)}
                            className="p-1 text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100 dark:hover:bg-slate-800 rounded cursor-pointer"
                            title="Edit Problem"
                          >
                            <Edit3 className="w-3.5 h-3.5" />
                          </button>
                          <button
                            onClick={() => setDeleteTarget(q)}
                            className="p-1 text-slate-400 dark:text-slate-500 hover:text-rose-600 dark:hover:text-rose-400 hover:bg-rose-50 dark:hover:bg-rose-950/40 rounded cursor-pointer"
                            title="Delete Problem"
                          >
                            <Trash2 className="w-3.5 h-3.5" />
                          </button>
                        </div>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>

          {/* Pagination Footer */}
          <div className="px-4 py-2.5 bg-slate-50/70 dark:bg-slate-800/60 border-t border-slate-200/80 dark:border-slate-800 flex items-center justify-between text-xs">
            <div className="flex items-center gap-3">
              <span className="text-slate-500 dark:text-slate-400 font-mono text-[11px]">
                Showing {(safeCurrentPage - 1) * pageSize + 1}–
                {Math.min(safeCurrentPage * pageSize, questions.length)} of {questions.length} problems
              </span>
              <div className="flex items-center gap-1 text-[11px] font-mono text-slate-400 dark:text-slate-500">
                <span>Per page:</span>
                {[15, 30, 50].map((sz) => (
                  <button
                    key={sz}
                    onClick={() => {
                      setPageSize(sz);
                      setCurrentPage(1);
                    }}
                    className={`px-1.5 py-0.5 rounded cursor-pointer ${
                      pageSize === sz ? 'bg-slate-200 dark:bg-slate-700 text-slate-900 dark:text-slate-100 font-bold' : 'hover:bg-slate-100 dark:hover:bg-slate-700 text-slate-500 dark:text-slate-400'
                    }`}
                  >
                    {sz}
                  </button>
                ))}
              </div>
            </div>

            <div className="flex items-center gap-2">
              <button
                disabled={safeCurrentPage === 1}
                onClick={() => setCurrentPage((p) => Math.max(1, p - 1))}
                className="px-2 py-0.5 text-xs border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 disabled:opacity-40 disabled:cursor-not-allowed hover:bg-slate-50 dark:hover:bg-slate-700 cursor-pointer"
              >
                Prev
              </button>
              <span className="text-slate-600 dark:text-slate-400 font-mono text-[11px]">
                {safeCurrentPage} / {totalPages}
              </span>
              <button
                disabled={safeCurrentPage === totalPages}
                onClick={() => setCurrentPage((p) => Math.min(totalPages, p + 1))}
                className="px-2 py-0.5 text-xs border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 disabled:opacity-40 disabled:cursor-not-allowed hover:bg-slate-50 dark:hover:bg-slate-700 cursor-pointer"
              >
                Next
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Delete Confirmation Modal */}
      <ConfirmModal
        isOpen={Boolean(deleteTarget)}
        title="Delete Problem Entry"
        message={`Are you sure you want to permanently delete "${deleteTarget?.title}" (${deleteTarget?.id})? This will update CSV persistence and re-index the DSA Trie and MinHeap.`}
        confirmLabel="Confirm Delete"
        cancelLabel="Cancel"
        isDestructive={true}
        isLoading={isDeleting}
        onConfirm={handleDelete}
        onCancel={() => setDeleteTarget(null)}
      />

      {/* Practice Queue Drawer */}
      <PracticeQueueDrawer
        isOpen={isQueueOpen}
        onClose={() => setIsQueueOpen(false)}
        queue={practiceQueue}
        onRemove={handleRemoveFromQueue}
      />

      {/* Start Drill Session Modal */}
      <StartSessionModal
        isOpen={isStartModalOpen}
        onClose={() => {
          setIsStartModalOpen(false);
          fetchQueue();
        }}
        defaultTopic={selectedTopic}
        defaultDifficulty={selectedDifficulty}
      />
    </div>
  );
};
