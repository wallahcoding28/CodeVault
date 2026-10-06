#include "utils/json.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace codevault::utils::json {

std::string escape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream ss;
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(static_cast<unsigned char>(c));
                    out += ss.str();
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

std::string serializeString(const std::string& val) {
    return "\"" + escape(val) + "\"";
}

std::string serializeInt(int64_t val) {
    return std::to_string(val);
}

std::string serializeDouble(double val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << val;
    return ss.str();
}

std::string serializeBool(bool val) {
    return val ? "true" : "false";
}

std::string serializeStringArray(const std::vector<std::string>& vec) {
    std::string out = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out += ",";
        out += serializeString(vec[i]);
    }
    out += "]";
    return out;
}

std::string questionToJson(const models::Question& q) {
    std::string out = "{";
    out += "\"id\":" + serializeString(q.getId()) + ",";
    out += "\"title\":" + serializeString(q.getTitle()) + ",";
    out += "\"description\":" + serializeString(q.getDescription()) + ",";
    out += "\"topic\":" + serializeString(models::topicToString(q.getTopic())) + ",";
    out += "\"difficulty\":" + serializeString(models::difficultyToString(q.getDifficulty())) + ",";
    out += "\"company\":" + serializeString(q.getCompany()) + ",";
    out += "\"platform\":" + serializeString(models::platformToString(q.getPlatform())) + ",";
    out += "\"source_url\":" + serializeString(q.getSourceUrl()) + ",";
    out += "\"status\":" + serializeString(models::statusToString(q.getStatus())) + ",";
    out += "\"is_favorite\":" + serializeBool(q.isFavorite()) + ",";
    out += "\"notes\":" + serializeString(q.getNotes()) + ",";
    out += "\"created_at\":" + serializeInt(q.getCreatedAt()) + ",";
    out += "\"updated_at\":" + serializeInt(q.getUpdatedAt()) + ",";
    out += "\"last_practiced_at\":" + serializeInt(q.getLastPracticedAt()) + ",";
    out += "\"next_revision_at\":" + serializeInt(q.getNextRevisionAt()) + ",";
    out += "\"revision_priority\":" + serializeInt(q.getRevisionPriority()) + ",";
    out += "\"tags\":" + serializeStringArray(q.getTags()) + ",";
    out += "\"owner_id\":" + serializeString(q.getOwnerId());
    out += "}";
    return out;
}

std::string questionsToJson(const std::vector<models::Question>& questions) {
    std::string out = "[";
    for (size_t i = 0; i < questions.size(); ++i) {
        if (i > 0) out += ",";
        out += questionToJson(questions[i]);
    }
    out += "]";
    return out;
}

