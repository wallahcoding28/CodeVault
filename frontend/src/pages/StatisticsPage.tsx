import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  Award,
  BookOpen,
  PieChart,
  BarChart3,
  Clock,
  Layers,
  Star,
  BrainCircuit,
  AlertCircle,
} from 'lucide-react';
import { api } from '../services/api';
import { DashboardSnapshot } from '../types';
import { ProgressBar } from '../components/ProgressBar';
import { TopicBadge } from '../components/Badge';

export const StatisticsPage: React.FC = () => {
  const navigate = useNavigate();
  const [data, setData] = useState<DashboardSnapshot | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchStats = async () => {
    try {
      setLoading(true);
      setError(null);
      const dash = await api.getDashboard();
      setData(dash);
    } catch (err: any) {
      console.error(err);
      setError('Unable to load statistics. Ensure CodeVault backend is running.');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchStats();
  }, []);

  if (loading) {
    return (
      <div className="max-w-6xl mx-auto px-6 py-16 flex flex-col items-center justify-center gap-2.5">
        <div className="w-7 h-7 border-2 border-slate-300 border-t-slate-900 rounded-full animate-spin"></div>
        <span className="text-xs font-mono text-slate-500">Aggregating curriculum metrics...</span>
      </div>
    );
  }

  if (error || !data) {
    return (
      <div className="max-w-6xl mx-auto px-6 py-16 flex flex-col items-center justify-center min-h-[50vh] text-center">
        <div className="w-12 h-12 rounded-full bg-rose-50 flex items-center justify-center mb-3">
          <AlertCircle className="w-6 h-6 text-rose-600" />
        </div>
        <h3 className="text-base font-bold text-slate-800">Statistics Unavailable</h3>
        <p className="text-xs text-slate-500 max-w-sm mt-1">{error || 'Failed to aggregate curriculum metrics.'}</p>
        <button
          onClick={fetchStats}
          className="mt-4 px-4 py-2 bg-slate-900 text-white rounded-md text-xs font-semibold hover:bg-slate-800 transition-colors"
        >
          Retry
        </button>
      </div>
    );
  }

  const { overall, difficulty, topic, status, revision } = data;

  const diffRows = [
    { name: 'Easy', count: difficulty.easyCount, pct: difficulty.easyPercentage, color: 'emerald' as const },
    { name: 'Medium', count: difficulty.mediumCount, pct: difficulty.mediumPercentage, color: 'amber' as const },
    { name: 'Hard', count: difficulty.hardCount, pct: difficulty.hardPercentage, color: 'rose' as const },
  ];

  const statusRows = [
    { name: 'Solved', count: status.solvedCount, pct: status.solvedPercentage, color: 'bg-emerald-500' },
    { name: 'InProgress', count: status.inProgressCount, pct: status.inProgressPercentage, color: 'bg-amber-500' },
    { name: 'Mastered', count: status.masteredCount, pct: status.masteredPercentage, color: 'bg-indigo-500' },
    { name: 'Unsolved', count: status.unsolvedCount, pct: status.unsolvedPercentage, color: 'bg-slate-400' },
  ];

  const box1to5Counts = [
    revision.priorityCounts[1] || 0,
    revision.priorityCounts[2] || 0,
    revision.priorityCounts[3] || 0,
    revision.priorityCounts[4] || 0,
    revision.priorityCounts[5] || 0,
  ];

  return (
    <div className="max-w-6xl mx-auto px-6 py-6 space-y-6">
      {/* Header */}
      <div>
        <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
          Preparation Statistics & Progress
        </h1>
        <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
          Calculated directly by C++ StatisticsService without simulated metrics or synthetic scores.
        </p>
      </div>

      {/* Top 4 Key Summary Ribbons */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3 font-mono">
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-4 shadow-2xs">
          <div className="flex items-center justify-between text-slate-400 dark:text-slate-500 mb-1">
            <span className="text-[10px] uppercase font-semibold">Total Catalog</span>
            <BookOpen className="w-3.5 h-3.5 text-slate-600 dark:text-slate-400" />
          </div>
          <div className="text-xl font-bold text-slate-900 dark:text-slate-100">
            {overall.totalQuestions}
          </div>
          <div className="text-[10px] text-slate-500 dark:text-slate-400 mt-0.5">
            {overall.solvedCount} problems solved
          </div>
        </div>

        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-4 shadow-2xs">
          <div className="flex items-center justify-between text-slate-400 dark:text-slate-500 mb-1">
            <span className="text-[10px] uppercase font-semibold">Completion</span>
            <Award className="w-3.5 h-3.5 text-emerald-600 dark:text-emerald-400" />
          </div>
          <div className="text-xl font-bold text-slate-900 dark:text-slate-100">
            {overall.completionPercentage.toFixed(1)}%
          </div>
          <div className="text-[10px] text-emerald-600 dark:text-emerald-400 font-medium mt-0.5">
            {overall.totalQuestions - overall.solvedCount} remaining
          </div>
        </div>

        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-4 shadow-2xs">
          <div className="flex items-center justify-between text-slate-400 dark:text-slate-500 mb-1">
            <span className="text-[10px] uppercase font-semibold">Due Revisions</span>
            <Clock className="w-3.5 h-3.5 text-rose-600 dark:text-rose-400" />
          </div>
          <div className="text-xl font-bold text-slate-900 dark:text-slate-100">
            {revision.dueCount}
          </div>
          <div className="text-[10px] text-slate-500 dark:text-slate-400 mt-0.5">
            {revision.upcomingCount} upcoming scheduled
          </div>
        </div>

        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-4 shadow-2xs">
          <div className="flex items-center justify-between text-slate-400 dark:text-slate-500 mb-1">
            <span className="text-[10px] uppercase font-semibold">Starred Problems</span>
            <Star className="w-3.5 h-3.5 text-amber-500 dark:text-amber-400" />
          </div>
          <div className="text-xl font-bold text-slate-900 dark:text-slate-100">
            {overall.favoriteCount}
          </div>
          <div className="text-[10px] text-amber-600 dark:text-amber-400 font-medium mt-0.5">
            Key interview benchmarks
          </div>
        </div>
      </div>

      {/* Grid: Difficulty Distribution & Status Distribution */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Difficulty Tier Mastery */}
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider flex items-center gap-1.5">
              <BarChart3 className="w-4 h-4 text-slate-600 dark:text-slate-400" />
              Difficulty Tier Mastery
            </h2>
            <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">
              Distribution per tier
            </span>
          </div>

          <div className="space-y-3 pt-1">
            {diffRows.map((tier) => (
              <div key={tier.name} className="space-y-1">
                <div className="flex justify-between items-center text-xs">
                  <span className="font-semibold text-slate-800 dark:text-slate-200">{tier.name}</span>
                  <span className="font-mono text-slate-500 dark:text-slate-400 text-[11px]">
                    {tier.count} problems ({tier.pct.toFixed(1)}%)
                  </span>
                </div>
                <ProgressBar percentage={tier.pct} color={tier.color} height="sm" />
              </div>
            ))}
          </div>
        </div>

        {/* Status Lifecycle Distribution */}
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider flex items-center gap-1.5">
              <PieChart className="w-4 h-4 text-slate-600 dark:text-slate-400" />
              Status Lifecycle
            </h2>
            <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">
              Preparation states
            </span>
          </div>

          <div className="grid grid-cols-2 gap-2.5">
            {statusRows.map((st) => (
              <div
                key={st.name}
                className="p-2.5 bg-slate-50 dark:bg-slate-800/60 border border-slate-200/80 dark:border-slate-700/80 rounded-md"
              >
                <div className="flex items-center gap-1.5 mb-1">
                  <div className={`w-2 h-2 rounded-full ${st.color}`} />
                  <span className="text-xs font-semibold text-slate-800 dark:text-slate-200 font-mono">
                    {st.name}
                  </span>
                </div>
                <div className="flex items-baseline justify-between mt-1">
                  <span className="text-base font-bold font-mono text-slate-900 dark:text-slate-100">
                    {st.count}
                  </span>
                  <span className="text-[10px] font-mono text-slate-400 dark:text-slate-400">
                    {st.pct.toFixed(1)}%
                  </span>
                </div>
              </div>
            ))}
          </div>

          {/* Segmented status ribbon */}
          <div className="w-full h-1.5 rounded-full overflow-hidden flex bg-slate-100 dark:bg-slate-800">
            {statusRows.map((st) => (
              <div
                key={st.name}
                style={{ width: `${st.pct}%` }}
                className={`${st.color} transition-all`}
                title={`${st.name}: ${st.count}`}
              />
            ))}
          </div>
        </div>
      </div>

      {/* Leitner SRS Box Retention Distribution */}
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-3">
        <div className="flex items-center justify-between">
          <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider flex items-center gap-1.5">
            <BrainCircuit className="w-4 h-4 text-indigo-600 dark:text-indigo-400" />
            Leitner SRS Retention Distribution
          </h2>
          <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">
            5 Spaced Intervals
          </span>
        </div>

        <div className="grid grid-cols-5 gap-2.5">
          {[
            { box: 1, interval: '1 Day', count: box1to5Counts[0] },
            { box: 2, interval: '3 Days', count: box1to5Counts[1] },
            { box: 3, interval: '7 Days', count: box1to5Counts[2] },
            { box: 4, interval: '14 Days', count: box1to5Counts[3] },
            { box: 5, interval: '30 Days', count: box1to5Counts[4] },
          ].map((b) => (
            <div
              key={b.box}
              className="p-3 bg-slate-50 dark:bg-slate-800/60 border border-slate-200/80 dark:border-slate-700/80 rounded-md text-center font-mono"
            >
              <div className="text-[10px] font-semibold text-slate-500 dark:text-slate-400 mb-0.5">
                Box {b.box} ({b.interval})
              </div>
              <div className="text-xl font-bold text-slate-900 dark:text-slate-100">
                {b.count}
              </div>
              <div className="text-[9px] text-slate-400 dark:text-slate-500 mt-0.5">
                problems
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Topic Coverage Breakdown Table */}
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg shadow-2xs overflow-hidden">
        <div className="px-5 py-3 border-b border-slate-200/80 dark:border-slate-800 flex items-center justify-between">
          <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider flex items-center gap-1.5">
            <Layers className="w-4 h-4 text-slate-600 dark:text-slate-400" />
            Topic Curriculum Coverage
          </h2>
          <span className="text-[11px] font-mono text-slate-400 dark:text-slate-500">
            {topic.distinctTopicsCount} Categories
          </span>
        </div>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs border-collapse">
            <thead>
              <tr className="bg-slate-50/90 dark:bg-slate-800/80 border-b border-slate-200/80 dark:border-slate-800 font-mono text-[11px] text-slate-500 dark:text-slate-400 uppercase tracking-wider">
                <th className="py-2.5 px-4">Topic Category</th>
                <th className="py-2.5 px-4 w-24">Problems</th>
                <th className="py-2.5 px-4 w-1/3">Share of Curriculum</th>
                <th className="py-2.5 px-4 text-right">Actions</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100 dark:divide-slate-800">
              {topic.topicCounts.map((t) => (
                <tr key={t.topic} className="hover:bg-slate-50/70 dark:hover:bg-slate-800/50 transition-colors">
                  <td className="py-2.5 px-4 font-semibold text-slate-900 dark:text-slate-100">
                    <TopicBadge topic={t.topic} />
                  </td>
                  <td className="py-2.5 px-4 font-mono text-slate-700 dark:text-slate-300">
                    {t.count}
                  </td>
                  <td className="py-2.5 px-4">
                    <div className="flex items-center gap-2.5">
                      <div className="flex-1">
                        <ProgressBar percentage={t.percentage} color="emerald" height="sm" />
                      </div>
                      <span className="font-mono text-[11px] text-slate-500 dark:text-slate-400 w-10 text-right">
                        {t.percentage.toFixed(1)}%
                      </span>
                    </div>
                  </td>
                  <td className="py-2.5 px-4 text-right">
                    <button
                      onClick={() => navigate(`/problems?topic=${t.topic}`)}
                      className="text-[11px] font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 font-medium"
                    >
                      Filter Problems →
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
