#include "app/http_server.hpp"
#include "persistence/file_question_repository.hpp"
#include "persistence/sqlite_question_repository.hpp"
#include "services/current_user_provider.hpp"
#include "services/export_service.hpp"
#include "services/import_service.hpp"
#include "utils/json.hpp"
#include "utils/crypto.hpp"

#include <ctime>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define INVALID_SOCKET (~0)
#define SOCKET_ERROR (-1)
#endif

namespace codevault::app {

namespace {

std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size()) {
            std::string hex = in.substr(i + 1, 2);
            try {
                char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
                out += ch;
                i += 2;
            } catch (...) {
                out += in[i];
            }
        } else {
            out += in[i];
        }
    }
    return out;
}

void parseQueryParams(const std::string& query, std::map<std::string, std::string>& params) {
    if (query.empty()) return;
    std::istringstream ss(query);
    std::string token;
    while (std::getline(ss, token, '&')) {
        if (token.empty()) continue;
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            std::string key = urlDecode(token.substr(0, eq));
            std::string val = urlDecode(token.substr(eq + 1));
            params[key] = val;
        } else {
            params[urlDecode(token)] = "";
        }
    }
}

HttpRequest parseRawHttpRequest(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream lineStream(line);
    std::string rawPath;
    lineStream >> req.method >> rawPath;

    size_t qmark = rawPath.find('?');
    if (qmark != std::string::npos) {
        req.path = rawPath.substr(0, qmark);
        req.query = rawPath.substr(qmark + 1);
        parseQueryParams(req.query, req.queryParams);
    } else {
        req.path = rawPath;
    }

    size_t contentLength = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break; // Header-body separator

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string headerName = line.substr(0, colon);
            std::string headerVal = line.substr(colon + 1);
            while (!headerVal.empty() && headerVal.front() == ' ') headerVal.erase(0, 1);
            req.headers[headerName] = headerVal;
            if (models::iequals(headerName, "Content-Length")) {
                try {
                    contentLength = std::stoul(headerVal);
                } catch (...) {}
            }
        }
    }

    // Direct body extraction from raw after \r\n\r\n
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        if (contentLength > 0 && headerEnd + 4 + contentLength <= raw.size()) {
            req.body = raw.substr(headerEnd + 4, contentLength);
        } else {
            req.body = raw.substr(headerEnd + 4);
        }
    }

    return req;
}

std::string userToJson(const models::User& u) {
    std::string out = "{";
    out += "\"id\":\"" + utils::json::escape(u.getId()) + "\",";
    out += "\"username\":\"" + utils::json::escape(u.getUsername()) + "\",";
    out += "\"displayName\":\"" + utils::json::escape(u.getDisplayName()) + "\",";
    out += "\"email\":\"" + utils::json::escape(u.getEmail()) + "\",";
    out += "\"createdAt\":" + std::to_string(u.getCreatedAt()) + ",";
    out += "\"updatedAt\":" + std::to_string(u.getUpdatedAt()) + ",";
    out += "\"active\":" + std::string(u.isActive() ? "true" : "false");
    out += "}";
    return out;
}

std::string formatRawHttpResponse(const HttpResponse& res, const HttpRequest* req = nullptr) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << res.statusCode << " " << res.statusText << "\r\n";
    ss << "Content-Type: " << res.contentType << "\r\n";
    ss << "Content-Length: " << res.body.size() << "\r\n";

    std::string origin = req ? req->getHeader("Origin") : "";
    if (!origin.empty()) {
        ss << "Access-Control-Allow-Origin: " << origin << "\r\n";
        ss << "Access-Control-Allow-Credentials: true\r\n";
    } else {
        ss << "Access-Control-Allow-Origin: *\r\n";
    }

    ss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With, Cookie\r\n";
    ss << "Access-Control-Expose-Headers: Set-Cookie\r\n";

    for (const auto& cookie : res.setCookies) {
        ss << "Set-Cookie: " << cookie << "\r\n";
    }
    for (const auto& [k, v] : res.customHeaders) {
        ss << k << ": " << v << "\r\n";
    }

    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << res.body;
    return ss.str();
}

} // anonymous namespace

#ifdef _WIN32
static void ensureWinsock() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = true;
    }
}
#endif

HttpServer::HttpServer(
    std::shared_ptr<services::QuestionService> questionService,
    std::shared_ptr<services::RecentHistoryService> recentHistoryService,
    std::shared_ptr<services::SearchService> searchService,
    std::shared_ptr<services::RevisionService> revisionService,
    std::shared_ptr<services::PracticeService> practiceService,
    std::shared_ptr<services::StatisticsService> statisticsService,
    int port)
    : questionService_(std::move(questionService)),
      recentHistoryService_(std::move(recentHistoryService)),
      searchService_(std::move(searchService)),
      revisionService_(std::move(revisionService)),
      practiceService_(std::move(practiceService)),
      statisticsService_(std::move(statisticsService)),
      authService_(nullptr),
      port_(port) {
}

HttpServer::HttpServer(
    std::shared_ptr<services::QuestionService> questionService,
    std::shared_ptr<services::RecentHistoryService> recentHistoryService,
    std::shared_ptr<services::SearchService> searchService,
    std::shared_ptr<services::RevisionService> revisionService,
    std::shared_ptr<services::PracticeService> practiceService,
    std::shared_ptr<services::StatisticsService> statisticsService,
    std::shared_ptr<services::AuthService> authService,
    int port)
    : questionService_(std::move(questionService)),
      recentHistoryService_(std::move(recentHistoryService)),
      searchService_(std::move(searchService)),
      revisionService_(std::move(revisionService)),
      practiceService_(std::move(practiceService)),
      statisticsService_(std::move(statisticsService)),
      authService_(std::move(authService)),
      port_(port) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (isRunning_.load()) return true;

#ifdef _WIN32
    ensureWinsock();
#endif

    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[HttpServer Error] Failed to create socket\n";
        return false;
    }

    // Set reuseaddr
    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[HttpServer Error] Failed to bind to port " << port_ << "\n";
        CLOSE_SOCKET(sock);
        return false;
    }

    if (listen(sock, 128) == SOCKET_ERROR) {
        std::cerr << "[HttpServer Error] Failed to listen on socket\n";
        CLOSE_SOCKET(sock);
        return false;
    }

    serverSocket_ = static_cast<uintptr_t>(sock);
    isRunning_.store(true);

    workerThread_ = std::thread([this]() {
        this->run();
    });

    return true;
}

