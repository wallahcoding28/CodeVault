#include "services/export_service.hpp"
#include "utils/json.hpp"

#include <chrono>
#include <sstream>

namespace codevault::services {

namespace {

std::string escapeCsvField(const std::string& field) {
    bool needsQuotes = field.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuotes) return field;
    std::string esc = "\"";
    for (char c : field) {
        if (c == '"') esc += "\"\"";
        else esc += c;
    }
    esc += "\"";
    return esc;
}

std::string sanitizeAnkiField(const std::string& text) {
    std::string res;
    res.reserve(text.size() + 16);
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\t') {
            res += "    "; // Replace tabs with 4 spaces to prevent column shift
        } else if (c == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            res += "<br>";
        } else if (c == '\n') {
            res += "<br>";
        } else {
            res += c;
        }
    }
    return res;
}

} // namespace

std::string ExportService::exportToJson(const std::vector<models::Question>& questions, int64_t exportedAtEpoch) {
    int64_t epoch = exportedAtEpoch;
    if (epoch <= 0) {
        epoch = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    std::ostringstream ss;
    ss << "{\"questions\":" << utils::json::questionsToJson(questions)
       << ",\"exportedAt\":" << epoch
       << ",\"totalCount\":" << questions.size()
       << "}";
    return ss.str();
}

std::string ExportService::exportToCsv(const std::vector<models::Question>& questions) {
    std::ostringstream ss;
    ss << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\r\n";

    for (const auto& q : questions) {
        std::string tagsStr;
        const auto& tags = q.getTags();
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i > 0) tagsStr += ";";
            tagsStr += tags[i];
        }

        ss << escapeCsvField(q.getId()) << ","
           << escapeCsvField(q.getTitle()) << ","
           << escapeCsvField(q.getDescription()) << ","
           << escapeCsvField(models::topicToString(q.getTopic())) << ","
           << escapeCsvField(models::difficultyToString(q.getDifficulty())) << ","
           << escapeCsvField(q.getCompany()) << ","
           << escapeCsvField(models::platformToString(q.getPlatform())) << ","
           << escapeCsvField(q.getSourceUrl()) << ","
           << escapeCsvField(models::statusToString(q.getStatus())) << ","
           << (q.isFavorite() ? "1" : "0") << ","
           << escapeCsvField(q.getNotes()) << ","
           << q.getCreatedAt() << ","
           << q.getUpdatedAt() << ","
           << q.getLastPracticedAt() << ","
           << q.getNextRevisionAt() << ","
           << q.getRevisionPriority() << ","
           << escapeCsvField(tagsStr) << ","
           << escapeCsvField(q.getOwnerId()) << "\r\n";
    }

    return ss.str();
}