std::string dashboardSnapshotToJson(const models::DashboardSnapshot& s) {
    std::string out = "{";
    out += "\"generatedAt\":" + serializeInt(s.generatedAt) + ",";

    // Overall
    out += "\"overall\":{";
    out += "\"totalQuestions\":" + serializeInt(static_cast<int64_t>(s.overall.totalQuestions)) + ",";
    out += "\"solvedCount\":" + serializeInt(static_cast<int64_t>(s.overall.solvedCount)) + ",";
    out += "\"inProgressCount\":" + serializeInt(static_cast<int64_t>(s.overall.inProgressCount)) + ",";
    out += "\"unsolvedCount\":" + serializeInt(static_cast<int64_t>(s.overall.unsolvedCount)) + ",";
    out += "\"masteredCount\":" + serializeInt(static_cast<int64_t>(s.overall.masteredCount)) + ",";
    out += "\"favoriteCount\":" + serializeInt(static_cast<int64_t>(s.overall.favoriteCount)) + ",";
    out += "\"dueForRevisionCount\":" + serializeInt(static_cast<int64_t>(s.overall.dueForRevisionCount)) + ",";
    out += "\"upcomingRevisionsCount\":" + serializeInt(static_cast<int64_t>(s.overall.upcomingRevisionsCount)) + ",";
    out += "\"totalScheduledCount\":" + serializeInt(static_cast<int64_t>(s.overall.totalScheduledCount)) + ",";
    out += "\"completionPercentage\":" + serializeDouble(s.overall.completionPercentage) + ",";
    out += "\"solvedPercentage\":" + serializeDouble(s.overall.solvedPercentage) + ",";
    out += "\"masteredPercentage\":" + serializeDouble(s.overall.masteredPercentage) + ",";
    out += "\"inProgressPercentage\":" + serializeDouble(s.overall.inProgressPercentage) + ",";
    out += "\"unsolvedPercentage\":" + serializeDouble(s.overall.unsolvedPercentage) + ",";
    out += "\"favoritePercentage\":" + serializeDouble(s.overall.favoritePercentage);
    out += "},";

    // Difficulty
    out += "\"difficulty\":{";
    out += "\"easyCount\":" + serializeInt(static_cast<int64_t>(s.difficulty.easyCount)) + ",";
    out += "\"mediumCount\":" + serializeInt(static_cast<int64_t>(s.difficulty.mediumCount)) + ",";
    out += "\"hardCount\":" + serializeInt(static_cast<int64_t>(s.difficulty.hardCount)) + ",";
    out += "\"easyPercentage\":" + serializeDouble(s.difficulty.easyPercentage) + ",";
    out += "\"mediumPercentage\":" + serializeDouble(s.difficulty.mediumPercentage) + ",";
    out += "\"hardPercentage\":" + serializeDouble(s.difficulty.hardPercentage) + ",";
    out += "\"totalQuestions\":" + serializeInt(static_cast<int64_t>(s.difficulty.totalQuestions));
    out += "},";

    // Topic
    out += "\"topic\":{";
    out += "\"totalQuestions\":" + serializeInt(static_cast<int64_t>(s.topic.totalQuestions)) + ",";
    out += "\"distinctTopicsCount\":" + serializeInt(static_cast<int64_t>(s.topic.distinctTopicsCount)) + ",";
    out += "\"topicCounts\":[";
    for (size_t i = 0; i < s.topic.topicCounts.size(); ++i) {
        if (i > 0) out += ",";
        const auto& tc = s.topic.topicCounts[i];
        out += "{";
        out += "\"topic\":" + serializeString(models::topicToString(tc.topic)) + ",";
        out += "\"topicName\":" + serializeString(tc.topicName) + ",";
        out += "\"count\":" + serializeInt(static_cast<int64_t>(tc.count)) + ",";
        out += "\"percentage\":" + serializeDouble(tc.percentage);
        out += "}";
    }
    out += "]},";

    // Status
    out += "\"status\":{";
    out += "\"unsolvedCount\":" + serializeInt(static_cast<int64_t>(s.status.unsolvedCount)) + ",";
    out += "\"inProgressCount\":" + serializeInt(static_cast<int64_t>(s.status.inProgressCount)) + ",";
    out += "\"solvedCount\":" + serializeInt(static_cast<int64_t>(s.status.solvedCount)) + ",";
    out += "\"masteredCount\":" + serializeInt(static_cast<int64_t>(s.status.masteredCount)) + ",";
    out += "\"unsolvedPercentage\":" + serializeDouble(s.status.unsolvedPercentage) + ",";
    out += "\"inProgressPercentage\":" + serializeDouble(s.status.inProgressPercentage) + ",";
    out += "\"solvedPercentage\":" + serializeDouble(s.status.solvedPercentage) + ",";
    out += "\"masteredPercentage\":" + serializeDouble(s.status.masteredPercentage);
    out += "},";

    // Revision
    out += "\"revision\":{";
    out += "\"dueCount\":" + serializeInt(static_cast<int64_t>(s.revision.dueCount)) + ",";
    out += "\"upcomingCount\":" + serializeInt(static_cast<int64_t>(s.revision.upcomingCount)) + ",";
    out += "\"scheduledCount\":" + serializeInt(static_cast<int64_t>(s.revision.scheduledCount)) + ",";
    out += "\"unscheduledCount\":" + serializeInt(static_cast<int64_t>(s.revision.unscheduledCount)) + ",";
    out += "\"duePercentageOfScheduled\":" + serializeDouble(s.revision.duePercentageOfScheduled) + ",";
    out += "\"scheduledPercentageOfTotal\":" + serializeDouble(s.revision.scheduledPercentageOfTotal) + ",";
    out += "\"priorityCounts\":[";
    for (int p = 0; p <= 5; ++p) {
        if (p > 0) out += ",";
        out += serializeInt(static_cast<int64_t>(s.revision.priorityCounts[p]));
    }
    out += "],";
    out += "\"levelCounts\":[";
    for (int l = 0; l <= 5; ++l) {
        if (l > 0) out += ",";
        out += serializeInt(static_cast<int64_t>(s.revision.levelCounts[l]));
    }
    out += "]},";

    // Practice
    out += "\"practice\":{";
    out += "\"practicedCount\":" + serializeInt(static_cast<int64_t>(s.practice.practicedCount)) + ",";
    out += "\"unpracticedCount\":" + serializeInt(static_cast<int64_t>(s.practice.unpracticedCount)) + ",";
    out += "\"practicedPercentage\":" + serializeDouble(s.practice.practicedPercentage) + ",";
    out += "\"lastPracticedTimestamp\":" + serializeInt(s.practice.lastPracticedTimestamp);
    out += "}";

    out += "}";
    return out;
}