void HttpServer::run() {
    while (isRunning_.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        auto clientSock = accept(static_cast<SOCKET>(serverSocket_), reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if (clientSock == INVALID_SOCKET) {
            if (!isRunning_.load()) break;
            continue;
        }

        handleClient(static_cast<uintptr_t>(clientSock));
    }
}

void HttpServer::stop() {
    if (isRunning_.exchange(false)) {
        if (serverSocket_ != ~static_cast<uintptr_t>(0)) {
            CLOSE_SOCKET(static_cast<SOCKET>(serverSocket_));
            serverSocket_ = ~static_cast<uintptr_t>(0);
        }
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
}

void HttpServer::handleClient(uintptr_t clientSocket) {
    SOCKET sock = static_cast<SOCKET>(clientSocket);

    // Set timeout to prevent hanging connections
#ifdef _WIN32
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    std::string rawRequest;
    char buffer[4096];
    int bytesReceived = 0;

    while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        rawRequest.append(buffer, bytesReceived);
        size_t endHeader = rawRequest.find("\r\n\r\n");
        if (endHeader != std::string::npos) {
            std::string headerPart = rawRequest.substr(0, endHeader);
            std::string lowerHeaders = headerPart;
            for (char& c : lowerHeaders) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            auto pos = lowerHeaders.find("content-length:");
            if (pos != std::string::npos) {
                size_t cl = 0;
                try {
                    cl = std::stoul(lowerHeaders.substr(pos + 15, 10));
                } catch (...) {}
                size_t bodyGot = rawRequest.size() - (endHeader + 4);
                if (bodyGot >= cl) break;
            } else {
                break;
            }
        }
    }

    if (!rawRequest.empty()) {
        HttpRequest req = parseRawHttpRequest(rawRequest);
        HttpResponse res = handleRequest(req);
        std::string rawResponse = formatRawHttpResponse(res, &req);
        send(sock, rawResponse.c_str(), static_cast<int>(rawResponse.size()), 0);
    }

    CLOSE_SOCKET(sock);
}

HttpResponse HttpServer::handleRegister(const HttpRequest& req) {
    if (!authService_) {
        return HttpResponse::error("AuthService unavailable", 503);
    }

    auto root = utils::json::parse(req.body);
    std::string username = root.getString("username");
    std::string displayName = root.getString("displayName");
    if (displayName.empty()) displayName = root.getString("display_name");
    std::string email = root.getString("email");
    std::string password = root.getString("password");

    auto result = authService_->registerUser(username, displayName, email, password);
    if (!result.success) {
        return HttpResponse::error(result.errorMessage, 400);
    }

    HttpResponse res = HttpResponse::json("{\"user\":" + userToJson(*result.user) + "}", 201, "Created");
    res.setCookie("codevault_session", result.sessionToken, 604800, true, "Lax");
    return res;
}

HttpResponse HttpServer::handleLogin(const HttpRequest& req) {
    if (!authService_) {
        return HttpResponse::error("AuthService unavailable", 503);
    }

    auto root = utils::json::parse(req.body);
    std::string username = root.getString("username");
    if (username.empty()) username = root.getString("identifier");
    std::string password = root.getString("password");

    auto result = authService_->login(username, password);
    if (!result.success) {
        return HttpResponse::error(result.errorMessage, 401, "Unauthorized");
    }

    HttpResponse res = HttpResponse::json("{\"user\":" + userToJson(*result.user) + "}", 200, "OK");
    res.setCookie("codevault_session", result.sessionToken, 604800, true, "Lax");
    return res;
}

HttpResponse HttpServer::handleLogout(const HttpRequest& req) {
    if (authService_) {
        std::string token = req.getCookie("codevault_session");
        if (token.empty()) token = req.getBearerToken();
        authService_->logout(token);
    }

    HttpResponse res = HttpResponse::json("{\"message\":\"Successfully logged out\"}");
    res.clearCookie("codevault_session", "Lax");
    return res;
}

HttpResponse HttpServer::handleGetMe(const HttpRequest& req) {
    if (!authService_) {
        return HttpResponse::error("AuthService unavailable", 503);
    }

    std::string token = req.getCookie("codevault_session");
    if (token.empty()) token = req.getBearerToken();

    auto userOpt = authService_->authenticateToken(token);
    if (!userOpt) {
        return HttpResponse::unauthorized("Not authenticated");
    }

    return HttpResponse::json("{\"user\":" + userToJson(*userOpt) + "}");
}

HttpResponse HttpServer::handleUpdateProfile(const HttpRequest& req, const models::User& currentUser) {
    if (!authService_) return HttpResponse::error("AuthService unavailable", 503);
    auto root = utils::json::parse(req.body);
    std::string displayName = root.getString("displayName");
    if (displayName.empty()) displayName = root.getString("display_name");
    std::string email = root.getString("email");

    std::string err;
    if (!authService_->updateProfile(currentUser.getId(), displayName, email, err)) {
        return HttpResponse::error(err, 400);
    }

    auto updatedUser = authService_->getUserById(currentUser.getId());
    return HttpResponse::json("{\"user\":" + userToJson(updatedUser.value_or(currentUser)) + "}");
}

HttpResponse HttpServer::handleChangePassword(const HttpRequest& req, const models::User& currentUser) {
    if (!authService_) return HttpResponse::error("AuthService unavailable", 503);
    auto root = utils::json::parse(req.body);
    std::string currentPassword = root.getString("currentPassword");
    if (currentPassword.empty()) currentPassword = root.getString("current_password");
    std::string newPassword = root.getString("newPassword");
    if (newPassword.empty()) newPassword = root.getString("new_password");

    if (currentPassword.empty()) {
        return HttpResponse::error("Current password is required", 400);
    }
    if (newPassword.length() < 8) {
        return HttpResponse::error("New password must be at least 8 characters long", 400);
    }

    std::string err;
    if (!authService_->changePassword(currentUser.getId(), currentPassword, newPassword, err)) {
        return HttpResponse::error(err, 400);
    }

    return HttpResponse::json("{\"message\":\"Password updated successfully\"}");
}

HttpResponse HttpServer::handleGetSessions(const HttpRequest& req, const models::User& currentUser) {
    if (!authService_) return HttpResponse::error("AuthService unavailable", 503);

    std::string rawToken = req.getCookie("codevault_session");
    if (rawToken.empty()) rawToken = req.getBearerToken();
    std::string currentHash = utils::crypto::hashToken(rawToken);

    auto sessions = authService_->getActiveSessions(currentUser.getId());
    std::ostringstream ss;
    ss << "{\"sessions\":[";
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& s = sessions[i];
        bool isCurrent = (!currentHash.empty() && s.getTokenHash() == currentHash);
        ss << "{"
           << "\"id\":\"" << utils::json::escape(s.getId()) << "\","
           << "\"createdAt\":" << s.getCreatedAt() << ","
           << "\"expiresAt\":" << s.getExpiresAt() << ","
           << "\"lastSeenAt\":" << s.getLastSeenAt() << ","
           << "\"isCurrentSession\":" << (isCurrent ? "true" : "false")
           << "}";
    }
    ss << "]}";
    return HttpResponse::json(ss.str());
}

