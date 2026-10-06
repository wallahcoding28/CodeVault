#pragma once

#include "models/enums.hpp"
#include "models/practice_result.hpp"
#include "models/question.hpp"
#include "models/revision_item.hpp"
#include "models/statistics_models.hpp"
#include "services/practice_service.hpp"
#include "services/revision_service.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace codevault::utils::json {

// Escapes a string for JSON output
std::string escape(const std::string& input);

// Low-level simple JSON builder helpers
std::string serializeString(const std::string& val);
std::string serializeInt(int64_t val);
std::string serializeDouble(double val);
std::string serializeBool(bool val);
std::string serializeStringArray(const std::vector<std::string>& vec);

// Domain Entity Serializers
std::string questionToJson(const models::Question& q);
std::string questionsToJson(const std::vector<models::Question>& questions);
std::string dashboardSnapshotToJson(const models::DashboardSnapshot& s);
std::string revisionItemToJson(const models::RevisionItem& item);
std::string revisionItemsToJson(const std::vector<models::RevisionItem>& items);
std::string sessionProgressToJson(const services::SessionProgress& p);
std::string revisionScheduleResultToJson(const services::RevisionScheduleResult& r);
std::string practiceNextResultToJson(const models::PracticeNextResult& r);

// Simple JSON Object Parser for request payloads
class JsonValue {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    Type type{Type::Null};
    bool boolValue{false};
    int64_t intValue{0};
    double doubleValue{0.0};
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    bool isNull() const noexcept { return type == Type::Null; }
    bool isString() const noexcept { return type == Type::String; }
    bool isNumber() const noexcept { return type == Type::Number; }
    bool isBool() const noexcept { return type == Type::Boolean; }
    bool isObject() const noexcept { return type == Type::Object; }
    bool isArray() const noexcept { return type == Type::Array; }

    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    int64_t getInt(const std::string& key, int64_t defaultVal = 0) const;
    double getDouble(const std::string& key, double defaultVal = 0.0) const;
    bool getBool(const std::string& key, bool defaultVal = false) const;
    std::vector<std::string> getStringArray(const std::string& key) const;
    const JsonValue* get(const std::string& key) const;
    bool hasKey(const std::string& key) const noexcept {
        return objectValue.find(key) != objectValue.end();
    }
};

// Parse a JSON string into a JsonValue tree
JsonValue parse(const std::string& jsonStr);

// Convert a JSON object payload into a Question entity (for POST/PUT)
bool jsonToQuestion(const JsonValue& root, models::Question& out, std::string& errorMessage);

} // namespace codevault::utils::json
