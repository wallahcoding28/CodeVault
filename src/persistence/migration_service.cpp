#include "persistence/migration_service.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace codevault::persistence {

namespace {

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    fields.push_back(current);
    return fields;
}

int64_t safeStoll(const std::string& str, int64_t defaultVal = 0) {
    if (str.empty()) return defaultVal;
    try {
        size_t idx = 0;
        int64_t val = std::stoll(str, &idx);
        return val;
    } catch (...) {
        return defaultVal;
    }
}

int32_t safeStoi(const std::string& str, int32_t defaultVal = 0) {
    if (str.empty()) return defaultVal;
    try {
        size_t idx = 0;
        int32_t val = std::stoi(str, &idx);
        return val;
    } catch (...) {
        return defaultVal;
    }
}

models::Question parseQuestionRecord(const std::vector<std::string>& tokens) {
    if (tokens.size() < 17) {
        return {};
    }

    std::string id = tokens[0];
    if (id == "id" || id.empty()) {
        return {};
    }

    std::string title = tokens[1];
    std::string description = tokens[2];
    auto topic = models::stringToTopic(tokens[3]);
    auto diff = models::stringToDifficulty(tokens[4]);
    std::string company = tokens[5];
    auto platform = models::stringToPlatform(tokens[6]);
    std::string url = tokens[7];
    auto status = models::stringToStatus(tokens[8]);
    bool is_favorite = (tokens[9] == "true" || tokens[9] == "1");
    std::string notes = tokens[10];

    int64_t created_at = safeStoll(tokens[11], 0);
    int64_t updated_at = safeStoll(tokens[12], 0);
    int64_t last_practiced = safeStoll(tokens[13], 0);
    int64_t next_revision = safeStoll(tokens[14], 0);
    int32_t priority = safeStoi(tokens[15], 2);

    std::vector<std::string> tags;
    std::stringstream tagStream(tokens[16]);
    std::string tagToken;
    while (std::getline(tagStream, tagToken, ';')) {
        if (!tagToken.empty()) {
            tags.push_back(tagToken);
        }
    }

    std::string owner_id = tokens.size() > 17 ? tokens[17] : "local_user";

    return models::Question(
        id, title, description, topic, diff, company, platform,
        url, status, is_favorite, notes, created_at, updated_at,
        last_practiced, next_revision, priority, tags, owner_id
    );
}

} // namespace

bool MigrationService::createCsvBackup(const std::string& csvPath, const std::string& backupPath) {
    if (!std::filesystem::exists(csvPath)) {
        return false;
    }

    std::string target = backupPath;
    if (target.empty()) {
        target = csvPath + ".bak";
    }

    try {
        std::filesystem::path p(target);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::filesystem::copy_file(
            csvPath,
            target,
            std::filesystem::copy_options::overwrite_existing
        );
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[Migration Warning] Failed to backup CSV: " << ex.what() << "\n";
        return false;
    }
}

MigrationReport MigrationService::migrateCsvToSqlite(
    const std::string& csvPath,
    SqliteQuestionRepository& repository,
    bool forceReimport) {

    MigrationReport report;
    report.csvPath = csvPath;
    report.dbPath = repository.getDbPath();
    report.backupPath = csvPath + ".bak";

    if (!std::filesystem::exists(csvPath)) {
        report.message = "Source CSV file does not exist: " + csvPath;
        report.success = false;
        return report;
    }

    // Step 1: Create safe backup before importing
    if (!createCsvBackup(csvPath, report.backupPath)) {
        report.warnings.push_back("Warning: Could not create backup at " + report.backupPath);
    }

    // Step 2: Open and parse CSV file
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        report.message = "Cannot open CSV file: " + csvPath;
        report.success = false;
        return report;
    }

    std::vector<models::Question> validQuestions;
    std::string line;
    size_t lineIndex = 0;

    while (std::getline(file, line)) {
        ++lineIndex;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }

        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto tokens = parseCsvLine(line);
        if (tokens.empty() || tokens[0] == "id") {
            continue; // Header row
        }

        report.csvRecordsRead++;

        models::Question q = parseQuestionRecord(tokens);
        if (!q.isValid()) {
            report.malformedRowsCount++;
            report.warnings.push_back("Line " + std::to_string(lineIndex) + ": Malformed or invalid question record.");
            continue;
        }

        report.validRecordsFound++;
        validQuestions.push_back(std::move(q));
    }

    // Step 3: Atomic batch write into SQLite
    bool txSuccess = repository.executeTransaction([&]() -> bool {
        for (const auto& q : validQuestions) {
            if (!forceReimport && repository.exists(q.getId())) {
                report.skippedExistingCount++;
                continue;
            }

            if (!repository.save(q)) {
                return false;
            }
            report.importedCount++;
        }
        return true;
    });

    report.finalDatabaseCount = repository.count();
    report.success = txSuccess;

    if (txSuccess) {
        report.message = "Successfully migrated " + std::to_string(report.importedCount) +
                         " questions into SQLite database. (" +
                         std::to_string(report.skippedExistingCount) + " existing questions preserved).";
    } else {
        report.message = "Transaction failed while importing records into SQLite database.";
    }

    return report;
}

} // namespace codevault::persistence