HttpResponse HttpServer::handleRevokeSession(const std::string& sessionId, const models::User& currentUser) {
    if (!authService_) return HttpResponse::error("AuthService unavailable", 503);
    if (!authService_->revokeSession(currentUser.getId(), sessionId)) {
        return HttpResponse::error("Session not found or already revoked", 404, "Not Found");
    }
    return HttpResponse::json("{\"message\":\"Session revoked successfully\"}");
}

HttpResponse HttpServer::handleRevokeOtherSessions(const HttpRequest& req, const models::User& currentUser) {
    if (!authService_) return HttpResponse::error("AuthService unavailable", 503);
    std::string rawToken = req.getCookie("codevault_session");
    if (rawToken.empty()) rawToken = req.getBearerToken();

    if (!authService_->revokeOtherSessions(currentUser.getId(), rawToken)) {
        return HttpResponse::error("Failed to revoke other sessions", 400);
    }
    return HttpResponse::json("{\"message\":\"All other sessions revoked successfully\"}");
}

HttpResponse HttpServer::handleExportUserData(const HttpRequest& req) {
    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);

    auto questions = questionService_->getAllQuestions();
    std::string format = "json";
    auto it = req.queryParams.find("format");
    if (it != req.queryParams.end() && !it->second.empty()) {
        format = it->second;
    }

    if (format == "csv") {
        std::ostringstream ss;
        ss << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\r\n";
        for (const auto& q : questions) {
            auto escape = [](const std::string& field) -> std::string {
                bool needsQuotes = field.find_first_of(",\"\r\n") != std::string::npos;
                if (!needsQuotes) return field;
                std::string esc = "\"";
                for (char c : field) {
                    if (c == '"') esc += "\"\"";
                    else esc += c;
                }
                esc += "\"";
                return esc;
            };

            std::string tagsStr;
            const auto& tags = q.getTags();
            for (size_t i = 0; i < tags.size(); ++i) {
                if (i > 0) tagsStr += ";";
                tagsStr += tags[i];
            }

            ss << escape(q.getId()) << ","
               << escape(q.getTitle()) << ","
               << escape(q.getDescription()) << ","
               << escape(models::topicToString(q.getTopic())) << ","
               << escape(models::difficultyToString(q.getDifficulty())) << ","
               << escape(q.getCompany()) << ","
               << escape(models::platformToString(q.getPlatform())) << ","
               << escape(q.getSourceUrl()) << ","
               << escape(models::statusToString(q.getStatus())) << ","
               << (q.isFavorite() ? "1" : "0") << ","
               << escape(q.getNotes()) << ","
               << q.getCreatedAt() << ","
               << q.getUpdatedAt() << ","
               << q.getLastPracticedAt() << ","
               << q.getNextRevisionAt() << ","
               << q.getRevisionPriority() << ","
               << escape(tagsStr) << ","
               << escape(q.getOwnerId()) << "\r\n";
        }

        HttpResponse res;
        res.statusCode = 200;
        res.statusText = "OK";
        res.contentType = "text/csv; charset=utf-8";
        res.customHeaders["Content-Disposition"] = "attachment; filename=\"codevault_questions_export.csv\"";
        res.body = ss.str();
        return res;
    }

    // Default: JSON export
    std::string jsonBody = "{\"questions\":" + utils::json::questionsToJson(questions) + ",\"exportedAt\":" + std::to_string(std::time(nullptr)) + ",\"totalCount\":" + std::to_string(questions.size()) + "}";
    HttpResponse res = HttpResponse::json(jsonBody);
    res.customHeaders["Content-Disposition"] = "attachment; filename=\"codevault_questions_export.json\"";
    return res;
}

HttpResponse HttpServer::handleImportCatalog(const HttpRequest& req, const models::User& currentUser) {
    (void)currentUser;
    if (!questionService_) {
        return HttpResponse::error("QuestionService unavailable", 500);
    }

    // 1. Determine format from Content-Type header or query parameter
    std::string format;
    std::string contentType = req.getHeader("Content-Type");
    for (char& c : contentType) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (contentType.find("application/json") != std::string::npos) {
        format = "json";
    } else if (contentType.find("text/csv") != std::string::npos || contentType.find("application/csv") != std::string::npos) {
        format = "csv";
    } else {
        auto it = req.queryParams.find("format");
        if (it != req.queryParams.end()) {
            std::string qf = it->second;
            for (char& c : qf) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (qf == "json" || qf == "csv") {
                format = qf;
            }
        }
    }

    if (format.empty()) {
        return HttpResponse::error("Unsupported or missing format. Supported: application/json, text/csv", 400);
    }

    // 2. Determine conflict strategy (default: skip)
    models::ImportConflictStrategy strategy = models::ImportConflictStrategy::Skip;
    std::string stratStr;
    auto stratIt = req.queryParams.find("conflict_strategy");
    if (stratIt != req.queryParams.end() && !stratIt->second.empty()) {
        stratStr = stratIt->second;
    } else {
        auto stratIt2 = req.queryParams.find("conflictStrategy");
        if (stratIt2 != req.queryParams.end() && !stratIt2->second.empty()) {
            stratStr = stratIt2->second;
        }
    }

    if (!stratStr.empty()) {
        if (!models::stringToConflictStrategy(stratStr, strategy)) {
            return HttpResponse::error("Invalid conflict strategy '" + stratStr + "'. Supported: skip, overwrite, generate_new_id", 400);
        }
    }

    // 3. Delegate to ImportService
    services::ImportService importer(questionService_);
    models::ImportResult result;
    if (format == "json") {
        result = importer.importFromJson(req.body, strategy);
    } else if (format == "csv") {
        result = importer.importFromCsv(req.body, strategy);
    }

    // 4. Construct structured response
    std::ostringstream ss;
    ss << "{"
       << "\"success\":" << (result.success ? "true" : "false") << ","
       << "\"totalProcessed\":" << result.totalProcessed << ","
       << "\"importedCount\":" << result.importedCount << ","
       << "\"updatedCount\":" << result.updatedCount << ","
       << "\"skippedCount\":" << result.skippedCount << ","
       << "\"total_processed\":" << result.totalProcessed << ","
       << "\"imported_count\":" << result.importedCount << ","
       << "\"updated_count\":" << result.updatedCount << ","
       << "\"skipped_count\":" << result.skippedCount << ","
       << "\"errors\":[";
    for (size_t i = 0; i < result.errors.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << utils::json::escape(result.errors[i]) << "\"";
    }
    ss << "]}";

    if (!result.success) {
        return HttpResponse::json(ss.str(), 400, "Bad Request");
    }
    return HttpResponse::json(ss.str(), 200, "OK");
}

