#include "persistence/file_question_repository.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace codevault::persistence {

FileQuestionRepository::FileQuestionRepository(std::string filePath)
    : filePath_(std::move(filePath)) {
  loadFromFile();
}

std::vector<models::Question> FileQuestionRepository::findAll() const {
  std::vector<models::Question> result;
  result.reserve(orderedIds_.size());
  for (const auto &id : orderedIds_) {
    auto it = cacheById_.find(id);
    if (it != cacheById_.end()) {
      result.push_back(it->second);
    }
  }
  return result;
}

std::optional<models::Question>
FileQuestionRepository::findById(const std::string &id) const {
  auto it = cacheById_.find(id);
  if (it != cacheById_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool FileQuestionRepository::exists(const std::string &id) const {
  return cacheById_.find(id) != cacheById_.end();
}

bool FileQuestionRepository::save(const models::Question &question) {
  if (!question.isValid()) {
    return false;
  }
  const std::string &id = question.getId();
  if (cacheById_.find(id) == cacheById_.end()) {
    orderedIds_.push_back(id);
  }
  cacheById_[id] = question;
  return flushToFile();
}

bool FileQuestionRepository::remove(const std::string &id) {
  auto it = cacheById_.find(id);
  if (it == cacheById_.end()) {
    return false;
  }
  cacheById_.erase(it);
  orderedIds_.erase(std::remove(orderedIds_.begin(), orderedIds_.end(), id),
                    orderedIds_.end());
  return flushToFile();
}

size_t FileQuestionRepository::count() const { return cacheById_.size(); }

std::vector<models::Question> FileQuestionRepository::findAllByOwner(const std::string& ownerId) const {
  std::vector<models::Question> result;
  for (const auto& id : orderedIds_) {
    auto it = cacheById_.find(id);
    if (it != cacheById_.end() && it->second.getOwnerId() == ownerId) {
      result.push_back(it->second);
    }
  }
  return result;
}

std::optional<models::Question> FileQuestionRepository::findByIdForOwner(const std::string& id, const std::string& ownerId) const {
  auto it = cacheById_.find(id);
  if (it != cacheById_.end() && it->second.getOwnerId() == ownerId) {
    return it->second;
  }
  return std::nullopt;
}

bool FileQuestionRepository::existsForOwner(const std::string& id, const std::string& ownerId) const {
  auto it = cacheById_.find(id);
  return (it != cacheById_.end() && it->second.getOwnerId() == ownerId);
}

bool FileQuestionRepository::saveForOwner(const models::Question& question, const std::string& ownerId) {
  auto it = cacheById_.find(question.getId());
  if (it != cacheById_.end() && it->second.getOwnerId() != ownerId) {
    return false; // Cross-owner update forbidden
  }
  models::Question scoped = question;
  scoped.setOwnerId(ownerId);
  return save(scoped);
}

bool FileQuestionRepository::removeForOwner(const std::string& id, const std::string& ownerId) {
  auto it = cacheById_.find(id);
  if (it == cacheById_.end() || it->second.getOwnerId() != ownerId) {
    return false;
  }
  return remove(id);
}

size_t FileQuestionRepository::countForOwner(const std::string& ownerId) const {
  size_t c = 0;
  for (const auto& pair : cacheById_) {
    if (pair.second.getOwnerId() == ownerId) {
      ++c;
    }
  }
  return c;
}

bool FileQuestionRepository::reload() {
  cacheById_.clear();
  orderedIds_.clear();
  loadFromFile();
  return true;
}

std::string FileQuestionRepository::escapeCsvField(const std::string &field) {
  bool needsQuotes = false;
  for (char ch : field) {
    if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
      needsQuotes = true;
      break;
    }
  }
  if (!needsQuotes) {
    return field;
  }

  std::string escaped = "\"";
  for (char ch : field) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }
  escaped += "\"";
  return escaped;
}

