#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <nlohmann_json.hpp>

#include <cstdint>
#include <string>
#include <vector>

class Language {
public:
  Language();
  ~Language();

  std::string Get(const std::string &p_Key);

  int32_t GetCode();
  std::string GetName();
  std::vector<std::string> GetTypefacePaths();
  nlohmann::json GetData();

  nlohmann::json Fallback();

private:
  nlohmann::json m_Data;
  int32_t m_Code;
  std::string m_Name;
  std::vector<std::string> m_TypefacePaths;
};

#endif