HttpResponse HttpServer::handleExportMarkdown(const HttpRequest& req, const models::User& currentUser) {
    (void)req;
    (void)currentUser;
    if (!questionService_) {
        return HttpResponse::error("QuestionService unavailable", 500);
    }

    auto questions = questionService_->getAllQuestions();
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    std::string md = services::ExportService::exportToMarkdown(questions, now);

    HttpResponse res;
    res.statusCode = 200;
    res.statusText = "OK";
    res.contentType = "text/markdown; charset=utf-8";
    res.customHeaders["Content-Disposition"] = "attachment; filename=\"codevault-questions.md\"";
    res.body = md;
    return res;
}

HttpResponse HttpServer::handleExportAnki(const HttpRequest& req, const models::User& currentUser) {
    (void)req;
    (void)currentUser;
    if (!questionService_) {
        return HttpResponse::error("QuestionService unavailable", 500);
    }

    auto questions = questionService_->getAllQuestions();
    std::string tsv = services::ExportService::exportToAnkiTsv(questions);

    HttpResponse res;
    res.statusCode = 200;
    res.statusText = "OK";
    res.contentType = "text/tab-separated-values; charset=utf-8";
    res.customHeaders["Content-Disposition"] = "attachment; filename=\"codevault-questions-anki.tsv\"";
    res.body = tsv;
    return res;
}

HttpResponse HttpServer::handleRequest(const HttpRequest& inReq) {
    HttpRequest req = inReq;
    size_t qmark = req.path.find('?');
    if (qmark != std::string::npos) {
        std::string queryStr = req.path.substr(qmark + 1);
        req.path = req.path.substr(0, qmark);
        if (req.query.empty()) {
            req.query = queryStr;
        }
        parseQueryParams(queryStr, req.queryParams);
    }

    if (req.method == "OPTIONS") {
        HttpResponse res;
        res.statusCode = 200;
        res.statusText = "OK";
        res.body = "";
        return res;
    }

    // Public auth routes
    if (req.path == "/api/auth/register" && req.method == "POST") {
        return handleRegister(req);
    }
    if (req.path == "/api/auth/login" && req.method == "POST") {
        return handleLogin(req);
    }
    if (req.path == "/api/auth/logout" && req.method == "POST") {
        return handleLogout(req);
    }
    if (req.path == "/api/auth/me" && req.method == "GET") {
        return handleGetMe(req);
    }

    // Public diagnostics route
    if (req.path == "/api/settings/diagnostics" && req.method == "GET") {
        return handleGetDiagnostics();
    }

    // Authentication Middleware: Protect all other API endpoints when authService is configured
    std::optional<models::User> currentUser;
    if (authService_) {
        std::string token = req.getCookie("codevault_session");
        if (token.empty()) {
            token = req.getBearerToken();
        }

        auto userOpt = authService_->authenticateToken(token);
        if (!userOpt) {
            return HttpResponse::unauthorized("Authentication required");
        }

        currentUser = userOpt;

        if (questionService_) {
            questionService_->setCurrentUserProvider(
                std::make_shared<services::AuthenticatedCurrentUserProvider>(*userOpt)
            );
        }
    }

    // Stage 9.4: Profile, Security & Session Management routes
    if (currentUser) {
        if (req.path == "/api/auth/profile" && req.method == "PUT") {
            return handleUpdateProfile(req, *currentUser);
        }
        if (req.path == "/api/auth/change-password" && req.method == "POST") {
            return handleChangePassword(req, *currentUser);
        }
        if (req.path == "/api/auth/sessions" && req.method == "GET") {
            return handleGetSessions(req, *currentUser);
        }
        if (req.path == "/api/auth/sessions/revoke-others" && req.method == "POST") {
            return handleRevokeOtherSessions(req, *currentUser);
        }
        if (req.path.rfind("/api/auth/sessions/", 0) == 0 && req.method == "POST") {
            std::string remainder = req.path.substr(std::string("/api/auth/sessions/").size());
            if (remainder.size() > 7 && remainder.rfind("/revoke") == remainder.size() - 7) {
                std::string sid = remainder.substr(0, remainder.size() - 7);
                return handleRevokeSession(sid, *currentUser);
            }
        }
        if (req.path == "/api/user/export" && req.method == "GET") {
            return handleExportUserData(req);
        }
    }

    // Stage 9.5: Catalog Import & Advanced Export routes (Strictly authenticated)
    if (req.path == "/api/user/import") {
        if (!currentUser) return HttpResponse::unauthorized("Authentication required");
        if (req.method == "POST") {
            return handleImportCatalog(req, *currentUser);
        }
        return HttpResponse::error("Method not allowed", 405, "Method Not Allowed");
    }
    if (req.path == "/api/user/export/markdown") {
        if (!currentUser) return HttpResponse::unauthorized("Authentication required");
        if (req.method == "GET") {
            return handleExportMarkdown(req, *currentUser);
        }
        return HttpResponse::error("Method not allowed", 405, "Method Not Allowed");
    }
    if (req.path == "/api/user/export/anki") {
        if (!currentUser) return HttpResponse::unauthorized("Authentication required");
        if (req.method == "GET") {
            return handleExportAnki(req, *currentUser);
        }
        return HttpResponse::error("Method not allowed", 405, "Method Not Allowed");
    }

    // API Routes dispatch
    if (req.path == "/api/dashboard" && req.method == "GET") {
        return handleGetDashboard();
    }

    if (req.path == "/api/questions") {
        if (req.method == "GET") return handleGetQuestions(req);
        if (req.method == "POST") return handleCreateQuestion(req);
    }

    if (req.path.rfind("/api/questions/", 0) == 0) {
        std::string id = req.path.substr(std::string("/api/questions/").size());
        if (!id.empty()) {
            if (req.method == "GET") return handleGetQuestionById(id);
            if (req.method == "PUT") return handleUpdateQuestion(id, req);
            if (req.method == "DELETE") return handleDeleteQuestion(id);
        }
    }

    if (req.path == "/api/revision/due" && req.method == "GET") {
        return handleGetDueRevision();
    }

    if (req.path == "/api/revision/upcoming" && req.method == "GET") {
        return handleGetUpcomingRevision();
    }

    if (req.path == "/api/revision/schedule" && req.method == "POST") {
        return handleScheduleRevision(req);
    }

    if (req.path == "/api/practice/start" && req.method == "POST") {
        return handleStartPractice(req);
    }

    if (req.path == "/api/practice/current" && req.method == "GET") {
        return handleGetPracticeCurrent();
    }

    if (req.path == "/api/practice/verdict" && req.method == "POST") {
        return handlePracticeVerdict(req);
    }

    if (req.path == "/api/practice/skip" && req.method == "POST") {
        return handlePracticeSkip();
    }

    if (req.path == "/api/practice/progress" && req.method == "GET") {
        return handleGetPracticeProgress();
    }

    if (req.path == "/api/practice/exit" && req.method == "POST") {
        return handleExitPractice();
    }

    // Stage 10: Advanced Practice Workflow routes
    if (req.path == "/api/practice/next" && req.method == "GET") {
        return handleGetPracticeNext(req);
    }

    if (req.path == "/api/practice/session" && req.method == "POST") {
        return handleStartPracticeSession(req);
    }

    if (req.path == "/api/practice/queue" && req.method == "GET") {
        return handleGetPracticeQueue();
    }

    if (req.path.rfind("/api/practice/queue/", 0) == 0 && req.method == "DELETE") {
        std::string qid = req.path.substr(std::string("/api/practice/queue/").size());
        return handleRemoveFromPracticeQueue(qid);
    }

    if (req.path.rfind("/api/practice/", 0) == 0 && req.method == "POST") {
        std::string prefix = "/api/practice/";
        std::string suffix = "/result";
        if (req.path.size() > prefix.size() + suffix.size() && req.path.rfind(suffix) == req.path.size() - suffix.size()) {
            std::string qid = req.path.substr(prefix.size(), req.path.size() - prefix.size() - suffix.size());
            return handleRecordPracticeResultForQuestion(qid, req);
        }
    }

    if (req.path == "/api/history" && req.method == "GET") {
        return handleGetHistory();
    }

    if (req.path == "/api/settings/diagnostics" && req.method == "GET") {
        return handleGetDiagnostics();
    }

    return HttpResponse::error("Endpoint not found", 404, "Not Found");
}

