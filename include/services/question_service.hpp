#pragma once

#include "models/enums.hpp"
#include "models/question.hpp"
#include "persistence/iquestion_repository.hpp"
#include "services/current_user_provider.hpp"
#include "services/search_service.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace codevault::services {

class RevisionService;

/**
 * @brief Represents the outcome of an entity validation check.
 */
struct ValidationResult {
    bool isValid{true};
    std::string errorMessage;

    static ValidationResult success() {
        return {true, ""};
    }

    static ValidationResult failure(std::string message) {
        return {false, std::move(message)};
    }
};

/**
 * @brief Application service orchestrating operations, business rules, and validation on questions.
 */
class QuestionService {
public:
    explicit QuestionService(
        std::shared_ptr<persistence::IQuestionRepository> repository,
        std::shared_ptr<SearchService> searchService = nullptr,
        std::shared_ptr<RevisionService> revisionService = nullptr,
        std::shared_ptr<ICurrentUserProvider> currentUserProvider = nullptr);

    /**
     * @brief Retrieve all questions currently tracked.
     */
    std::vector<models::Question> getAllQuestions() const;

    /**
     * @brief Look up a single question by ID.
     */
    std::optional<models::Question> getQuestionById(const std::string& id) const;

    /**
     * @brief Check whether a question with the given ID exists.
     */
    bool questionExists(const std::string& id) const;

    /**
     * @brief Validate question attributes against business rules.
     */
    ValidationResult validateQuestion(const models::Question& question, bool isCreating = false) const;

    /**
     * @brief Create and persist a new question.
     */
    ValidationResult createQuestion(const models::Question& question);

    /**
     * @brief Update an existing persisted question.
     */
    ValidationResult updateQuestion(const models::Question& question);

    /**
     * @brief Delete a question by ID.
     */
    bool deleteQuestion(const std::string& id);

    /**
     * @brief Legacy save method for backward compatibility.
     */
    bool saveQuestion(const models::Question& question);

    /**
     * @brief Count total questions.
     */
    size_t getQuestionCount() const;

    /**
     * @brief Count questions by difficulty.
     */
    size_t countByDifficulty(models::Difficulty difficulty) const;

    /**
     * @brief Count questions by status.
     */
    size_t countByStatus(models::Status status) const;

    /**
     * @brief Generate next sequential question ID (e.g. Q-1010).
     */
     std::string generateNextId() const;

    /**
     * @brief Access the search and index service.
     */
    std::shared_ptr<SearchService> getSearchService() const noexcept {
        return searchService_;
    }

    /**
     * @brief Access the revision service if configured.
     */
    std::shared_ptr<RevisionService> getRevisionService() const noexcept {
        return revisionService_;
    }

    /**
     * @brief Access the underlying repository.
     */
    std::shared_ptr<persistence::IQuestionRepository> getRepository() const noexcept {
        return repository_;
    }

    /**
     * @brief Search questions whose titles match a prefix using the Trie index.
     */
    std::vector<models::Question> searchQuestionsByTitlePrefix(const std::string& prefix) const;

    // --- Multi-User Scoping Operations ---
    /**
     * @brief Retrieve ID of active owner context.
     */
    std::string getCurrentUserId() const;

    /**
     * @brief Switch or set active user provider.
     */
    void setCurrentUserProvider(std::shared_ptr<ICurrentUserProvider> provider);

    /**
     * @brief Rebuild in-memory search and revision indexes for the active user.
     */
    void refreshUserScope();

    /**
     * @brief Retrieve all questions across all owners (unscoped administrative lookup).
     */
    std::vector<models::Question> getAllQuestionsUnscoped() const;

    /**
     * @brief Look up a question by ID across all owners (unscoped administrative lookup).
     */
    std::optional<models::Question> getQuestionByIdUnscoped(const std::string& id) const;

private:
    std::shared_ptr<persistence::IQuestionRepository> repository_;
    std::shared_ptr<SearchService> searchService_;
    std::shared_ptr<RevisionService> revisionService_;
    std::shared_ptr<ICurrentUserProvider> currentUserProvider_;
};

} // namespace codevault::services
