import React from 'react';
import { Navigate, useLocation } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { Code2, Loader2 } from 'lucide-react';

export const ProtectedRoute: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const { isAuthenticated, isLoading } = useAuth();
  const location = useLocation();

  if (isLoading) {
    return (
      <div className="min-h-screen bg-slate-50 flex flex-col items-center justify-center p-4">
        <div className="flex items-center gap-2 mb-4">
          <div className="w-8 h-8 rounded-lg bg-slate-900 flex items-center justify-center text-white">
            <Code2 className="w-5 h-5 text-indigo-400" />
          </div>
          <span className="font-bold text-slate-900 text-lg tracking-tight">CodeVault</span>
        </div>
        <div className="flex items-center gap-2 text-xs font-mono text-slate-500">
          <Loader2 className="w-4 h-4 animate-spin text-slate-600" />
          <span>Verifying session credentials...</span>
        </div>
      </div>
    );
  }

  if (!isAuthenticated) {
    return <Navigate to="/login" state={{ from: location }} replace />;
  }

  return <>{children}</>;
};
