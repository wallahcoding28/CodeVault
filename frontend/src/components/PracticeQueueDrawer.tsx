import React from 'react';
import { useNavigate } from 'react-router-dom';
import { X, PlaySquare, Trash2, Clock, ExternalLink } from 'lucide-react';
import { Question } from '../types';
import { DifficultyBadge, TopicBadge } from './Badge';

interface PracticeQueueDrawerProps {
  isOpen: boolean;
  onClose: () => void;
  queue: Question[];
  onRemove: (questionId: string) => Promise<void>;
  onClear?: () => Promise<void>;
  isLoading?: boolean;
}

export const PracticeQueueDrawer: React.FC<PracticeQueueDrawerProps> = ({
  isOpen,
  onClose,
  queue,
  onRemove,
  isLoading = false,
}) => {
  const navigate = useNavigate();

  React.useEffect(() => {
    if (!isOpen) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 overflow-hidden">
      {/* Backdrop */}
      <div
        className="fixed inset-0 bg-slate-900/40 backdrop-blur-xs transition-opacity"
        onClick={onClose}
      />

      <div className="fixed inset-y-0 right-0 max-w-full flex pl-10">
        <div className="w-screen max-w-md bg-white dark:bg-slate-900 border-l border-slate-200/90 dark:border-slate-800 shadow-2xl flex flex-col">
          {/* Header */}
          <div className="p-4 border-b border-slate-200/90 dark:border-slate-800 flex items-center justify-between bg-slate-50/80 dark:bg-slate-850">
            <div className="flex items-center gap-2">
              <div className="w-7 h-7 rounded-md bg-slate-900 dark:bg-indigo-600 text-white flex items-center justify-center">
                <PlaySquare className="w-4 h-4" />
              </div>
              <div>
                <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 tracking-tight">Active Practice Queue</h2>
                <p className="text-[11px] font-mono text-slate-500 dark:text-slate-400">
                  FIFO Order · {queue.length} {queue.length === 1 ? 'problem' : 'problems'}
                </p>
              </div>
            </div>
            <button
              onClick={onClose}
              className="p-1 rounded-md text-slate-400 hover:text-slate-600 dark:hover:text-slate-200 hover:bg-slate-200/50 dark:hover:bg-slate-800 transition-colors cursor-pointer"
            >
              <X className="w-4 h-4" />
            </button>
          </div>

          {/* Action Bar */}
          {queue.length > 0 && (
            <div className="px-4 py-2.5 bg-slate-50/50 dark:bg-slate-800/50 border-b border-slate-100 dark:border-slate-800 flex items-center justify-between">
              <span className="text-[11px] font-mono text-slate-500 dark:text-slate-400">
                Next up: <span className="font-semibold text-slate-800 dark:text-slate-200">{queue[0].title}</span>
              </span>
              <button
                onClick={() => {
                  onClose();
                  navigate('/practice');
                }}
                className="inline-flex items-center gap-1.5 px-3 py-1 bg-slate-900 dark:bg-indigo-600 text-white text-xs font-semibold rounded-md hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-2xs cursor-pointer"
              >
                <PlaySquare className="w-3.5 h-3.5" />
                <span>Start Drill</span>
              </button>
            </div>
          )}

          {/* Queue List */}
          <div className="flex-1 overflow-y-auto p-4 space-y-2.5">
            {isLoading ? (
              <div className="py-12 flex flex-col items-center justify-center gap-2 text-slate-400 dark:text-slate-500">
                <div className="w-6 h-6 border-2 border-slate-300 dark:border-slate-700 border-t-slate-800 dark:border-t-indigo-400 rounded-full animate-spin" />
                <span className="text-xs font-mono">Syncing practice queue...</span>
              </div>
            ) : queue.length === 0 ? (
              <div className="py-16 text-center px-4">
                <div className="w-10 h-10 rounded-full bg-slate-100 dark:bg-slate-800 text-slate-400 dark:text-slate-500 flex items-center justify-center mx-auto mb-3">
                  <Clock className="w-5 h-5" />
                </div>
                <h3 className="text-xs font-bold text-slate-800 dark:text-slate-200 uppercase tracking-wider mb-1">Queue is Empty</h3>
                <p className="text-xs text-slate-500 dark:text-slate-400 mb-4 max-w-xs mx-auto">
                  Add problems from the problem list or start a filtered drill session to populate your queue.
                </p>
                <button
                  onClick={() => {
                    onClose();
                    navigate('/problems');
                  }}
                  className="px-3.5 py-1.5 text-xs font-semibold text-slate-800 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded-md hover:bg-slate-50 dark:hover:bg-slate-700 shadow-2xs cursor-pointer"
                >
                  Browse Problems
                </button>
              </div>
            ) : (
              queue.map((q, idx) => (
                <div
                  key={q.id}
                  className="group bg-white dark:bg-slate-850 border border-slate-200/90 dark:border-slate-800 rounded-lg p-3 hover:border-slate-300 dark:hover:border-slate-700 hover:shadow-2xs transition-all relative"
                >
                  <div className="flex items-start justify-between gap-2 mb-1.5">
                    <div className="flex items-center gap-1.5">
                      <span className="w-5 h-5 rounded bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-300 font-mono text-[10px] font-bold flex items-center justify-center shrink-0">
                        {idx + 1}
                      </span>
                      <button
                        onClick={() => {
                          onClose();
                          navigate(`/problems/${q.id}`);
                        }}
                        className="text-xs font-bold text-slate-900 dark:text-slate-100 hover:text-indigo-600 dark:hover:text-indigo-400 transition-colors text-left line-clamp-1 cursor-pointer"
                        title={q.title}
                      >
                        {q.title}
                      </button>
                    </div>
                    <button
                      onClick={() => onRemove(q.id)}
                      className="text-slate-400 hover:text-rose-600 dark:hover:text-rose-400 p-1 rounded hover:bg-rose-50 dark:hover:bg-rose-950/40 transition-colors shrink-0 cursor-pointer"
                      title="Remove from queue"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>

                  <div className="flex items-center gap-2 pl-6.5 text-[11px]">
                    <DifficultyBadge difficulty={q.difficulty} />
                    <TopicBadge topic={q.topic} />
                    {q.company && (
                      <span className="font-mono text-slate-500 dark:text-slate-400 text-[10px] truncate max-w-[100px]">
                        {q.company}
                      </span>
                    )}
                  </div>
                </div>
              ))
            )}
          </div>

          {/* Footer */}
          {queue.length > 0 && (
            <div className="p-3 border-t border-slate-200 dark:border-slate-800 bg-slate-50 dark:bg-slate-850 flex items-center justify-between text-xs font-mono text-slate-500 dark:text-slate-400">
              <span>{queue.length} queued</span>
              <button
                onClick={() => {
                  onClose();
                  navigate('/practice');
                }}
                className="text-slate-900 dark:text-slate-100 font-semibold hover:underline flex items-center gap-1 cursor-pointer"
              >
                <span>Enter Practice Mode</span>
                <ExternalLink className="w-3 h-3" />
              </button>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
