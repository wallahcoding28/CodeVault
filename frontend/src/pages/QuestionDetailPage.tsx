import React, { useEffect, useState } from 'react';
import { useParams, useNavigate, useSearchParams } from 'react-router-dom';
import {
  ArrowLeft,
  Calendar,
  Clock,
  ExternalLink,
  Star,
  CheckCircle2,
  AlertCircle,
  HelpCircle,
  Edit3,
  Trash2,
  Building2,
  Tag,
  PlaySquare,
  BookOpen,
  Sparkles,
  ArrowRight,
  ListOrdered,
  X,
} from 'lucide-react';
import { api } from '../services/api';
import { Question, Status, SinglePracticeResultResponse } from '../types';
import { DifficultyBadge, TopicBadge } from '../components/Badge';
import { ConfirmModal } from '../components/ConfirmModal';

export const QuestionDetailPage: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const recommendationReason = searchParams.get('reason');

  const [question, setQuestion] = useState<Question | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  // Queue and Practice Next state
  const [queueInfo, setQueueInfo] = useState<{ inQueue: boolean; position: number } | null>(null);
  const [lastVerdictResult, setLastVerdictResult] = useState<SinglePracticeResultResponse | null>(null);

  // Modals & Actions
  const [showScheduleModal, setShowScheduleModal] = useState(false);
  const [scheduleDays, setScheduleDays] = useState(3);
  const [showDeleteModal, setShowDeleteModal] = useState(false);
  const [isActing, setIsActing] = useState(false);

  const fetchQueueStatus = async (questionId: string) => {
    try {
      const qRes = await api.getPracticeQueue();
      const idx = qRes.queue.findIndex((item) => item.id === questionId);
      if (idx !== -1) {
        setQueueInfo({ inQueue: true, position: idx + 1 });
      } else {
        setQueueInfo({ inQueue: false, position: 0 });
      }
    } catch {
      setQueueInfo(null);
    }
  };

  const fetchQuestion = async () => {
    if (!id) return;
    try {
      setLoading(true);
      setError(null);
      const data = await api.getQuestionById(id);
      setQuestion(data);
      fetchQueueStatus(id);
    } catch (err: any) {
      console.error(err);
      setError('Problem not found or backend server is unreachable.');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    setLastVerdictResult(null);
    fetchQuestion();
  }, [id]);

  useEffect(() => {
    if (!showScheduleModal) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setShowScheduleModal(false);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [showScheduleModal]);

  // Keyboard shortcut for Practice Next when verdict recorded
  useEffect(() => {
    if (!lastVerdictResult) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.target && ['INPUT', 'TEXTAREA'].includes((e.target as HTMLElement).tagName)) return;
      if (e.key === 'n' || e.key === 'N') {
        e.preventDefault();
        handlePracticeNext();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [lastVerdictResult]);

  const handleToggleFavorite = async () => {
    if (!question) return;
    try {
      const updated = await api.updateQuestion(question.id, { is_favorite: !question.is_favorite });
      setQuestion(updated);
    } catch (err) {
      console.error(err);
    }
  };

  const handleStatusChange = async (newStatus: Status) => {
    if (!question) return;
    try {
      setIsActing(true);
      const updated = await api.updateQuestion(question.id, { status: newStatus });
      setQuestion(updated);
    } catch (err: any) {
      alert(err.message || 'Failed to update status');
    } finally {
      setIsActing(false);
    }
  };

  const handleRecordVerdict = async (verdict: 'Solved' | 'NeedsReview') => {
    if (!question) return;
    try {
      setIsActing(true);
      const res = await api.recordPracticeResultForQuestion(question.id, verdict);
      if (res && res.question) {
        setQuestion(res.question);
      }
      setLastVerdictResult(res);
      setQueueInfo({ inQueue: false, position: 0 });
    } catch (err: any) {
      alert(err.message || 'Failed to record verdict');
    } finally {
      setIsActing(false);
    }
  };

  const handleRemoveFromQueue = async () => {
    if (!question) return;
    try {
      setIsActing(true);
      await api.removeFromPracticeQueue(question.id);
      setQueueInfo({ inQueue: false, position: 0 });
    } catch (err: any) {
      alert(err.message || 'Failed to remove from practice queue');
    } finally {
      setIsActing(false);
    }
  };

  const handlePracticeNext = async () => {
    try {
      setIsActing(true);
      const nextRes = await api.getPracticeNext();
      if (nextRes.hasQuestion && nextRes.question) {
        navigate(
          `/problems/${nextRes.question.id}?reason=${encodeURIComponent(
            nextRes.recommendationReason || ''
          )}`
        );
        setLastVerdictResult(null);
      } else {
        alert('All caught up! No practice problems currently recommended.');
      }
    } catch (err: any) {
      alert(err.message || 'Failed to fetch next practice problem');
    } finally {
      setIsActing(false);
    }
  };

  const handleScheduleRevision = async () => {
    if (!question) return;
    try {
      setIsActing(true);
      const nextEpoch = Math.floor(Date.now() / 1000) + scheduleDays * 86400;
      const priority = scheduleDays === 1 ? 1 : scheduleDays === 3 ? 2 : scheduleDays === 7 ? 3 : scheduleDays === 14 ? 4 : 5;
      await api.scheduleRevision(question.id, nextEpoch, priority);
      await fetchQuestion();
      setShowScheduleModal(false);
    } catch (err: any) {
      alert(err.message || 'Failed to schedule revision');
    } finally {
      setIsActing(false);
    }
  };

  const handleStartPracticeThis = async () => {
    if (!question) return;
    try {
      setIsActing(true);
      await api.startPracticeSession({ questionIds: [question.id] });
      navigate('/practice');
    } catch (err: any) {
      alert(err.message || 'Failed to start practice session');
    } finally {
      setIsActing(false);
    }
  };

  const handleDelete = async () => {
    if (!question) return;
    try {
      setIsActing(true);
      await api.deleteQuestion(question.id);
      navigate('/problems');
    } catch (err: any) {
      alert(err.message || 'Failed to delete question');
      setIsActing(false);
    }
  };

  const formatDate = (epochSeconds: number) => {
    if (!epochSeconds || epochSeconds === 0) return 'Never';
    return new Date(epochSeconds * 1000).toLocaleDateString('en-US', {
      year: 'numeric',
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  const getRelativeRevisionTime = (epochSeconds: number) => {
    if (!epochSeconds || epochSeconds === 0) return 'Not scheduled';
    const now = Math.floor(Date.now() / 1000);
    const diff = epochSeconds - now;
    if (diff <= 0) return 'Due now';
    if (diff <= 86400) return 'Tomorrow';
    const days = Math.ceil(diff / 86400);
    return `In ${days} days`;
  };

  if (loading) {
    return (
      <div className="max-w-5xl mx-auto px-6 py-16 flex flex-col items-center justify-center gap-2.5">
        <div className="w-7 h-7 border-2 border-slate-300 border-t-slate-900 rounded-full animate-spin"></div>
        <span className="text-xs font-mono text-slate-500">Loading problem workbench...</span>
      </div>
    );
  }

  if (error || !question) {
    return (
      <div className="max-w-2xl mx-auto px-6 py-16 text-center">
        <AlertCircle className="w-8 h-8 text-rose-600 mx-auto mb-3" />
        <h2 className="text-base font-bold text-slate-900 mb-1">Problem Not Found</h2>
        <p className="text-xs text-slate-500 mb-5">{error || 'Invalid problem ID.'}</p>
        <button
          onClick={() => navigate('/problems')}
          className="inline-flex items-center gap-1.5 px-3.5 py-1.5 text-xs font-semibold text-white bg-slate-900 rounded-md hover:bg-slate-800"
        >
          <ArrowLeft className="w-3.5 h-3.5" />
          Back to Problems
        </button>
      </div>
    );
  }

  const boxDescriptions = [
    { box: 1, interval: '1 day', desc: 'Daily verification' },
    { box: 2, interval: '3 days', desc: 'Short interval recall' },
    { box: 3, interval: '7 days', desc: 'Weekly retention check' },
    { box: 4, interval: '14 days', desc: 'Bi-weekly consolidation' },
    { box: 5, interval: '30 days', desc: 'Long-term mastery' },
  ];

  return (
    <div className="max-w-7xl mx-auto px-6 py-6">
      {/* Top Breadcrumb & Action Bar */}
      <div className="flex items-center justify-between mb-4">
        <button
          onClick={() => navigate('/problems')}
          className="inline-flex items-center gap-1 text-xs font-mono text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 transition-colors"
        >
          <ArrowLeft className="w-3.5 h-3.5" />
          <span>Back to Problems</span>
        </button>

        <div className="flex items-center gap-2">
          <button
            onClick={handleToggleFavorite}
            className={`inline-flex items-center gap-1.5 px-2.5 py-1 text-xs font-medium border rounded-md transition-colors ${
              question.is_favorite
                ? 'bg-amber-50 dark:bg-amber-950/40 border-amber-300 dark:border-amber-800 text-amber-900 dark:text-amber-200'
                : 'bg-white dark:bg-slate-800 border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700'
            }`}
          >
            <Star
              className={`w-3.5 h-3.5 ${
                question.is_favorite ? 'fill-amber-400 text-amber-500 dark:text-amber-400' : 'text-slate-400 dark:text-slate-500'
              }`}
            />
            <span>{question.is_favorite ? 'Favorited' : 'Favorite'}</span>
          </button>

          <button
            onClick={() => navigate(`/problems/${question.id}/edit`)}
            className="inline-flex items-center gap-1 px-2.5 py-1 text-xs font-medium bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 shadow-2xs"
            title="Edit problem"
          >
            <Edit3 className="w-3.5 h-3.5 text-slate-500 dark:text-slate-400" />
            <span>Edit</span>
          </button>

          <button
            onClick={() => setShowDeleteModal(true)}
            className="inline-flex items-center gap-1 px-2.5 py-1 text-xs font-medium text-rose-600 dark:text-rose-400 bg-white dark:bg-slate-800 border border-rose-200 dark:border-rose-900/60 rounded-md hover:bg-rose-50 dark:hover:bg-rose-950/40 shadow-2xs"
            title="Delete problem"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Recommendation Banner if navigated from Practice Next */}
      {recommendationReason && (
        <div className="mb-4 p-3.5 bg-indigo-50/80 dark:bg-indigo-950/40 border border-indigo-200 dark:border-indigo-800/60 rounded-lg flex items-center justify-between text-xs shadow-2xs">
          <div className="flex items-center gap-2">
            <span className="font-mono text-indigo-700 dark:text-indigo-300 font-bold uppercase tracking-wider text-[10px] px-2 py-0.5 bg-indigo-100 dark:bg-indigo-900/60 rounded border border-indigo-200 dark:border-indigo-800 flex items-center gap-1">
              <Sparkles className="w-3 h-3 text-indigo-600 dark:text-indigo-400" />
              Practice Next Target
            </span>
            <span className="text-indigo-950 dark:text-indigo-200 font-medium">
              {recommendationReason}
            </span>
          </div>
          <button
            onClick={handlePracticeNext}
            disabled={isActing}
            className="inline-flex items-center gap-1 font-mono text-[11px] font-semibold text-indigo-700 dark:text-indigo-300 hover:text-indigo-900 dark:hover:text-indigo-200 hover:underline transition-colors shrink-0"
          >
            <span>Skip to Next</span>
            <ArrowRight className="w-3 h-3" />
          </button>
        </div>
      )}

      {/* Main 2-Area Workbench Layout */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-5 items-start">
        {/* LEFT / MAIN WORKSPACE AREA (8 cols) */}
        <div className="lg:col-span-8 space-y-4">
          {/* Problem Header Card */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-6 shadow-2xs">
            {/* Meta tags */}
            <div className="flex flex-wrap items-center gap-2 mb-2 text-xs">
              <span className="font-mono text-xs font-bold text-slate-400 dark:text-slate-500">{question.id}</span>
              <DifficultyBadge difficulty={question.difficulty} />
              <TopicBadge topic={question.topic} />
              {question.platform && (
                <span className="px-2 py-0.5 font-mono text-[11px] bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 rounded font-medium border border-slate-200 dark:border-slate-700">
                  {question.platform}
                </span>
              )}
              {question.company && (
                <span className="inline-flex items-center gap-1 px-2 py-0.5 text-[11px] font-mono bg-slate-50 dark:bg-slate-800/60 text-slate-600 dark:text-slate-300 border border-slate-200 dark:border-slate-700 rounded">
                  <Building2 className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                  {question.company}
                </span>
              )}
            </div>

            <h1 className="text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100 mb-2">
              {question.title}
            </h1>

            {question.source_url && (
              <a
                href={question.source_url}
                target="_blank"
                rel="noopener noreferrer"
                className="inline-flex items-center gap-1 text-xs font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 underline underline-offset-2"
              >
                <span>External Problem Statement</span>
                <ExternalLink className="w-3 h-3" />
              </a>
            )}

            {/* Problem Statement */}
            <div className="mt-5 pt-4 border-t border-slate-100 dark:border-slate-800">
              <h2 className="text-xs font-mono font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-2.5">
                Problem Description
              </h2>

              {question.description && question.description.trim().length > 0 ? (
                <div className="text-xs text-slate-800 dark:text-slate-200 leading-relaxed font-sans whitespace-pre-wrap bg-slate-50/70 dark:bg-slate-800/40 p-4 rounded-md border border-slate-200/80 dark:border-slate-800">
                  {question.description}
                </div>
              ) : (
                <div className="py-2 text-xs font-mono text-slate-400 dark:text-slate-500 italic">
                  No description provided. Reference external problem statement or solution notes below.
                </div>
              )}
            </div>

            {/* Tags row */}
            {question.tags && question.tags.length > 0 && (
              <div className="flex flex-wrap items-center gap-1.5 mt-5 pt-4 border-t border-slate-100 dark:border-slate-800">
                <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500 mr-1 flex items-center gap-1">
                  <Tag className="w-3 h-3" /> Tags:
                </span>
                {question.tags.map((tag, idx) => (
                  <span
                    key={idx}
                    className="px-2 py-0.5 text-[11px] font-mono bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 rounded border border-slate-200 dark:border-slate-700"
                  >
                    #{tag}
                  </span>
                ))}
              </div>
            )}
          </div>

          {/* Solution Approach, Invariants & Notes */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-6 shadow-2xs">
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-2">
                <BookOpen className="w-4 h-4 text-slate-500 dark:text-slate-400" />
                <h2 className="text-xs font-mono font-bold text-slate-700 dark:text-slate-300 uppercase tracking-wider">
                  Approach, Invariants & Complexity Notes
                </h2>
              </div>
              <button
                onClick={() => navigate(`/problems/${question.id}/edit`)}
                className="text-[11px] font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 font-medium"
              >
                Edit Notes
              </button>
            </div>

            {question.notes && question.notes.trim().length > 0 ? (
              <div className="text-xs text-slate-800 dark:text-slate-200 font-mono leading-relaxed whitespace-pre-wrap bg-slate-50/70 dark:bg-slate-800/40 p-4 rounded-md border border-slate-200 dark:border-slate-800">
                {question.notes}
              </div>
            ) : (
              <div className="text-center py-6 bg-slate-50 dark:bg-slate-800/40 rounded-md border border-dashed border-slate-200 dark:border-slate-700">
                <p className="text-xs text-slate-400 dark:text-slate-500 font-mono mb-2">
                  No solution approach or complexity notes recorded yet.
                </p>
                <button
                  onClick={() => navigate(`/problems/${question.id}/edit`)}
                  className="text-xs text-indigo-600 dark:text-indigo-400 font-semibold hover:underline"
                >
                  + Add Notes & Invariants
                </button>
              </div>
            )}
          </div>

          {/* Timestamp Telemetry */}
          <div className="flex items-center justify-between text-[11px] font-mono text-slate-400 dark:text-slate-500 px-1">
            <span className="flex items-center gap-1">
              <Calendar className="w-3 h-3" /> Created: {formatDate(question.created_at)}
            </span>
            <span className="flex items-center gap-1">
              <Clock className="w-3 h-3" /> Last Modified: {formatDate(question.updated_at)}
            </span>
          </div>
        </div>

        {/* RIGHT / SECONDARY AREA (4 cols) */}
        <div className="lg:col-span-4 space-y-4">
          {/* Preparation Status & Practice CTA Card */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-4">
            <div className="flex items-center justify-between">
              <h2 className="text-xs font-mono font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider">
                Preparation Status
              </h2>
              <span className="text-[11px] font-mono font-bold text-slate-700 dark:text-slate-200">
                {question.status}
              </span>
            </div>

            {/* Active Queue Status */}
            {queueInfo?.inQueue && (
              <div className="p-2.5 bg-amber-50/70 dark:bg-amber-950/30 border border-amber-200 dark:border-amber-900/50 rounded-md flex items-center justify-between text-xs">
                <div className="flex items-center gap-1.5 font-mono text-amber-900 dark:text-amber-200">
                  <ListOrdered className="w-3.5 h-3.5 text-amber-600 dark:text-amber-400" />
                  <span className="font-semibold">Queue Slot #{queueInfo.position}</span>
                </div>
                <button
                  onClick={handleRemoveFromQueue}
                  disabled={isActing}
                  className="inline-flex items-center gap-1 text-[11px] font-mono text-amber-700 dark:text-amber-300 hover:text-amber-900 dark:hover:text-amber-100 hover:underline"
                  title="Remove from practice drill queue"
                >
                  <X className="w-3 h-3" />
                  <span>Remove</span>
                </button>
              </div>
            )}

            {/* Post-Verdict Transition Card */}
            {lastVerdictResult && (
              <div className="p-3 bg-emerald-50 dark:bg-emerald-950/40 border border-emerald-300 dark:border-emerald-800/60 rounded-md space-y-2">
                <div className="flex items-center justify-between">
                  <span className="text-xs font-bold text-emerald-900 dark:text-emerald-200 font-mono flex items-center gap-1.5">
                    <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400" />
                    Verdict: {lastVerdictResult.verdict}
                  </span>
                  {lastVerdictResult.schedule && (
                    <span className="text-[10px] font-mono font-bold bg-emerald-100 dark:bg-emerald-900/60 text-emerald-800 dark:text-emerald-200 px-1.5 py-0.5 rounded">
                      Box {lastVerdictResult.schedule.nextLevel} ({Math.max(1, Math.round(lastVerdictResult.schedule.intervalSeconds / 86400))}d)
                    </span>
                  )}
                </div>
                <p className="text-[11px] text-emerald-800 dark:text-emerald-300 leading-tight">
                  Problem status and Leitner spaced repetition interval have been updated in backend.
                </p>
                <button
                  onClick={handlePracticeNext}
                  disabled={isActing}
                  className="w-full py-2 px-3 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded transition-colors shadow-xs flex items-center justify-center gap-1.5 font-mono"
                >
                  <span>Practice Next Problem</span>
                  <ArrowRight className="w-3.5 h-3.5" />
                  <kbd className="ml-1 px-1 py-0.2 bg-slate-800 dark:bg-slate-700 border border-slate-700 dark:border-slate-600 rounded text-[10px] text-slate-300">
                    N
                  </kbd>
                </button>
              </div>
            )}

            {/* Primary Action Button */}
            <button
              onClick={handleStartPracticeThis}
              className="w-full py-2.5 px-4 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded-md transition-colors shadow-xs flex items-center justify-center gap-2"
            >
              <PlaySquare className="w-4 h-4" />
              <span>Practice / Solve Now</span>
            </button>

            {/* Quick Verdict Triggers */}
            <div className="grid grid-cols-2 gap-2 pt-1">
              <button
                disabled={isActing}
                onClick={() => handleRecordVerdict('Solved')}
                className="flex items-center justify-center gap-1.5 py-1.5 px-2.5 text-xs font-semibold text-emerald-800 dark:text-emerald-300 bg-emerald-50 dark:bg-emerald-950/40 border border-emerald-300 dark:border-emerald-800 rounded hover:bg-emerald-100 dark:hover:bg-emerald-900/60 transition-colors shadow-2xs"
              >
                <CheckCircle2 className="w-3.5 h-3.5 text-emerald-600 dark:text-emerald-400" />
                <span>Mark Solved</span>
              </button>

              <button
                disabled={isActing}
                onClick={() => handleRecordVerdict('NeedsReview')}
                className="flex items-center justify-center gap-1.5 py-1.5 px-2.5 text-xs font-semibold text-amber-800 dark:text-amber-300 bg-amber-50 dark:bg-amber-950/40 border border-amber-300 dark:border-amber-800 rounded hover:bg-amber-100 dark:hover:bg-amber-900/60 transition-colors shadow-2xs"
              >
                <HelpCircle className="w-3.5 h-3.5 text-amber-600 dark:text-amber-400" />
                <span>Needs Review</span>
              </button>
            </div>

            {/* Status Picker */}
            <div className="pt-2 border-t border-slate-100 dark:border-slate-800">
              <label className="block text-[11px] font-mono text-slate-500 dark:text-slate-400 mb-1">
                Lifecycle State
              </label>
              <select
                value={question.status}
                onChange={(e) => handleStatusChange(e.target.value as Status)}
                disabled={isActing}
                className="w-full px-2.5 py-1.5 text-xs font-mono border border-slate-200 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
              >
                <option value="Unsolved">Unsolved</option>
                <option value="InProgress">InProgress</option>
                <option value="Solved">Solved</option>
                <option value="Mastered">Mastered</option>
              </select>
            </div>
          </div>

          {/* Leitner Spaced Repetition (SRS) Status Card */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-3.5">
            <div className="flex items-center justify-between">
              <h2 className="text-xs font-mono font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider">
                Leitner SRS Status
              </h2>
              <span className="px-2 py-0.5 text-xs font-mono font-bold bg-indigo-50 dark:bg-indigo-950/40 text-indigo-700 dark:text-indigo-300 border border-indigo-200 dark:border-indigo-800/60 rounded">
                Box {question.revision_priority} / 5
              </span>
            </div>

            {/* Leitner Box Progress Meter */}
            <div className="grid grid-cols-5 gap-1">
              {[1, 2, 3, 4, 5].map((b) => (
                <div
                  key={b}
                  className={`h-1.5 rounded-xs transition-all ${
                    b <= question.revision_priority
                      ? 'bg-indigo-600 dark:bg-indigo-500'
                      : 'bg-slate-200 dark:bg-slate-700'
                  }`}
                  title={`Box ${b}`}
                />
              ))}
            </div>

            <p className="text-[11px] text-slate-500 dark:text-slate-400 leading-normal">
              {boxDescriptions[Math.min(question.revision_priority - 1, 4)]?.desc} — spaced interval of{' '}
              <strong className="text-slate-800 dark:text-slate-200 font-mono">
                {boxDescriptions[Math.min(question.revision_priority - 1, 4)]?.interval}
              </strong>.
            </p>

            {/* SRS Timing Metadata */}
            <div className="space-y-2 pt-2 border-t border-slate-100 dark:border-slate-800 text-xs font-mono">
              <div className="flex justify-between items-center">
                <span className="text-slate-500 dark:text-slate-400 text-[11px]">Revision Due:</span>
                <span className="font-bold text-slate-800 dark:text-slate-200">
                  {getRelativeRevisionTime(question.next_revision_at)}
                </span>
              </div>
              <div className="flex justify-between items-center">
                <span className="text-slate-500 dark:text-slate-400 text-[11px]">Due Date:</span>
                <span className="text-slate-600 dark:text-slate-400 text-[11px]">
                  {formatDate(question.next_revision_at)}
                </span>
              </div>
              <div className="flex justify-between items-center">
                <span className="text-slate-500 dark:text-slate-400 text-[11px]">Last Practiced:</span>
                <span className="text-slate-600 dark:text-slate-400 text-[11px]">
                  {formatDate(question.last_practiced_at)}
                </span>
              </div>
            </div>

            <button
              onClick={() => setShowScheduleModal(true)}
              className="w-full py-1.5 text-xs font-semibold text-slate-700 dark:text-slate-200 bg-slate-50 dark:bg-slate-800 hover:bg-slate-100 dark:hover:bg-slate-700 border border-slate-200 dark:border-slate-700 rounded transition-colors text-center font-mono mt-2"
            >
              Reschedule Interval
            </button>
          </div>
        </div>
      </div>

      {/* Schedule Revision Modal */}
      {showScheduleModal && (
        <div
          onClick={(e) => {
            if (e.target === e.currentTarget) setShowScheduleModal(false);
          }}
          className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/60 backdrop-blur-xs animate-in fade-in"
        >
          <div className="bg-white dark:bg-slate-900 rounded-lg border border-slate-200 dark:border-slate-800 shadow-xl max-w-sm w-full p-5">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-1">
              Schedule Spaced Revision
            </h3>
            <p className="text-xs text-slate-500 dark:text-slate-400 mb-4">
              Select interval for this problem to enter the priority MinHeap for active recall.
            </p>

            <div className="space-y-1.5 mb-5">
              {[1, 3, 7, 14, 30].map((days) => (
                <button
                  key={days}
                  type="button"
                  onClick={() => setScheduleDays(days)}
                  className={`w-full py-2 px-3 text-xs font-mono rounded border flex items-center justify-between ${
                    scheduleDays === days
                      ? 'border-indigo-600 dark:border-indigo-500 bg-indigo-50/60 dark:bg-indigo-950/40 text-indigo-900 dark:text-indigo-200 font-bold'
                      : 'border-slate-200 dark:border-slate-700 bg-white dark:bg-slate-800 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700'
                  }`}
                >
                  <span>{days} {days === 1 ? 'day' : 'days'}</span>
                  <span className="text-[10px] text-slate-400 dark:text-slate-500">
                    Box {days === 1 ? 1 : days === 3 ? 2 : days === 7 ? 3 : days === 14 ? 4 : 5}
                  </span>
                </button>
              ))}
            </div>

            <div className="flex items-center justify-end gap-2">
              <button
                type="button"
                onClick={() => setShowScheduleModal(false)}
                className="px-3 py-1.5 text-xs text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200"
              >
                Cancel
              </button>
              <button
                type="button"
                disabled={isActing}
                onClick={handleScheduleRevision}
                className="px-4 py-1.5 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded disabled:opacity-50"
              >
                {isActing ? 'Scheduling...' : 'Set Interval'}
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Delete Confirmation Modal */}
      <ConfirmModal
        isOpen={showDeleteModal}
        title="Delete Question Record"
        message={`Are you sure you want to permanently delete "${question.title}" (${question.id})? This action will remove it from CSV persistence and all DSA indexing structures.`}
        confirmLabel="Confirm Delete"
        cancelLabel="Cancel"
        isDestructive={true}
        isLoading={isActing}
        onConfirm={handleDelete}
        onCancel={() => setShowDeleteModal(false)}
      />
    </div>
  );
};
