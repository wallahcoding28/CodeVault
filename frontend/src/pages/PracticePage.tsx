import React, { useEffect, useState } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import {
  PlaySquare,
  CheckCircle2,
  HelpCircle,
  SkipForward,
  LogOut,
  ExternalLink,
  Award,
  Building2,
  Eye,
  EyeOff,
  ListOrdered,
  Sparkles,
  ArrowRight,
} from 'lucide-react';
import { api } from '../services/api';
import {
  Question,
  SessionProgress,
  Topic,
  Difficulty,
} from '../types';
import { DifficultyBadge, TopicBadge } from '../components/Badge';
import { ProgressBar } from '../components/ProgressBar';
import { PracticeQueueDrawer } from '../components/PracticeQueueDrawer';

const TOPICS: Topic[] = [
  'Arrays',
  'Strings',
  'LinkedLists',
  'StacksQueues',
  'Trees',
  'Graphs',
  'DynamicProgramming',
  'BinarySearch',
  'RecursionBacktracking',
  'Greedy',
  'Heaps',
  'BitManipulation',
  'MathGeometry',
  'Other',
];

export const PracticePage: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();

  // Mode & configuration state
  const [mode, setMode] = useState<string>(
    searchParams.get('mode') || 'Due'
  );
  const [selectedTopic, setSelectedTopic] = useState<string>('Arrays');
  const [selectedDifficulty, setSelectedDifficulty] = useState<Difficulty>('Medium');

  // Active session state
  const [currentQuestion, setCurrentQuestion] = useState<Question | null>(null);
  const [progress, setProgress] = useState<SessionProgress | null>(null);
  const [isStarting, setIsStarting] = useState(false);
  const [isRecording, setIsRecording] = useState(false);
  const [showNotes, setShowNotes] = useState(false);
  const [sessionCompleted, setSessionCompleted] = useState(false);

  // Queue drawer state
  const [isQueueDrawerOpen, setIsQueueDrawerOpen] = useState(false);
  const [practiceQueue, setPracticeQueue] = useState<Question[]>([]);
  const [queueCount, setQueueCount] = useState<number>(0);

  const fetchQueue = async () => {
    try {
      const qRes = await api.getPracticeQueue();
      setPracticeQueue(qRes.queue);
      setQueueCount(qRes.count);
    } catch {
      // ignore
    }
  };

  const handleRemoveFromQueue = async (questionId: string) => {
    try {
      await api.removeFromPracticeQueue(questionId);
      await fetchQueue();
    } catch (err: any) {
      alert(err.message || 'Failed to remove question from queue');
    }
  };

  // Check if an active session already exists in C++ backend
  useEffect(() => {
    const checkActiveSession = async () => {
      fetchQueue();
      try {
        const qid = searchParams.get('questionId');
        if (qid) {
          const prog = await api.startPracticeSession({ questionIds: [qid] });
          setProgress(prog);
        }
        const cur = await api.getCurrentPracticeQuestion();
        if (cur.hasQuestion && cur.question) {
          setCurrentQuestion(cur.question);
          if (cur.progress) setProgress(cur.progress);
        }
      } catch {
        // Backend idle or no active session
      }
    };
    checkActiveSession();
  }, [searchParams]);

  const handlePracticeNext = async () => {
    try {
      const nextRes = await api.getPracticeNext();
      if (nextRes.hasQuestion && nextRes.question) {
        navigate(
          `/problems/${nextRes.question.id}?reason=${encodeURIComponent(
            nextRes.recommendationReason || ''
          )}`
        );
      } else {
        alert('All caught up! No practice problems currently recommended.');
      }
    } catch (err: any) {
      alert(err.message || 'Failed to fetch next practice problem');
    }
  };

  const handleStartSession = async () => {
    try {
      setIsStarting(true);
      setSessionCompleted(false);

      let filterStr = 'all';
      if (mode === 'Due') filterStr = 'due';
      else if (mode === 'Unsolved') filterStr = 'all_unsolved';
      else if (mode === 'Favorites') filterStr = 'favorites';
      else if (mode === 'Topic') filterStr = `topic:${selectedTopic}`;
      else if (mode === 'Difficulty') filterStr = `difficulty:${selectedDifficulty}`;

      const prog = await api.startPracticeSession({ filter: filterStr });
      setProgress(prog);

      const cur = await api.getCurrentPracticeQuestion();
      if (cur.hasQuestion && cur.question) {
        setCurrentQuestion(cur.question);
        setShowNotes(false);
      } else {
        setSessionCompleted(true);
      }
    } catch (err: any) {
      alert(err.message || 'Failed to start practice session. Ensure questions match this mode.');
    } finally {
      setIsStarting(false);
    }
  };

  const handleVerdict = async (verdict: 'Solved' | 'NeedsReview') => {
    if (!currentQuestion) return;
    try {
      setIsRecording(true);
      const res = await api.submitPracticeVerdict(verdict);
      setProgress(res.progress);
      if (res.hasNext && res.nextQuestion) {
        setCurrentQuestion(res.nextQuestion);
        setShowNotes(false);
      } else {
        setCurrentQuestion(null);
        setSessionCompleted(true);
      }
    } catch (err: any) {
      alert(err.message || 'Failed to record verdict.');
    } finally {
      setIsRecording(false);
    }
  };

  const handleSkip = async () => {
    try {
      setIsRecording(true);
      await api.skipPracticeQuestion();
      const cur = await api.getCurrentPracticeQuestion();
      if (cur.hasQuestion && cur.question) {
        setCurrentQuestion(cur.question);
        if (cur.progress) setProgress(cur.progress);
        setShowNotes(false);
      } else {
        setCurrentQuestion(null);
        setSessionCompleted(true);
      }
    } catch (err: any) {
      alert(err.message || 'Failed to skip question.');
    } finally {
      setIsRecording(false);
    }
  };

  const handleExitSession = async () => {
    try {
      await api.exitPracticeSession();
      setCurrentQuestion(null);
      setSessionCompleted(true);
    } catch (err) {
      console.error(err);
    }
  };

  // Keyboard shortcuts for active solving session
  useEffect(() => {
    if (!currentQuestion || !progress || sessionCompleted) return;

    const handleKeyDown = (e: KeyboardEvent) => {
      // Don't trigger if user is holding modifier keys (e.g. Ctrl+R, Cmd+S)
      if (e.ctrlKey || e.metaKey || e.altKey) {
        return;
      }

      // Don't trigger if user is focusing an input, textarea, or contentEditable
      const target = e.target as HTMLElement | null;
      if (
        target &&
        (['INPUT', 'TEXTAREA', 'SELECT'].includes(target.tagName) ||
          target.isContentEditable ||
          target.getAttribute('role') === 'textbox')
      ) {
        return;
      }

      if (e.key === '1' || e.key === 's' || e.key === 'S') {
        e.preventDefault();
        handleVerdict('Solved');
      } else if (e.key === '2' || e.key === 'r' || e.key === 'R') {
        e.preventDefault();
        handleVerdict('NeedsReview');
      } else if (e.key === '3' || e.key === 'k' || e.key === 'K') {
        e.preventDefault();
        handleSkip();
      } else if (e.key === 'h' || e.key === 'H' || e.key === 'n' || e.key === 'N') {
        e.preventDefault();
        setShowNotes((prev) => !prev);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [currentQuestion, progress, sessionCompleted]);

  // If session completed summary is visible
  if (sessionCompleted && progress) {
    return (
      <div className="max-w-xl mx-auto px-6 py-16 text-center">
        <div className="w-12 h-12 bg-emerald-50 text-emerald-600 rounded-full flex items-center justify-center mx-auto mb-4 border border-emerald-200">
          <Award className="w-6 h-6" />
        </div>
        <h2 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100 mb-1">
          Practice Drill Completed
        </h2>
        <p className="text-xs text-slate-500 dark:text-slate-400 mb-6 font-mono">
          Reviewed queue across {progress.total} algorithmic problems.
        </p>

        {/* Stats card */}
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs mb-6">
          <div className="grid grid-cols-3 gap-4 divide-x divide-slate-100 dark:divide-slate-800">
            <div>
              <div className="text-2xl font-bold font-mono text-emerald-600 dark:text-emerald-400">
                {progress.completed}
              </div>
              <div className="text-[11px] font-mono text-slate-500 dark:text-slate-400 mt-1 uppercase">
                Solved
              </div>
            </div>
            <div>
              <div className="text-2xl font-bold font-mono text-slate-400 dark:text-slate-500">
                {progress.skipped}
              </div>
              <div className="text-[11px] font-mono text-slate-500 dark:text-slate-400 mt-1 uppercase">
                Skipped
              </div>
            </div>
            <div>
              <div className="text-2xl font-bold font-mono text-indigo-600 dark:text-indigo-400">
                {progress.remaining}
              </div>
              <div className="text-[11px] font-mono text-slate-500 dark:text-slate-400 mt-1 uppercase">
                Remaining
              </div>
            </div>
          </div>
        </div>

        <div className="flex flex-wrap items-center justify-center gap-3">
          <button
            onClick={handlePracticeNext}
            className="inline-flex items-center gap-1.5 px-4 py-2 text-xs font-semibold text-white bg-indigo-600 hover:bg-indigo-700 rounded shadow-xs transition-colors font-mono"
          >
            <Sparkles className="w-3.5 h-3.5" />
            <span>Practice Next Problem (SRS)</span>
            <ArrowRight className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => {
              setSessionCompleted(false);
              setProgress(null);
            }}
            className="px-4 py-2 text-xs font-semibold text-slate-700 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded hover:bg-slate-50 dark:hover:bg-slate-700 shadow-2xs transition-colors"
          >
            Start Another Drill
          </button>
          <button
            onClick={() => navigate('/problems')}
            className="px-4 py-2 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 rounded hover:bg-slate-800 dark:hover:bg-indigo-500 shadow-xs transition-colors"
          >
            Return to Problems
          </button>
        </div>
      </div>
    );
  }

  // Active Focused Practice View
  if (currentQuestion && progress) {
    const q = currentQuestion;
    const currentStep = progress.completed + progress.skipped + 1;
    const progressPercent = progress.total > 0
      ? Math.round(((progress.completed + progress.skipped) / progress.total) * 100)
      : 0;

    return (
      <div className="max-w-4xl mx-auto px-6 py-6 pb-28">
        {/* Top Session Progress Bar & Controls */}
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-3.5 mb-5 shadow-2xs">
          <div className="flex items-center justify-between text-xs mb-2 font-mono">
            <div className="flex items-center gap-2">
              <span className="font-semibold text-slate-900 dark:text-slate-100">
                Problem {currentStep} of {progress.total}
              </span>
              <span className="text-slate-400 dark:text-slate-500">({progressPercent}% complete)</span>
            </div>
            <div className="flex items-center gap-3">
              <span className="hidden sm:inline text-[11px] text-slate-400 dark:text-slate-500">
                Keys: <kbd className="px-1.5 py-0.5 bg-slate-100 dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-700 dark:text-slate-300 rounded text-[10px]">1</kbd> Solved{' '}
                <kbd className="px-1.5 py-0.5 bg-slate-100 dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-700 dark:text-slate-300 rounded text-[10px]">2</kbd> Review{' '}
                <kbd className="px-1.5 py-0.5 bg-slate-100 dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-700 dark:text-slate-300 rounded text-[10px]">3</kbd> Skip
              </span>
              <button
                onClick={() => setIsQueueDrawerOpen(true)}
                className="inline-flex items-center gap-1 text-xs text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-slate-100 bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 px-2 py-0.5 rounded transition-colors font-mono"
                title="Inspect practice queue"
              >
                <ListOrdered className="w-3.5 h-3.5 text-slate-500 dark:text-slate-400" />
                <span>Queue ({queueCount})</span>
              </button>
              <button
                onClick={handleExitSession}
                className="inline-flex items-center gap-1 text-xs text-slate-500 dark:text-slate-400 hover:text-rose-600 dark:hover:text-rose-400 transition-colors"
              >
                <LogOut className="w-3.5 h-3.5" />
                <span>Exit Drill</span>
              </button>
            </div>
          </div>
          <ProgressBar percentage={progressPercent} color="emerald" height="sm" />
        </div>

        {/* Main Problem Workspace */}
        <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-6 shadow-2xs space-y-5">
          {/* Metadata Header */}
          <div className="flex flex-wrap items-center gap-2 text-xs">
            <span className="font-mono text-slate-400 dark:text-slate-500 font-bold text-xs">{q.id}</span>
            <DifficultyBadge difficulty={q.difficulty} />
            <TopicBadge topic={q.topic} />
            {q.platform && (
              <span className="px-2 py-0.5 font-mono text-[10px] bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 rounded font-medium border border-slate-200 dark:border-slate-700">
                {q.platform}
              </span>
            )}
            {q.company && (
              <span className="inline-flex items-center gap-1 px-2 py-0.5 text-[11px] font-mono bg-slate-50 dark:bg-slate-800/60 text-slate-600 dark:text-slate-300 border border-slate-200 dark:border-slate-700 rounded">
                <Building2 className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                {q.company}
              </span>
            )}
            <span className="ml-auto font-mono text-[11px] text-indigo-700 dark:text-indigo-300 bg-indigo-50 dark:bg-indigo-950/40 px-2 py-0.5 rounded border border-indigo-100 dark:border-indigo-800/60">
              Box {q.revision_priority} (SRS)
            </span>
          </div>

          {/* Problem Title */}
          <div>
            <h1 className="text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100 leading-tight">
              {q.title}
            </h1>
            {q.source_url && (
              <div className="mt-1">
                <a
                  href={q.source_url}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center gap-1 text-xs font-mono text-indigo-600 dark:text-indigo-400 hover:text-indigo-800 dark:hover:text-indigo-300 underline underline-offset-2"
                >
                  <span>Open problem in {q.platform || 'source'}</span>
                  <ExternalLink className="w-3 h-3" />
                </a>
              </div>
            )}
          </div>

          {/* Problem Statement */}
          <div className="space-y-2">
            <div className="text-[11px] font-mono font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider">
              Problem Description
            </div>
            {q.description && q.description.trim().length > 0 ? (
              <div className="text-sm text-slate-800 dark:text-slate-200 leading-relaxed font-sans whitespace-pre-wrap bg-slate-50/70 dark:bg-slate-800/40 p-4 rounded-md border border-slate-200/80 dark:border-slate-800">
                {q.description}
              </div>
            ) : (
              <div className="text-xs text-slate-400 dark:text-slate-500 italic p-3 bg-slate-50 dark:bg-slate-800/50 rounded border border-slate-100 dark:border-slate-800">
                No description provided. Use the external link or recall the invariant from your notes.
              </div>
            )}
          </div>

          {/* Tags */}
          {q.tags && q.tags.length > 0 && (
            <div className="flex flex-wrap items-center gap-1 pt-1">
              <span className="text-[10px] font-mono text-slate-400 dark:text-slate-500 uppercase tracking-wider mr-1">Tags:</span>
              {q.tags.map((tag, idx) => (
                <span
                  key={idx}
                  className="px-2 py-0.5 text-[10px] font-mono bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-300 rounded border border-slate-200 dark:border-slate-700"
                >
                  #{tag}
                </span>
              ))}
            </div>
          )}

          {/* Collapsible Approach & Notes Reveal Drawer */}
          <div className="border-t border-slate-100 dark:border-slate-800 pt-4">
            <button
              onClick={() => setShowNotes(!showNotes)}
              className="inline-flex items-center gap-1.5 text-xs font-semibold text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-slate-100 transition-colors"
            >
              {showNotes ? (
                <>
                  <EyeOff className="w-3.5 h-3.5 text-slate-500 dark:text-slate-400" />
                  <span>Hide Approach & Invariant Notes</span>
                </>
              ) : (
                <>
                  <Eye className="w-3.5 h-3.5 text-indigo-600 dark:text-indigo-400" />
                  <span className="text-indigo-600 dark:text-indigo-400">Reveal Solution Approach & Complexity (Self-Check)</span>
                </>
              )}
              <span className="text-[10px] font-mono text-slate-400 dark:text-slate-500 ml-1">
                ({showNotes ? 'Press N to hide' : 'Press N to reveal'})
              </span>
            </button>

            {showNotes && (
              <div className="mt-3 p-4 bg-slate-900 dark:bg-slate-950 text-slate-100 rounded-md border border-slate-800 text-xs font-mono leading-relaxed whitespace-pre-wrap">
                <div className="text-[10px] font-bold text-slate-400 uppercase tracking-wider mb-2 border-b border-slate-800 pb-1">
                  Recorded Notes & Algorithmic Invariants
                </div>
                {q.notes || 'No approach notes recorded for this question.'}
              </div>
            )}
          </div>
        </div>

        {/* Floating/Anchored Action Verdict Dock */}
        <div className="fixed bottom-0 left-0 right-0 z-40 bg-white/95 dark:bg-slate-900/95 backdrop-blur-md border-t border-slate-200 dark:border-slate-800 py-3 px-6 shadow-lg">
          <div className="max-w-4xl mx-auto flex items-center justify-between gap-4">
            <div className="text-xs font-mono text-slate-500 dark:text-slate-400 hidden sm:block">
              Record outcome for <strong className="text-slate-900 dark:text-slate-100">{q.title}</strong>:
            </div>

            <div className="flex items-center gap-3 w-full sm:w-auto">
              <button
                disabled={isRecording}
                onClick={() => handleVerdict('Solved')}
                className="flex-1 sm:flex-initial py-2.5 px-5 text-xs font-semibold text-white bg-emerald-600 hover:bg-emerald-700 rounded-md shadow-xs flex items-center justify-center gap-2 transition-colors disabled:opacity-50"
              >
                <CheckCircle2 className="w-4 h-4" />
                <span>Mark Solved</span>
                <kbd className="hidden md:inline px-1 py-0.2 bg-emerald-700/60 rounded text-[10px] font-mono">1</kbd>
              </button>

              <button
                disabled={isRecording}
                onClick={() => handleVerdict('NeedsReview')}
                className="flex-1 sm:flex-initial py-2.5 px-5 text-xs font-semibold text-white bg-amber-600 hover:bg-amber-700 rounded-md shadow-xs flex items-center justify-center gap-2 transition-colors disabled:opacity-50"
              >
                <HelpCircle className="w-4 h-4" />
                <span>Needs Review</span>
                <kbd className="hidden md:inline px-1 py-0.2 bg-amber-700/60 rounded text-[10px] font-mono">2</kbd>
              </button>

              <button
                disabled={isRecording}
                onClick={handleSkip}
                className="flex-1 sm:flex-initial py-2.5 px-4 text-xs font-semibold text-slate-700 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 hover:bg-slate-50 dark:hover:bg-slate-700 rounded-md shadow-2xs flex items-center justify-center gap-1.5 transition-colors disabled:opacity-50"
              >
                <SkipForward className="w-3.5 h-3.5 text-slate-400 dark:text-slate-500" />
                <span>Skip</span>
                <kbd className="hidden md:inline px-1 py-0.2 bg-slate-100 dark:bg-slate-700 rounded text-[10px] font-mono">3</kbd>
              </button>
            </div>
          </div>
        </div>

        <PracticeQueueDrawer
          isOpen={isQueueDrawerOpen}
          onClose={() => {
            setIsQueueDrawerOpen(false);
            fetchQueue();
          }}
          queue={practiceQueue}
          onRemove={handleRemoveFromQueue}
        />
      </div>
    );
  }

  // Strategy / Mode Selection View
  return (
    <div className="max-w-4xl mx-auto px-6 py-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-5">
        <div>
          <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
            Focused Practice Session
          </h1>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
            Loads selected problems into an in-memory custom C++ FIFO Queue for deliberate, distraction-free recall drills.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => setIsQueueDrawerOpen(true)}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-mono font-medium text-slate-700 dark:text-slate-200 bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 rounded hover:bg-slate-50 dark:hover:bg-slate-700 shadow-2xs transition-colors"
          >
            <ListOrdered className="w-3.5 h-3.5 text-slate-500 dark:text-slate-400" />
            <span>Practice Queue ({queueCount})</span>
          </button>

          <button
            onClick={handlePracticeNext}
            className="inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-mono font-semibold text-indigo-700 dark:text-indigo-300 bg-indigo-50 dark:bg-indigo-950/40 border border-indigo-200 dark:border-indigo-800 rounded hover:bg-indigo-100 dark:hover:bg-indigo-900/60 transition-colors shadow-2xs"
          >
            <Sparkles className="w-3.5 h-3.5 text-indigo-600 dark:text-indigo-400" />
            <span>Practice Next</span>
          </button>
        </div>
      </div>

      <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-lg p-5 shadow-2xs space-y-5">
        {/* Practice Mode Selector */}
        <div>
          <label className="block text-xs font-semibold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider mb-2.5">
            Select Practice Strategy
          </label>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-2.5">
            {[
              {
                id: 'Due',
                title: 'Due Spaced Revisions',
                desc: 'Problems past their Leitner SRS interval pulled from MinHeap.',
              },
              {
                id: 'Unsolved',
                title: 'Unsolved Curriculum',
                desc: 'Unsolved problems requiring initial mastery.',
              },
              {
                id: 'Favorites',
                title: 'Starred Benchmarks',
                desc: 'Key problems marked as favorites for review.',
              },
              {
                id: 'Topic',
                title: 'By Topic',
                desc: 'Deep-dive into a single algorithmic category.',
              },
              {
                id: 'Difficulty',
                title: 'By Difficulty',
                desc: 'Targeted difficulty tier practice (Easy/Medium/Hard).',
              },
              {
                id: 'All',
                title: 'All Problems',
                desc: 'Comprehensive review across entire problem inventory.',
              },
            ].map((m) => (
              <div
                key={m.id}
                onClick={() => setMode(m.id)}
                className={`p-3.5 rounded-md border cursor-pointer transition-all ${
                  mode === m.id
                    ? 'border-indigo-600 dark:border-indigo-500 bg-indigo-50/50 dark:bg-indigo-950/30 ring-1 ring-indigo-600 dark:ring-indigo-500'
                    : 'border-slate-200 dark:border-slate-700 bg-white dark:bg-slate-800/80 hover:border-slate-300 dark:hover:border-slate-600'
                }`}
              >
                <div className="flex items-center justify-between mb-1">
                  <span className="text-xs font-bold text-slate-900 dark:text-slate-100 font-mono">
                    {m.title}
                  </span>
                  {mode === m.id && (
                    <div className="w-2 h-2 rounded-full bg-indigo-600 dark:bg-indigo-400" />
                  )}
                </div>
                <p className="text-[11px] text-slate-500 dark:text-slate-400 leading-normal">
                  {m.desc}
                </p>
              </div>
            ))}
          </div>
        </div>

        {/* Dynamic Filters depending on mode */}
        {mode === 'Topic' && (
          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider mb-1.5">
              Select DSA Topic
            </label>
            <select
              value={selectedTopic}
              onChange={(e) => setSelectedTopic(e.target.value)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100"
            >
              {TOPICS.map((t) => (
                <option key={t} value={t}>
                  {t}
                </option>
              ))}
            </select>
          </div>
        )}

        {mode === 'Difficulty' && (
          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-100 uppercase font-mono tracking-wider mb-1.5">
              Select Difficulty Tier
            </label>
            <select
              value={selectedDifficulty}
              onChange={(e) => setSelectedDifficulty(e.target.value as Difficulty)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100"
            >
              <option value="Easy">Easy</option>
              <option value="Medium">Medium</option>
              <option value="Hard">Hard</option>
            </select>
          </div>
        )}

        {/* Launch action */}
        <div className="pt-3 border-t border-slate-100 dark:border-slate-800 flex items-center justify-end">
          <button
            onClick={handleStartSession}
            disabled={isStarting}
            className="inline-flex items-center gap-2 px-5 py-2 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 hover:bg-slate-800 dark:hover:bg-indigo-500 rounded transition-colors shadow-xs disabled:opacity-50"
          >
            <PlaySquare className="w-3.5 h-3.5" />
            <span>{isStarting ? 'Initializing Queue...' : 'Begin Practice Queue'}</span>
          </button>
        </div>
      </div>

      <PracticeQueueDrawer
        isOpen={isQueueDrawerOpen}
        onClose={() => {
          setIsQueueDrawerOpen(false);
          fetchQueue();
        }}
        queue={practiceQueue}
        onRemove={handleRemoveFromQueue}
      />
    </div>
  );
};
