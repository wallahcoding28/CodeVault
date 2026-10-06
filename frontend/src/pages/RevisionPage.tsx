import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  CheckCircle2,
  PlaySquare,
  ArrowRight,
  BrainCircuit,
} from 'lucide-react';
import { api } from '../services/api';
import { Question, DashboardSnapshot, RevisionItem } from '../types';
import { DifficultyBadge, TopicBadge } from '../components/Badge';

export const RevisionPage: React.FC = () => {
  const navigate = useNavigate();

  const [dueItems, setDueItems] = useState<RevisionItem[]>([]);
  const [upcomingItems, setUpcomingItems] = useState<RevisionItem[]>([]);
  const [questionsMap, setQuestionsMap] = useState<Record<string, Question>>({});
  const [dashboard, setDashboard] = useState<DashboardSnapshot | null>(null);
  const [loading, setLoading] = useState(true);

  // Schedule modal
  const [selectedQuestionId, setSelectedQuestionId] = useState<string | null>(null);
  const [rescheduleDays, setRescheduleDays] = useState(3);
  const [isActing, setIsActing] = useState(false);

  const loadRevisionData = async () => {
    try {
      setLoading(true);
      const [due, upcoming, dash, allQuestions] = await Promise.all([
        api.getDueRevisions(),
        api.getUpcomingRevisions(),
        api.getDashboard(),
        api.getQuestions(),
      ]);
      setDueItems(due);
      setUpcomingItems(upcoming);
      setDashboard(dash);

      const qMap: Record<string, Question> = {};
      for (const q of allQuestions) {
        qMap[q.id] = q;
      }
      setQuestionsMap(qMap);
    } catch (err: any) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadRevisionData();
  }, []);

  useEffect(() => {
    if (!selectedQuestionId) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setSelectedQuestionId(null);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [selectedQuestionId]);

  const handleQuickVerdict = async (questionId: string, verdict: 'Solved' | 'NeedsReview') => {
    try {
      const q = questionsMap[questionId];
      const now = Math.floor(Date.now() / 1000);

      let nextRevisionAt = now + 86400;
      let priority = 1;
      let newStatus: 'Solved' | 'InProgress' | 'Mastered' = 'InProgress';

      if (verdict === 'Solved') {
        let curLevel = 1;
        if (q && q.last_practiced_at && q.next_revision_at > q.last_practiced_at) {
          const delta = q.next_revision_at - q.last_practiced_at;
          if (delta <= 86400 + 3600) curLevel = 1;
          else if (delta <= 3 * 86400 + 3600) curLevel = 2;
          else if (delta <= 7 * 86400 + 3600) curLevel = 3;
          else if (delta <= 14 * 86400 + 3600) curLevel = 4;
          else curLevel = 5;
        } else if (q && q.revision_priority) {
          curLevel = Math.max(1, Math.min(q.revision_priority, 5));
        }

        const nextLevel = Math.min(curLevel + 1, 5);
        const intervalDays = nextLevel === 1 ? 1 : nextLevel === 2 ? 3 : nextLevel === 3 ? 7 : nextLevel === 4 ? 14 : 30;
        priority = nextLevel >= 5 ? 5 : nextLevel >= 3 ? 4 : 3;
        nextRevisionAt = now + intervalDays * 86400;
        newStatus = nextLevel >= 5 ? 'Mastered' : 'Solved';
      } else {
        nextRevisionAt = now + 86400;
        priority = 1;
        newStatus = 'InProgress';
      }

      await api.scheduleRevision(questionId, nextRevisionAt, priority);
      await api.updateQuestion(questionId, {
        status: newStatus,
        last_practiced_at: now,
      });
      await loadRevisionData();
    } catch (err: any) {
      alert(err.message || 'Failed to record verdict');
    }
  };

  const handleReschedule = async () => {
    if (!selectedQuestionId) return;
    try {
      setIsActing(true);
      const nextEpoch = Math.floor(Date.now() / 1000) + rescheduleDays * 86400;
      const priority = rescheduleDays === 1 ? 1 : rescheduleDays === 3 ? 2 : rescheduleDays === 7 ? 3 : rescheduleDays === 14 ? 4 : 5;
      await api.scheduleRevision(selectedQuestionId, nextEpoch, priority);
      setSelectedQuestionId(null);
      await loadRevisionData();
    } catch (err: any) {
      alert(err.message || 'Failed to reschedule');
    } finally {
      setIsActing(false);
    }
  };

  const formatDate = (epoch: number) => {
    if (!epoch) return 'Not scheduled';
    return new Date(epoch * 1000).toLocaleDateString('en-US', {
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  const getRelativeDueTime = (epoch: number) => {
    if (!epoch) return '—';
    const now = Math.floor(Date.now() / 1000);
    const diff = epoch - now;
    if (diff <= 0) return 'Due now';
    if (diff <= 86400) return 'Tomorrow';
    const days = Math.ceil(diff / 86400);
    return `In ${days} days`;
  };

  const getPriorityLabel = (priority: number) => {
    switch (priority) {
      case 1:
        return <span className="text-[10px] font-mono font-bold text-rose-700 dark:text-rose-400 bg-rose-50 dark:bg-rose-950/40 px-1.5 py-0.5 rounded border border-rose-200 dark:border-rose-800/60">Urgent</span>;
      case 2:
        return <span className="text-[10px] font-mono font-bold text-amber-700 dark:text-amber-400 bg-amber-50 dark:bg-amber-950/40 px-1.5 py-0.5 rounded border border-amber-200 dark:border-amber-800/60">High</span>;
      case 3:
        return <span className="text-[10px] font-mono text-indigo-700 dark:text-indigo-400 bg-indigo-50 dark:bg-indigo-950/40 px-1.5 py-0.5 rounded border border-indigo-200 dark:border-indigo-800/60">Normal</span>;
      case 4:
        return <span className="text-[10px] font-mono text-slate-600 dark:text-slate-300 bg-slate-100 dark:bg-slate-800 px-1.5 py-0.5 rounded border border-transparent dark:border-slate-700">Low</span>;
      default:
        return <span className="text-[10px] font-mono text-emerald-700 dark:text-emerald-400 bg-emerald-50 dark:bg-emerald-950/40 px-1.5 py-0.5 rounded border border-emerald-200 dark:border-emerald-800/60">Mastered</span>;
    }
  };

  if (loading) {
    return (
      <div className="max-w-7xl mx-auto px-6 py-16 flex flex-col items-center justify-center gap-2.5">
        <div className="w-7 h-7 border-2 border-slate-300 dark:border-slate-700 border-t-slate-900 dark:border-t-slate-100 rounded-full animate-spin"></div>
        <span className="text-xs font-mono text-slate-500 dark:text-slate-400">Querying C++ Revision MinHeap...</span>
      </div>
    );
  }

  const priorityCounts = dashboard?.revision.priorityCounts || [0, 0, 0, 0, 0, 0];
  const box1to5Counts = [
    priorityCounts[1] || 0,
    priorityCounts[2] || 0,
    priorityCounts[3] || 0,
    priorityCounts[4] || 0,
    priorityCounts[5] || 0,
  ];
  const totalInBoxes = box1to5Counts.reduce((acc, count) => acc + count, 0);

  return (
    <div className="max-w-7xl mx-auto px-6 py-6">
      {/* Header & Quick Action */}
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 mb-5">
        <div>
          <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
            Revision Workspace
          </h1>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
            5-Box Leitner spaced repetition schedule with automated priority promotions and MinHeap sorting.
          </p>
        </div>

        {dueItems.length > 0 && (
          <button
            onClick={() => navigate('/practice?mode=Due')}
            className="inline-flex items-center gap-1.5 px-3.5 py-1.5 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 rounded-md hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs"
          >
            <PlaySquare className="w-3.5 h-3.5" />
            <span>Practice Due Revisions ({dueItems.length})</span>
          </button>
        )}
      </div>

      {/* Leitner Box Distribution Strip & SRS Rules */}
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 mb-6 shadow-2xs space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-100 dark:border-slate-800 pb-3">
          <div className="flex items-center gap-1.5 text-xs font-mono font-bold text-slate-800 dark:text-slate-200 uppercase tracking-wider">
            <BrainCircuit className="w-4 h-4 text-indigo-600 dark:text-indigo-400" />
            <span>Leitner 5-Box Spaced Repetition System ({totalInBoxes} active items)</span>
          </div>
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
            {dueItems.length} due for review today
          </span>
        </div>

        {/* 5 Boxes */}
        <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-2.5">
          {[
            { box: 1, interval: '1 Day', label: 'Daily Review', sub: 'Unlearned / Reset', color: 'border-rose-200 dark:border-rose-900/60 bg-rose-50/50 dark:bg-rose-950/20 text-rose-900 dark:text-rose-200', numColor: 'text-rose-950 dark:text-rose-100' },
            { box: 2, interval: '3 Days', label: 'Short Interval', sub: 'Early Recall', color: 'border-amber-200 dark:border-amber-900/60 bg-amber-50/50 dark:bg-amber-950/20 text-amber-900 dark:text-amber-200', numColor: 'text-amber-950 dark:text-amber-100' },
            { box: 3, interval: '7 Days', label: 'Weekly Recall', sub: 'Consolidation', color: 'border-sky-200 dark:border-sky-900/60 bg-sky-50/50 dark:bg-sky-950/20 text-sky-900 dark:text-sky-200', numColor: 'text-sky-950 dark:text-sky-100' },
            { box: 4, interval: '14 Days', label: 'Bi-Weekly', sub: 'Reinforcement', color: 'border-indigo-200 dark:border-indigo-900/60 bg-indigo-50/50 dark:bg-indigo-950/20 text-indigo-900 dark:text-indigo-200', numColor: 'text-indigo-950 dark:text-indigo-100' },
            { box: 5, interval: '30 Days', label: 'Mastered', sub: 'Long-term Memory', color: 'border-emerald-200 dark:border-emerald-900/60 bg-emerald-50/50 dark:bg-emerald-950/20 text-emerald-900 dark:text-emerald-200', numColor: 'text-emerald-950 dark:text-emerald-100' },
          ].map((item, idx) => {
            const count = box1to5Counts[idx] || 0;
            const pct = totalInBoxes > 0 ? Math.round((count / totalInBoxes) * 100) : 0;
            return (
              <div
                key={item.box}
                className={`p-3 rounded-md border ${item.color} flex flex-col justify-between`}
              >
                <div>
                  <div className="flex items-center justify-between text-[11px] font-mono font-semibold mb-0.5">
                    <span>Box {item.box}</span>
                    <span className="text-slate-500 dark:text-slate-400 font-normal">{item.interval}</span>
                  </div>
                  <div className={`text-xl font-bold font-mono ${item.numColor}`}>{count}</div>
                  <div className="text-[10px] text-slate-600 dark:text-slate-400 mt-0.5 font-sans">
                    {item.label}
                  </div>
                </div>
                <div className="mt-2 text-[10px] font-mono text-slate-500 dark:text-slate-400 border-t border-slate-200/40 dark:border-slate-700/60 pt-1">
                  {pct}% of curriculum
                </div>
              </div>
            );
          })}
        </div>

        {/* SRS Invariant Explainer */}
        <div className="bg-slate-50 dark:bg-slate-800/60 border border-slate-200 dark:border-slate-700/80 rounded p-2.5 text-[11px] text-slate-600 dark:text-slate-300 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-2 font-mono">
          <div className="flex items-center gap-2">
            <span className="font-bold text-slate-800 dark:text-slate-200">SRS Transitions:</span>
            <span><strong className="text-emerald-700 dark:text-emerald-400">Pass</strong> advances +1 Box (interval expands).</span>
            <span>·</span>
            <span><strong className="text-rose-700 dark:text-rose-400">Fail</strong> resets back to Box 1 (daily review).</span>
          </div>
          <span className="text-slate-400 dark:text-slate-500 text-[10px]">
            Schedule managed by C++ MinHeap
          </span>
        </div>
      </div>

      {/* Due Now Section */}
      <div className="mb-6">
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center gap-2">
            <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
              Due Now
            </h2>
            <span className="px-1.5 py-0.2 text-[10px] font-mono font-bold bg-rose-100 dark:bg-rose-950/60 text-rose-700 dark:text-rose-300 rounded-full border border-rose-200 dark:border-rose-800">
              {dueItems.length}
            </span>
          </div>
          {dueItems.length > 0 && (
            <button
              onClick={() => navigate('/practice?mode=Due')}
              className="text-xs font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 font-medium inline-flex items-center gap-1"
            >
              Start Revision Queue <ArrowRight className="w-3 h-3" />
            </button>
          )}
        </div>

        {dueItems.length === 0 ? (
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-8 text-center shadow-2xs">
            <CheckCircle2 className="w-7 h-7 text-emerald-600 dark:text-emerald-400 mx-auto mb-2" />
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-1">
              Zero Revisions Due
            </h3>
            <p className="text-xs text-slate-500 dark:text-slate-400 max-w-md mx-auto mb-3">
              All scheduled problems have been revised! New items will appear as their Leitner intervals elapse.
            </p>
            <button
              onClick={() => navigate('/problems')}
              className="px-3 py-1.5 text-xs font-semibold text-slate-700 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded hover:bg-slate-50 dark:hover:bg-slate-700 shadow-2xs"
            >
              Browse Problems Library
            </button>
          </div>
        ) : (
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg shadow-2xs overflow-hidden">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="bg-slate-50/90 dark:bg-slate-800/80 border-b border-slate-200/80 dark:border-slate-800 font-mono text-[11px] text-slate-500 dark:text-slate-400 uppercase tracking-wider">
                  <th className="py-2.5 px-3 w-14">ID</th>
                  <th className="py-2.5 px-3">Title</th>
                  <th className="py-2.5 px-3 w-32">Topic</th>
                  <th className="py-2.5 px-3 w-28">Difficulty</th>
                  <th className="py-2.5 px-3 w-20">Priority</th>
                  <th className="py-2.5 px-3 w-20">Box</th>
                  <th className="py-2.5 px-3 w-28">Due State</th>
                  <th className="py-2.5 px-3 text-right w-44">Verdicts</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100 dark:divide-slate-800">
                {dueItems.map((item) => {
                  const q = questionsMap[item.questionId];
                  return (
                    <tr
                      key={item.questionId}
                      className="hover:bg-slate-50/70 dark:hover:bg-slate-800/50 transition-colors cursor-pointer group"
                      onClick={() => navigate(`/problems/${item.questionId}`)}
                    >
                      <td className="py-2.5 px-3 font-mono text-slate-400 dark:text-slate-500 text-[11px]">
                        {item.questionId}
                      </td>
                      <td className="py-2.5 px-3">
                        <div className="font-semibold text-slate-900 dark:text-slate-100 group-hover:text-indigo-600 dark:group-hover:text-indigo-400 transition-colors">
                          {q ? q.title : item.questionId}
                        </div>
                        {q?.platform && (
                          <div className="text-[10px] font-mono text-slate-400 dark:text-slate-500">
                            {q.platform}
                          </div>
                        )}
                      </td>
                      <td className="py-2.5 px-3">
                        {q ? <TopicBadge topic={q.topic} /> : '—'}
                      </td>
                      <td className="py-2.5 px-3">
                        {q ? <DifficultyBadge difficulty={q.difficulty} /> : '—'}
                      </td>
                      <td className="py-2.5 px-3">
                        {getPriorityLabel(item.priority)}
                      </td>
                      <td className="py-2.5 px-3">
                        <span className="px-1.5 py-0.2 text-[10px] font-mono bg-indigo-50 dark:bg-indigo-950/40 text-indigo-700 dark:text-indigo-300 border border-indigo-100 dark:border-indigo-800/60 rounded">
                          Box {item.priority}
                        </span>
                      </td>
                      <td className="py-2.5 px-3 font-mono text-[11px] text-rose-600 dark:text-rose-400 font-bold">
                        {getRelativeDueTime(item.nextRevisionAt)}
                      </td>
                      <td className="py-2.5 px-3 text-right" onClick={(e) => e.stopPropagation()}>
                        <div className="inline-flex items-center gap-1">
                          <button
                            onClick={() => handleQuickVerdict(item.questionId, 'Solved')}
                            className="px-2 py-1 text-[11px] font-mono font-semibold text-emerald-700 dark:text-emerald-400 bg-emerald-50 dark:bg-emerald-950/40 hover:bg-emerald-100 dark:hover:bg-emerald-900/60 border border-emerald-200 dark:border-emerald-800 rounded"
                            title="Pass problem (+1 Leitner Box)"
                          >
                            Pass
                          </button>
                          <button
                            onClick={() => handleQuickVerdict(item.questionId, 'NeedsReview')}
                            className="px-2 py-1 text-[11px] font-mono font-semibold text-amber-700 dark:text-amber-400 bg-amber-50 dark:bg-amber-950/40 hover:bg-amber-100 dark:hover:bg-amber-900/60 border border-amber-200 dark:border-amber-800 rounded"
                            title="Fail problem (Reset to Box 1)"
                          >
                            Fail
                          </button>
                          <button
                            onClick={() => setSelectedQuestionId(item.questionId)}
                            className="px-2 py-1 text-[11px] font-mono text-slate-600 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-800 border border-slate-200 dark:border-slate-700 rounded"
                          >
                            Reschedule
                          </button>
                        </div>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* Upcoming Section */}
      <div>
        <div className="flex items-center gap-2 mb-3">
          <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
            Upcoming
          </h2>
          <span className="text-xs font-mono text-slate-400 dark:text-slate-500">
            (Ordered by C++ Priority MinHeap)
          </span>
        </div>

        {upcomingItems.length === 0 ? (
          <p className="text-xs text-slate-400 dark:text-slate-500 font-mono">
            No future revisions scheduled.
          </p>
        ) : (
          <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg shadow-2xs overflow-hidden">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="bg-slate-50/90 dark:bg-slate-800/80 border-b border-slate-200/80 dark:border-slate-800 font-mono text-[11px] text-slate-500 dark:text-slate-400 uppercase tracking-wider">
                  <th className="py-2.5 px-3 w-14">ID</th>
                  <th className="py-2.5 px-3">Title</th>
                  <th className="py-2.5 px-3 w-32">Topic</th>
                  <th className="py-2.5 px-3 w-28">Difficulty</th>
                  <th className="py-2.5 px-3 w-20">Priority</th>
                  <th className="py-2.5 px-3 w-20">Box</th>
                  <th className="py-2.5 px-3 w-32">Scheduled Date</th>
                  <th className="py-2.5 px-3 w-28">Relative</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100 dark:divide-slate-800">
                {upcomingItems.map((item) => {
                  const q = questionsMap[item.questionId];
                  return (
                    <tr
                      key={item.questionId}
                      onClick={() => navigate(`/problems/${item.questionId}`)}
                      className="hover:bg-slate-50/70 dark:hover:bg-slate-800/50 transition-colors cursor-pointer"
                    >
                      <td className="py-2.5 px-3 font-mono text-slate-400 dark:text-slate-500 text-[11px]">
                        {item.questionId}
                      </td>
                      <td className="py-2.5 px-3 font-semibold text-slate-900 dark:text-slate-100">
                        {q ? q.title : item.questionId}
                      </td>
                      <td className="py-2.5 px-3">
                        {q ? <TopicBadge topic={q.topic} /> : '—'}
                      </td>
                      <td className="py-2.5 px-3">
                        {q ? <DifficultyBadge difficulty={q.difficulty} /> : '—'}
                      </td>
                      <td className="py-2.5 px-3">
                        {getPriorityLabel(item.priority)}
                      </td>
                      <td className="py-2.5 px-3">
                        <span className="px-1.5 py-0.2 text-[10px] font-mono bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 border border-slate-200 dark:border-slate-700 rounded">
                          Box {item.priority}
                        </span>
                      </td>
                      <td className="py-2.5 px-3 font-mono text-[11px] text-slate-600 dark:text-slate-400">
                        {formatDate(item.nextRevisionAt)}
                      </td>
                      <td className="py-2.5 px-3 font-mono text-[11px] text-indigo-700 dark:text-indigo-400 font-medium">
                        {getRelativeDueTime(item.nextRevisionAt)}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* Reschedule Modal */}
      {selectedQuestionId && (
        <div
          onClick={(e) => {
            if (e.target === e.currentTarget) setSelectedQuestionId(null);
          }}
          className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/60 backdrop-blur-xs animate-in fade-in"
        >
          <div className="bg-white dark:bg-slate-900 rounded-lg border border-slate-200 dark:border-slate-800 shadow-xl max-w-sm w-full p-5">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-1">
              Reschedule Problem
            </h3>
            <p className="text-xs text-slate-500 dark:text-slate-400 mb-4">
              Set new interval for {questionsMap[selectedQuestionId]?.title || selectedQuestionId}.
            </p>

            <div className="space-y-1.5 mb-5">
              {[1, 3, 7, 14, 30].map((days) => (
                <button
                  key={days}
                  type="button"
                  onClick={() => setRescheduleDays(days)}
                  className={`w-full py-2 px-3 text-xs font-mono rounded border flex items-center justify-between ${
                    rescheduleDays === days
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
                onClick={() => setSelectedQuestionId(null)}
                className="px-3 py-1.5 text-xs text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200"
              >
                Cancel
              </button>
              <button
                type="button"
                disabled={isActing}
                onClick={handleReschedule}
                className="px-4 py-1.5 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded disabled:opacity-50"
              >
                {isActing ? 'Updating...' : 'Save Schedule'}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