std::string revisionItemToJson(const models::RevisionItem& item) {
    std::string out = "{";
    out += "\"questionId\":" + serializeString(item.questionId) + ",";
    out += "\"nextRevisionAt\":" + serializeInt(item.nextRevisionAt) + ",";
    out += "\"priority\":" + serializeInt(item.priority);
    out += "}";
    return out;
}

std::string revisionItemsToJson(const std::vector<models::RevisionItem>& items) {
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out += ",";
        out += revisionItemToJson(items[i]);
    }
    out += "]";
    return out;
}

std::string sessionProgressToJson(const services::SessionProgress& p) {
    std::string out = "{";
    out += "\"total\":" + serializeInt(static_cast<int64_t>(p.total)) + ",";
    out += "\"completed\":" + serializeInt(static_cast<int64_t>(p.completed)) + ",";
    out += "\"remaining\":" + serializeInt(static_cast<int64_t>(p.remaining)) + ",";
    out += "\"skipped\":" + serializeInt(static_cast<int64_t>(p.skipped));
    out += "}";
    return out;
}

std::string revisionScheduleResultToJson(const services::RevisionScheduleResult& r) {
    std::string out = "{";
    out += "\"nextLevel\":" + serializeInt(r.nextLevel) + ",";
    out += "\"nextRevisionAt\":" + serializeInt(r.nextRevisionAt) + ",";
    out += "\"revisionPriority\":" + serializeInt(r.revisionPriority) + ",";
    out += "\"lastPracticedAt\":" + serializeInt(r.lastPracticedAt) + ",";
    out += "\"newStatus\":" + serializeString(models::statusToString(r.newStatus)) + ",";
    out += "\"intervalSeconds\":" + serializeInt(r.intervalSeconds);
    out += "}";
    return out;
}

std::string practiceNextResultToJson(const models::PracticeNextResult& r) {
    std::string out = "{";
    out += "\"hasQuestion\":" + serializeBool(r.hasQuestion) + ",";
    if (r.hasQuestion && r.question.has_value()) {
        out += "\"question\":" + questionToJson(r.question.value()) + ",";
    } else {
        out += "\"question\":null,";
    }
    out += "\"recommendationReason\":" + serializeString(r.recommendationReason);
    out += "}";
    return out;
}

