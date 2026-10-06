import React from 'react';
import { Difficulty, Status, Topic, Platform } from '../types';

export const DifficultyBadge: React.FC<{ difficulty: Difficulty; className?: string }> = ({ difficulty, className = '' }) => {
  switch (difficulty) {
    case 'Easy':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-semibold bg-emerald-50 dark:bg-emerald-950/60 text-emerald-700 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 mr-1.5"></span>
          Easy
        </span>
      );
    case 'Medium':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-semibold bg-amber-50 dark:bg-amber-950/60 text-amber-800 dark:text-amber-300 border border-amber-200 dark:border-amber-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-amber-500 mr-1.5"></span>
          Medium
        </span>
      );
    case 'Hard':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-semibold bg-rose-50 dark:bg-rose-950/60 text-rose-700 dark:text-rose-300 border border-rose-200 dark:border-rose-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-rose-500 mr-1.5"></span>
          Hard
        </span>
      );
    default:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-200 dark:border-slate-700 ${className}`}>
          Unknown
        </span>
      );
  }
};

export const StatusBadge: React.FC<{ status: Status; className?: string }> = ({ status, className = '' }) => {
  switch (status) {
    case 'Solved':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-md text-xs font-medium bg-emerald-50 dark:bg-emerald-950/60 text-emerald-700 dark:text-emerald-300 border border-emerald-200 dark:border-emerald-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 mr-1.5"></span>
          Solved
        </span>
      );
    case 'Mastered':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-md text-xs font-medium bg-blue-50 dark:bg-blue-950/60 text-blue-700 dark:text-blue-300 border border-blue-200 dark:border-blue-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-blue-500 mr-1.5"></span>
          Mastered
        </span>
      );
    case 'InProgress':
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-md text-xs font-medium bg-amber-50 dark:bg-amber-950/60 text-amber-800 dark:text-amber-300 border border-amber-200 dark:border-amber-800 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-amber-500 mr-1.5"></span>
          In Progress
        </span>
      );
    case 'Unsolved':
    default:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded-md text-xs font-medium bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-200 dark:border-slate-700 ${className}`}>
          <span className="w-1.5 h-1.5 rounded-full bg-slate-400 mr-1.5"></span>
          Unsolved
        </span>
      );
  }
};

export const TopicBadge: React.FC<{ topic: Topic; className?: string }> = ({ topic, className = '' }) => {
  const formatTopic = (t: string) => {
    switch (t) {
      case 'LinkedLists': return 'Linked Lists';
      case 'StacksQueues': return 'Stacks & Queues';
      case 'DynamicProgramming': return 'Dynamic Programming';
      case 'BinarySearch': return 'Binary Search';
      case 'RecursionBacktracking': return 'Recursion & Backtracking';
      case 'BitManipulation': return 'Bit Manipulation';
      case 'MathGeometry': return 'Math & Geometry';
      default: return t;
    }
  };

  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-medium bg-slate-100 dark:bg-slate-800 text-slate-700 dark:text-slate-300 border border-slate-200/80 dark:border-slate-700 ${className}`}>
      {formatTopic(topic)}
    </span>
  );
};

export const PlatformBadge: React.FC<{ platform: Platform; className?: string }> = ({ platform, className = '' }) => {
  return (
    <span className={`inline-flex items-center px-1.5 py-0.5 rounded text-2xs font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-200 dark:border-slate-700 ${className}`}>
      {platform}
    </span>
  );
};

export const PriorityBadge: React.FC<{ priority: number; className?: string }> = ({ priority, className = '' }) => {
  switch (priority) {
    case 1:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-semibold bg-rose-50 dark:bg-rose-950/60 text-rose-700 dark:text-rose-300 border border-rose-200 dark:border-rose-800 ${className}`}>
          P1 Urgent
        </span>
      );
    case 2:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-semibold bg-amber-50 dark:bg-amber-950/60 text-amber-800 dark:text-amber-300 border border-amber-200 dark:border-amber-800 ${className}`}>
          P2 High
        </span>
      );
    case 3:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-semibold bg-blue-50 dark:bg-blue-950/60 text-blue-700 dark:text-blue-300 border border-blue-200 dark:border-blue-800 ${className}`}>
          P3 Normal
        </span>
      );
    case 4:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-medium bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-200 dark:border-slate-700 ${className}`}>
          P4 Low
        </span>
      );
    default:
      return (
        <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-medium bg-slate-50 dark:bg-slate-850 text-slate-500 dark:text-slate-400 border border-slate-200 dark:border-slate-700 ${className}`}>
          P5 Minimal
        </span>
      );
  }
};
