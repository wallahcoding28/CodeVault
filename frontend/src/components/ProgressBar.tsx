import React from 'react';

interface ProgressBarProps {
  percentage?: number; // 0 to 100
  value?: number; // alias for percentage
  label?: string;
  sublabel?: string;
  color?: 'emerald' | 'amber' | 'rose' | 'blue' | 'slate';
  height?: 'sm' | 'md' | 'lg';
  showPercentage?: boolean;
}

export const ProgressBar: React.FC<ProgressBarProps> = ({
  percentage,
  value,
  label,
  sublabel,
  color = 'slate',
  height = 'md',
  showPercentage = true,
}) => {
  const actualVal = value !== undefined ? value : (percentage !== undefined ? percentage : 0);
  const clamped = Math.max(0, Math.min(100, isNaN(actualVal) ? 0 : actualVal));

  const getColorClass = () => {
    switch (color) {
      case 'emerald': return 'bg-emerald-500';
      case 'amber': return 'bg-amber-500';
      case 'rose': return 'bg-rose-500';
      case 'blue': return 'bg-blue-600';
      default: return 'bg-slate-900';
    }
  };

  const getHeightClass = () => {
    switch (height) {
      case 'sm': return 'h-1.5';
      case 'lg': return 'h-3';
      default: return 'h-2';
    }
  };

  return (
    <div className="w-full">
      {(label || showPercentage) && (
        <div className="flex justify-between items-center text-xs mb-1.5">
          <span className="font-medium text-slate-700 dark:text-slate-300">{label}</span>
          {showPercentage && (
            <span className="font-mono tabular-nums text-slate-600 dark:text-slate-400 font-semibold">
              {clamped.toFixed(1)}% {sublabel && <span className="text-slate-400 dark:text-slate-500 font-normal">({sublabel})</span>}
            </span>
          )}
        </div>
      )}
      <div className={`w-full bg-slate-100 dark:bg-slate-800 rounded-full overflow-hidden ${getHeightClass()} border border-slate-200/50 dark:border-slate-700`}>
        <div
          className={`${getHeightClass()} rounded-full transition-all duration-300 ${getColorClass()}`}
          style={{ width: `${clamped}%` }}
        />
      </div>
    </div>
  );
};
