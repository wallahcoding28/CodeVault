#pragma once

#include "persistence/iquestion_repository.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace codevault::persistence {

/**
 * @brief Local file-based implementation of IQuestionRepository supporting CSV formatted data.
 */
class FileQuestionRepository : public IQuestionRepository {
public:
    explicit FileQuestionRepository(std::string filePath);

    std::vector<models::Question> findAll() const override;
    std::optional<models::Question> findById(const std::string& id) const override;
    bool exists(const std::string& id) const override;
    bool save(const models::Question& question) override;
    bool remove(const std::string& id) override;
    size_t count() const override;

    // --- Owner-Scoped Operations ---
    std::vector<models::Question> findAllByOwner(const std::string& ownerId) const override;
    std::optional<models::Question> findByIdForOwner(const std::string& id, const std::string& ownerId) const override;
    bool existsForOwner(const std::string& id, const std::string& ownerId) const override;
    bool saveForOwner(const models::Question& question, const std::string& ownerId) override;
    bool removeForOwner(const std::string& id, const std::string& ownerId) override;
    size_t countForOwner(const std::string& ownerId) const override;

    /**
     * @brief Reload cache from disk file.
     */
    bool reload();

    /**
     * @brief Return the underlying file path.
     */
    const std::string& getFilePath() const noexcept { return filePath_; }

    /**
     * @brief Explicitly flush in-memory questions to disk file.
     */
    bool flushToFile();

private:
    std::string filePath_;
    std::unordered_map<std::string, models::Question> cacheById_;
    std::vector<std::string> orderedIds_;

    void loadFromFile();
    static std::string escapeCsvField(const std::string& field);
    static std::vector<std::string> parseCsvLine(const std::string& line);
    static models::Question parseQuestionRecord(const std::vector<std::string>& tokens);
};

} // namespace codevault::persistence
