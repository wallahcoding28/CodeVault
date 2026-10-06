#include "services/question_service.hpp"
#include "services/revision_service.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace codevault::services {

static std::string trimString(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

QuestionService::QuestionService(
    std::shared_ptr<persistence::IQuestionRepository> repository,
    std::shared_ptr<SearchService> searchService,
    std::shared_ptr<RevisionService> revisionService,
    std::shared_ptr<ICurrentUserProvider> currentUserProvider)
    : repository_(std::move(repository)),
      searchService_(std::move(searchService)),
      revisionService_(std::move(revisionService)),
      currentUserProvider_(std::move(currentUserProvider)) {
    if (!currentUserProvider_) {
        currentUserProvider_ = std::make_shared<StaticCurrentUserProvider>("local_user");
    }
    if (!searchService_) {
        searchService_ = std::make_shared<SearchService>();
    }
    refreshUserScope();
}

std::string QuestionService::getCurrentUserId() const {
    if (currentUserProvider_) {
        return currentUserProvider_->getCurrentUserId();
    }
    return "local_user";
}

void QuestionService::setCurrentUserProvider(std::shared_ptr<ICurrentUserProvider> provider) {
    currentUserProvider_ = std::move(provider);
    if (!currentUserProvider_) {
        currentUserProvider_ = std::make_shared<StaticCurrentUserProvider>("local_user");
    }
    refreshUserScope();
}

void QuestionService::refreshUserScope() {
    if (!repository_) return;
    auto questions = repository_->findAllByOwner(getCurrentUserId());
    if (searchService_) {
        searchService_->rebuildIndex(questions);
    }
    if (revisionService_) {
        revisionService_->loadFromQuestions(questions);
    }
}

std::vector<models::Question> QuestionService::getAllQuestions() const {
    if (!repository_) return {};
    return repository_->findAllByOwner(getCurrentUserId());
}

std::optional<models::Question> QuestionService::getQuestionById(const std::string& id) const {
    if (!repository_) return std::nullopt;
    return repository_->findByIdForOwner(id, getCurrentUserId());
}

bool QuestionService::questionExists(const std::string& id) const {
    if (!repository_) return false;
    return repository_->existsForOwner(id, getCurrentUserId());
}

std::vector<models::Question> QuestionService::getAllQuestionsUnscoped() const {
    if (!repository_) return {};
    return repository_->findAll();
}

std::optional<models::Question> QuestionService::getQuestionByIdUnscoped(const std::string& id) const {
    if (!repository_) return std::nullopt;
    return repository_->findById(id);
}

ValidationResult QuestionService::validateQuestion(const models::Question& question, bool isCreating) const {
    std::string id = trimString(question.getId());
    if (id.empty()) {
        return ValidationResult::failure("Question ID cannot be empty.");
    }

    for (char ch : id) {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '-' && ch != '_') {
            return ValidationResult::failure("Question ID contains invalid characters. Only alphanumeric, '-', and '_' are allowed.");
        }
    }

    if (isCreating && repository_ && repository_->exists(id)) {
        return ValidationResult::failure("Question with ID '" + id + "' already exists.");
    }

    std::string title = trimString(question.getTitle());
    if (title.empty()) {
        return ValidationResult::failure("Question title cannot be empty.");
    }
    if (title.size() > 255) {
        return ValidationResult::failure("Question title must not exceed 255 characters.");
    }

    if (question.getDifficulty() == models::Difficulty::Unknown) {
        return ValidationResult::failure("Difficulty must be specified as Easy, Medium, or Hard.");
    }

    const std::string& url = question.getSourceUrl();
    if (!url.empty()) {
        if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
            return ValidationResult::failure("Source URL must start with http:// or https://");
        }
        for (char ch : url) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                return ValidationResult::failure("Source URL must not contain spaces.");
            }
        }
    }

    if (question.getRevisionPriority() < 1 || question.getRevisionPriority() > 5) {
        return ValidationResult::failure("Revision priority must be between 1 (High) and 5 (Low).");
    }

    return ValidationResult::success();
}

ValidationResult QuestionService::createQuestion(const models::Question& question) {
    if (!repository_) {
        return ValidationResult::failure("Question repository is not initialized.");
    }

    auto val = validateQuestion(question, true);
    if (!val.isValid) {
        return val;
    }

    const std::string currentOwner = getCurrentUserId();
    models::Question scopedQuestion = question;
    scopedQuestion.setOwnerId(currentOwner);

    if (!repository_->saveForOwner(scopedQuestion, currentOwner)) {
        return ValidationResult::failure("Failed to persist question to repository storage.");
    }

    if (searchService_) {
        searchService_->indexQuestion(scopedQuestion);
    }

    if (revisionService_ && scopedQuestion.getNextRevisionAt() > 0) {
        revisionService_->scheduleQuestion(
            scopedQuestion.getId(),
            scopedQuestion.getNextRevisionAt(),
            scopedQuestion.getRevisionPriority()
        );
    }

    return ValidationResult::success();
}

