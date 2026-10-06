#pragma once

#include "models/user.hpp"
#include "services/auth_service.hpp"
#include "services/practice_service.hpp"
#include "services/question_service.hpp"
#include "services/recent_history_service.hpp"
#include "services/revision_service.hpp"
#include "services/search_service.hpp"
#include "services/statistics_service.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace codevault::app {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string getHeader(const std::string& name) const {
        for (const auto& [k, v] : headers) {
            if (models::iequals(k, name)) {
                return v;
            }
        }
        return "";
    }

    std::string getCookie(const std::string& name) const {
        std::string cookieHeader = getHeader("Cookie");
        if (cookieHeader.empty()) return "";
        std::istringstream stream(cookieHeader);
        std::string part;
        while (std::getline(stream, part, ';')) {
            while (!part.empty() && part.front() == ' ') part.erase(0, 1);
            size_t eq = part.find('=');
            if (eq != std::string::npos) {
                std::string k = part.substr(0, eq);
                std::string v = part.substr(eq + 1);
                if (k == name) return v;
            }
        }
        return "";
    }

    std::string getBearerToken() const {
        std::string authHeader = getHeader("Authorization");
        if (authHeader.rfind("Bearer ", 0) == 0) {
            return authHeader.substr(7);
        }
        return "";
    }
};

struct HttpResponse {
    int statusCode{200};
    std::string statusText{"OK"};
    std::string contentType{"application/json; charset=utf-8"};
    std::string body;
    std::vector<std::string> setCookies;
    std::map<std::string, std::string> customHeaders;

    static HttpResponse json(const std::string& body, int status = 200, const std::string& statusText = "OK") {
        HttpResponse res;
        res.statusCode = status;
        res.statusText = statusText;
        res.contentType = "application/json; charset=utf-8";
        res.body = body;
        return res;
    }

    static HttpResponse error(const std::string& message, int status = 400, const std::string& statusText = "Bad Request") {
        HttpResponse res;
        res.statusCode = status;
        res.statusText = statusText;
        res.contentType = "application/json; charset=utf-8";
        res.body = "{\"error\":\"" + message + "\"}";
        return res;
    }

    static HttpResponse unauthorized(const std::string& message = "Authentication required") {
        return error(message, 401, "Unauthorized");
    }

    static HttpResponse forbidden(const std::string& message = "Forbidden") {
        return error(message, 403, "Forbidden");
    }

    void setCookie(const std::string& name, const std::string& value, int maxAge = 604800, bool httpOnly = true, const std::string& sameSite = "Lax", bool secure = false) {
        std::string cookie = name + "=" + value + "; Path=/; Max-Age=" + std::to_string(maxAge) + "; SameSite=" + sameSite;
        if (httpOnly) cookie += "; HttpOnly";
        if (secure) cookie += "; Secure";
        setCookies.push_back(cookie);
    }

    void clearCookie(const std::string& name, const std::string& sameSite = "Lax") {
        std::string cookie = name + "=; Path=/; Max-Age=0; SameSite=" + sameSite + "; HttpOnly";
        setCookies.push_back(cookie);
    }
};

/**
 * @brief Zero-dependency C++17 HTTP/REST Server for CodeVault Web Integration.
 *
 * Implements the API contract with session authentication, cookie management,
 * and owner-isolated service dispatch.
 */
class HttpServer {
public:
    HttpServer(
        std::shared_ptr<services::QuestionService> questionService,
        std::shared_ptr<services::RecentHistoryService> recentHistoryService,
        std::shared_ptr<services::SearchService> searchService,
        std::shared_ptr<services::RevisionService> revisionService,
        std::shared_ptr<services::PracticeService> practiceService,
        std::shared_ptr<services::StatisticsService> statisticsService,
        int port = 8080);

