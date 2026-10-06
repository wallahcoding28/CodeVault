#pragma once

#include "models/enums.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace codevault::models {

/**
 * @brief Represents a single algorithmic coding question.
 */
class Question {
public:
    Question() = default;

    // Primary constructor
    Question(std::string id,
             std::string title,
             std::string description,
             Topic topic,
             Difficulty difficulty,
             std::string company,
             Platform platform,
             std::string source_url,
             Status status,
             bool is_favorite,
             std::string notes,
             int64_t created_at,
             int64_t updated_at,
             int64_t last_practiced_at,
             int64_t next_revision_at,
             int32_t revision_priority,
             std::vector<std::string> tags,
             std::string owner_id = "local_user")
        : id_(std::move(id)),
          title_(std::move(title)),
          description_(std::move(description)),
          topic_(topic),
          difficulty_(difficulty),
          company_(std::move(company)),
          platform_(platform),
          source_url_(std::move(source_url)),
          status_(status),
          is_favorite_(is_favorite),
          notes_(std::move(notes)),
          created_at_(created_at),
          updated_at_(updated_at),
          last_practiced_at_(last_practiced_at),
          next_revision_at_(next_revision_at),
          revision_priority_(revision_priority),
          tags_(std::move(tags)),
          owner_id_(std::move(owner_id)) {}

    // Getters
    const std::string& getId() const noexcept { return id_; }
    const std::string& getTitle() const noexcept { return title_; }
    const std::string& getDescription() const noexcept { return description_; }
    Topic getTopic() const noexcept { return topic_; }
    Difficulty getDifficulty() const noexcept { return difficulty_; }
    const std::string& getCompany() const noexcept { return company_; }
    Platform getPlatform() const noexcept { return platform_; }
    const std::string& getSourceUrl() const noexcept { return source_url_; }
    Status getStatus() const noexcept { return status_; }
    bool isFavorite() const noexcept { return is_favorite_; }
    const std::string& getNotes() const noexcept { return notes_; }
    int64_t getCreatedAt() const noexcept { return created_at_; }
    int64_t getUpdatedAt() const noexcept { return updated_at_; }
    int64_t getLastPracticedAt() const noexcept { return last_practiced_at_; }
    int64_t getNextRevisionAt() const noexcept { return next_revision_at_; }
    int32_t getRevisionPriority() const noexcept { return revision_priority_; }
    const std::vector<std::string>& getTags() const noexcept { return tags_; }
    const std::string& getOwnerId() const noexcept { return owner_id_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setTitle(std::string title) { title_ = std::move(title); }
    void setDescription(std::string desc) { description_ = std::move(desc); }
    void setTopic(Topic topic) { topic_ = topic; }
    void setDifficulty(Difficulty diff) { difficulty_ = diff; }
    void setCompany(std::string company) { company_ = std::move(company); }
    void setPlatform(Platform plat) { platform_ = plat; }
    void setSourceUrl(std::string url) { source_url_ = std::move(url); }
    void setStatus(Status status) { status_ = status; }
    void setFavorite(bool fav) { is_favorite_ = fav; }
    void setNotes(std::string notes) { notes_ = std::move(notes); }
    void setUpdatedAt(int64_t ts) { updated_at_ = ts; }
    void setLastPracticedAt(int64_t ts) { last_practiced_at_ = ts; }
    void setNextRevisionAt(int64_t ts) { next_revision_at_ = ts; }
    void setRevisionPriority(int32_t p) { revision_priority_ = p; }
    void setTags(std::vector<std::string> tags) { tags_ = std::move(tags); }
    void setOwnerId(std::string ownerId) { owner_id_ = std::move(ownerId); }

    // Entity validation
    bool isValid() const noexcept {
        return !id_.empty() && !title_.empty();
    }

private:
    std::string id_;
    std::string title_;
    std::string description_;
    Topic topic_{Topic::Other};
    Difficulty difficulty_{Difficulty::Unknown};
    std::string company_;
    Platform platform_{Platform::Custom};
    std::string source_url_;
    Status status_{Status::Todo};
    bool is_favorite_{false};
    std::string notes_;
    int64_t created_at_{0};
    int64_t updated_at_{0};
    int64_t last_practiced_at_{0};
    int64_t next_revision_at_{0};
    int32_t revision_priority_{2};
    std::vector<std::string> tags_;
    std::string owner_id_{"local_user"};
};

} // namespace codevault::models