bool FileQuestionRepository::flushToFile() {
  if (filePath_.empty()) {
    return false;
  }

  try {
    std::filesystem::path p(filePath_);
    if (p.has_parent_path()) {
      std::filesystem::create_directories(p.parent_path());
    }
  } catch (...) {
    // Proceed and let ofstream error if directory is invalid
  }

  std::ofstream file(filePath_, std::ios::out | std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  // Write standard header
  file << "id,title,description,topic,difficulty,company,platform,source_url,"
          "status,is_favorite,notes,created_at,updated_at,last_practiced_at,"
          "next_revision_at,revision_priority,tags,owner_id\n";

  for (const auto &id : orderedIds_) {
    auto it = cacheById_.find(id);
    if (it == cacheById_.end())
      continue;
    const auto &q = it->second;

    std::string tagsStr;
    const auto &tags = q.getTags();
    for (size_t i = 0; i < tags.size(); ++i) {
      tagsStr += tags[i];
      if (i + 1 < tags.size()) {
        tagsStr += ';';
      }
    }

    file << escapeCsvField(q.getId()) << "," << escapeCsvField(q.getTitle())
         << "," << escapeCsvField(q.getDescription()) << ","
         << escapeCsvField(models::topicToString(q.getTopic())) << ","
         << escapeCsvField(models::difficultyToString(q.getDifficulty())) << ","
         << escapeCsvField(q.getCompany()) << ","
         << escapeCsvField(models::platformToString(q.getPlatform())) << ","
         << escapeCsvField(q.getSourceUrl()) << ","
         << escapeCsvField(models::statusToString(q.getStatus())) << ","
         << (q.isFavorite() ? "true" : "false") << ","
         << escapeCsvField(q.getNotes()) << "," << q.getCreatedAt() << ","
         << q.getUpdatedAt() << "," << q.getLastPracticedAt() << ","
         << q.getNextRevisionAt() << "," << q.getRevisionPriority() << ","
         << escapeCsvField(tagsStr) << "," << escapeCsvField(q.getOwnerId())
         << "\n";
  }

  return file.good();
}

std::vector<std::string>
FileQuestionRepository::parseCsvLine(const std::string &line) {
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

static int64_t safeStoll(const std::string &str, int64_t defaultVal = 0) {
  if (str.empty())
    return defaultVal;
  try {
    size_t idx = 0;
    int64_t val = std::stoll(str, &idx);
    return val;
  } catch (...) {
    return defaultVal;
  }
}

static int32_t safeStoi(const std::string &str, int32_t defaultVal = 0) {
  if (str.empty())
    return defaultVal;
  try {
    size_t idx = 0;
    int32_t val = std::stoi(str, &idx);
    return val;
  } catch (...) {
    return defaultVal;
  }
}

models::Question FileQuestionRepository::parseQuestionRecord(
    const std::vector<std::string> &tokens) {
  // Expected at least 17 fields:
  // id,title,description,topic,difficulty,company,platform,source_url,status,
  // is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,
  // revision_priority,tags,owner_id
  if (tokens.size() < 17) {
    return {};
  }

  std::string id = tokens[0];
  if (id == "id" || id.empty()) {
    return {}; // Header or invalid row
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

  // Tags separated by semicolon
  std::vector<std::string> tags;
  std::stringstream tagStream(tokens[16]);
  std::string tagToken;
  while (std::getline(tagStream, tagToken, ';')) {
    if (!tagToken.empty()) {
      tags.push_back(tagToken);
    }
  }

  std::string owner_id = tokens.size() > 17 ? tokens[17] : "local_user";

  return models::Question(id, title, description, topic, diff, company,
                          platform, url, status, is_favorite, notes, created_at,
                          updated_at, last_practiced, next_revision, priority,
                          tags, owner_id);
}

void FileQuestionRepository::loadFromFile() {
  std::ifstream file(filePath_);
  if (!file.is_open()) {
    return;
  }

  std::string line;

  while (std::getline(file, line)) {
    while (!line.empty() &&
           (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
      line.pop_back();
    }

    if (line.empty() || line[0] == '#') {
      continue;
    }

    auto tokens = parseCsvLine(line);
    if (tokens.empty() || tokens[0] == "id") {
      continue; // Header row
    }

    models::Question q = parseQuestionRecord(tokens);
    if (q.isValid()) {
      const std::string &id = q.getId();
      if (cacheById_.find(id) == cacheById_.end()) {
        orderedIds_.push_back(id);
      }
      cacheById_[id] = q;
    }
  }
}

} // namespace codevault::persistence