    HttpServer(
        std::shared_ptr<services::QuestionService> questionService,
        std::shared_ptr<services::RecentHistoryService> recentHistoryService,
        std::shared_ptr<services::SearchService> searchService,
        std::shared_ptr<services::RevisionService> revisionService,
        std::shared_ptr<services::PracticeService> practiceService,
        std::shared_ptr<services::StatisticsService> statisticsService,
        std::shared_ptr<services::AuthService> authService,
        int port = 8080);

    ~HttpServer();

    bool start();
    void run();
    void stop();

    bool isRunning() const noexcept { return isRunning_.load(); }
    int getPort() const noexcept { return port_; }

    void setAuthService(std::shared_ptr<services::AuthService> authService) {
        authService_ = std::move(authService);
    }

    std::shared_ptr<services::AuthService> getAuthService() const {
        return authService_;
    }

    // Internal request dispatcher (public for unit testing routes directly)
    HttpResponse handleRequest(const HttpRequest& req);

private:
    std::shared_ptr<services::QuestionService> questionService_;
    std::shared_ptr<services::RecentHistoryService> recentHistoryService_;
    std::shared_ptr<services::SearchService> searchService_;
    std::shared_ptr<services::RevisionService> revisionService_;
    std::shared_ptr<services::PracticeService> practiceService_;
    std::shared_ptr<services::StatisticsService> statisticsService_;
    std::shared_ptr<services::AuthService> authService_;
    int port_{8080};
    std::atomic<bool> isRunning_{false};
    std::thread workerThread_;

    // Low-level socket handle
    uintptr_t serverSocket_{~static_cast<uintptr_t>(0)};

    // Auth route handlers
    HttpResponse handleRegister(const HttpRequest& req);
    HttpResponse handleLogin(const HttpRequest& req);
    HttpResponse handleLogout(const HttpRequest& req);
    HttpResponse handleGetMe(const HttpRequest& req);

    // Stage 9.4: Profile, Security & Session Management handlers
    HttpResponse handleUpdateProfile(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleChangePassword(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleGetSessions(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleRevokeSession(const std::string& sessionId, const models::User& currentUser);
    HttpResponse handleRevokeOtherSessions(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleExportUserData(const HttpRequest& req);

    // Stage 9.5: Catalog Import & Advanced Export handlers
    HttpResponse handleImportCatalog(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleExportMarkdown(const HttpRequest& req, const models::User& currentUser);
    HttpResponse handleExportAnki(const HttpRequest& req, const models::User& currentUser);

    // Domain route handlers
    HttpResponse handleGetQuestions(const HttpRequest& req);
    HttpResponse handleGetQuestionById(const std::string& id);
    HttpResponse handleCreateQuestion(const HttpRequest& req);
    HttpResponse handleUpdateQuestion(const std::string& id, const HttpRequest& req);
    HttpResponse handleDeleteQuestion(const std::string& id);

    HttpResponse handleGetDashboard();
    HttpResponse handleGetDueRevision();
    HttpResponse handleGetUpcomingRevision();
    HttpResponse handleScheduleRevision(const HttpRequest& req);

    HttpResponse handleStartPractice(const HttpRequest& req);
    HttpResponse handleGetPracticeCurrent();
    HttpResponse handlePracticeVerdict(const HttpRequest& req);
    HttpResponse handlePracticeSkip();
    HttpResponse handleGetPracticeProgress();
    HttpResponse handleExitPractice();

    // Stage 10: Advanced Practice Workflow handlers
    HttpResponse handleGetPracticeNext(const HttpRequest& req);
    HttpResponse handleStartPracticeSession(const HttpRequest& req);
    HttpResponse handleGetPracticeQueue();
    HttpResponse handleRemoveFromPracticeQueue(const std::string& questionId);
    HttpResponse handleRecordPracticeResultForQuestion(const std::string& questionId, const HttpRequest& req);

    HttpResponse handleGetHistory();
    HttpResponse handleGetDiagnostics();

    // Socket client connection handler
    void handleClient(uintptr_t clientSocket);
};

} // namespace codevault::app