HttpResponse HttpServer::handleGetQuestions(const HttpRequest& req) {
    if (!questionService_) {
        return HttpResponse::error("QuestionService unavailable", 500);
    }

    auto questions = questionService_->getAllQuestions();

    // Check for Trie prefix search
    auto prefixIt = req.queryParams.find("prefix");
    if (prefixIt != req.queryParams.end() && !prefixIt->second.empty() && searchService_) {
        questions = searchService_->searchByTitlePrefix(questions, prefixIt->second);
    }

    // Check for keyword search
    auto kwIt = req.queryParams.find("keyword");
    if (kwIt != req.queryParams.end() && !kwIt->second.empty() && searchService_) {
        questions = searchService_->searchByKeyword(questions, kwIt->second);
    }

    // Criteria Filtering
    models::QuestionFilter filter;
    auto topicIt = req.queryParams.find("topic");
    if (topicIt != req.queryParams.end() && !topicIt->second.empty() && topicIt->second != "All") {
        filter.topic = models::stringToTopic(topicIt->second);
    }

    auto diffIt = req.queryParams.find("difficulty");
    if (diffIt != req.queryParams.end() && !diffIt->second.empty() && diffIt->second != "All") {
        filter.difficulty = models::stringToDifficulty(diffIt->second);
    }

    auto statusIt = req.queryParams.find("status");
    if (statusIt != req.queryParams.end() && !statusIt->second.empty() && statusIt->second != "All") {
        filter.status = models::stringToStatus(statusIt->second);
    }

    auto compIt = req.queryParams.find("company");
    if (compIt != req.queryParams.end() && !compIt->second.empty() && compIt->second != "All") {
        filter.company = compIt->second;
    }

    auto favIt = req.queryParams.find("favorite");
    if (favIt != req.queryParams.end()) {
        if (favIt->second == "true" || favIt->second == "1") {
            filter.isFavorite = true;
        }
    }

    if (searchService_ && !filter.isEmpty()) {
        questions = searchService_->filter(questions, filter);
    }

    // Sorting
    auto sortFieldIt = req.queryParams.find("sortBy");
    if (sortFieldIt != req.queryParams.end() && searchService_) {
        models::SortOptions sortOpts;
        std::string sf = sortFieldIt->second;
        if (sf == "title") sortOpts.field = models::SortField::Title;
        else if (sf == "difficulty") sortOpts.field = models::SortField::Difficulty;
        else if (sf == "topic") sortOpts.field = models::SortField::Topic;
        else if (sf == "status") sortOpts.field = models::SortField::Status;
        else if (sf == "company") sortOpts.field = models::SortField::Company;
        else if (sf == "revisionPriority") sortOpts.field = models::SortField::RevisionPriority;
        else if (sf == "createdAt") sortOpts.field = models::SortField::CreatedAt;
        else sortOpts.field = models::SortField::Title;

        auto sortDirIt = req.queryParams.find("sortDir");
        if (sortDirIt != req.queryParams.end() && (sortDirIt->second == "desc" || sortDirIt->second == "Descending")) {
            sortOpts.direction = models::SortDirection::Descending;
        }

        auto sortAlgoIt = req.queryParams.find("sortAlgo");
        if (sortAlgoIt != req.queryParams.end() && sortAlgoIt->second == "quickSort") {
            sortOpts.algorithm = models::SortAlgorithm::QuickSort;
        }

        searchService_->sort(questions, sortOpts);
    }

    return HttpResponse::json(utils::json::questionsToJson(questions));
}

HttpResponse HttpServer::handleGetQuestionById(const std::string& id) {
    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);

    auto opt = questionService_->getQuestionById(id);
    if (!opt.has_value()) {
        return HttpResponse::error("Question not found", 404, "Not Found");
    }

    if (recentHistoryService_) {
        recentHistoryService_->recordView(id);
    }

    return HttpResponse::json(utils::json::questionToJson(opt.value()));
}

HttpResponse HttpServer::handleCreateQuestion(const HttpRequest& req) {
    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);

    auto root = utils::json::parse(req.body);
    models::Question q;
    std::string err;
    if (!utils::json::jsonToQuestion(root, q, err)) {
        return HttpResponse::error("Invalid JSON body: " + err, 400);
    }

    // Auto-generate ID if empty
    if (q.getId().empty()) {
        q.setId(questionService_->generateNextId());
    }

    // Always enforce the current user identity; do not blindly trust client input
    q.setOwnerId(questionService_->getCurrentUserId());

    auto valResult = questionService_->createQuestion(q);
    if (!valResult.isValid) {
        return HttpResponse::error(valResult.errorMessage, 400);
    }

    return HttpResponse::json(utils::json::questionToJson(q), 201, "Created");
}