// ----------------------------------------------------------------------------
// JSON Parser Implementation
// ----------------------------------------------------------------------------

namespace {

void skipWhitespace(const std::string& s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
        ++i;
    }
}

std::string parseString(const std::string& s, size_t& i) {
    if (i >= s.size() || s[i] != '"') return "";
    ++i; // skip open quote
    std::string res;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') {
            return res;
        }
        if (c == '\\' && i < s.size()) {
            char next = s[i++];
            switch (next) {
                case '"': res += '"'; break;
                case '\\': res += '\\'; break;
                case '/': res += '/'; break;
                case 'b': res += '\b'; break;
                case 'f': res += '\f'; break;
                case 'n': res += '\n'; break;
                case 'r': res += '\r'; break;
                case 't': res += '\t'; break;
                case 'u':
                    if (i + 4 <= s.size()) {
                        std::string hexStr = s.substr(i, 4);
                        i += 4;
                        try {
                            int code = std::stoi(hexStr, nullptr, 16);
                            if (code < 128) res += static_cast<char>(code);
                        } catch (...) {}
                    }
                    break;
                default: res += next; break;
            }
        } else {
            res += c;
        }
    }
    return res;
}

JsonValue parseValue(const std::string& s, size_t& i);

JsonValue parseObject(const std::string& s, size_t& i) {
    JsonValue val;
    val.type = JsonValue::Type::Object;
    if (i >= s.size() || s[i] != '{') return val;
    ++i; // skip '{'

    skipWhitespace(s, i);
    if (i < s.size() && s[i] == '}') {
        ++i;
        return val;
    }

    while (i < s.size()) {
        skipWhitespace(s, i);
        if (i >= s.size() || s[i] != '"') break;
        std::string key = parseString(s, i);
        skipWhitespace(s, i);
        if (i >= s.size() || s[i] != ':') break;
        ++i; // skip ':'
        skipWhitespace(s, i);
        JsonValue member = parseValue(s, i);
        val.objectValue[key] = std::move(member);

        skipWhitespace(s, i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        if (i < s.size() && s[i] == '}') {
            ++i;
            break;
        }
    }
    return val;
}

JsonValue parseArray(const std::string& s, size_t& i) {
    JsonValue val;
    val.type = JsonValue::Type::Array;
    if (i >= s.size() || s[i] != '[') return val;
    ++i; // skip '['

    skipWhitespace(s, i);
    if (i < s.size() && s[i] == ']') {
        ++i;
        return val;
    }

    while (i < s.size()) {
        skipWhitespace(s, i);
        JsonValue elem = parseValue(s, i);
        val.arrayValue.push_back(std::move(elem));

        skipWhitespace(s, i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        if (i < s.size() && s[i] == ']') {
            ++i;
            break;
        }
    }
    return val;
}

JsonValue parseValue(const std::string& s, size_t& i) {
    skipWhitespace(s, i);
    if (i >= s.size()) return JsonValue{};

    if (s[i] == '{') return parseObject(s, i);
    if (s[i] == '[') return parseArray(s, i);
    if (s[i] == '"') {
        JsonValue val;
        val.type = JsonValue::Type::String;
        val.stringValue = parseString(s, i);
        return val;
    }
    if (s[i] == 't' || s[i] == 'f') {
        JsonValue val;
        val.type = JsonValue::Type::Boolean;
        if (s.substr(i, 4) == "true") {
            val.boolValue = true;
            i += 4;
        } else if (s.substr(i, 5) == "false") {
            val.boolValue = false;
            i += 5;
        }
        return val;
    }
    if (s[i] == 'n' && s.substr(i, 4) == "null") {
        i += 4;
        return JsonValue{};
    }

    // Number
    size_t start = i;
    if (s[i] == '-') ++i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) {
        ++i;
    }
    std::string numStr = s.substr(start, i - start);
    JsonValue val;
    val.type = JsonValue::Type::Number;
    try {
        if (numStr.find('.') != std::string::npos || numStr.find('e') != std::string::npos || numStr.find('E') != std::string::npos) {
            val.doubleValue = std::stod(numStr);
            val.intValue = static_cast<int64_t>(val.doubleValue);
        } else {
            val.intValue = std::stoll(numStr);
            val.doubleValue = static_cast<double>(val.intValue);
        }
    } catch (...) {
        val.intValue = 0;
        val.doubleValue = 0.0;
    }
    return val;
}

} // anonymous namespace

