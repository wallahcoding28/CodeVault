#include "services/import_service.hpp"
#include "persistence/sqlite_question_repository.hpp"
#include "utils/json.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace codevault::services {

namespace {

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static int64_t safeStoll(const std::string& str, int64_t defaultVal = 0) {
    if (str.empty()) return defaultVal;
    try {
        size_t idx = 0;
        int64_t val = std::stoll(str, &idx);
        return val;
    } catch (...) {
        return defaultVal;
    }
}

static int32_t safeStoi(const std::string& str, int32_t defaultVal = 0) {
    if (str.empty()) return defaultVal;
    try {
        size_t idx = 0;
        int32_t val = std::stoi(str, &idx);
        return val;
    } catch (...) {
        return defaultVal;
    }
}

} // namespace

ImportService::ImportService(std::shared_ptr<QuestionService> questionService)
    : questionService_(std::move(questionService)) {}

std::vector<std::vector<std::string>> ImportService::parseRfc4180Csv(
    const std::string& input,
    std::string& outError) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> currentRow;
    std::string currentField;
    bool inQuotes = false;
    const size_t len = input.size();

    for (size_t i = 0; i < len; ++i) {
        char c = input[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < len && input[i + 1] == '"') {
                    currentField += '"';
                    ++i; // skip escaped quote
                } else {
                    inQuotes = false;
                }
            } else {
                currentField += c; // Preserve newlines, commas inside quotes
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                currentRow.push_back(currentField);
                currentField.clear();
            } else if (c == '\r') {
                if (i + 1 < len && input[i + 1] == '\n') {
                    ++i;
                }
                currentRow.push_back(currentField);
                currentField.clear();
                rows.push_back(currentRow);
                currentRow.clear();
            } else if (c == '\n') {
                currentRow.push_back(currentField);
                currentField.clear();
                rows.push_back(currentRow);
                currentRow.clear();
            } else {
                currentField += c;
            }
        }
    }

    if (inQuotes) {
        outError = "Malformed CSV: unclosed quote detected";
        return {};
    }

    if (!currentField.empty() || !currentRow.empty()) {
        currentRow.push_back(currentField);
        rows.push_back(currentRow);
    }

    return rows;
}

std::string ImportService::generateUniqueId(int64_t& inMemoryMaxNum) const {
    ++inMemoryMaxNum;
    return "Q-" + std::to_string(inMemoryMaxNum);
}

