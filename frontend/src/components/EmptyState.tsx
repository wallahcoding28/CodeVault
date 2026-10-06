import React from 'react';
import { LucideIcon, FolderSearch } from 'lucide-react';

interface EmptyStateProps {
  icon?: LucideIcon;
  title: string;
  description: string;
  actionText?: string;
  actionLabel?: string;
  onAction?: () => void;
  secondaryActionLabel?: string;
  onSecondaryAction?: () => void;
}

export const EmptyState: React.FC<EmptyStateProps> = ({
  icon: Icon = FolderSearch,
  title,
  description,
  actionText,
  actionLabel,
  onAction,
  secondaryActionLabel,
  onSecondaryAction,
}) => {
  const btnLabel = actionLabel || actionText;

  return (
    <div className="border-2 border-dashed border-slate-200 dark:border-slate-800 rounded-xl p-12 text-center bg-white/50 dark:bg-slate-900/50 my-6 transition-colors">
      <div className="w-12 h-12 mx-auto rounded-full bg-slate-100 dark:bg-slate-800 flex items-center justify-center text-slate-500 dark:text-slate-400 mb-4">
        <Icon className="w-6 h-6" />
      </div>
      <h3 className="text-base font-semibold text-slate-900 dark:text-slate-100 mb-1">{title}</h3>
      <p className="text-sm text-slate-500 dark:text-slate-400 max-w-sm mx-auto mb-6">{description}</p>
      <div className="flex flex-wrap items-center justify-center gap-3">
        {btnLabel && onAction && (
          <button
            onClick={onAction}
            className="inline-flex items-center gap-2 px-4 py-2 rounded-lg text-xs font-semibold bg-slate-900 dark:bg-indigo-600 text-white hover:bg-slate-800 dark:hover:bg-indigo-500 transition-colors shadow-xs cursor-pointer"
          >
            {btnLabel}
          </button>
        )}
        {secondaryActionLabel && onSecondaryAction && (
          <button
            onClick={onSecondaryAction}
            className="inline-flex items-center gap-2 px-4 py-2 rounded-lg text-xs font-semibold bg-white dark:bg-slate-800 border border-slate-300 dark:border-slate-700 text-slate-700 dark:text-slate-200 hover:bg-slate-50 dark:hover:bg-slate-700 transition-colors shadow-2xs cursor-pointer"
          >
            {secondaryActionLabel}
          </button>
        )}
      </div>
    </div>
  );
};