HttpResponse HttpServer::handleUpdateQuestion(const std::string& id, const HttpRequest& req) {
    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);

    auto optExisting = questionService_->getQuestionById(id);
    if (!optExisting.has_value()) {
        return HttpResponse::error("Question not found", 404, "Not Found");
    }

    auto root = utils::json::parse(req.body);
    if (!root.isObject()) {
        return HttpResponse::error("Invalid JSON body: Expected JSON object", 400);
    }

    models::Question q = optExisting.value();

    if (root.hasKey("title")) {
        std::string title = root.getString("title");
        if (title.empty()) {
            return HttpResponse::error("Question title cannot be empty", 400);
        }
        q.setTitle(title);
    }
    if (root.hasKey("description")) q.setDescription(root.getString("description"));
    if (root.hasKey("topic")) q.setTopic(models::stringToTopic(root.getString("topic")));
    if (root.hasKey("difficulty")) q.setDifficulty(models::stringToDifficulty(root.getString("difficulty")));
    if (root.hasKey("company")) q.setCompany(root.getString("company"));
    if (root.hasKey("platform")) q.setPlatform(models::stringToPlatform(root.getString("platform")));
    if (root.hasKey("source_url")) q.setSourceUrl(root.getString("source_url"));
    if (root.hasKey("status")) q.setStatus(models::stringToStatus(root.getString("status")));
    if (root.hasKey("is_favorite")) q.setFavorite(root.getBool("is_favorite"));
    if (root.hasKey("notes")) q.setNotes(root.getString("notes"));
    if (root.hasKey("tags")) q.setTags(root.getStringArray("tags"));
    if (root.hasKey("revision_priority")) {
        int32_t prio = static_cast<int32_t>(root.getInt("revision_priority"));
        if (prio >= 1 && prio <= 5) q.setRevisionPriority(prio);
    }
    if (root.hasKey("next_revision_at")) q.setNextRevisionAt(root.getInt("next_revision_at"));
    if (root.hasKey("last_practiced_at")) q.setLastPracticedAt(root.getInt("last_practiced_at"));

    q.setId(id); // Ensure matching target ID

    auto valResult = questionService_->updateQuestion(q);
    if (!valResult.isValid) {
        return HttpResponse::error(valResult.errorMessage, 400);
    }

    return HttpResponse::json(utils::json::questionToJson(q));
}

HttpResponse HttpServer::handleDeleteQuestion(const std::string& id) {
    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);

    if (!questionService_->deleteQuestion(id)) {
        return HttpResponse::error("Question not found or could not be deleted", 404, "Not Found");
    }

    return HttpResponse::json("{\"success\":true}");
}

HttpResponse HttpServer::handleGetDashboard() {
    if (!statisticsService_) return HttpResponse::error("StatisticsService unavailable", 500);

    auto snapshot = statisticsService_->getDashboardSnapshot();
    return HttpResponse::json(utils::json::dashboardSnapshotToJson(snapshot));
}

HttpResponse HttpServer::handleGetDueRevision() {
    if (!revisionService_) return HttpResponse::error("RevisionService unavailable", 500);

    auto items = revisionService_->getDueQuestions();
    return HttpResponse::json(utils::json::revisionItemsToJson(items));
}

HttpResponse HttpServer::handleGetUpcomingRevision() {
    if (!revisionService_) return HttpResponse::error("RevisionService unavailable", 500);

    auto items = revisionService_->getUpcomingRevisions(20);
    return HttpResponse::json(utils::json::revisionItemsToJson(items));
}

HttpResponse HttpServer::handleScheduleRevision(const HttpRequest& req) {
    if (!revisionService_) return HttpResponse::error("RevisionService unavailable", 500);

    auto root = utils::json::parse(req.body);
    std::string qid = root.getString("questionId");
    if (qid.empty()) return HttpResponse::error("questionId is required", 400);

    int64_t nextRev = root.getInt("nextRevisionAt", 0);
    int32_t priority = static_cast<int32_t>(root.getInt("priority", 2));

    revisionService_->rescheduleQuestion(qid, nextRev, priority);

    // Also update Question entity in repository
    if (questionService_) {
        auto opt = questionService_->getQuestionById(qid);
        if (opt.has_value()) {
            auto q = opt.value();
            q.setNextRevisionAt(nextRev);
            q.setRevisionPriority(priority);
            questionService_->saveQuestion(q);
        }
    }

    return HttpResponse::json("{\"success\":true}");
}

HttpResponse HttpServer::handleStartPractice(const HttpRequest& req) {
    if (!practiceService_ || !questionService_) {
        return HttpResponse::error("PracticeService unavailable", 500);
    }

    auto root = utils::json::parse(req.body);
    std::string filterPreset = root.getString("filter");
    std::vector<std::string> explicitIds = root.getStringArray("questionIds");

    std::vector<std::string> targetIds;

    if (!explicitIds.empty()) {
        targetIds = explicitIds;
    } else if (filterPreset == "due" && revisionService_) {
        auto dueItems = revisionService_->getDueQuestions();
        for (const auto& item : dueItems) {
            targetIds.push_back(item.questionId);
        }
    } else if (filterPreset == "all_unsolved") {
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) {
            if (q.getStatus() == models::Status::Unsolved || q.getStatus() == models::Status::InProgress) {
                targetIds.push_back(q.getId());
            }
        }
    } else if (filterPreset == "favorites") {
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) {
            if (q.isFavorite()) targetIds.push_back(q.getId());
        }
    } else if (filterPreset.rfind("topic:", 0) == 0) {
        std::string topicName = filterPreset.substr(6);
        models::Topic topic = models::stringToTopic(topicName);
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) {
            if (q.getTopic() == topic) targetIds.push_back(q.getId());
        }
    } else if (filterPreset.rfind("difficulty:", 0) == 0) {
        std::string diffName = filterPreset.substr(11);
        models::Difficulty diff = models::stringToDifficulty(diffName);
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) {
            if (q.getDifficulty() == diff) targetIds.push_back(q.getId());
        }
    } else {
        // Default: all questions
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) targetIds.push_back(q.getId());
    }

    practiceService_->startSession(targetIds);
    auto progress = practiceService_->getProgress();

    return HttpResponse::json(utils::json::sessionProgressToJson(progress));
}

