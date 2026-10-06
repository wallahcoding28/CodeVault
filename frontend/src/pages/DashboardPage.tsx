import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  Code2,
  RotateCw,
  PlaySquare,
  ArrowRight,
  Clock,
  Sparkles,
  AlertCircle,
  Plus,
  Upload,
} from 'lucide-react';
import { api } from '../services/api';
import { DashboardSnapshot, Question, PracticeNextResult } from '../types';
import { ProgressBar } from '../components/ProgressBar';
import { DifficultyBadge, StatusBadge, TopicBadge } from '../components/Badge';

export const DashboardPage: React.FC = () => {
  const navigate = useNavigate();
  const [snapshot, setSnapshot] = useState<DashboardSnapshot | null>(null);
  const [practiceNext, setPracticeNext] = useState<PracticeNextResult | null>(null);
  const [recentQuestions, setRecentQuestions] = useState<Question[]>([]);
  const [dueQuestions, setDueQuestions] = useState<Question[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const loadData = async () => {
    try {
      setLoading(true);
      setError(null);
      const [dash, allQuestions, dueItems, nextResult] = await Promise.all([
        api.getDashboard(),
        api.getQuestions(),
        api.getDueRevisions(),
        api.getPracticeNext().catch(() => null),
      ]);
      setSnapshot(dash);
      setPracticeNext(nextResult);

      // Sort by last modified for recent problems
      const sorted = [...allQuestions].sort((a, b) => b.updated_at - a.updated_at).slice(0, 5);
      setRecentQuestions(sorted);

      // Match due questions
      const dueIds = new Set(dueItems.map((d) => d.questionId));
      const matchedDue = allQuestions.filter((q) => dueIds.has(q.id)).slice(0, 4);
      setDueQuestions(matchedDue);
    } catch (err: any) {
      console.error(err);
      setError('Unable to load cockpit data. Ensure CodeVault backend is running.');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  if (loading) {
    return (
      <div className="max-w-6xl mx-auto px-6 py-16 flex items-center justify-center min-h-[50vh]">
        <div className="flex flex-col items-center gap-2.5 text-slate-500 dark:text-slate-400">
          <div className="w-7 h-7 border-2 border-slate-300 dark:border-slate-700 border-t-slate-900 dark:border-t-indigo-500 rounded-full animate-spin"></div>
          <span className="text-xs font-mono">Loading daily cockpit...</span>
        </div>
      </div>
    );
  }

  if (error || !snapshot) {
    return (
      <div className="max-w-6xl mx-auto px-6 py-16 flex flex-col items-center justify-center min-h-[50vh] text-center">
        <div className="w-12 h-12 rounded-full bg-rose-50 dark:bg-rose-950/40 flex items-center justify-center mb-3">
          <AlertCircle className="w-6 h-6 text-rose-600 dark:text-rose-400" />
        </div>
        <h3 className="text-base font-bold text-slate-800 dark:text-slate-200">Cockpit Data Unavailable</h3>
        <p className="text-xs text-slate-500 dark:text-slate-400 max-w-sm mt-1">{error || 'Failed to initialize daily metrics.'}</p>
        <button
          onClick={loadData}
          className="mt-4 px-4 py-2 bg-slate-900 dark:bg-indigo-600 text-white rounded-md text-xs font-semibold hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors cursor-pointer"
        >
          Retry
        </button>
      </div>
    );
  }

  const { overall, difficulty, topic, revision } = snapshot;

  return (
    <div className="max-w-6xl mx-auto px-6 py-6 space-y-6">
      {/* Top Cockpit Header */}
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-2 text-xs font-mono text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-1">
              <span>Personal Practice Cockpit</span>
            </div>
            <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
              Welcome back
            </h1>
            <p className="text-xs text-slate-500 dark:text-slate-400 mt-1">
              You have <strong className="text-slate-800 dark:text-slate-200 font-semibold">{overall.solvedCount} of {overall.totalQuestions}</strong> problems solved ({overall.completionPercentage.toFixed(1)}%).{' '}
              {revision.dueCount > 0 ? (
                <span>
                  <strong className="text-rose-700 dark:text-rose-400 font-bold">{revision.dueCount} revision {revision.dueCount === 1 ? 'target is' : 'targets are'}</strong> due right now.
                </span>
              ) : (
                <span className="text-emerald-700 dark:text-emerald-400 font-medium">All scheduled spaced revisions are up to date.</span>
              )}
            </p>
          </div>

          {/* Primary Quick Actions */}
          <div className="flex flex-wrap items-center gap-2">
            {revision.dueCount > 0 ? (
              <button
                onClick={() => navigate('/practice?mode=Due')}
                className="inline-flex items-center gap-1.5 px-3.5 py-2 rounded-md text-xs font-semibold bg-rose-600 text-white hover:bg-rose-700 transition-colors shadow-xs cursor-pointer"
              >
                <RotateCw className="w-3.5 h-3.5" />
                <span>Practice Due ({revision.dueCount})</span>
              </button>
            ) : overall.totalQuestions > 0 ? (
              <button
                onClick={() => navigate('/practice?mode=Unsolved')}
                className="inline-flex items-center gap-1.5 px-3.5 py-2 rounded-md text-xs font-semibold bg-slate-900 dark:bg-indigo-600 text-white hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs cursor-pointer"
              >
                <PlaySquare className="w-3.5 h-3.5" />
                <span>Practice Unsolved</span>
              </button>
            ) : null}

            <button
              onClick={() => navigate('/problems')}
              className="inline-flex items-center gap-1.5 px-3.5 py-2 rounded-md text-xs font-semibold bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs cursor-pointer"
            >
              <Code2 className="w-3.5 h-3.5" />
              <span>Browse Problems</span>
            </button>
          </div>
        </div>

        {/* 4 Clean Metric Ribbons */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mt-5 pt-4 border-t border-slate-100 dark:border-slate-800 font-mono">
          <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded-md border border-slate-100 dark:border-slate-750">
            <div className="text-[10px] text-slate-400 dark:text-slate-400 uppercase font-semibold">Solved Problems</div>
            <div className="text-xl font-bold text-slate-900 dark:text-slate-100 mt-0.5">
              {overall.solvedCount} <span className="text-xs text-slate-400 dark:text-slate-500 font-normal">/ {overall.totalQuestions}</span>
            </div>
            <div className="text-[10px] text-emerald-600 dark:text-emerald-400 font-medium mt-0.5">
              {overall.completionPercentage.toFixed(1)}% complete
            </div>
          </div>

          <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded-md border border-slate-100 dark:border-slate-750">
            <div className="text-[10px] text-slate-400 dark:text-slate-400 uppercase font-semibold">Due for Revision</div>
            <div className={`text-xl font-bold mt-0.5 ${revision.dueCount > 0 ? 'text-rose-600 dark:text-rose-400' : 'text-slate-800 dark:text-slate-200'}`}>
              {revision.dueCount}
            </div>
            <div className="text-[10px] text-slate-500 dark:text-slate-400 mt-0.5">
              {revision.upcomingCount} upcoming scheduled
            </div>
          </div>

          <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded-md border border-slate-100 dark:border-slate-750">
            <div className="text-[10px] text-slate-400 dark:text-slate-400 uppercase font-semibold">Mastered (Box 5)</div>
            <div className="text-xl font-bold text-slate-900 dark:text-slate-100 mt-0.5">
              {overall.masteredCount}
            </div>
            <div className="text-[10px] text-indigo-600 dark:text-indigo-400 font-medium mt-0.5">
              30-day retention tier
            </div>
          </div>

          <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded-md border border-slate-100 dark:border-slate-750">
            <div className="text-[10px] text-slate-400 dark:text-slate-400 uppercase font-semibold">Starred Benchmarks</div>
            <div className="text-xl font-bold text-slate-900 dark:text-slate-100 mt-0.5">
              {overall.favoriteCount}
            </div>
            <div className="text-[10px] text-amber-600 dark:text-amber-400 font-medium mt-0.5">
              High-priority review
            </div>
          </div>
        </div>
      </div>

      {/* First-Run Guidance Card when 0 questions */}
      {overall.totalQuestions === 0 && (
        <div className="bg-white dark:bg-slate-900 border-2 border-dashed border-slate-200 dark:border-slate-800 rounded-xl p-8 text-center">
          <div className="w-12 h-12 mx-auto rounded-full bg-slate-100 dark:bg-slate-800 flex items-center justify-center text-slate-600 dark:text-slate-300 mb-3">
            <Code2 className="w-6 h-6 text-indigo-600 dark:text-indigo-400" />
          </div>
          <h2 className="text-base font-bold text-slate-900 dark:text-slate-100 mb-1">
            Your coding practice workspace is ready.
          </h2>
          <p className="text-xs text-slate-500 dark:text-slate-400 max-w-md mx-auto mb-5">
            You currently have 0 problems in your library. Add your first algorithmic problem manually or import an existing CSV/JSON question catalog to start practicing.
          </p>
          <div className="flex flex-wrap items-center justify-center gap-3">
            <button
              onClick={() => navigate('/problems/new')}
              className="inline-flex items-center gap-1.5 px-4 py-2 rounded-md text-xs font-semibold bg-slate-900 dark:bg-indigo-600 text-white hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs cursor-pointer"
            >
              <Plus className="w-3.5 h-3.5" />
              <span>Add First Problem</span>
            </button>
            <button
              onClick={() => navigate('/settings?tab=export')}
              className="inline-flex items-center gap-1.5 px-4 py-2 rounded-md text-xs font-semibold bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs cursor-pointer"
            >
              <Upload className="w-3.5 h-3.5" />
              <span>Import Problems</span>
            </button>
          </div>
        </div>
      )}

      {/* Main 2-Column Content Layout */}
      {overall.totalQuestions > 0 && (
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Left Column (2 Cols): Due Revisions Preview & Topics */}
          <div className="lg:col-span-2 space-y-6">
            {/* Practice Next Recommendation */}
            {practiceNext && practiceNext.hasQuestion && practiceNext.question && (
              <div className="bg-white dark:bg-slate-900 border border-indigo-200/90 dark:border-indigo-900/60 rounded-lg p-5 shadow-2xs space-y-3">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Sparkles className="w-4 h-4 text-indigo-600 dark:text-indigo-400" />
                    <h2 className="text-xs font-mono font-bold text-indigo-900 dark:text-indigo-300 uppercase tracking-wider">
                      Recommended Next Problem
                    </h2>
                  </div>
                  <span className="text-[11px] font-mono text-indigo-700 dark:text-indigo-400 font-medium">
                    {practiceNext.recommendationReason}
                  </span>
                </div>

                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 pt-1">
                  <div>
                    <div
                      onClick={() =>
                        navigate(
                          `/problems/${practiceNext.question!.id}?reason=${encodeURIComponent(
                            practiceNext.recommendationReason || ''
                          )}`
                        )
                      }
                      className="text-sm font-bold text-slate-900 dark:text-slate-100 hover:text-indigo-600 dark:hover:text-indigo-400 cursor-pointer"
                    >
                      {practiceNext.question.title}
                    </div>
                    <div className="flex items-center gap-2 mt-1 text-[11px] font-mono text-slate-500 dark:text-slate-400">
                      <span>{practiceNext.question.id}</span>
                      <span>·</span>
                      <DifficultyBadge difficulty={practiceNext.question.difficulty} />
                      <TopicBadge topic={practiceNext.question.topic} />
                    </div>
                  </div>

                  <button
                    onClick={() =>
                      navigate(
                        `/problems/${practiceNext.question!.id}?reason=${encodeURIComponent(
                          practiceNext.recommendationReason || ''
                        )}`
                      )
                    }
                    className="inline-flex items-center justify-center gap-1.5 px-4 py-2 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded-md transition-colors shadow-xs shrink-0 font-mono cursor-pointer"
                  >
                    <span>Solve This Next</span>
                    <ArrowRight className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            )}

            {/* Due Revisions Action List */}
            {dueQuestions.length > 0 && (
              <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-3">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <RotateCw className="w-4 h-4 text-rose-600 dark:text-rose-400" />
                    <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
                      Due for Revision Today
                    </h2>
                  </div>
                  <button
                    onClick={() => navigate('/revision')}
                    className="text-xs font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 font-medium inline-flex items-center gap-1 cursor-pointer"
                  >
                    View All ({revision.dueCount}) <ArrowRight className="w-3 h-3" />
                  </button>
                </div>

                <div className="divide-y divide-slate-100 dark:divide-slate-800">
                  {dueQuestions.map((q) => (
                    <div
                      key={q.id}
                      onClick={() => navigate(`/problems/${q.id}`)}
                      className="py-2.5 px-2 -mx-2 hover:bg-slate-50 dark:hover:bg-slate-800/60 cursor-pointer rounded flex items-center justify-between transition-colors"
                    >
                      <div className="min-w-0 pr-3">
                        <div className="text-xs font-semibold text-slate-900 dark:text-slate-100 truncate hover:text-indigo-600 dark:hover:text-indigo-400">
                          {q.title}
                        </div>
                        <div className="flex items-center gap-2 mt-0.5 text-[10px] font-mono text-slate-400 dark:text-slate-500">
                          <span>{q.id}</span>
                          <span>·</span>
                          <span>{q.topic}</span>
                        </div>
                      </div>
                      <div className="flex items-center gap-2 shrink-0">
                        <DifficultyBadge difficulty={q.difficulty} />
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            navigate('/practice?mode=Due');
                          }}
                          className="px-2 py-1 text-[11px] font-mono font-medium text-slate-700 dark:text-slate-300 bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 rounded cursor-pointer"
                        >
                          Drill
                        </button>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Difficulty Tier Progress */}
            <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-4">
              <div className="flex items-center justify-between">
                <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
                  Difficulty Distribution
                </h2>
                <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">
                  {overall.totalQuestions} cataloged
                </span>
              </div>

              <div className="space-y-3">
                <div>
                  <ProgressBar
                    label="Easy"
                    sublabel={`${difficulty.easyCount} problems`}
                    percentage={difficulty.easyPercentage}
                    color="emerald"
                  />
                </div>
                <div>
                  <ProgressBar
                    label="Medium"
                    sublabel={`${difficulty.mediumCount} problems`}
                    percentage={difficulty.mediumPercentage}
                    color="amber"
                  />
                </div>
                <div>
                  <ProgressBar
                    label="Hard"
                    sublabel={`${difficulty.hardCount} problems`}
                    percentage={difficulty.hardPercentage}
                    color="rose"
                  />
                </div>
              </div>
            </div>

            {/* Topic Coverage Progress */}
            <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-3">
              <div className="flex items-center justify-between">
                <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
                  Topic Progress
                </h2>
                <button
                  onClick={() => navigate('/statistics')}
                  className="text-xs font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 font-medium inline-flex items-center gap-1 cursor-pointer"
                >
                  All Topics <ArrowRight className="w-3 h-3" />
                </button>
              </div>

              <div className="grid grid-cols-1 sm:grid-cols-2 gap-2.5">
                {topic.topicCounts.slice(0, 6).map((tc) => (
                  <div
                    key={tc.topic}
                    onClick={() => navigate(`/problems?topic=${tc.topic}`)}
                    className="p-2.5 bg-slate-50 dark:bg-slate-800/60 hover:bg-slate-100 dark:hover:bg-slate-800 cursor-pointer rounded border border-slate-200/80 dark:border-slate-750 transition-colors flex items-center justify-between text-xs"
                  >
                    <div className="min-w-0 pr-2">
                      <TopicBadge topic={tc.topic} />
                    </div>
                    <div className="text-right font-mono shrink-0">
                      <span className="font-bold text-slate-800 dark:text-slate-200">{tc.count}</span>
                      <span className="text-slate-400 dark:text-slate-500 text-[10px] ml-1">({tc.percentage.toFixed(0)}%)</span>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>

          {/* Right Column (1 Col): Recent Problems Activity */}
          <div className="space-y-6">
            <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-3">
              <div className="flex items-center justify-between">
                <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider">
                  Recent Problems
                </h2>
                <Clock className="w-3.5 h-3.5 text-slate-400" />
              </div>

              <div className="divide-y divide-slate-100 dark:divide-slate-800">
                {recentQuestions.map((q) => (
                  <div
                    key={q.id}
                    onClick={() => navigate(`/problems/${q.id}`)}
                    className="py-2.5 px-2 -mx-2 hover:bg-slate-50 dark:hover:bg-slate-800/60 cursor-pointer rounded transition-colors flex items-start justify-between gap-2"
                  >
                    <div className="min-w-0">
                      <div className="text-xs font-semibold text-slate-900 dark:text-slate-100 truncate hover:text-indigo-600 dark:hover:text-indigo-400">
                        {q.title}
                      </div>
                      <div className="flex items-center gap-1.5 mt-0.5 text-[10px] font-mono text-slate-400 dark:text-slate-500">
                        <span>{q.id}</span>
                        <span>·</span>
                        <DifficultyBadge difficulty={q.difficulty} className="text-[9px] px-1 py-0" />
                      </div>
                    </div>
                    <div className="shrink-0 pt-0.5">
                      <StatusBadge status={q.status} className="text-[10px] px-1.5 py-0" />
                    </div>
                  </div>
                ))}
              </div>

              <button
                onClick={() => navigate('/problems')}
                className="w-full text-center py-1.5 text-xs font-mono font-medium text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 transition-colors border-t border-slate-100 dark:border-slate-800 pt-2.5 cursor-pointer"
              >
                View All {overall.totalQuestions} Problems →
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