models::ImportResult ImportService::importFromCsv(
    const std::string& csvContent,
    models::ImportConflictStrategy strategy) {
    models::ImportResult result;
    if (csvContent.empty()) {
        result.success = true;
        return result;
    }

    std::string parseErr;
    auto rows = parseRfc4180Csv(csvContent, parseErr);
    if (!parseErr.empty()) {
        result.success = false;
        result.errors.push_back(parseErr);
        return result;
    }

    if (rows.empty()) {
        result.success = true;
        return result;
    }

    // 1. Header validation
    const auto& header = rows[0];
    std::vector<std::string> normalizedHeader;
    normalizedHeader.reserve(header.size());
    for (const auto& h : header) {
        normalizedHeader.push_back(toLower(trim(h)));
    }

    auto getColIdx = [&](const std::string& name) -> int {
        for (size_t i = 0; i < normalizedHeader.size(); ++i) {
            if (normalizedHeader[i] == name) return static_cast<int>(i);
        }
        return -1;
    };

    int titleIdx = getColIdx("title");
    if (titleIdx < 0) {
        result.success = false;
        result.errors.push_back("CSV header missing required 'title' column.");
        return result;
    }

    int idIdx = getColIdx("id");
    int descIdx = getColIdx("description");
    int topicIdx = getColIdx("topic");
    int diffIdx = getColIdx("difficulty");
    int companyIdx = getColIdx("company");
    int platformIdx = getColIdx("platform");
    int urlIdx = getColIdx("source_url");
    if (urlIdx < 0) urlIdx = getColIdx("url");
    int statusIdx = getColIdx("status");
    int favIdx = getColIdx("is_favorite");
    if (favIdx < 0) favIdx = getColIdx("favorite");
    int notesIdx = getColIdx("notes");
    int createdIdx = getColIdx("created_at");
    int updatedIdx = getColIdx("updated_at");
    int practicedIdx = getColIdx("last_practiced_at");
    int revisionIdx = getColIdx("next_revision_at");
    int priorityIdx = getColIdx("revision_priority");
    if (priorityIdx < 0) priorityIdx = getColIdx("priority");
    int tagsIdx = getColIdx("tags");

    // 2. Parse data rows
    std::vector<models::Question> questions;
    questions.reserve(rows.size() - 1);

    auto getField = [](const std::vector<std::string>& row, int idx) -> std::string {
        if (idx >= 0 && idx < static_cast<int>(row.size())) {
            return row[static_cast<size_t>(idx)];
        }
        return {};
    };

    for (size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        // Skip empty rows
        bool allEmpty = true;
        for (const auto& f : row) {
            if (!trim(f).empty()) {
                allEmpty = false;
                break;
            }
        }
        if (allEmpty) continue;

        std::string title = getField(row, titleIdx);
        std::string id = getField(row, idIdx);
        std::string desc = getField(row, descIdx);
        std::string topicStr = getField(row, topicIdx);
        std::string diffStr = getField(row, diffIdx);
        std::string company = getField(row, companyIdx);
        std::string platformStr = getField(row, platformIdx);
        std::string url = getField(row, urlIdx);
        std::string statusStr = getField(row, statusIdx);
        std::string favStr = getField(row, favIdx);
        bool isFav = (favStr == "1" || favStr == "true" || favStr == "True");
        std::string notes = getField(row, notesIdx);
        int64_t createdAt = safeStoll(getField(row, createdIdx), 0);
        int64_t updatedAt = safeStoll(getField(row, updatedIdx), 0);
        int64_t practicedAt = safeStoll(getField(row, practicedIdx), 0);
        int64_t nextRevAt = safeStoll(getField(row, revisionIdx), 0);
        int32_t priority = safeStoi(getField(row, priorityIdx), 2);
        if (priority < 1 || priority > 5) priority = 2;

        std::vector<std::string> tags;
        std::string tagsRaw = getField(row, tagsIdx);
        if (!tagsRaw.empty()) {
            std::stringstream ss(tagsRaw);
            std::string t;
            while (std::getline(ss, t, ';')) {
                std::string trimmedTag = trim(t);
                if (!trimmedTag.empty()) {
                    tags.push_back(trimmedTag);
                }
            }
        }

        models::Question q(
            id,
            title,
            desc,
            models::stringToTopic(topicStr.empty() ? "Other" : topicStr),
            models::stringToDifficulty(diffStr.empty() ? "Medium" : diffStr),
            company,
            models::stringToPlatform(platformStr.empty() ? "Custom" : platformStr),
            url,
            models::stringToStatus(statusStr.empty() ? "Unsolved" : statusStr),
            isFav,
            notes,
            createdAt,
            updatedAt,
            practicedAt,
            nextRevAt,
            priority,
            tags,
            "local_user" // temporary, will be bound to authenticated owner in importQuestions
        );

        questions.push_back(std::move(q));
    }

    return importQuestions(questions, strategy);
}

