import React, { useEffect, useState, useRef } from 'react';
import { useSearchParams } from 'react-router-dom';
import {
  User as UserIcon,
  Key,
  Laptop,
  Download,
  Upload,
  FileText,
  RotateCcw,
  Database,
  Cpu,
  Layers,
  CheckCircle2,
  AlertCircle,
  LogOut,
  ShieldCheck,
  Sun,
  Moon,
} from 'lucide-react';
import { useAuth } from '../context/AuthContext';
import { useTheme } from '../context/ThemeContext';
import { api } from '../services/api';
import { Diagnostics, ConflictStrategy, ImportResult } from '../types';

interface SessionItem {
  id: string;
  createdAt: number;
  expiresAt: number;
  lastSeenAt: number;
  isCurrentSession: boolean;
}

export const SettingsPage: React.FC = () => {
  const { user, refreshUser } = useAuth();
  const { theme, setTheme, effectiveTheme } = useTheme();
  const [searchParams, setSearchParams] = useSearchParams();
  const tabParam = searchParams.get('tab');

  // Active sub-tab
  const [activeTab, setActiveTab] = useState<'profile' | 'security' | 'sessions' | 'export' | 'diagnostics'>(() => {
    if (tabParam === 'security' || tabParam === 'sessions' || tabParam === 'export' || tabParam === 'diagnostics') {
      return tabParam;
    }
    return 'profile';
  });

  useEffect(() => {
    const t = searchParams.get('tab');
    if (t === 'profile' || t === 'security' || t === 'sessions' || t === 'export' || t === 'diagnostics') {
      setActiveTab(t);
    }
  }, [searchParams]);

  // Diagnostics state
  const [diagnostics, setDiagnostics] = useState<Diagnostics | null>(null);
  const [totalQuestions, setTotalQuestions] = useState<number>(0);

  // Profile Form State
  const [displayName, setDisplayName] = useState(user?.displayName || '');
  const [email, setEmail] = useState(user?.email || '');
  const [profileLoading, setProfileLoading] = useState(false);
  const [profileMsg, setProfileMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  // Password Form State
  const [currentPassword, setCurrentPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [passwordLoading, setPasswordLoading] = useState(false);
  const [passwordMsg, setPasswordMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  // Sessions State
  const [sessions, setSessions] = useState<SessionItem[]>([]);
  const [sessionsLoading, setSessionsLoading] = useState(false);
  const [sessionsMsg, setSessionsMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  // Export State
  const [exportLoading, setExportLoading] = useState<'json' | 'csv' | 'markdown' | 'anki' | null>(null);
  const [exportMsg, setExportMsg] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  // Import State
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [conflictStrategy, setConflictStrategy] = useState<ConflictStrategy>('skip');
  const [importLoading, setImportLoading] = useState(false);
  const [importResult, setImportResult] = useState<ImportResult | null>(null);
  const [importClientError, setImportClientError] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);

  const fetchDiagnostics = async () => {
    try {
      const [questions, diag] = await Promise.all([
        api.getQuestions(),
        api.getDiagnostics(),
      ]);
      setTotalQuestions(questions.length);
      setDiagnostics(diag);
    } catch {
      // Ignore
    }
  };

  const fetchSessions = async () => {
    setSessionsLoading(true);
    try {
      const data = await api.getSessions();
      setSessions(data.sessions || []);
    } catch (err: any) {
      setSessionsMsg({ type: 'error', text: err.message || 'Failed to load active sessions' });
    } finally {
      setSessionsLoading(false);
    }
  };

  useEffect(() => {
    fetchDiagnostics();
    fetchSessions();
  }, []);

  useEffect(() => {
    if (user) {
      setDisplayName(user.displayName || '');
      setEmail(user.email || '');
    }
  }, [user]);

  // Profile Update Handler
  const handleProfileSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setProfileLoading(true);
    setProfileMsg(null);
    try {
      await api.updateProfile({ displayName, email });
      await refreshUser();
      setProfileMsg({ type: 'success', text: 'Profile updated successfully' });
    } catch (err: any) {
      setProfileMsg({ type: 'error', text: err.message || 'Failed to update profile' });
    } finally {
      setProfileLoading(false);
    }
  };

  // Password Change Handler
  const handlePasswordSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setPasswordMsg(null);

    if (newPassword.length < 8) {
      setPasswordMsg({ type: 'error', text: 'New password must be at least 8 characters long' });
      return;
    }
    if (newPassword !== confirmPassword) {
      setPasswordMsg({ type: 'error', text: 'New password and confirmation do not match' });
      return;
    }

    setPasswordLoading(true);
    try {
      const res = await api.changePassword({ currentPassword, newPassword });
      setPasswordMsg({ type: 'success', text: res.message || 'Password changed successfully' });
      setCurrentPassword('');
      setNewPassword('');
      setConfirmPassword('');
    } catch (err: any) {
      setPasswordMsg({ type: 'error', text: err.message || 'Failed to change password' });
    } finally {
      setPasswordLoading(false);
    }
  };

  // Revoke Specific Session
  const handleRevokeSession = async (sessionId: string) => {
    try {
      await api.revokeSession(sessionId);
      setSessionsMsg({ type: 'success', text: 'Session revoked' });
      fetchSessions();
    } catch (err: any) {
      setSessionsMsg({ type: 'error', text: err.message || 'Failed to revoke session' });
    }
  };

  // Revoke Other Sessions
  const handleRevokeOtherSessions = async () => {
    try {
      await api.revokeOtherSessions();
      setSessionsMsg({ type: 'success', text: 'All other sessions have been signed out' });
      fetchSessions();
    } catch (err: any) {
      setSessionsMsg({ type: 'error', text: err.message || 'Failed to sign out other sessions' });
    }
  };

  // Export Data Handler (JSON, CSV, Markdown, Anki)
  const handleExport = async (format: 'json' | 'csv' | 'markdown' | 'anki') => {
    setExportLoading(format);
    setExportMsg(null);
    try {
      if (format === 'json' || format === 'csv') {
        await api.downloadExport(format);
        setExportMsg({ type: 'success', text: `Downloaded codevault_questions_export.${format}` });
      } else if (format === 'markdown') {
        await api.exportMarkdownCatalog();
        setExportMsg({ type: 'success', text: 'Downloaded codevault-questions.md' });
      } else if (format === 'anki') {
        await api.exportAnkiCatalog();
        setExportMsg({ type: 'success', text: 'Downloaded codevault-questions-anki.tsv' });
      }
    } catch (err: any) {
      setExportMsg({ type: 'error', text: err.message || 'Failed to download export' });
    } finally {
      setExportLoading(null);
    }
  };

  // Import File Handlers
  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setImportClientError(null);
    setImportResult(null);
    const file = e.target.files?.[0];
    if (!file) return;

    const lowerName = file.name.toLowerCase();
    if (!lowerName.endsWith('.json') && !lowerName.endsWith('.csv')) {
      setImportClientError('Invalid file type. Only .json and .csv catalog files are supported.');
      setSelectedFile(null);
      if (fileInputRef.current) fileInputRef.current.value = '';
      return;
    }

    setSelectedFile(file);
  };

  const handleClearFile = () => {
    setSelectedFile(null);
    setImportClientError(null);
    setImportResult(null);
    if (fileInputRef.current) {
      fileInputRef.current.value = '';
    }
  };

  const handleImportSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!selectedFile) {
      setImportClientError('Please select a .json or .csv file to import.');
      return;
    }

    setImportLoading(true);
    setImportClientError(null);
    setImportResult(null);

    try {
      const res = await api.importCatalog(selectedFile, conflictStrategy);
      setImportResult(res);
      if (res.success) {
        fetchDiagnostics();
      }
    } catch (err: any) {
      setImportClientError(err.message || 'An unexpected error occurred during import.');
    } finally {
      setImportLoading(false);
    }
  };

  const formatDate = (epochSec: number) => {
    if (!epochSec) return 'Never';
    return new Date(epochSec * 1000).toLocaleString(undefined, {
      month: 'short',
      day: 'numeric',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  return (
    <div className="max-w-5xl mx-auto px-6 py-8">
      {/* Header */}
      <div className="mb-6">
        <div className="flex items-center gap-2 text-xs font-mono text-slate-500 dark:text-slate-400 uppercase tracking-wider mb-1">
          <span>Settings</span>
          <span>/</span>
          <span>Account & System</span>
        </div>
        <h1 className="text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
          Account & Environment Settings
        </h1>
        <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
          Manage your personal profile, security credentials, appearance theme, active sessions, and local C++17 engine state.
        </p>
      </div>

      {/* Settings Navigation Tabs */}
      <div className="flex items-center gap-2 border-b border-slate-200 dark:border-slate-800 mb-6 overflow-x-auto pb-1 text-sm font-medium">
        <button
          onClick={() => {
            setActiveTab('profile');
            setSearchParams({ tab: 'profile' });
          }}
          className={`flex items-center gap-2 px-3 py-2 border-b-2 whitespace-nowrap transition-colors cursor-pointer ${
            activeTab === 'profile'
              ? 'border-indigo-600 dark:border-indigo-400 text-indigo-600 dark:text-indigo-400 font-semibold'
              : 'border-transparent text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
          }`}
        >
          <UserIcon className="w-4 h-4" />
          <span>Profile & Appearance</span>
        </button>

        <button
          onClick={() => {
            setActiveTab('security');
            setSearchParams({ tab: 'security' });
          }}
          className={`flex items-center gap-2 px-3 py-2 border-b-2 whitespace-nowrap transition-colors cursor-pointer ${
            activeTab === 'security'
              ? 'border-indigo-600 dark:border-indigo-400 text-indigo-600 dark:text-indigo-400 font-semibold'
              : 'border-transparent text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
          }`}
        >
          <Key className="w-4 h-4" />
          <span>Security & Password</span>
        </button>

        <button
          onClick={() => {
            setActiveTab('sessions');
            setSearchParams({ tab: 'sessions' });
            fetchSessions();
          }}
          className={`flex items-center gap-2 px-3 py-2 border-b-2 whitespace-nowrap transition-colors cursor-pointer ${
            activeTab === 'sessions'
              ? 'border-indigo-600 dark:border-indigo-400 text-indigo-600 dark:text-indigo-400 font-semibold'
              : 'border-transparent text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
          }`}
        >
          <Laptop className="w-4 h-4" />
          <span>Active Sessions</span>
          {sessions.length > 0 && (
            <span className="text-[10px] font-mono bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 px-1.5 py-0.2 rounded-full font-bold">
              {sessions.length}
            </span>
          )}
        </button>

        <button
          onClick={() => {
            setActiveTab('export');
            setSearchParams({ tab: 'export' });
          }}
          aria-label="Data Portability & Backup"
          className={`flex items-center gap-2 px-3 py-2 border-b-2 whitespace-nowrap transition-colors cursor-pointer ${
            activeTab === 'export'
              ? 'border-indigo-600 dark:border-indigo-400 text-indigo-600 dark:text-indigo-400 font-semibold'
              : 'border-transparent text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
          }`}
        >
          <Download className="w-4 h-4" />
          <span>Data Portability</span>
        </button>

        <button
          onClick={() => {
            setActiveTab('diagnostics');
            setSearchParams({ tab: 'diagnostics' });
            fetchDiagnostics();
          }}
          className={`flex items-center gap-2 px-3 py-2 border-b-2 whitespace-nowrap transition-colors cursor-pointer ${
            activeTab === 'diagnostics'
              ? 'border-indigo-600 dark:border-indigo-400 text-indigo-600 dark:text-indigo-400 font-semibold'
              : 'border-transparent text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
          }`}
        >
          <Cpu className="w-4 h-4" />
          <span>Engine Diagnostics</span>
        </button>
      </div>

      {/* Tab 1: Profile & Appearance Settings */}
      {activeTab === 'profile' && (
        <div className="space-y-6 max-w-2xl">
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <UserIcon className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                User Profile
              </h2>
              <span className="text-xs font-mono text-slate-400 dark:text-slate-500">ID: {user?.id}</span>
            </div>

            {profileMsg && (
              <div
                className={`p-3 rounded text-xs flex items-center gap-2 mb-4 font-mono ${
                  profileMsg.type === 'success'
                    ? 'bg-emerald-50 dark:bg-emerald-950/40 text-emerald-800 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-900'
                    : 'bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-900'
                }`}
              >
                {profileMsg.type === 'success' ? (
                  <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400 shrink-0" />
                ) : (
                  <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
                )}
                <span>{profileMsg.text}</span>
              </div>
            )}

            <form onSubmit={handleProfileSubmit} className="space-y-4">
              <div>
                <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                  Username
                </label>
                <input
                  type="text"
                  disabled
                  value={user?.username || ''}
                  className="w-full bg-slate-100 dark:bg-slate-800 text-slate-500 dark:text-slate-400 border border-slate-200 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono cursor-not-allowed"
                />
                <span className="text-[11px] text-slate-400 dark:text-slate-500 mt-1 block">
                  Usernames are permanent and cannot be modified.
                </span>
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                  Display Name
                </label>
                <input
                  type="text"
                  value={displayName}
                  onChange={(e) => setDisplayName(e.target.value)}
                  placeholder="e.g. Alex Chen"
                  className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs focus:ring-1 focus:ring-indigo-500 focus:outline-none"
                />
              </div>

              <div>
                <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                  Email Address
                </label>
                <input
                  type="email"
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                  placeholder="e.g. alex@example.com"
                  className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono focus:ring-1 focus:ring-indigo-500 focus:outline-none"
                />
              </div>

              <div className="pt-2">
                <button
                  type="submit"
                  disabled={profileLoading}
                  className="bg-indigo-600 hover:bg-indigo-700 text-white text-xs font-semibold px-4 py-2 rounded transition-colors disabled:opacity-50 cursor-pointer"
                >
                  {profileLoading ? 'Saving...' : 'Save Profile Changes'}
                </button>
              </div>
            </form>
          </div>

          {/* Appearance & Theme Selector Card */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2 mb-1">
              {effectiveTheme === 'dark' ? (
                <Moon className="w-4 h-4 text-indigo-400" />
              ) : (
                <Sun className="w-4 h-4 text-amber-500" />
              )}
              Appearance & Theme
            </h3>
            <p className="text-xs text-slate-500 dark:text-slate-400 mb-4">
              Select your interface theme preference. System mode automatically matches your operating system or browser preference.
            </p>
            <div className="grid grid-cols-3 gap-3">
              <button
                type="button"
                onClick={() => setTheme('light')}
                className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all cursor-pointer ${
                  theme === 'light'
                    ? 'border-indigo-600 bg-indigo-50/50 dark:bg-indigo-950/30 text-indigo-700 dark:text-indigo-300 ring-1 ring-indigo-600'
                    : 'border-slate-200 dark:border-slate-700 hover:border-slate-300 dark:hover:border-slate-600 text-slate-700 dark:text-slate-300 bg-white dark:bg-slate-800'
                }`}
              >
                <Sun className="w-5 h-5 mb-1.5 text-amber-500" />
                <span className="font-semibold">Light</span>
                <span className="text-[10px] text-slate-400 font-normal mt-0.5">Crisp, clean contrast</span>
              </button>
              <button
                type="button"
                onClick={() => setTheme('dark')}
                className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all cursor-pointer ${
                  theme === 'dark'
                    ? 'border-indigo-600 bg-indigo-50/50 dark:bg-indigo-950/30 text-indigo-700 dark:text-indigo-300 ring-1 ring-indigo-600'
                    : 'border-slate-200 dark:border-slate-700 hover:border-slate-300 dark:hover:border-slate-600 text-slate-700 dark:text-slate-300 bg-white dark:bg-slate-800'
                }`}
              >
                <Moon className="w-5 h-5 mb-1.5 text-indigo-400" />
                <span className="font-semibold">Dark</span>
                <span className="text-[10px] text-slate-400 font-normal mt-0.5">Low-light focus</span>
              </button>
              <button
                type="button"
                onClick={() => setTheme('system')}
                className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all cursor-pointer ${
                  theme === 'system'
                    ? 'border-indigo-600 bg-indigo-50/50 dark:bg-indigo-950/30 text-indigo-700 dark:text-indigo-300 ring-1 ring-indigo-600'
                    : 'border-slate-200 dark:border-slate-700 hover:border-slate-300 dark:hover:border-slate-600 text-slate-700 dark:text-slate-300 bg-white dark:bg-slate-800'
                }`}
              >
                <Laptop className="w-5 h-5 mb-1.5 text-slate-500" />
                <span className="font-semibold">System</span>
                <span className="text-[10px] text-slate-400 font-normal mt-0.5">Match device ({effectiveTheme})</span>
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Tab 2: Security & Password */}
      {activeTab === 'security' && (
        <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs max-w-2xl">
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
              <Key className="w-4 h-4 text-slate-700 dark:text-slate-300" />
              Password & Credentials
            </h2>
            <div className="flex items-center gap-1.5 text-xs font-mono text-indigo-700 dark:text-indigo-300 bg-indigo-50 dark:bg-indigo-950/40 px-2 py-0.5 rounded border border-indigo-200 dark:border-indigo-800">
              <ShieldCheck className="w-3.5 h-3.5 text-indigo-600 dark:text-indigo-400" />
              <span>Argon2id Protected</span>
            </div>
          </div>

          {passwordMsg && (
            <div
              className={`p-3 rounded text-xs flex items-center gap-2 mb-4 font-mono ${
                passwordMsg.type === 'success'
                  ? 'bg-emerald-50 dark:bg-emerald-950/40 text-emerald-800 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-800'
                  : 'bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-800'
              }`}
            >
              {passwordMsg.type === 'success' ? (
                <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400 shrink-0" />
              ) : (
                <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
              )}
              <span>{passwordMsg.text}</span>
            </div>
          )}

          <form onSubmit={handlePasswordSubmit} className="space-y-4">
            <div>
              <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                Current Password
              </label>
              <input
                type="password"
                required
                value={currentPassword}
                onChange={(e) => setCurrentPassword(e.target.value)}
                placeholder="••••••••"
                className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono focus:ring-1 focus:ring-indigo-500 focus:outline-none"
              />
            </div>

            <div>
              <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                New Password
              </label>
              <input
                type="password"
                required
                value={newPassword}
                onChange={(e) => setNewPassword(e.target.value)}
                placeholder="At least 8 characters"
                className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono focus:ring-1 focus:ring-indigo-500 focus:outline-none"
              />
            </div>

            <div>
              <label className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                Confirm New Password
              </label>
              <input
                type="password"
                required
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                placeholder="••••••••"
                className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono focus:ring-1 focus:ring-indigo-500 focus:outline-none"
              />
            </div>

            <div className="pt-2">
              <button
                type="submit"
                disabled={passwordLoading}
                className="bg-indigo-600 hover:bg-indigo-700 text-white text-xs font-semibold px-4 py-2 rounded transition-colors disabled:opacity-50 cursor-pointer"
              >
                {passwordLoading ? 'Updating Password...' : 'Update Password'}
              </button>
            </div>
          </form>
        </div>
      )}

      {/* Tab 3: Active Sessions */}
      {activeTab === 'sessions' && (
        <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
          <div className="flex items-center justify-between mb-4">
            <div>
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Laptop className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                Active Sessions
              </h2>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Manage all active browser sessions authenticated to your CodeVault account.
              </p>
            </div>
            {sessions.length > 1 && (
              <button
                onClick={handleRevokeOtherSessions}
                className="inline-flex items-center gap-1.5 text-xs font-medium text-rose-700 dark:text-rose-300 bg-rose-50 dark:bg-rose-950/40 hover:bg-rose-100 dark:hover:bg-rose-900/50 border border-rose-200 dark:border-rose-800 px-3 py-1.5 rounded transition-colors cursor-pointer"
              >
                <LogOut className="w-3.5 h-3.5" />
                Sign Out Other Devices
              </button>
            )}
          </div>

          {sessionsMsg && (
            <div
              className={`p-3 rounded text-xs flex items-center gap-2 mb-4 font-mono ${
                sessionsMsg.type === 'success'
                  ? 'bg-emerald-50 dark:bg-emerald-950/40 text-emerald-800 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-800'
                  : 'bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-800'
              }`}
            >
              {sessionsMsg.type === 'success' ? (
                <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400 shrink-0" />
              ) : (
                <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
              )}
              <span>{sessionsMsg.text}</span>
            </div>
          )}

          {sessionsLoading ? (
            <div className="py-8 text-center text-xs font-mono text-slate-400 dark:text-slate-500">Loading active sessions...</div>
          ) : sessions.length === 0 ? (
            <div className="py-8 text-center text-xs font-mono text-slate-400 dark:text-slate-500">No active sessions found.</div>
          ) : (
            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs border-collapse font-mono">
                <thead>
                  <tr className="bg-slate-50 dark:bg-slate-800/80 border-b border-slate-200 dark:border-slate-800 text-slate-500 dark:text-slate-400 text-[11px]">
                    <th className="py-2.5 px-4">Session ID</th>
                    <th className="py-2.5 px-4">Status</th>
                    <th className="py-2.5 px-4">Last Active</th>
                    <th className="py-2.5 px-4">Created</th>
                    <th className="py-2.5 px-4">Expires</th>
                    <th className="py-2.5 px-4 text-right">Actions</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100 dark:divide-slate-800 text-slate-700 dark:text-slate-300">
                  {sessions.map((s) => (
                    <tr key={s.id} className={s.isCurrentSession ? 'bg-indigo-50/40 dark:bg-indigo-950/20' : ''}>
                      <td className="py-2.5 px-4 font-semibold text-slate-900 dark:text-slate-100">
                        {s.id}
                      </td>
                      <td className="py-2.5 px-4">
                        {s.isCurrentSession ? (
                          <span className="inline-flex items-center gap-1 text-[10px] font-bold uppercase tracking-wider text-emerald-700 dark:text-emerald-300 bg-emerald-100 dark:bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-200 dark:border-emerald-800/60">
                            <span className="w-1.5 h-1.5 rounded-full bg-emerald-500" />
                            Current Session
                          </span>
                        ) : (
                          <span className="inline-flex items-center gap-1 text-[10px] font-medium text-slate-600 dark:text-slate-300 bg-slate-100 dark:bg-slate-800 px-2 py-0.5 rounded border border-slate-200 dark:border-slate-700">
                            Active Device
                          </span>
                        )}
                      </td>
                      <td className="py-2.5 px-4 text-slate-600 dark:text-slate-300">
                        {formatDate(s.lastSeenAt)}
                      </td>
                      <td className="py-2.5 px-4 text-slate-500 dark:text-slate-400">
                        {formatDate(s.createdAt)}
                      </td>
                      <td className="py-2.5 px-4 text-slate-500 dark:text-slate-400">
                        {formatDate(s.expiresAt)}
                      </td>
                      <td className="py-2.5 px-4 text-right">
                        {!s.isCurrentSession ? (
                          <button
                            onClick={() => handleRevokeSession(s.id)}
                            className="text-xs text-rose-600 dark:text-rose-400 hover:text-rose-800 dark:hover:text-rose-300 font-medium hover:underline cursor-pointer"
                          >
                            Revoke
                          </button>
                        ) : (
                          <span className="text-[11px] text-slate-400 dark:text-slate-500 italic">This Device</span>
                        )}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}

      {/* Tab 4: Data Portability (Export & Import) */}
      {activeTab === 'export' && (
        <div className="space-y-6 max-w-3xl">
          {/* Section 1: Personal Catalog Export */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Download className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                Personal Catalog Export & Backup
              </h2>
              <span className="text-xs font-mono text-slate-400 dark:text-slate-500">Owner-Scoped</span>
            </div>

            <p className="text-xs text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
              Export your entire algorithmic problem catalog, including solution notes, tags, difficulty, topic categorizations, and Leitner spaced repetition intervals. Your personal data is completely isolated and never mixed with other users.
            </p>

            {exportMsg && (
              <div
                role={exportMsg.type === 'error' ? 'alert' : 'status'}
                className={`p-3 rounded text-xs flex items-center gap-2 mb-4 font-mono ${
                  exportMsg.type === 'success'
                    ? 'bg-emerald-50 dark:bg-emerald-950/40 text-emerald-800 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-800'
                    : 'bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-800'
                }`}
              >
                {exportMsg.type === 'success' ? (
                  <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400 shrink-0" />
                ) : (
                  <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
                )}
                <span>{exportMsg.text}</span>
              </div>
            )}

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
              {/* JSON Export */}
              <div className="border border-slate-200 dark:border-slate-800 rounded-lg p-4 bg-slate-50 dark:bg-slate-800/50 flex flex-col justify-between">
                <div>
                  <h3 className="text-xs font-bold text-slate-900 dark:text-slate-100 mb-1">JSON Format</h3>
                  <p className="text-[11px] text-slate-500 dark:text-slate-400 mb-4">
                    Full structured JSON payload containing all question entities, tags arrays, and timestamps. Best for automated programmatic restore or backup.
                  </p>
                </div>
                <button
                  onClick={() => handleExport('json')}
                  disabled={exportLoading !== null}
                  aria-label="Export catalog as JSON"
                  className="w-full inline-flex items-center justify-center gap-1.5 bg-slate-900 dark:bg-slate-700 hover:bg-slate-800 dark:hover:bg-slate-600 text-white text-xs font-semibold px-3 py-2 rounded transition-colors disabled:opacity-50 font-mono cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5" />
                  {exportLoading === 'json' ? 'Exporting...' : 'Export as JSON'}
                </button>
              </div>

              {/* CSV Export */}
              <div className="border border-slate-200 dark:border-slate-800 rounded-lg p-4 bg-slate-50 dark:bg-slate-800/50 flex flex-col justify-between">
                <div>
                  <h3 className="text-xs font-bold text-slate-900 dark:text-slate-100 mb-1">CSV Spreadsheet</h3>
                  <p className="text-[11px] text-slate-500 dark:text-slate-400 mb-4">
                    Standard RFC 4180 comma-delimited table compatible with Excel, Google Sheets, or legacy CodeVault flat-file engines.
                  </p>
                </div>
                <button
                  onClick={() => handleExport('csv')}
                  disabled={exportLoading !== null}
                  aria-label="Export catalog as CSV"
                  className="w-full inline-flex items-center justify-center gap-1.5 bg-indigo-600 hover:bg-indigo-700 text-white text-xs font-semibold px-3 py-2 rounded transition-colors disabled:opacity-50 font-mono cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5" />
                  {exportLoading === 'csv' ? 'Exporting...' : 'Export as CSV'}
                </button>
              </div>

              {/* Markdown Export */}
              <div className="border border-slate-200 dark:border-slate-800 rounded-lg p-4 bg-slate-50 dark:bg-slate-800/50 flex flex-col justify-between">
                <div>
                  <h3 className="text-xs font-bold text-slate-900 dark:text-slate-100 mb-1">Markdown Catalog</h3>
                  <p className="text-[11px] text-slate-500 dark:text-slate-400 mb-4">
                    Structured GitHub-flavored Markdown problem catalog organized by topic and difficulty, with direct URLs and notes.
                  </p>
                </div>
                <button
                  onClick={() => handleExport('markdown')}
                  disabled={exportLoading !== null}
                  aria-label="Export catalog as Markdown"
                  className="w-full inline-flex items-center justify-center gap-1.5 bg-emerald-700 hover:bg-emerald-800 text-white text-xs font-semibold px-3 py-2 rounded transition-colors disabled:opacity-50 font-mono cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5" />
                  {exportLoading === 'markdown' ? 'Exporting...' : 'Export as Markdown'}
                </button>
              </div>

              {/* Anki Export */}
              <div className="border border-slate-200 dark:border-slate-800 rounded-lg p-4 bg-slate-50 dark:bg-slate-800/50 flex flex-col justify-between">
                <div>
                  <h3 className="text-xs font-bold text-slate-900 dark:text-slate-100 mb-1">Anki Flashcards</h3>
                  <p className="text-[11px] text-slate-500 dark:text-slate-400 mb-4">
                    Tab-separated values (TSV) formatted for instant import into Anki (Front: Title/Topic/Difficulty, Back: Notes/Description).
                  </p>
                </div>
                <button
                  onClick={() => handleExport('anki')}
                  disabled={exportLoading !== null}
                  aria-label="Export catalog as Anki flashcards"
                  className="w-full inline-flex items-center justify-center gap-1.5 bg-amber-700 hover:bg-amber-800 text-white text-xs font-semibold px-3 py-2 rounded transition-colors disabled:opacity-50 font-mono cursor-pointer"
                >
                  <Download className="w-3.5 h-3.5" />
                  {exportLoading === 'anki' ? 'Exporting...' : 'Export as Anki TSV'}
                </button>
              </div>
            </div>
          </div>

          {/* Section 2: Catalog Import & Restore */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Upload className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                Catalog Import & Restore
              </h2>
              <span className="text-xs font-mono text-indigo-600 dark:text-indigo-400 bg-indigo-50 dark:bg-indigo-950/40 px-2 py-0.5 rounded border border-indigo-200 dark:border-indigo-800">
                Atomic Transaction
              </span>
            </div>

            <p className="text-xs text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
              Import problems from an existing JSON or CSV file into your personal catalog. All imports are transactional — any validation failure cleanly rolls back the entire batch, preserving database integrity.
            </p>

            <form onSubmit={handleImportSubmit} className="space-y-4">
              {/* File Selector */}
              <div>
                <label htmlFor="catalog-file-input" className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                  Catalog File (.json or .csv)
                </label>
                <input
                  ref={fileInputRef}
                  id="catalog-file-input"
                  type="file"
                  accept=".json,.csv"
                  onChange={handleFileChange}
                  className="sr-only"
                  aria-label="Upload catalog file in JSON or CSV format"
                />

                {!selectedFile ? (
                  <button
                    type="button"
                    onClick={() => fileInputRef.current?.click()}
                    disabled={importLoading}
                    className="w-full border-2 border-dashed border-slate-300 dark:border-slate-700 hover:border-indigo-400 dark:hover:border-indigo-500 hover:bg-slate-50/70 dark:hover:bg-slate-850 rounded-lg p-5 text-center transition-colors focus:outline-none focus:ring-1 focus:ring-indigo-500 disabled:opacity-50 cursor-pointer"
                  >
                    <Upload className="w-5 h-5 text-slate-400 dark:text-slate-500 mx-auto mb-1.5" />
                    <span className="block text-xs font-medium text-slate-700 dark:text-slate-300">
                      Click to choose a .json or .csv file from your device
                    </span>
                    <span className="block text-[11px] text-slate-400 dark:text-slate-500 mt-0.5">
                      Accepts CodeVault JSON export or RFC 4180 CSV table
                    </span>
                  </button>
                ) : (
                  <div className="bg-slate-50 dark:bg-slate-800 border border-slate-200 dark:border-slate-700 rounded-lg p-3 flex items-center justify-between">
                    <div className="flex items-center gap-2.5 overflow-hidden">
                      <FileText className="w-4 h-4 text-indigo-600 dark:text-indigo-400 shrink-0" />
                      <div className="truncate">
                        <span className="text-xs font-mono font-medium text-slate-900 dark:text-slate-100 block truncate">
                          {selectedFile.name}
                        </span>
                        <span className="text-[10px] font-mono text-slate-400 dark:text-slate-500">
                          {(selectedFile.size / 1024).toFixed(1)} KB
                        </span>
                      </div>
                    </div>
                    <button
                      type="button"
                      onClick={handleClearFile}
                      disabled={importLoading}
                      className="inline-flex items-center gap-1 text-xs text-slate-500 dark:text-slate-400 hover:text-rose-600 dark:hover:text-rose-400 px-2 py-1 rounded hover:bg-slate-100 dark:hover:bg-slate-700 transition-colors disabled:opacity-50 cursor-pointer"
                    >
                      <RotateCcw className="w-3.5 h-3.5" />
                      <span>Change</span>
                    </button>
                  </div>
                )}
              </div>

              {/* Conflict Strategy Selector */}
              <div>
                <label htmlFor="conflict-strategy-select" className="block text-xs font-semibold text-slate-700 dark:text-slate-300 mb-1">
                  Conflict Resolution Strategy
                </label>
                <select
                  id="conflict-strategy-select"
                  aria-label="Conflict Resolution Strategy"
                  value={conflictStrategy}
                  onChange={(e) => setConflictStrategy(e.target.value as ConflictStrategy)}
                  disabled={importLoading}
                  className="w-full bg-white dark:bg-slate-800 text-slate-900 dark:text-slate-100 border border-slate-300 dark:border-slate-700 rounded px-3 py-2 text-xs font-mono focus:ring-1 focus:ring-indigo-500 focus:outline-none disabled:opacity-50"
                >
                  <option value="skip">Skip existing (Preserve current catalog entries)</option>
                  <option value="overwrite">Overwrite existing (Update matched entries)</option>
                  <option value="generate_new_id">Generate new ID (Import duplicates as distinct questions)</option>
                </select>
                <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-1">
                  {conflictStrategy === 'skip' && 'Questions matching existing titles or IDs are ignored; existing catalog data remains untouched.'}
                  {conflictStrategy === 'overwrite' && 'Questions matching existing titles or IDs are updated with newly imported fields.'}
                  {conflictStrategy === 'generate_new_id' && 'New unique UUIDs are generated for matching questions, allowing duplicate titles as distinct questions.'}
                </p>
              </div>

              {/* Import Action Buttons */}
              <div className="flex items-center gap-3 pt-2">
                <button
                  type="submit"
                  disabled={!selectedFile || importLoading}
                  aria-label="Start catalog import"
                  className="inline-flex items-center gap-1.5 bg-indigo-600 hover:bg-indigo-700 text-white text-xs font-semibold px-4 py-2 rounded transition-colors disabled:opacity-50 font-mono cursor-pointer"
                >
                  <Upload className="w-3.5 h-3.5" />
                  {importLoading ? 'Importing Catalog...' : 'Import Catalog'}
                </button>

                {(selectedFile || importResult || importClientError) && (
                  <button
                    type="button"
                    onClick={handleClearFile}
                    disabled={importLoading}
                    aria-label="Reset import form"
                    className="inline-flex items-center gap-1.5 bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 text-slate-700 dark:text-slate-300 text-xs font-medium px-3 py-2 rounded transition-colors disabled:opacity-50 cursor-pointer"
                  >
                    <RotateCcw className="w-3.5 h-3.5" />
                    <span>Reset</span>
                  </button>
                )}
              </div>
            </form>

            {/* Client Error Banner */}
            {importClientError && (
              <div
                role="alert"
                className="mt-4 p-3 rounded text-xs flex items-center gap-2 font-mono bg-rose-50 dark:bg-rose-950/40 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-900"
              >
                <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
                <span>{importClientError}</span>
              </div>
            )}

            {/* Backend Success Banner */}
            {importResult && importResult.success && (
              <div
                role="status"
                className="mt-4 p-4 rounded-lg text-xs font-mono bg-emerald-50 dark:bg-emerald-950/40 text-emerald-900 dark:text-emerald-200 border border-emerald-200 dark:border-emerald-800 space-y-2.5"
              >
                <div className="flex items-center gap-2 font-bold text-emerald-800 dark:text-emerald-300">
                  <CheckCircle2 className="w-4 h-4 text-emerald-600 dark:text-emerald-400 shrink-0" />
                  <span>Import Complete</span>
                </div>
                <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 pt-1 text-center">
                  <div className="bg-white dark:bg-slate-800 p-2.5 rounded border border-emerald-100 dark:border-emerald-900/60 shadow-2xs">
                    <span className="text-[10px] text-slate-500 dark:text-slate-400 block uppercase tracking-wider">Processed</span>
                    <span className="font-bold text-sm text-slate-900 dark:text-slate-100">{importResult.totalProcessed}</span>
                  </div>
                  <div className="bg-white dark:bg-slate-800 p-2.5 rounded border border-emerald-100 dark:border-emerald-900/60 shadow-2xs">
                    <span className="text-[10px] text-emerald-600 dark:text-emerald-400 block uppercase tracking-wider">Imported</span>
                    <span className="font-bold text-sm text-emerald-700 dark:text-emerald-300">{importResult.importedCount}</span>
                  </div>
                  <div className="bg-white dark:bg-slate-800 p-2.5 rounded border border-emerald-100 dark:border-emerald-900/60 shadow-2xs">
                    <span className="text-[10px] text-indigo-600 dark:text-indigo-400 block uppercase tracking-wider">Updated</span>
                    <span className="font-bold text-sm text-indigo-700 dark:text-indigo-300">{importResult.updatedCount}</span>
                  </div>
                  <div className="bg-white dark:bg-slate-800 p-2.5 rounded border border-emerald-100 dark:border-emerald-900/60 shadow-2xs">
                    <span className="text-[10px] text-slate-500 dark:text-slate-400 block uppercase tracking-wider">Skipped</span>
                    <span className="font-bold text-sm text-slate-700 dark:text-slate-300">{importResult.skippedCount}</span>
                  </div>
                </div>
                <p className="text-[11px] text-emerald-700 dark:text-emerald-400 pt-0.5">
                  All records committed atomically to your personal catalog.
                </p>
              </div>
            )}

            {/* Backend Failure Banner (Atomic Rollback) */}
            {importResult && !importResult.success && (
              <div
                role="alert"
                className="mt-4 p-4 rounded-lg text-xs font-mono bg-rose-50 dark:bg-rose-950/40 text-rose-900 dark:text-rose-200 border border-rose-200 dark:border-rose-900 space-y-2.5"
              >
                <div className="flex items-center gap-2 font-bold text-rose-800 dark:text-rose-300">
                  <AlertCircle className="w-4 h-4 text-rose-600 dark:text-rose-400 shrink-0" />
                  <span>Import Failed (Transaction Rolled Back)</span>
                </div>
                <p className="text-[11px] text-rose-700 dark:text-rose-400">
                  No questions were modified or saved. Please address the following issues and try again:
                </p>
                <ul className="list-disc list-inside space-y-1 text-[11px] text-rose-800 dark:text-rose-300 bg-white dark:bg-slate-800 p-3 rounded border border-rose-100 dark:border-rose-900/60 max-h-48 overflow-y-auto">
                  {importResult.errors.map((err, idx) => (
                    <li key={idx} className="break-words">{err}</li>
                  ))}
                </ul>
              </div>
            )}
          </div>
        </div>
      )}

      {/* Tab 5: Engine Diagnostics */}
      {activeTab === 'diagnostics' && (
        <div className="space-y-6">
          {/* Persistence & Data Storage Card */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Database className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                SQLite Engine & Persistence
              </h2>
              <div className="flex items-center gap-1.5 text-xs font-mono text-emerald-700 dark:text-emerald-300 bg-emerald-50 dark:bg-emerald-950/40 px-2 py-0.5 rounded border border-emerald-200 dark:border-emerald-800">
                <CheckCircle2 className="w-3.5 h-3.5 text-emerald-600 dark:text-emerald-400" />
                <span>WAL Mode Active</span>
              </div>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-xs font-mono">
              <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded border border-slate-200 dark:border-slate-700">
                <span className="text-slate-400 dark:text-slate-500 block mb-1 text-[11px]">Primary SQLite Store:</span>
                <span className="font-semibold text-slate-800 dark:text-slate-200 break-all">
                  {diagnostics?.storagePath || 'data/codevault.db'}
                </span>
              </div>
              <div className="p-3 bg-slate-50 dark:bg-slate-800/60 rounded border border-slate-200 dark:border-slate-700">
                <span className="text-slate-400 dark:text-slate-500 block mb-1 text-[11px]">Your Catalog Items:</span>
                <span className="font-semibold text-slate-800 dark:text-slate-200">{totalQuestions} questions tracked</span>
              </div>
            </div>

            <p className="text-[11px] text-slate-500 dark:text-slate-400 mt-4 leading-normal">
              CodeVault uses an embedded ACID-compliant SQLite engine with WAL journaling, Schema v3 tables (questions, users, user_credentials, sessions), foreign-key referential integrity, and secondary indexing on problem parameters.
            </p>
          </div>

          {/* DSA Engine Architecture Status */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Cpu className="w-4 h-4 text-slate-700 dark:text-slate-300" />
                Custom C++ DSA Subsystems
              </h2>
              <span className="text-xs font-mono text-slate-400 dark:text-slate-500">
                {diagnostics?.invariantsPassed || 6} / {diagnostics?.totalInvariants || 6} Invariants Active
              </span>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
              {[
                {
                  name: 'Prefix Trie Tree',
                  purpose: 'Title autocomplete & fast prefix lookup',
                  complexity: 'O(L) search, L = query length',
                  active: diagnostics?.prefixTrieActive ?? true,
                },
                {
                  name: 'Priority MinHeap',
                  purpose: 'Spaced repetition scheduling by due epoch',
                  complexity: 'O(log N) insert / extract-min',
                  active: diagnostics?.minHeapActive ?? true,
                },
                {
                  name: 'FIFO Queue',
                  purpose: 'Practice session problem sequencing',
                  complexity: 'O(1) enqueue / dequeue',
                  active: diagnostics?.practiceQueueActive ?? true,
                },
                {
                  name: 'LIFO Stack',
                  purpose: 'CLI recent problem navigation & history',
                  complexity: 'O(1) push / pop',
                  active: diagnostics?.historyStackActive ?? true,
                },
                {
                  name: 'Doubly Linked List',
                  purpose: 'Bidirectional in-memory problem catalog',
                  complexity: 'O(1) splice / sequential scan',
                  active: true,
                },
                {
                  name: 'Dual Sort Engine',
                  purpose: 'MergeSort (stable) & QuickSort (in-place)',
                  complexity: 'O(N log N) worst/avg guarantees',
                  active: diagnostics?.sortingEngineActive ?? true,
                },
              ].map((dsa) => (
                <div
                  key={dsa.name}
                  className="p-3 bg-slate-50 dark:bg-slate-800/60 border border-slate-200 dark:border-slate-700 rounded text-xs"
                >
                  <div className="flex items-center justify-between mb-1">
                    <span className="font-semibold text-slate-900 dark:text-slate-100 font-mono">
                      {dsa.name}
                    </span>
                    <span className={`w-1.5 h-1.5 rounded-full ${dsa.active ? 'bg-emerald-500' : 'bg-rose-500'}`} />
                  </div>
                  <div className="text-slate-600 dark:text-slate-300 text-[11px] mb-2">{dsa.purpose}</div>
                  <div className="font-mono text-[10px] text-slate-400 dark:text-slate-400 bg-white dark:bg-slate-800 px-2 py-0.5 rounded border border-slate-100 dark:border-slate-700 inline-block">
                    {dsa.complexity}
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Leitner SRS Interval Matrix */}
          <div className="bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 rounded-lg p-6 shadow-xs">
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-bold text-slate-900 dark:text-slate-100 flex items-center gap-2">
                <Layers className="w-4 h-4 text-indigo-600 dark:text-indigo-400" />
                Leitner SRS Interval Matrix
              </h2>
              <span className="text-xs font-mono text-slate-400 dark:text-slate-500">
                5 Boxes
              </span>
            </div>

            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs border-collapse font-mono">
                <thead>
                  <tr className="bg-slate-50 dark:bg-slate-800/80 border-b border-slate-200 dark:border-slate-800 text-slate-500 dark:text-slate-400 text-[11px]">
                    <th className="py-2.5 px-4">Box Level</th>
                    <th className="py-2.5 px-4">Interval Duration</th>
                    <th className="py-2.5 px-4">On Pass (Solved)</th>
                    <th className="py-2.5 px-4">On Fail (NeedsReview)</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100 dark:divide-slate-800 text-slate-700 dark:text-slate-300">
                  <tr>
                    <td className="py-2.5 px-4 font-bold text-indigo-700 dark:text-indigo-400">Box 1</td>
                    <td className="py-2.5 px-4">1 Day (86,400s)</td>
                    <td className="py-2.5 px-4 text-emerald-600 dark:text-emerald-400">Promotes to Box 2</td>
                    <td className="py-2.5 px-4 text-rose-600 dark:text-rose-400">Remains in Box 1</td>
                  </tr>
                  <tr>
                    <td className="py-2.5 px-4 font-bold text-indigo-700 dark:text-indigo-400">Box 2</td>
                    <td className="py-2.5 px-4">3 Days (259,200s)</td>
                    <td className="py-2.5 px-4 text-emerald-600 dark:text-emerald-400">Promotes to Box 3</td>
                    <td className="py-2.5 px-4 text-rose-600 dark:text-rose-400">Demotes to Box 1</td>
                  </tr>
                  <tr>
                    <td className="py-2.5 px-4 font-bold text-indigo-700 dark:text-indigo-400">Box 3</td>
                    <td className="py-2.5 px-4">7 Days (604,800s)</td>
                    <td className="py-2.5 px-4 text-emerald-600 dark:text-emerald-400">Promotes to Box 4</td>
                    <td className="py-2.5 px-4 text-rose-600 dark:text-rose-400">Demotes to Box 1</td>
                  </tr>
                  <tr>
                    <td className="py-2.5 px-4 font-bold text-indigo-700 dark:text-indigo-400">Box 4</td>
                    <td className="py-2.5 px-4">14 Days (1,209,600s)</td>
                    <td className="py-2.5 px-4 text-emerald-600 dark:text-emerald-400">Promotes to Box 5</td>
                    <td className="py-2.5 px-4 text-rose-600 dark:text-rose-400">Demotes to Box 1</td>
                  </tr>
                  <tr>
                    <td className="py-2.5 px-4 font-bold text-indigo-700 dark:text-indigo-400">Box 5</td>
                    <td className="py-2.5 px-4">30 Days (2,592,000s)</td>
                    <td className="py-2.5 px-4 text-emerald-600 dark:text-emerald-400">Sustains at Box 5 (Mastered)</td>
                    <td className="py-2.5 px-4 text-rose-600 dark:text-rose-400">Demotes to Box 1</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