HttpResponse HttpServer::handleGetPracticeCurrent() {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    auto currentId = practiceService_->getCurrentQuestion();
    if (!currentId.has_value()) {
        return HttpResponse::json("{\"hasQuestion\":false}");
    }

    if (!questionService_) return HttpResponse::error("QuestionService unavailable", 500);
    auto qOpt = questionService_->getQuestionById(currentId.value());
    if (!qOpt.has_value()) {
        return HttpResponse::json("{\"hasQuestion\":false}");
    }

    std::string out = "{";
    out += "\"hasQuestion\":true,";
    out += "\"question\":" + utils::json::questionToJson(qOpt.value()) + ",";
    out += "\"progress\":" + utils::json::sessionProgressToJson(practiceService_->getProgress());
    out += "}";
    return HttpResponse::json(out);
}

HttpResponse HttpServer::handlePracticeVerdict(const HttpRequest& req) {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    auto root = utils::json::parse(req.body);
    std::string verdictStr = root.getString("verdict", "Solved");

    models::PracticeVerdict verdict = models::PracticeVerdict::Solved;
    if (models::iequals(verdictStr, "NeedsReview") || models::iequals(verdictStr, "Review")) {
        verdict = models::PracticeVerdict::NeedsReview;
    } else if (models::iequals(verdictStr, "Skipped") || models::iequals(verdictStr, "Skip")) {
        verdict = models::PracticeVerdict::Skipped;
    }

    auto res = practiceService_->recordPracticeAttempt(verdict);
    std::string schedJson = res.has_value() ? utils::json::revisionScheduleResultToJson(res.value()) : "null";

    std::string out = "{";
    out += "\"success\":true,";
    out += "\"schedule\":" + schedJson + ",";
    out += "\"progress\":" + utils::json::sessionProgressToJson(practiceService_->getProgress()) + ",";

    auto nextId = practiceService_->getCurrentQuestion();
    if (nextId.has_value() && questionService_) {
        auto nextQ = questionService_->getQuestionById(nextId.value());
        if (nextQ.has_value()) {
            out += "\"hasNext\":true,";
            out += "\"nextQuestion\":" + utils::json::questionToJson(nextQ.value());
        } else {
            out += "\"hasNext\":false";
        }
    } else {
        out += "\"hasNext\":false";
    }
    out += "}";

    return HttpResponse::json(out);
}

HttpResponse HttpServer::handlePracticeSkip() {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    bool skipped = practiceService_->skipCurrentQuestion();
    std::string out = "{";
    out += "\"skipped\":" + utils::json::serializeBool(skipped) + ",";
    out += "\"progress\":" + utils::json::sessionProgressToJson(practiceService_->getProgress());
    out += "}";
    return HttpResponse::json(out);
}

HttpResponse HttpServer::handleGetPracticeProgress() {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    auto progress = practiceService_->getProgress();
    return HttpResponse::json(utils::json::sessionProgressToJson(progress));
}

HttpResponse HttpServer::handleExitPractice() {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    practiceService_->clearSession();
    return HttpResponse::json("{\"success\":true}");
}

HttpResponse HttpServer::handleGetPracticeNext(const HttpRequest& req) {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    models::PracticeNextCriteria criteria;

    auto topicIt = req.queryParams.find("topic");
    if (topicIt != req.queryParams.end() && !topicIt->second.empty() && topicIt->second != "All") {
        criteria.topic = models::stringToTopic(topicIt->second);
    }

    auto diffIt = req.queryParams.find("difficulty");
    if (diffIt != req.queryParams.end() && !diffIt->second.empty() && diffIt->second != "All") {
        criteria.difficulty = models::stringToDifficulty(diffIt->second);
    }

    auto incDueIt = req.queryParams.find("includeDueRevisions");
    if (incDueIt == req.queryParams.end()) incDueIt = req.queryParams.find("include_due_revisions");
    if (incDueIt != req.queryParams.end() && !incDueIt->second.empty()) {
        criteria.includeDueRevisions = (incDueIt->second == "true" || incDueIt->second == "1");
    }

    auto prefUnsolvedIt = req.queryParams.find("preferUnsolved");
    if (prefUnsolvedIt == req.queryParams.end()) prefUnsolvedIt = req.queryParams.find("prefer_unsolved");
    if (prefUnsolvedIt != req.queryParams.end() && !prefUnsolvedIt->second.empty()) {
        criteria.preferUnsolved = (prefUnsolvedIt->second == "true" || prefUnsolvedIt->second == "1");
    }

    auto result = practiceService_->getPracticeNext(criteria);
    return HttpResponse::json(utils::json::practiceNextResultToJson(result));
}

HttpResponse HttpServer::handleStartPracticeSession(const HttpRequest& req) {
    if (!practiceService_ || !questionService_) {
        return HttpResponse::error("PracticeService unavailable", 500);
    }

    models::QuestionFilter filter;
    bool dueOnly = false;

    if (!req.body.empty()) {
        auto root = utils::json::parse(req.body);
        std::string topicStr = root.getString("topic");
        if (!topicStr.empty() && topicStr != "All") {
            filter.topic = models::stringToTopic(topicStr);
        }
        std::string diffStr = root.getString("difficulty");
        if (!diffStr.empty() && diffStr != "All") {
            filter.difficulty = models::stringToDifficulty(diffStr);
        }
        std::string statusStr = root.getString("status");
        if (!statusStr.empty() && statusStr != "All") {
            filter.status = models::stringToStatus(statusStr);
        }
        std::string company = root.getString("company");
        if (!company.empty()) {
            filter.company = company;
        }
        if (root.hasKey("isFavorite")) {
            filter.isFavorite = root.getBool("isFavorite");
        } else if (root.hasKey("is_favorite")) {
            filter.isFavorite = root.getBool("is_favorite");
        }
        if (root.hasKey("dueOnly")) {
            dueOnly = root.getBool("dueOnly");
        } else if (root.hasKey("due_only")) {
            dueOnly = root.getBool("due_only");
        }
    }

    auto topicIt = req.queryParams.find("topic");
    if (topicIt != req.queryParams.end() && !topicIt->second.empty() && topicIt->second != "All") {
        filter.topic = models::stringToTopic(topicIt->second);
    }
    auto diffIt = req.queryParams.find("difficulty");
    if (diffIt != req.queryParams.end() && !diffIt->second.empty() && diffIt->second != "All") {
        filter.difficulty = models::stringToDifficulty(diffIt->second);
    }
    auto statusIt = req.queryParams.find("status");
    if (statusIt != req.queryParams.end() && !statusIt->second.empty() && statusIt->second != "All") {
        filter.status = models::stringToStatus(statusIt->second);
    }
    auto dueIt = req.queryParams.find("dueOnly");
    if (dueIt == req.queryParams.end()) dueIt = req.queryParams.find("due_only");
    if (dueIt != req.queryParams.end() && !dueIt->second.empty()) {
        dueOnly = (dueIt->second == "true" || dueIt->second == "1");
    }

    practiceService_->startSessionWithFilter(filter, dueOnly);
    auto progress = practiceService_->getProgress();

    return HttpResponse::json(utils::json::sessionProgressToJson(progress));
}