models::ImportResult ImportService::importFromJson(
    const std::string& jsonContent,
    models::ImportConflictStrategy strategy) {
    models::ImportResult result;
    if (jsonContent.empty()) {
        result.success = false;
        result.errors.push_back("JSON content is empty");
        return result;
    }

    auto root = utils::json::parse(jsonContent);
    std::vector<utils::json::JsonValue> items;

    if (root.isArray()) {
        items = root.arrayValue;
    } else if (root.isObject() && root.hasKey("questions")) {
        const auto* qArr = root.get("questions");
        if (qArr && qArr->isArray()) {
            items = qArr->arrayValue;
        } else {
            result.success = false;
            result.errors.push_back("Expected 'questions' property to be an array");
            return result;
        }
    } else {
        result.success = false;
        result.errors.push_back("Expected JSON array or object containing 'questions' array");
        return result;
    }

    std::vector<models::Question> questions;
    questions.reserve(items.size());

    for (size_t i = 0; i < items.size(); ++i) {
        models::Question q;
        std::string err;
        if (!utils::json::jsonToQuestion(items[i], q, err)) {
            result.success = false;
            result.errors.push_back("Failed parsing question at index " + std::to_string(i) + ": " + err);
            return result;
        }
        questions.push_back(std::move(q));
    }

    return importQuestions(questions, strategy);
}

