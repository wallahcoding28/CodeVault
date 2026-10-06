import React from 'react';

interface StatCardProps {
  title: string;
  value: string | number;
  subtitle?: string;
  trend?: string;
  trendPositive?: boolean;
  icon?: React.ReactNode;
  accentColor?: 'emerald' | 'amber' | 'rose' | 'blue' | 'slate';
}

export const StatCard: React.FC<StatCardProps> = ({
  title,
  value,
  subtitle,
  trend,
  trendPositive = true,
  icon,
  accentColor = 'slate',
}) => {
  const getAccentBg = () => {
    switch (accentColor) {
      case 'emerald': return 'text-emerald-600 dark:text-emerald-400 bg-emerald-50 dark:bg-emerald-950/60';
      case 'amber': return 'text-amber-600 dark:text-amber-400 bg-amber-50 dark:bg-amber-950/60';
      case 'rose': return 'text-rose-600 dark:text-rose-400 bg-rose-50 dark:bg-rose-950/60';
      case 'blue': return 'text-blue-600 dark:text-blue-400 bg-blue-50 dark:bg-blue-950/60';
      default: return 'text-slate-700 dark:text-slate-300 bg-slate-100 dark:bg-slate-800';
    }
  };

  return (
    <div className="bg-white dark:bg-slate-900 border border-slate-200/90 dark:border-slate-800 rounded-xl p-5 shadow-xs transition-shadow hover:shadow-sm">
      <div className="flex items-center justify-between">
        <span className="text-xs font-semibold text-slate-500 dark:text-slate-400 uppercase tracking-wider">{title}</span>
        {icon && (
          <div className={`p-2 rounded-lg ${getAccentBg()}`}>
            {icon}
          </div>
        )}
      </div>
      <div className="mt-2 flex items-baseline gap-2">
        <span className="text-2xl font-bold font-mono tabular-nums text-slate-900 dark:text-slate-100 tracking-tight">{value}</span>
        {trend && (
          <span className={`text-xs font-medium px-1.5 py-0.5 rounded ${
            trendPositive ? 'bg-emerald-50 dark:bg-emerald-950/60 text-emerald-700 dark:text-emerald-300' : 'bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400'
          }`}>
            {trend}
          </span>
        )}
      </div>
      {subtitle && <p className="mt-1 text-xs text-slate-500 dark:text-slate-400">{subtitle}</p>}
    </div>
  );
};