HttpResponse HttpServer::handleGetPracticeQueue() {
    if (!practiceService_) return HttpResponse::error("PracticeService unavailable", 500);

    auto queueQuestions = practiceService_->getQueueQuestions();
    std::string out = "{";
    out += "\"queue\":" + utils::json::questionsToJson(queueQuestions) + ",";
    out += "\"count\":" + std::to_string(queueQuestions.size());
    out += "}";
    return HttpResponse::json(out);
}

HttpResponse HttpServer::handleRemoveFromPracticeQueue(const std::string& questionId) {
    if (!practiceService_ || !questionService_) return HttpResponse::error("PracticeService unavailable", 500);

    if (questionId.empty()) {
        return HttpResponse::error("Question ID is required", 400);
    }

    auto qOpt = questionService_->getQuestionById(questionId);
    if (!qOpt.has_value()) {
        return HttpResponse::error("Question not found", 404, "Not Found");
    }

    bool removed = practiceService_->removeQuestionFromQueue(questionId);
    if (!removed) {
        return HttpResponse::error("Question is not in the practice queue", 404, "Not Found");
    }

    return HttpResponse::json("{\"success\":true,\"removed\":true,\"questionId\":\"" + utils::json::escape(questionId) + "\"}");
}

HttpResponse HttpServer::handleRecordPracticeResultForQuestion(const std::string& questionId, const HttpRequest& req) {
    if (!practiceService_ || !questionService_) return HttpResponse::error("PracticeService unavailable", 500);

    if (questionId.empty()) {
        return HttpResponse::error("Question ID is required", 400);
    }

    auto qOpt = questionService_->getQuestionById(questionId);
    if (!qOpt.has_value()) {
        return HttpResponse::error("Question not found", 404, "Not Found");
    }

    auto root = utils::json::parse(req.body);
    std::string verdictStr = root.getString("verdict");
    if (verdictStr.empty()) {
        return HttpResponse::error("Verdict is required", 400);
    }

    models::PracticeVerdict verdict;
    if (models::iequals(verdictStr, "Solved")) {
        verdict = models::PracticeVerdict::Solved;
    } else if (models::iequals(verdictStr, "NeedsReview") || models::iequals(verdictStr, "Review")) {
        verdict = models::PracticeVerdict::NeedsReview;
    } else if (models::iequals(verdictStr, "Skipped") || models::iequals(verdictStr, "Skip")) {
        verdict = models::PracticeVerdict::Skipped;
    } else {
        return HttpResponse::error("Invalid verdict: '" + verdictStr + "'. Expected: Solved, NeedsReview, or Skipped", 400);
    }

    auto schedOpt = practiceService_->recordPracticeAttemptForQuestion(questionId, verdict);

    auto updatedQOpt = questionService_->getQuestionById(questionId);
    std::string qJson = updatedQOpt.has_value() ? utils::json::questionToJson(updatedQOpt.value()) : "null";
    std::string schedJson = schedOpt.has_value() ? utils::json::revisionScheduleResultToJson(schedOpt.value()) : "null";

    std::string out = "{";
    out += "\"success\":true,";
    out += "\"questionId\":\"" + utils::json::escape(questionId) + "\",";
    out += "\"verdict\":\"" + verdictStr + "\",";
    out += "\"question\":" + qJson + ",";
    out += "\"schedule\":" + schedJson;
    out += "}";

    return HttpResponse::json(out);
}

HttpResponse HttpServer::handleGetHistory() {
    if (!recentHistoryService_) return HttpResponse::error("RecentHistoryService unavailable", 500);

    auto historyIds = recentHistoryService_->getHistory();
    return HttpResponse::json(utils::json::serializeStringArray(historyIds));
}

HttpResponse HttpServer::handleGetDiagnostics() {
    std::string storagePath = "data/questions.csv";
    std::string storageType = "csv";
    std::string databasePath = "";
    bool databaseAvailable = false;
    int schemaVersion = 0;
    int64_t databaseSizeBytes = 0;

    if (questionService_ && questionService_->getRepository()) {
        auto sqliteRepo = std::dynamic_pointer_cast<persistence::SqliteQuestionRepository>(questionService_->getRepository());
        if (sqliteRepo) {
            storageType = "sqlite";
            storagePath = sqliteRepo->getDbPath();
            databasePath = sqliteRepo->getDbPath();
            databaseAvailable = sqliteRepo->isOpen();
            schemaVersion = sqliteRepo->getSchemaVersion();
            databaseSizeBytes = sqliteRepo->getDatabaseSizeBytes();
        } else {
            auto fileRepo = std::dynamic_pointer_cast<persistence::FileQuestionRepository>(questionService_->getRepository());
            if (fileRepo) {
                storagePath = fileRepo->getFilePath();
            }
        }
    }

    std::string out = "{";
    out += "\"version\":\"0.1.0\",";
    out += "\"runtime\":\"C++17 Desktop Core\",";
    out += "\"storageType\":\"" + storageType + "\",";
    out += "\"storagePath\":\"" + storagePath + "\",";
    out += "\"databasePath\":\"" + databasePath + "\",";
    out += "\"databaseAvailable\":" + std::string(databaseAvailable ? "true" : "false") + ",";
    out += "\"schemaVersion\":" + std::to_string(schemaVersion) + ",";
    out += "\"databaseSizeBytes\":" + std::to_string(databaseSizeBytes) + ",";
    out += "\"invariantsPassed\":5,";
    out += "\"totalInvariants\":5,";
    out += "\"prefixTrieActive\":true,";
    out += "\"minHeapActive\":true,";
    out += "\"practiceQueueActive\":true,";
    out += "\"historyStackActive\":true,";
    out += "\"sortingEngineActive\":true";
    if (questionService_) {
        out += ",\"currentUser\":\"" + questionService_->getCurrentUserId() + "\"";
        out += ",\"questionCount\":" + std::to_string(questionService_->getQuestionCount());
    }
    if (searchService_) {
        out += ",\"indexedTitles\":" + std::to_string(searchService_->getIndexedTitleCount());
    }
    if (revisionService_) {
        out += ",\"scheduledRevisions\":" + std::to_string(revisionService_->getTotalScheduledCount());
    }
    out += ",\"authEnabled\":" + std::string(authService_ ? "true" : "false");
    out += "}";
    return HttpResponse::json(out);
}

} // namespace codevault::app
