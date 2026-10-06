#pragma once

#include "models/question.hpp"

#include <optional>
#include <string>
#include <vector>

namespace codevault::persistence {

/**
 * @brief Abstract repository contract decoupling storage mechanism from application services.
 */
class IQuestionRepository {
public:
    virtual ~IQuestionRepository() = default;

    /**
     * @brief Load and return all questions.
     */
    virtual std::vector<models::Question> findAll() const = 0;

    /**
     * @brief Find a specific question by unique identifier.
     */
    virtual std::optional<models::Question> findById(const std::string& id) const = 0;

    /**
     * @brief Check whether a question with the given ID exists.
     */
    virtual bool exists(const std::string& id) const = 0;

    /**
     * @brief Persist or update a question record.
     */
    virtual bool save(const models::Question& question) = 0;

    /**
     * @brief Remove a question by unique identifier.
     */
    virtual bool remove(const std::string& id) = 0;

    /**
     * @brief Count total questions currently stored.
     */
    virtual size_t count() const = 0;

    // --- Owner-Scoped Operations ---
    /**
     * @brief Load and return all questions owned by a specific user.
     */
    virtual std::vector<models::Question> findAllByOwner(const std::string& ownerId) const = 0;

    /**
     * @brief Find a question by ID ensuring it belongs to the given owner.
     */
    virtual std::optional<models::Question> findByIdForOwner(const std::string& id, const std::string& ownerId) const = 0;

    /**
     * @brief Check whether a question with the given ID exists for the specified owner.
     */
    virtual bool existsForOwner(const std::string& id, const std::string& ownerId) const = 0;

    /**
     * @brief Persist or update a question record enforcing ownership.
     */
    virtual bool saveForOwner(const models::Question& question, const std::string& ownerId) = 0;

    /**
     * @brief Remove a question by ID ensuring it belongs to the given owner.
     */
    virtual bool removeForOwner(const std::string& id, const std::string& ownerId) = 0;

    /**
     * @brief Count total questions owned by a specific user.
     */
    virtual size_t countForOwner(const std::string& ownerId) const = 0;
};

} // namespace codevault::persistence
