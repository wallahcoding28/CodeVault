#include "app/cli_shell.hpp"
#include "app/http_server.hpp"
#include "persistence/file_question_repository.hpp"
#include "persistence/migration_service.hpp"
#include "persistence/sqlite_question_repository.hpp"
#include "persistence/sqlite_user_repository.hpp"
#include "persistence/sqlite_session_repository.hpp"
#include "services/auth_service.hpp"
#include "services/practice_service.hpp"
#include "services/question_service.hpp"
#include "services/recent_history_service.hpp"
#include "services/revision_service.hpp"
#include "services/statistics_service.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    std::string targetPath;
    bool runServer = false;
    bool runCli = false;
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server" || arg == "-s") {
            runServer = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                try {
                    port = std::stoi(argv[++i]);
                } catch (...) {}
            }
        } else if (arg == "--cli" || arg == "-c") {
            runCli = true;
        } else if (arg == "--both") {
            runServer = true;
            runCli = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "CodeVault - Coding Practice & Spaced Revision Platform\n"
                      << "Usage:\n"
                      << "  codevault                 Run interactive terminal CLI (default with SQLite)\n"
                      << "  codevault --cli [data]    Run interactive terminal CLI\n"
                      << "  codevault --server [port] Start HTTP REST API server for Web UI\n"
                      << "  codevault --both [port]   Start server in background and launch CLI\n"
                      << "  codevault [data.db/csv]   Run with custom database or CSV file\n";
            return 0;
        } else if (arg[0] != '-') {
            targetPath = arg;
        }
    }

    // Default mode: run CLI if server is not explicitly requested
    if (!runServer && !runCli) {
        runCli = true;
    }

    // Ensure data directory exists
    try {
        std::filesystem::create_directories("data");
    } catch (...) {}

    // Seed CSV from sample data if user data CSV is missing
    const std::string defaultCsvPath = "data/questions.csv";
    if (!std::filesystem::exists(defaultCsvPath) && std::filesystem::exists("data/sample/questions.csv")) {
        try {
            std::filesystem::copy_file("data/sample/questions.csv", defaultCsvPath);
        } catch (...) {}
    }

    try {
        std::shared_ptr<codevault::persistence::IQuestionRepository> repository;
        std::string activeSourceDescription;

        // If user specifically requested a CSV file, support FileQuestionRepository for backward safety
        if (!targetPath.empty() && targetPath.size() > 4 && targetPath.substr(targetPath.size() - 4) == ".csv") {
            repository = std::make_shared<codevault::persistence::FileQuestionRepository>(targetPath);
            activeSourceDescription = "CSV File: " + targetPath;
        } else {
            // Default primary persistence engine: SQLite 3
            std::string dbPath = targetPath.empty() ? "data/codevault.db" : targetPath;
            auto sqliteRepo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);

            // Auto-migrate from CSV if SQLite database is newly created or empty
            if (sqliteRepo->count() == 0 && std::filesystem::exists(defaultCsvPath)) {
                std::cout << "[Database] Initializing SQLite repository at " << dbPath << "...\n";
                auto report = codevault::persistence::MigrationService::migrateCsvToSqlite(
                    defaultCsvPath,
                    *sqliteRepo,
                    false
                );
                if (report.success) {
                    std::cout << "[Migration] " << report.message << "\n";
                    if (!report.backupPath.empty()) {
                        std::cout << "[Migration] CSV backup verified at " << report.backupPath << "\n";
                    }
                } else {
                    std::cerr << "[Migration Error] " << report.message << "\n";
                }
            }

            repository = sqliteRepo;
            activeSourceDescription = "SQLite Database: " + dbPath + " (Schema v" +
                                      std::to_string(sqliteRepo->getSchemaVersion()) + ")";
        }

        // Wire up revision service
        auto revisionService = std::make_shared<codevault::services::RevisionService>();

        // Wire up application service layer (links repository, search, and revision)
        auto questionService = std::make_shared<codevault::services::QuestionService>(
            repository,
            nullptr,
            revisionService
        );

        // Wire up practice service (coordinates queue, question service, and revision service)
        auto practiceService = std::make_shared<codevault::services::PracticeService>(
            questionService,
            revisionService
        );

        // Wire up recent history service
        auto recentHistoryService = std::make_shared<codevault::services::RecentHistoryService>();

        // Wire up statistics service
        auto statisticsService = std::make_shared<codevault::services::StatisticsService>(
            questionService,
            revisionService
        );

        // Wire up auth service if using SQLite
        std::shared_ptr<codevault::services::AuthService> authService;
        auto sqliteRepo = std::dynamic_pointer_cast<codevault::persistence::SqliteQuestionRepository>(repository);
        if (sqliteRepo) {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(sqliteRepo->getDbPath());
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(sqliteRepo->getDbPath());
            authService = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
        }

        std::unique_ptr<codevault::app::HttpServer> server;
        if (runServer) {
            server = std::make_unique<codevault::app::HttpServer>(
                questionService,
                recentHistoryService,
                questionService->getSearchService(),
                revisionService,
                practiceService,
                statisticsService,
                authService,
                port
            );

            if (!server->start()) {
                std::cerr << "[Fatal Error] Failed to start HTTP API server on port " << port << "\n";
                return 1;
            }

            std::cout << "==================================================\n"
                      << "CodeVault API Server listening on http://localhost:" << port << "\n"
                      << "REST API Base: http://localhost:" << port << "/api\n"
                      << "Persistence:   " << activeSourceDescription << "\n"
                      << "==================================================\n";

            if (!runCli) {
                std::cout << "Server running. Press Enter or Ctrl+C to terminate.\n";
                std::string line;
                std::getline(std::cin, line);
                server->stop();
                return 0;
            }
        }

        if (runCli) {
            codevault::app::CliShell shell(
                questionService,
                recentHistoryService,
                questionService->getSearchService(),
                revisionService,
                practiceService,
                statisticsService
            );
            shell.run();
        }

        if (server && server->isRunning()) {
            server->stop();
        }
    } catch (const std::exception& ex) {
        std::cerr << "[Fatal Error] " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
