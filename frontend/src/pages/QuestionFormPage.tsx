import React, { useEffect, useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import {
  ArrowLeft,
  Save,
  AlertCircle,
  Star,
} from 'lucide-react';
import { api } from '../services/api';
import {
  Difficulty,
  Status,
  Topic,
  Platform,
  Question,
} from '../types';

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

const PLATFORMS: Platform[] = [
  'LeetCode',
  'HackerRank',
  'Codeforces',
  'GeeksforGeeks',
  'CodeStudio',
  'Custom',
];

export const QuestionFormPage: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const isEdit = Boolean(id);

  const [loading, setLoading] = useState(isEdit);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Form Fields
  const [title, setTitle] = useState('');
  const [description, setDescription] = useState('');
  const [platform, setPlatform] = useState<Platform>('LeetCode');
  const [sourceUrl, setSourceUrl] = useState('');
  const [topic, setTopic] = useState<Topic>('Arrays');
  const [difficulty, setDifficulty] = useState<Difficulty>('Medium');
  const [status, setStatus] = useState<Status>('Unsolved');
  const [company, setCompany] = useState('');
  const [tagsInput, setTagsInput] = useState('');
  const [notes, setNotes] = useState('');
  const [revisionPriority, setRevisionPriority] = useState<number>(1);
  const [isFavorite, setIsFavorite] = useState(false);

  // Load existing question for editing
  useEffect(() => {
    if (isEdit && id) {
      const load = async () => {
        try {
          setLoading(true);
          const q = await api.getQuestionById(id);
          setTitle(q.title);
          setDescription(q.description || '');
          setPlatform(q.platform || 'LeetCode');
          setSourceUrl(q.source_url || '');
          setTopic(q.topic);
          setDifficulty(q.difficulty);
          setStatus(q.status);
          setCompany(q.company || '');
          setTagsInput(q.tags ? q.tags.join(', ') : '');
          setNotes(q.notes || '');
          setRevisionPriority(q.revision_priority || 1);
          setIsFavorite(q.is_favorite);
        } catch (err: any) {
          setError(err.message || 'Failed to load question details.');
        } finally {
          setLoading(false);
        }
      };
      load();
    }
  }, [id, isEdit]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!title.trim()) {
      setError('Problem title is required.');
      return;
    }

    try {
      setSaving(true);
      setError(null);

      const tags = tagsInput
        .split(',')
        .map((t) => t.trim())
        .filter((t) => t.length > 0);

      const payload: Partial<Question> = {
        title: title.trim(),
        description: description.trim(),
        platform,
        source_url: sourceUrl.trim(),
        topic,
        difficulty,
        status,
        company: company.trim(),
        tags,
        notes: notes.trim(),
        revision_priority: revisionPriority,
        is_favorite: isFavorite,
      };

      if (isEdit && id) {
        await api.updateQuestion(id, payload);
        navigate(`/problems/${id}`);
      } else {
        const created = await api.createQuestion(payload);
        navigate(`/problems/${created.id}`);
      }
    } catch (err: any) {
      console.error(err);
      setError(err.message || 'Failed to save question to CodeVault.');
    } finally {
      setSaving(false);
    }
  };

  if (loading) {
    return (
      <div className="max-w-4xl mx-auto px-6 py-16 flex flex-col items-center justify-center gap-3">
        <div className="w-8 h-8 border-2 border-slate-300 border-t-slate-900 rounded-full animate-spin"></div>
        <span className="text-xs font-mono text-slate-500">Loading problem record...</span>
      </div>
    );
  }

  return (
    <div className="max-w-4xl mx-auto px-6 py-8">
      {/* Header and Back Link */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <button
            onClick={() => navigate(-1)}
            className="inline-flex items-center gap-1.5 text-xs font-mono text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 transition-colors mb-2"
          >
            <ArrowLeft className="w-3.5 h-3.5" />
            <span>Cancel and Go Back</span>
          </button>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
            {isEdit ? `Edit Problem ${id}` : 'Record New Problem'}
          </h1>
          <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
            Synchronizes with C++ domain model and updates CSV persistence upon save.
          </p>
        </div>

        <button
          type="button"
          onClick={() => setIsFavorite(!isFavorite)}
          className={`inline-flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium border rounded transition-colors ${
            isFavorite
              ? 'bg-amber-50 dark:bg-amber-950/40 border-amber-300 dark:border-amber-800 text-amber-900 dark:text-amber-200'
              : 'bg-white dark:bg-slate-800 border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700'
          }`}
        >
          <Star
            className={`w-3.5 h-3.5 ${
              isFavorite ? 'fill-amber-400 text-amber-500 dark:text-amber-400' : 'text-slate-400 dark:text-slate-500'
            }`}
          />
          <span>{isFavorite ? 'Favorited' : 'Mark Favorite'}</span>
        </button>
      </div>

      {error && (
        <div className="mb-6 p-4 bg-rose-50 dark:bg-rose-950/40 border border-rose-200 dark:border-rose-900/60 rounded-lg flex items-center gap-3 text-xs text-rose-800 dark:text-rose-300">
          <AlertCircle className="w-4 h-4 shrink-0 text-rose-600 dark:text-rose-400" />
          <span>{error}</span>
        </div>
      )}

      {/* Form Card */}
      <form onSubmit={handleSubmit} className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs space-y-6">
        {/* Title */}
        <div>
          <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
            Problem Title <span className="text-rose-600 dark:text-rose-400">*</span>
          </label>
          <input
            type="text"
            required
            placeholder="e.g. 3Sum, LRU Cache, Course Schedule"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 placeholder:text-slate-400 dark:placeholder:text-slate-500 font-medium focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 focus:border-slate-900 dark:focus:border-indigo-500"
          />
        </div>

        {/* Platform & External URL */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Platform
            </label>
            <select
              value={platform}
              onChange={(e) => setPlatform(e.target.value as Platform)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            >
              {PLATFORMS.map((p) => (
                <option key={p} value={p}>
                  {p}
                </option>
              ))}
            </select>
          </div>

          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Source URL
            </label>
            <input
              type="url"
              placeholder="https://leetcode.com/problems/..."
              value={sourceUrl}
              onChange={(e) => setSourceUrl(e.target.value)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 placeholder:text-slate-400 dark:placeholder:text-slate-500 font-mono focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            />
          </div>
        </div>

        {/* Topic, Difficulty, Status */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              DSA Topic
            </label>
            <select
              value={topic}
              onChange={(e) => setTopic(e.target.value as Topic)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            >
              {TOPICS.map((t) => (
                <option key={t} value={t}>
                  {t}
                </option>
              ))}
            </select>
          </div>

          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Difficulty
            </label>
            <select
              value={difficulty}
              onChange={(e) => setDifficulty(e.target.value as Difficulty)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            >
              <option value="Easy">Easy</option>
              <option value="Medium">Medium</option>
              <option value="Hard">Hard</option>
            </select>
          </div>

          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Current Status
            </label>
            <select
              value={status}
              onChange={(e) => setStatus(e.target.value as Status)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            >
              <option value="Unsolved">Unsolved</option>
              <option value="InProgress">InProgress</option>
              <option value="Solved">Solved</option>
              <option value="Mastered">Mastered</option>
            </select>
          </div>
        </div>

        {/* Company & Tags */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Company (Target Interviewer)
            </label>
            <input
              type="text"
              placeholder="e.g. Google, Amazon, Microsoft, Meta"
              value={company}
              onChange={(e) => setCompany(e.target.value)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 placeholder:text-slate-400 dark:placeholder:text-slate-500 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
              Tags (Comma separated)
            </label>
            <input
              type="text"
              placeholder="e.g. two-pointer, prefix-sum, sliding-window"
              value={tagsInput}
              onChange={(e) => setTagsInput(e.target.value)}
              className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-800 dark:text-slate-100 placeholder:text-slate-400 dark:placeholder:text-slate-500 font-mono focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500"
            />
          </div>
        </div>

        {/* Revision Leitner Box */}
        <div>
          <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
            Initial Leitner Box (Retention Priority)
          </label>
          <div className="grid grid-cols-5 gap-2">
            {[1, 2, 3, 4, 5].map((lvl) => (
              <button
                key={lvl}
                type="button"
                onClick={() => setRevisionPriority(lvl)}
                className={`py-2 px-3 text-xs font-mono rounded border text-center transition-all ${
                  revisionPriority === lvl
                    ? 'border-indigo-600 dark:border-indigo-500 bg-indigo-50 dark:bg-indigo-950/40 text-indigo-900 dark:text-indigo-200 font-bold'
                    : 'border-slate-200 dark:border-slate-700 bg-slate-50 dark:bg-slate-800 text-slate-600 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-700'
                }`}
              >
                Box {lvl}
              </button>
            ))}
          </div>
        </div>

        {/* Notes, Complexity & Implementation Invariants */}
        <div>
          <label className="block text-xs font-semibold text-slate-900 dark:text-slate-200 uppercase font-mono tracking-wider mb-1.5">
            Approach Invariants, Complexities & Solution Notes
          </label>
          <textarea
            rows={6}
            placeholder="Write key algorithmic insights, corner cases, time complexity O(...) and space complexity O(...)"
            value={notes}
            onChange={(e) => setNotes(e.target.value)}
            className="w-full px-3 py-2 text-xs border border-slate-300 dark:border-slate-700 rounded bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 placeholder:text-slate-400 dark:placeholder:text-slate-500 font-mono leading-relaxed focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 focus:border-slate-900 dark:focus:border-indigo-500"
          ></textarea>
        </div>

        {/* Action Buttons */}
        <div className="flex items-center justify-end gap-3 pt-4 border-t border-slate-100 dark:border-slate-800">
          <button
            type="button"
            onClick={() => navigate(-1)}
            className="px-4 py-2 text-xs font-medium text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 transition-colors"
          >
            Cancel
          </button>
          <button
            type="submit"
            disabled={saving}
            className="inline-flex items-center gap-2 px-5 py-2 text-xs font-semibold text-white bg-slate-900 dark:bg-indigo-600 rounded hover:bg-slate-800 dark:hover:bg-indigo-500 disabled:opacity-50 transition-colors shadow-xs"
          >
            <Save className="w-3.5 h-3.5" />
            <span>{saving ? 'Persisting to CodeVault...' : isEdit ? 'Update Problem' : 'Save Problem'}</span>
          </button>
        </div>
      </form>
    </div>
  );
};
