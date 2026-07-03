#pragma once
#include <cmath>
#include <filesystem>
#include <string>

// Use environment variable or fallback to $HOME/.local/share/<project>/results
inline std::string get_or_create_dir(std::string_view name)
{
  const char * env = std::getenv("PANDA_PROSTHESIS_RUNTIME_CONFIG_PATH");
  std::string dir;
  if(env)
  {
    dir = std::string(env);
  }
  else
  {
    const char * home = std::getenv("HOME");
    if(home)
    {
      dir = std::string(home) + "/.local/share/mc-rtc/controllers/panda_prosthesis/" + std::string{name};
    }
    else
    {
      dir = "/tmp/mc-rtc/controllers/panda_prosthesis/" + std::string{name};
    }
  }
  std::filesystem::create_directories(dir);
  return dir;
}

/**
 * \brief   Return the filenames of all files that have the specified extension
 *          in the specified directory and all subdirectories.
 */
inline std::vector<std::string> get_all_filenames(std::filesystem::path const & root, std::string const & ext = "")
{
  std::vector<std::string> paths;

  if(std::filesystem::exists(root) && std::filesystem::is_directory(root))
  {
    for(auto const & entry : std::filesystem::recursive_directory_iterator(root))
    {
      if(std::filesystem::is_regular_file(entry) && (ext.empty() || entry.path().extension() == ext))
        paths.emplace_back(entry.path().filename().c_str());
    }
  }

  return paths;
}

inline double truncate(double value, double precision = 2)
{
  return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

template<typename VectorT>
VectorT truncate(const VectorT & value, double precision = 2)
{
  VectorT r;
  for(unsigned i = 0; i < value.size(); ++i)
  {
    r[i] = truncate(value[i], precision);
  }
  return r;
}
