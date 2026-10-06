import React, { useState, useEffect, useRef } from 'react';
import { NavLink, Outlet, useLocation, useNavigate } from 'react-router-dom';
import {
  Code2,
  PlaySquare,
  RotateCw,
  LayoutDashboard,
  BarChart3,
  Settings,
  Plus,
  Search,
  CheckCircle2,
  XCircle,
  Terminal,
  LogOut,
  User as UserIcon,
  ChevronDown,
  Sun,
  Moon,
  Laptop,
} from 'lucide-react';
import { api } from '../services/api';
import { useAuth } from '../context/AuthContext';
import { useTheme } from '../context/ThemeContext';

export const AppLayout: React.FC = () => {
  const { user, logout } = useAuth();
  const { theme, effectiveTheme, setTheme } = useTheme();
  const location = useLocation();
  const navigate = useNavigate();
  const [totalQuestions, setTotalQuestions] = useState<number | null>(null);
  const [dueRevisions, setDueRevisions] = useState<number | null>(null);
  const [practiceQueueCount, setPracticeQueueCount] = useState<number | null>(null);
  const [serverOnline, setServerOnline] = useState<boolean>(true);
  const [searchQuery, setSearchQuery] = useState('');
  const [accountMenuOpen, setAccountMenuOpen] = useState(false);
  const menuRef = useRef<HTMLDivElement>(null);

  // Close account menu on Escape key or click outside
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && accountMenuOpen) {
        setAccountMenuOpen(false);
      }
    };
    const handleClickOutside = (e: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
        setAccountMenuOpen(false);
      }
    };
    if (accountMenuOpen) {
      document.addEventListener('keydown', handleKeyDown);
      document.addEventListener('mousedown', handleClickOutside);
    }
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      document.removeEventListener('mousedown', handleClickOutside);
    };
  }, [accountMenuOpen]);

  const fetchNavBadges = async () => {
    try {
      const [snap, qRes] = await Promise.all([
        api.getDashboard(),
        api.getPracticeQueue().catch(() => ({ queue: [], count: 0 })),
      ]);
      setTotalQuestions(snap.overall.totalQuestions);
      setDueRevisions(snap.revision.dueCount);
      setPracticeQueueCount(qRes.count);
      setServerOnline(true);
    } catch {
      setServerOnline(false);
    }
  };

  useEffect(() => {
    fetchNavBadges();
    const interval = setInterval(fetchNavBadges, 10000);
    return () => clearInterval(interval);
  }, []);

  const handleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (searchQuery.trim()) {
      navigate(`/problems?keyword=${encodeURIComponent(searchQuery.trim())}`);
    }
  };

  // Compute Breadcrumb according to coding-platform workflow
  const getBreadcrumb = () => {
    const p = location.pathname;
    if (p === '/problems' || p === '/questions') return 'Problems / Problem Library';
    if (p === '/problems/new' || p === '/questions/new') return 'Problems / New Problem';
    if (p.startsWith('/problems/') || p.startsWith('/questions/')) return 'Problems / Workspace';
    if (p === '/practice') return 'Practice / Focused Drill';
    if (p === '/revision') return 'Revision / Leitner 5-Box SRS';
    if (p === '/dashboard' || p === '/') return 'Dashboard / Daily Overview';
    if (p === '/statistics') return 'Statistics / Curriculum Progress';
    if (p === '/settings') return 'Settings / Engine & Persistence';
    return 'CodeVault';
  };

  return (
    <div className="flex h-screen bg-slate-50 dark:bg-slate-950 text-slate-900 dark:text-slate-100 overflow-hidden font-sans transition-colors">
      {/* Sidebar Navigation */}
      <aside className="w-60 flex flex-col bg-white dark:bg-slate-900 border-r border-slate-200/90 dark:border-slate-800 z-20 shrink-0">
        {/* Brand Header */}
        <div className="h-14 flex items-center px-5 border-b border-slate-200/80 dark:border-slate-800 gap-2.5">
          <div className="w-7 h-7 rounded-md bg-slate-900 dark:bg-indigo-600 text-white flex items-center justify-center font-mono font-bold text-xs shadow-xs">
            CV
          </div>
          <div>
            <div className="font-bold text-sm tracking-tight text-slate-900 dark:text-slate-100 leading-tight">CodeVault</div>
            <div className="text-[10px] text-slate-500 dark:text-slate-400 font-mono tracking-tight leading-none mt-0.5">Coding Practice & SRS</div>
          </div>
        </div>

        {/* Navigation Links — Primary Workflow: Problems -> Practice -> Revision -> Dashboard -> Statistics */}
        <nav className="flex-1 px-3 py-3 space-y-1 overflow-y-auto">
          {/* 1. Problems (Primary Centerpiece) */}
          <NavLink
            to="/problems"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive || location.pathname === '/questions'
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <Code2 className="w-4 h-4 shrink-0" />
            <span className="flex-1">Problems</span>
            {totalQuestions !== null && (
              <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 group-hover:bg-slate-200">
                {totalQuestions}
              </span>
            )}
          </NavLink>

          {/* 2. Practice */}
          <NavLink
            to="/practice"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <PlaySquare className="w-4 h-4 shrink-0" />
            <span className="flex-1">Practice</span>
            {practiceQueueCount !== null && practiceQueueCount > 0 && (
              <span className="text-[10px] font-mono font-bold px-1.5 py-0.2 rounded bg-indigo-100 dark:bg-indigo-950/80 text-indigo-800 dark:text-indigo-300 border border-indigo-200 dark:border-indigo-800">
                {practiceQueueCount}
              </span>
            )}
          </NavLink>

          {/* 3. Revision */}
          <NavLink
            to="/revision"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <RotateCw className="w-4 h-4 shrink-0" />
            <span className="flex-1">Revision</span>
            {dueRevisions !== null && dueRevisions > 0 && (
              <span className="text-[10px] font-mono font-bold px-1.5 py-0.2 rounded bg-rose-100 dark:bg-rose-950/80 text-rose-800 dark:text-rose-300 border border-rose-200 dark:border-rose-800">
                {dueRevisions}
              </span>
            )}
          </NavLink>

          {/* 4. Dashboard */}
          <NavLink
            to="/dashboard"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <LayoutDashboard className="w-4 h-4 shrink-0" />
            <span className="flex-1">Dashboard</span>
          </NavLink>

          {/* 5. Statistics */}
          <NavLink
            to="/statistics"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <BarChart3 className="w-4 h-4 shrink-0" />
            <span className="flex-1">Statistics</span>
          </NavLink>

          <div className="pt-3 pb-1 px-3">
            <span className="text-[10px] font-mono font-semibold text-slate-400 dark:text-slate-500 uppercase tracking-wider">System</span>
          </div>

          {/* 6. Settings */}
          <NavLink
            to="/settings"
            className={({ isActive }) =>
              `flex items-center gap-2.5 px-3 py-2 rounded-md text-xs font-semibold transition-colors ${
                isActive
                  ? 'bg-slate-900 dark:bg-indigo-600 text-white shadow-xs'
                  : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-100 hover:bg-slate-100/80 dark:hover:bg-slate-800/80'
              }`
            }
          >
            <Settings className="w-4 h-4 shrink-0" />
            <span className="flex-1">Settings</span>
          </NavLink>
        </nav>

        {/* Sidebar Footer — Real System Telemetry, No Fake Data */}
        <div className="p-3 border-t border-slate-200/80 dark:border-slate-800 space-y-2.5">
          <button
            onClick={() => navigate('/practice')}
            className="w-full flex items-center justify-center gap-2 py-2 px-3 rounded-md text-xs font-semibold bg-slate-900 dark:bg-indigo-600 text-white hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs"
          >
            <PlaySquare className="w-3.5 h-3.5" />
            Practice Session
          </button>

          {/* Backend Status & Engine Info */}
          <div className="flex items-center justify-between px-2 pt-1 text-[11px] text-slate-500 dark:text-slate-400 font-mono">
            <div className="flex items-center gap-1.5 truncate">
              <Terminal className="w-3 h-3 text-slate-400 shrink-0" />
              <span className="truncate">C++17 Engine</span>
            </div>
            <div className="flex items-center gap-1 shrink-0" title={serverOnline ? 'C++ REST Gateway Online (127.0.0.1:8080)' : 'C++ Backend Offline'}>
              {serverOnline ? (
                <span className="flex items-center gap-1 text-emerald-600 dark:text-emerald-400 text-[10px]">
                  <CheckCircle2 className="w-3 h-3 text-emerald-500" />
                  <span>8080</span>
                </span>
              ) : (
                <span className="flex items-center gap-1 text-rose-600 dark:text-rose-400 text-[10px]">
                  <XCircle className="w-3 h-3 text-rose-500" />
                  <span>Down</span>
                </span>
              )}
            </div>
          </div>
        </div>
      </aside>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col min-w-0 overflow-hidden">
        {/* Top Header Bar */}
        <header className="h-14 flex items-center justify-between px-6 bg-white dark:bg-slate-900 border-b border-slate-200/90 dark:border-slate-800 z-10 shrink-0 transition-colors">
          {/* Breadcrumb Path */}
          <div className="flex items-center gap-2 text-xs font-semibold text-slate-600 dark:text-slate-400">
            <span>{getBreadcrumb()}</span>
          </div>

          {/* Search & Actions */}
          <div className="flex items-center gap-2.5">
            <form onSubmit={handleSearchSubmit} className="relative">
              <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-slate-400" />
              <input
                type="text"
                placeholder="Search problems... (Press Enter)"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="w-64 pl-8 pr-3 py-1.5 bg-slate-50 dark:bg-slate-800 border border-slate-200 dark:border-slate-700 rounded-md text-xs text-slate-800 dark:text-slate-200 placeholder-slate-400 dark:placeholder-slate-500 focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 focus:bg-white dark:focus:bg-slate-800 transition-all font-mono"
              />
            </form>

            <button
              onClick={() => navigate('/problems/new')}
              className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-semibold bg-white dark:bg-slate-800 border border-slate-200 dark:border-slate-700 text-slate-700 dark:text-slate-300 hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs"
            >
              <Plus className="w-3.5 h-3.5" />
              New Problem
            </button>

            <button
              onClick={() => navigate('/practice')}
              className="inline-flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-semibold bg-slate-900 dark:bg-indigo-600 text-white hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs"
            >
              <PlaySquare className="w-3.5 h-3.5" />
              Practice Drill
            </button>

            {/* Clickable User Account Menu */}
            {user && (
              <div ref={menuRef} className="relative pl-3 ml-1 border-l border-slate-200 dark:border-slate-800">
                <button
                  type="button"
                  id="user-menu-button"
                  aria-haspopup="menu"
                  aria-expanded={accountMenuOpen}
                  aria-label="User account menu"
                  onClick={() => setAccountMenuOpen((prev) => !prev)}
                  className="flex items-center gap-2 px-2.5 py-1.5 rounded-md bg-slate-100 hover:bg-slate-200/80 dark:bg-slate-800 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 text-xs font-mono transition-colors focus:outline-none focus:ring-1 focus:ring-slate-900 dark:focus:ring-indigo-500 cursor-pointer"
                >
                  <div className="w-5 h-5 rounded-full bg-slate-900 dark:bg-indigo-600 text-white flex items-center justify-center font-bold text-[10px]">
                    {(user.displayName || user.username).charAt(0).toUpperCase()}
                  </div>
                  <span className="font-semibold max-w-[120px] truncate">
                    {user.displayName || user.username}
                  </span>
                  <ChevronDown className={`w-3.5 h-3.5 text-slate-500 transition-transform ${accountMenuOpen ? 'rotate-180' : ''}`} />
                </button>

                {accountMenuOpen && (
                  <div
                    role="menu"
                    aria-orientation="vertical"
                    aria-labelledby="user-menu-button"
                    className="absolute right-0 mt-2 w-64 rounded-lg bg-white dark:bg-slate-900 border border-slate-200 dark:border-slate-800 shadow-lg py-1.5 z-50 text-xs animate-in fade-in zoom-in-95 duration-100"
                  >
                    {/* User Header */}
                    <div className="px-3.5 py-2.5 border-b border-slate-100 dark:border-slate-800">
                      <div className="font-bold text-slate-900 dark:text-slate-100 truncate">
                        {user.displayName || user.username}
                      </div>
                      <div className="text-[11px] font-mono text-slate-500 dark:text-slate-400 truncate">
                        @{user.username}
                      </div>
                      {user.email && (
                        <div className="text-[11px] text-slate-400 dark:text-slate-500 truncate mt-0.5">
                          {user.email}
                        </div>
                      )}
                    </div>

                    {/* Nav Actions */}
                    <div className="py-1">
                      <button
                        role="menuitem"
                        onClick={() => {
                          setAccountMenuOpen(false);
                          navigate('/settings?tab=profile');
                        }}
                        className="w-full flex items-center gap-2.5 px-3.5 py-2 text-left text-slate-700 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-800 transition-colors cursor-pointer"
                      >
                        <UserIcon className="w-4 h-4 text-slate-500" />
                        <span>Profile</span>
                      </button>
                      <button
                        role="menuitem"
                        onClick={() => {
                          setAccountMenuOpen(false);
                          navigate('/settings');
                        }}
                        className="w-full flex items-center gap-2.5 px-3.5 py-2 text-left text-slate-700 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-800 transition-colors cursor-pointer"
                      >
                        <Settings className="w-4 h-4 text-slate-500" />
                        <span>Settings</span>
                      </button>
                    </div>

                    {/* Theme Selector */}
                    <div className="px-3.5 py-2 border-t border-slate-100 dark:border-slate-800">
                      <div className="text-[11px] font-semibold text-slate-500 dark:text-slate-400 mb-1.5 flex items-center gap-1.5">
                        {effectiveTheme === 'dark' ? (
                          <Moon className="w-3.5 h-3.5 text-indigo-400" />
                        ) : (
                          <Sun className="w-3.5 h-3.5 text-amber-500" />
                        )}
                        <span>Theme</span>
                      </div>
                      <div className="grid grid-cols-3 gap-1 bg-slate-100 dark:bg-slate-800 p-0.5 rounded-md">
                        <button
                          type="button"
                          onClick={() => setTheme('light')}
                          className={`flex items-center justify-center gap-1 py-1 rounded text-[11px] font-medium transition-colors cursor-pointer ${
                            theme === 'light'
                              ? 'bg-white dark:bg-slate-900 text-slate-900 dark:text-slate-100 shadow-xs'
                              : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
                          }`}
                        >
                          <Sun className="w-3 h-3" />
                          <span>Light</span>
                        </button>
                        <button
                          type="button"
                          onClick={() => setTheme('dark')}
                          className={`flex items-center justify-center gap-1 py-1 rounded text-[11px] font-medium transition-colors cursor-pointer ${
                            theme === 'dark'
                              ? 'bg-white dark:bg-slate-900 text-slate-900 dark:text-slate-100 shadow-xs'
                              : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
                          }`}
                        >
                          <Moon className="w-3 h-3" />
                          <span>Dark</span>
                        </button>
                        <button
                          type="button"
                          onClick={() => setTheme('system')}
                          className={`flex items-center justify-center gap-1 py-1 rounded text-[11px] font-medium transition-colors cursor-pointer ${
                            theme === 'system'
                              ? 'bg-white dark:bg-slate-900 text-slate-900 dark:text-slate-100 shadow-xs'
                              : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
                          }`}
                        >
                          <Laptop className="w-3 h-3" />
                          <span>System</span>
                        </button>
                      </div>
                    </div>

                    {/* Logout */}
                    <div className="pt-1 border-t border-slate-100 dark:border-slate-800">
                      <button
                        role="menuitem"
                        onClick={() => {
                          setAccountMenuOpen(false);
                          logout();
                        }}
                        className="w-full flex items-center gap-2.5 px-3.5 py-2 text-left text-rose-600 dark:text-rose-400 hover:bg-rose-50 dark:hover:bg-rose-950/40 transition-colors font-medium cursor-pointer"
                      >
                        <LogOut className="w-4 h-4" />
                        <span>Sign out</span>
                      </button>
                    </div>
                  </div>
                )}
              </div>
            )}
          </div>
        </header>

        {/* Nested Page Outlet */}
        <main className="flex-1 overflow-y-auto bg-slate-50 dark:bg-slate-950 transition-colors">
          <Outlet />
        </main>
      </div>
    </div>
  );
};