std::string ExportService::exportToMarkdown(const std::vector<models::Question>& questions, int64_t exportedAtEpoch) {
    int64_t epoch = exportedAtEpoch;
    if (epoch <= 0) {
        epoch = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    std::ostringstream ss;
    ss << "# CodeVault Problem Catalog Export\n\n";
    ss << "> Total Problems: " << questions.size() << " | Exported At: " << epoch << "\n\n";

    // Table of contents / summary table
    ss << "## Problem Index\n\n";
    ss << "| ID | Title | Topic | Difficulty | Status | Platform |\n";
    ss << "| :--- | :--- | :--- | :--- | :--- | :--- |\n";

    for (const auto& q : questions) {
        auto sanitizeCell = [](const std::string& str) -> std::string {
            std::string out;
            for (char c : str) {
                if (c == '|') out += "&#124;";
                else if (c == '\n' || c == '\r') out += " ";
                else out += c;
            }
            return out;
        };

        ss << "| " << sanitizeCell(q.getId())
           << " | " << sanitizeCell(q.getTitle())
           << " | " << models::topicToString(q.getTopic())
           << " | " << models::difficultyToString(q.getDifficulty())
           << " | " << models::statusToString(q.getStatus())
           << " | " << models::platformToString(q.getPlatform())
           << " |\n";
    }

    ss << "\n---\n\n";
    ss << "## Problem Details\n\n";

    for (size_t i = 0; i < questions.size(); ++i) {
        const auto& q = questions[i];
        ss << "### " << q.getId() << " — " << q.getTitle() << "\n\n";
        ss << "- **Topic**: " << models::topicToString(q.getTopic()) << "\n";
        ss << "- **Difficulty**: " << models::difficultyToString(q.getDifficulty()) << "\n";
        ss << "- **Status**: " << models::statusToString(q.getStatus()) << "\n";
        ss << "- **Platform**: " << models::platformToString(q.getPlatform()) << "\n";
        ss << "- **Company**: " << (q.getCompany().empty() ? "-" : q.getCompany()) << "\n";

        if (!q.getSourceUrl().empty()) {
            ss << "- **Source URL**: [" << q.getSourceUrl() << "](" << q.getSourceUrl() << ")\n";
        } else {
            ss << "- **Source URL**: -\n";
        }

        const auto& tags = q.getTags();
        if (!tags.empty()) {
            ss << "- **Tags**: ";
            for (size_t t = 0; t < tags.size(); ++t) {
                if (t > 0) ss << ", ";
                ss << tags[t];
            }
            ss << "\n";
        } else {
            ss << "- **Tags**: None\n";
        }

        ss << "- **Favorite**: " << (q.isFavorite() ? "Yes" : "No") << "\n";
        ss << "- **Revision Priority**: " << q.getRevisionPriority() << "\n\n";

        ss << "#### Description\n\n";
        if (!q.getDescription().empty()) {
            ss << q.getDescription() << "\n\n";
        } else {
            ss << "*(No description provided)*\n\n";
        }

        ss << "#### Notes\n\n";
        if (!q.getNotes().empty()) {
            ss << q.getNotes() << "\n\n";
        } else {
            ss << "*(No notes provided)*\n\n";
        }

        if (i + 1 < questions.size()) {
            ss << "---\n\n";
        }
    }

    return ss.str();
}

std::string ExportService::exportToAnkiTsv(const std::vector<models::Question>& questions) {
    std::ostringstream ss;

    for (const auto& q : questions) {
        // Column 1: Front
        std::string front = q.getTitle() + "<br><small><b>Topic:</b> " + models::topicToString(q.getTopic())
                          + " | <b>Difficulty:</b> " + models::difficultyToString(q.getDifficulty())
                          + " | <b>Platform:</b> " + models::platformToString(q.getPlatform()) + "</small>";

        // Column 2: Back
        std::string back;
        if (!q.getDescription().empty()) {
            back += "<b>Description:</b><br>" + sanitizeAnkiField(q.getDescription()) + "<br><br>";
        }
        if (!q.getNotes().empty()) {
            back += "<b>Notes:</b><br>" + sanitizeAnkiField(q.getNotes());
        }
        if (!q.getSourceUrl().empty()) {
            if (!back.empty()) back += "<br><br>";
            back += "<b>Source:</b> <a href=\"" + q.getSourceUrl() + "\">" + q.getSourceUrl() + "</a>";
        }
        if (back.empty()) {
            back = "*(No notes or description recorded)*";
        }

        // Column 3: Tags (space-separated, replacing spaces within tags with hyphens)
        std::string tags;
        auto addTag = [&](const std::string& raw) {
            if (raw.empty()) return;
            std::string clean;
            for (char c : raw) {
                if (c == ' ' || c == '\t' || c == ';') clean += "-";
                else clean += c;
            }
            if (!clean.empty()) {
                if (!tags.empty()) tags += " ";
                tags += clean;
            }
        };

        addTag(models::topicToString(q.getTopic()));
        addTag(models::difficultyToString(q.getDifficulty()));
        addTag(models::platformToString(q.getPlatform()));
        addTag(models::statusToString(q.getStatus()));
        for (const auto& t : q.getTags()) {
            addTag(t);
        }

        ss << sanitizeAnkiField(front) << "\t"
           << sanitizeAnkiField(back) << "\t"
           << sanitizeAnkiField(tags) << "\n";
    }

    return ss.str();
}

} // namespace codevault::services