ValidationResult QuestionService::updateQuestion(const models::Question& question) {
    if (!repository_) {
        return ValidationResult::failure("Question repository is not initialized.");
    }

    const std::string currentOwner = getCurrentUserId();
    if (!repository_->existsForOwner(question.getId(), currentOwner)) {
        return ValidationResult::failure("Question with ID '" + question.getId() + "' does not exist.");
    }

    auto val = validateQuestion(question, false);
    if (!val.isValid) {
        return val;
    }

    auto oldOpt = repository_->findByIdForOwner(question.getId(), currentOwner);
    std::string oldTitle = oldOpt.has_value() ? oldOpt->getTitle() : "";

    models::Question scopedQuestion = question;
    scopedQuestion.setOwnerId(currentOwner);

    if (!repository_->saveForOwner(scopedQuestion, currentOwner)) {
        return ValidationResult::failure("Failed to save updated question to repository storage.");
    }

    if (searchService_) {
        searchService_->updateQuestion(oldTitle, scopedQuestion);
    }

    if (revisionService_) {
        if (scopedQuestion.getNextRevisionAt() > 0) {
            revisionService_->rescheduleQuestion(
                scopedQuestion.getId(),
                scopedQuestion.getNextRevisionAt(),
                scopedQuestion.getRevisionPriority()
            );
        } else {
            revisionService_->removeQuestion(scopedQuestion.getId());
        }
    }

    return ValidationResult::success();
}

bool QuestionService::deleteQuestion(const std::string& id) {
    if (!repository_) return false;
    const std::string currentOwner = getCurrentUserId();
    if (!repository_->existsForOwner(id, currentOwner)) return false;
    auto oldOpt = repository_->findByIdForOwner(id, currentOwner);

    if (!repository_->removeForOwner(id, currentOwner)) {
        return false;
    }

    if (searchService_ && oldOpt.has_value()) {
        searchService_->removeQuestion(oldOpt.value());
    }

    if (revisionService_) {
        revisionService_->removeQuestion(id);
    }
    return true;
}

bool QuestionService::saveQuestion(const models::Question& question) {
    if (!repository_) return false;
    if (repository_->existsForOwner(question.getId(), getCurrentUserId())) {
        return updateQuestion(question).isValid;
    }
    return createQuestion(question).isValid;
}

std::vector<models::Question> QuestionService::searchQuestionsByTitlePrefix(const std::string& prefix) const {
    if (!searchService_) return {};
    auto ids = searchService_->searchQuestionIdsByPrefix(prefix);
    std::vector<models::Question> results;
    results.reserve(ids.size());
    for (const auto& id : ids) {
        auto qOpt = getQuestionById(id);
        if (qOpt.has_value()) {
            results.push_back(qOpt.value());
        }
    }
    return results;
}

size_t QuestionService::getQuestionCount() const {
    if (!repository_) return 0;
    return repository_->countForOwner(getCurrentUserId());
}

size_t QuestionService::countByDifficulty(models::Difficulty difficulty) const {
    if (!repository_) return 0;
    const auto questions = repository_->findAllByOwner(getCurrentUserId());
    return std::count_if(questions.begin(), questions.end(), [difficulty](const auto& q) {
        return q.getDifficulty() == difficulty;
    });
}

size_t QuestionService::countByStatus(models::Status status) const {
    if (!repository_) return 0;
    const auto questions = repository_->findAllByOwner(getCurrentUserId());
    return std::count_if(questions.begin(), questions.end(), [status](const auto& q) {
        return q.getStatus() == status;
    });
}

std::string QuestionService::generateNextId() const {
    if (!repository_) return "Q-1001";
    // Scan all questions to avoid primary key ID collisions across users
    const auto questions = repository_->findAll();
    int64_t maxNum = 1000;

    for (const auto& q : questions) {
        const std::string& qid = q.getId();
        if (qid.rfind("Q-", 0) == 0 && qid.size() > 2) {
            try {
                size_t idx = 0;
                int64_t num = std::stoll(qid.substr(2), &idx);
                if (idx == qid.size() - 2) {
                    maxNum = std::max(maxNum, num);
                }
            } catch (...) {
                // Ignore non-numeric suffix
            }
        }
    }

    return "Q-" + std::to_string(maxNum + 1);
}

} // namespace codevault::services