models::ImportResult ImportService::importQuestions(
    const std::vector<models::Question>& questions,
    models::ImportConflictStrategy strategy) {
    models::ImportResult result;
    if (!questionService_) {
        result.success = false;
        result.errors.push_back("QuestionService is not initialized.");
        return result;
    }

    auto repo = questionService_->getRepository();
    if (!repo) {
        result.success = false;
        result.errors.push_back("Repository is not initialized.");
        return result;
    }

    if (questions.empty()) {
        result.success = true;
        return result;
    }

    const std::string currentOwner = questionService_->getCurrentUserId();

    // 1. Establish baseline maximum question ID for unique generation across entire database
    int64_t maxNum = 1000;
    const auto allQuestions = repo->findAll();
    for (const auto& q : allQuestions) {
        const std::string& qid = q.getId();
        if (qid.rfind("Q-", 0) == 0 && qid.size() > 2) {
            try {
                size_t idx = 0;
                int64_t num = std::stoll(qid.substr(2), &idx);
                if (idx == qid.size() - 2) {
                    maxNum = std::max(maxNum, num);
                }
            } catch (...) {}
        }
    }

    // 2. Pre-validation pass: Validate every question before database mutation
    for (size_t i = 0; i < questions.size(); ++i) {
        const auto& q = questions[i];
        std::string title = trim(q.getTitle());
        if (title.empty()) {
            result.errors.push_back("Question at index " + std::to_string(i) + " has an empty title.");
        }
        if (title.size() > 255) {
            result.errors.push_back("Question at index " + std::to_string(i) + " title exceeds 255 characters.");
        }
        if (q.getDifficulty() == models::Difficulty::Unknown) {
            result.errors.push_back("Question at index " + std::to_string(i) + " has unknown difficulty.");
        }
        const std::string& url = q.getSourceUrl();
        if (!url.empty()) {
            if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
                result.errors.push_back("Question at index " + std::to_string(i) + " URL must start with http:// or https://");
            }
            for (char ch : url) {
                if (std::isspace(static_cast<unsigned char>(ch))) {
                    result.errors.push_back("Question at index " + std::to_string(i) + " URL contains spaces.");
                    break;
                }
            }
        }
        if (q.getRevisionPriority() < 1 || q.getRevisionPriority() > 5) {
            result.errors.push_back("Question at index " + std::to_string(i) + " priority must be between 1 and 5.");
        }
    }

    if (result.hasErrors()) {
        result.success = false;
        return result;
    }

    // 3. Plan modifications and resolve conflict strategies
    enum class ActionType { Insert, Update, Skip };
    struct PlanItem {
        ActionType action;
        models::Question question;
    };

    std::vector<PlanItem> plan;
    plan.reserve(questions.size());

    // Track IDs claimed in this current batch to avoid internal duplicate collisions
    std::vector<std::string> batchAssignedIds;

    auto isIdClaimed = [&](const std::string& id) -> bool {
        if (repo->exists(id)) return true;
        return std::find(batchAssignedIds.begin(), batchAssignedIds.end(), id) != batchAssignedIds.end();
    };

    for (const auto& originalQ : questions) {
        models::Question q = originalQ;
        // Enforce mandatory ownership invariant: override any payload owner_id
        q.setOwnerId(currentOwner);

        std::string id = trim(q.getId());

        if (id.empty()) {
            // Unspecified ID: always generate next unique ID
            id = generateUniqueId(maxNum);
            q.setId(id);
            batchAssignedIds.push_back(id);
            plan.push_back({ActionType::Insert, std::move(q)});
            continue;
        }

        bool existsForUser = repo->existsForOwner(id, currentOwner);
        bool existsAnywhere = isIdClaimed(id);

        if (existsForUser) {
            if (strategy == models::ImportConflictStrategy::Skip) {
                plan.push_back({ActionType::Skip, std::move(q)});
            } else if (strategy == models::ImportConflictStrategy::Overwrite) {
                plan.push_back({ActionType::Update, std::move(q)});
            } else if (strategy == models::ImportConflictStrategy::GenerateNewId) {
                std::string newId = generateUniqueId(maxNum);
                q.setId(newId);
                batchAssignedIds.push_back(newId);
                plan.push_back({ActionType::Insert, std::move(q)});
            }
        } else if (existsAnywhere) {
            // ID belongs to another user or was claimed earlier in this batch
            if (strategy == models::ImportConflictStrategy::Skip) {
                plan.push_back({ActionType::Skip, std::move(q)});
            } else if (strategy == models::ImportConflictStrategy::Overwrite) {
                // Must NEVER overwrite another user's question! Safely assign fresh ID for current user
                std::string newId = generateUniqueId(maxNum);
                q.setId(newId);
                batchAssignedIds.push_back(newId);
                plan.push_back({ActionType::Insert, std::move(q)});
            } else if (strategy == models::ImportConflictStrategy::GenerateNewId) {
                std::string newId = generateUniqueId(maxNum);
                q.setId(newId);
                batchAssignedIds.push_back(newId);
                plan.push_back({ActionType::Insert, std::move(q)});
            }
        } else {
            // Does not exist anywhere: insert with incoming ID
            batchAssignedIds.push_back(id);
            plan.push_back({ActionType::Insert, std::move(q)});
        }
    }

    // 4. Atomic execution
    auto sqliteRepo = dynamic_cast<persistence::SqliteQuestionRepository*>(repo.get());
    bool executionOk = false;

    if (sqliteRepo != nullptr) {
        executionOk = sqliteRepo->executeTransaction([&]() -> bool {
            for (const auto& item : plan) {
                if (item.action == ActionType::Insert || item.action == ActionType::Update) {
                    if (!repo->saveForOwner(item.question, currentOwner)) {
                        return false;
                    }
                }
            }
            return true;
        });
    } else {
        // Fallback for non-SQLite repos
        executionOk = true;
        for (const auto& item : plan) {
            if (item.action == ActionType::Insert || item.action == ActionType::Update) {
                if (!repo->saveForOwner(item.question, currentOwner)) {
                    executionOk = false;
                    break;
                }
            }
        }
    }

    if (!executionOk) {
        result.success = false;
        result.errors.push_back("Failed to execute atomic import transaction in repository storage.");
        questionService_->refreshUserScope();
        return result;
    }

    // 5. Update outcome counts
    for (const auto& item : plan) {
        if (item.action == ActionType::Insert) {
            result.importedCount++;
        } else if (item.action == ActionType::Update) {
            result.updatedCount++;
        } else if (item.action == ActionType::Skip) {
            result.skippedCount++;
        }
        result.totalProcessed++;
    }

    // 6. Synchronize DSA Indexes (PrefixTrie & MinHeap)
    questionService_->refreshUserScope();

    result.success = true;
    return result;
}

} // namespace codevault::services
