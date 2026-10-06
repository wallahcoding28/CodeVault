import React from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { AuthProvider } from './context/AuthContext';
import { ThemeProvider } from './context/ThemeContext';
import { ProtectedRoute } from './components/ProtectedRoute';
import { AppLayout } from './layouts/AppLayout';
import { LoginPage } from './pages/LoginPage';
import { RegisterPage } from './pages/RegisterPage';
import { DashboardPage } from './pages/DashboardPage';
import { QuestionsPage } from './pages/QuestionsPage';
import { QuestionDetailPage } from './pages/QuestionDetailPage';
import { QuestionFormPage } from './pages/QuestionFormPage';
import { PracticePage } from './pages/PracticePage';
import { RevisionPage } from './pages/RevisionPage';
import { StatisticsPage } from './pages/StatisticsPage';
import { SettingsPage } from './pages/SettingsPage';

export const App: React.FC = () => {
  return (
    <BrowserRouter>
      <ThemeProvider>
        <AuthProvider>
          <Routes>
            {/* Public Auth Routes */}
            <Route path="/login" element={<LoginPage />} />
            <Route path="/register" element={<RegisterPage />} />

            {/* Protected Application Routes */}
            <Route
              path="/"
              element={
                <ProtectedRoute>
                  <AppLayout />
                </ProtectedRoute>
              }
            >
              {/* Authenticated landing page defaults to Dashboard */}
              <Route index element={<Navigate to="/dashboard" replace />} />
              <Route path="problems" element={<QuestionsPage />} />
              <Route path="problems/new" element={<QuestionFormPage />} />
              <Route path="problems/:id" element={<QuestionDetailPage />} />
              <Route path="problems/:id/edit" element={<QuestionFormPage />} />

              {/* Backward compatibility aliases */}
              <Route path="questions" element={<Navigate to="/problems" replace />} />
              <Route path="questions/new" element={<Navigate to="/problems/new" replace />} />
              <Route path="questions/:id" element={<QuestionDetailPage />} />
              <Route path="questions/:id/edit" element={<QuestionFormPage />} />

              {/* Workflow Pages */}
              <Route path="practice" element={<PracticePage />} />
              <Route path="revision" element={<RevisionPage />} />
              <Route path="dashboard" element={<DashboardPage />} />
              <Route path="statistics" element={<StatisticsPage />} />
              <Route path="settings" element={<SettingsPage />} />

              {/* Fallback */}
              <Route path="*" element={<Navigate to="/dashboard" replace />} />
            </Route>
          </Routes>
        </AuthProvider>
      </ThemeProvider>
    </BrowserRouter>
  );
};

export default App;
