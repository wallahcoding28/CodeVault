import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { PlaySquare, X, Clock } from 'lucide-react';
import { api } from '../services/api';
import { Topic, Difficulty, Status, PracticeSessionFilterPayload } from '../types';

interface StartSessionModalProps {
  isOpen: boolean;
  onClose: () => void;
  defaultTopic?: string;
  defaultDifficulty?: string;
}

const TOPICS: { key: Topic | 'All'; label: string }[] = [
  { key: 'All', label: 'All Topics' },
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

export const StartSessionModal: React.FC<StartSessionModalProps> = ({
  isOpen,
  onClose,
  defaultTopic = 'All',
  defaultDifficulty = 'All',
}) => {
  const navigate = useNavigate();
  const [topic, setTopic] = useState<string>(defaultTopic || 'All');
  const [difficulty, setDifficulty] = useState<string>(defaultDifficulty || 'All');
  const [status, setStatus] = useState<string>('All');
  const [dueOnly, setDueOnly] = useState<boolean>(false);
  const [isStarting, setIsStarting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  React.useEffect(() => {
    if (!isOpen) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  const handleStart = async (e: React.FormEvent) => {
    e.preventDefault();
    try {
      setIsStarting(true);
      setError(null);

      const filter: PracticeSessionFilterPayload = {
        dueOnly,
      };
      if (topic !== 'All') filter.topic = topic as Topic;
      if (difficulty !== 'All') filter.difficulty = difficulty as Difficulty;
      if (status !== 'All') filter.status = status as Status;

      const progress = await api.startFilteredPracticeSession(filter);
      if (progress.total === 0) {
        setError('No questions matched the selected criteria.');
        return;
      }

      onClose();
      navigate('/practice');
    } catch (err: any) {
      setError(err.message || 'Failed to start practice session');
    } finally {
      setIsStarting(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/40 backdrop-blur-xs">
      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-xl shadow-2xl max-w-md w-full overflow-hidden">
        {/* Header */}
        <div className="px-5 py-4 border-b border-slate-100 dark:border-slate-800 flex items-center justify-between bg-slate-50/60 dark:bg-slate-850">
          <div className="flex items-center gap-2.5">
            <div className="w-8 h-8 rounded-lg bg-slate-900 dark:bg-indigo-600 text-white flex items-center justify-center shadow-xs">
              <PlaySquare className="w-4 h-4" />
            </div>
            <div>
              <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 tracking-tight">Configure Practice Drill</h3>
              <p className="text-[11px] font-mono text-slate-500 dark:text-slate-400">Targeted FIFO Practice Queue</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-md text-slate-400 hover:text-slate-600 dark:hover:text-slate-200 hover:bg-slate-200/50 dark:hover:bg-slate-800 cursor-pointer"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Form Body */}
        <form onSubmit={handleStart} className="p-5 space-y-4">
          {error && (
            <div className="p-3 text-xs bg-rose-50 dark:bg-rose-950/40 border border-rose-200 dark:border-rose-900 text-rose-700 dark:text-rose-300 rounded-md font-mono">
              {error}
            </div>
          )}

          {/* Topic Select */}
          <div>
            <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1.5">Algorithmic Topic</label>
            <select
              value={topic}
              onChange={(e) => setTopic(e.target.value)}
              className="w-full text-xs border border-slate-300 dark:border-slate-700 rounded-md px-3 py-2 bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 font-mono"
            >
              {TOPICS.map((t) => (
                <option key={t.key} value={t.key}>
                  {t.label}
                </option>
              ))}
            </select>
          </div>

          {/* Difficulty Select */}
          <div>
            <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1.5">Difficulty Tier</label>
            <select
              value={difficulty}
              onChange={(e) => setDifficulty(e.target.value)}
              className="w-full text-xs border border-slate-300 dark:border-slate-700 rounded-md px-3 py-2 bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 font-mono"
            >
              <option value="All">All Difficulties</option>
              <option value="Easy">Easy</option>
              <option value="Medium">Medium</option>
              <option value="Hard">Hard</option>
            </select>
          </div>

          {/* Status Select */}
          <div>
            <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1.5">Status Filter</label>
            <select
              value={status}
              onChange={(e) => setStatus(e.target.value)}
              className="w-full text-xs border border-slate-300 dark:border-slate-700 rounded-md px-3 py-2 bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 font-mono"
            >
              <option value="All">All Problem Statuses</option>
              <option value="Unsolved">Unsolved Only</option>
              <option value="InProgress">In Progress / Attempted</option>
              <option value="Solved">Solved & Mastered</option>
            </select>
          </div>

          {/* Due Revisions Toggle */}
          <div className="pt-1">
            <label className="flex items-center gap-2 cursor-pointer select-none">
              <input
                type="checkbox"
                checked={dueOnly}
                onChange={(e) => setDueOnly(e.target.checked)}
                className="rounded border-slate-300 dark:border-slate-700 text-slate-900 dark:text-indigo-500 focus:ring-slate-900 dark:focus:ring-indigo-500 w-4 h-4"
              />
              <span className="text-xs text-slate-700 dark:text-slate-300 font-medium flex items-center gap-1.5">
                <Clock className="w-3.5 h-3.5 text-rose-500 dark:text-rose-400" />
                <span>Limit strictly to problems due for Spaced Revision</span>
              </span>
            </label>
          </div>

          {/* Actions */}
          <div className="pt-3 flex items-center justify-end gap-2 border-t border-slate-100 dark:border-slate-800">
            <button
              type="button"
              onClick={onClose}
              className="px-3.5 py-1.5 text-xs font-medium text-slate-600 dark:text-slate-300 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 cursor-pointer"
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={isStarting}
              className="px-4 py-1.5 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 rounded-md hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs flex items-center gap-1.5 disabled:opacity-50 cursor-pointer"
            >
              <PlaySquare className="w-3.5 h-3.5" />
              <span>{isStarting ? 'Starting Session...' : 'Start Session'}</span>
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