std::string JsonValue::getString(const std::string& key, const std::string& defaultVal) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end()) {
        if (it->second.isString()) return it->second.stringValue;
        if (it->second.isNumber()) return std::to_string(it->second.intValue);
        if (it->second.isBool()) return it->second.boolValue ? "true" : "false";
    }
    return defaultVal;
}

int64_t JsonValue::getInt(const std::string& key, int64_t defaultVal) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end() && it->second.isNumber()) {
        return it->second.intValue;
    }
    return defaultVal;
}

double JsonValue::getDouble(const std::string& key, double defaultVal) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end() && it->second.isNumber()) {
        return it->second.doubleValue;
    }
    return defaultVal;
}

bool JsonValue::getBool(const std::string& key, bool defaultVal) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end() && it->second.isBool()) {
        return it->second.boolValue;
    }
    return defaultVal;
}

std::vector<std::string> JsonValue::getStringArray(const std::string& key) const {
    std::vector<std::string> res;
    auto it = objectValue.find(key);
    if (it != objectValue.end() && it->second.isArray()) {
        for (const auto& elem : it->second.arrayValue) {
            if (elem.isString()) res.push_back(elem.stringValue);
        }
    }
    return res;
}

const JsonValue* JsonValue::get(const std::string& key) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end()) {
        return &it->second;
    }
    return nullptr;
}

JsonValue parse(const std::string& jsonStr) {
    size_t i = 0;
    return parseValue(jsonStr, i);
}

bool jsonToQuestion(const JsonValue& root, models::Question& out, std::string& errorMessage) {
    if (!root.isObject()) {
        errorMessage = "Expected JSON object";
        return false;
    }

    std::string title = root.getString("title");
    if (title.empty()) {
        errorMessage = "Question title cannot be empty";
        return false;
    }

    std::string id = root.getString("id");
    std::string desc = root.getString("description");
    std::string topicStr = root.getString("topic", "Other");
    std::string diffStr = root.getString("difficulty", "Medium");
    std::string company = root.getString("company");
    std::string platformStr = root.getString("platform", "Custom");
    std::string sourceUrl = root.getString("source_url");
    std::string statusStr = root.getString("status", "Unsolved");
    bool isFav = root.getBool("is_favorite", false);
    std::string notes = root.getString("notes");
    int64_t createdAt = root.getInt("created_at", 0);
    int64_t updatedAt = root.getInt("updated_at", 0);
    int64_t lastPracticedAt = root.getInt("last_practiced_at", 0);
    int64_t nextRevisionAt = root.getInt("next_revision_at", 0);
    int32_t priority = static_cast<int32_t>(root.getInt("revision_priority", 2));
    if (priority < 1 || priority > 5) priority = 2;
    std::vector<std::string> tags = root.getStringArray("tags");
    std::string ownerId = root.getString("owner_id", "local_user");

    out = models::Question(
        id,
        title,
        desc,
        models::stringToTopic(topicStr),
        models::stringToDifficulty(diffStr),
        company,
        models::stringToPlatform(platformStr),
        sourceUrl,
        models::stringToStatus(statusStr),
        isFav,
        notes,
        createdAt,
        updatedAt,
        lastPracticedAt,
        nextRevisionAt,
        priority,
        tags,
        ownerId
    );

    return true;
}

} // namespace codevault::utils::json
