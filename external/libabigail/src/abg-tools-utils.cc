// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2021 Red Hat, Inc.

///@file

// In case we have a bad fts we include this before config.h because
// it can't handle _FILE_OFFSET_BITS.  Everything we need here is fine
// if its declarations just come first.  Also, include sys/types.h
// before fts. On some systems fts.h is not self contained.
#ifdef BAD_FTS
  #include <sys/types.h>
  #include <fts.h>
#endif

// For package configuration macros.
#include "config.h"

// In case we don't have a bad fts then we need to include fts.h after
// config.h.
#ifndef BAD_FTS
  #include <sys/types.h>
  #include <fts.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>

#include "abg-dwarf-reader.h"
#include "abg-internal.h"
#include "abg-regex.h"

// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include <abg-ir.h>
#include "abg-config.h"
#include "abg-tools-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

using std::string;

namespace abigail
{

using namespace abigail::suppr;
using namespace abigail::ini;

/// @brief Namespace for a set of utility function used by tools based
/// on libabigail.
namespace tools_utils
{

/// Get the value of $libdir variable of the autotools build
/// system.  This is where shared libraries are usually installed.
///
/// @return a constant string (doesn't have to be free-ed by the
/// caller) that represent the value of the $libdir variable in the
/// autotools build system, or NULL if it's not set.
const char*
get_system_libdir()
{
#ifndef ABIGAIL_ROOT_SYSTEM_LIBDIR
#error the macro ABIGAIL_ROOT_SYSTEM_LIBDIR must be set at compile time
#endif

  static __thread const char* system_libdir(ABIGAIL_ROOT_SYSTEM_LIBDIR);
  return system_libdir;
}

/// The bitwise 'OR' operator for abidiff_status bit masks.
///
/// @param l the left hand side operand of the OR operator.
///
/// @param r the right hand side operand of the OR operator.
///
/// @return the result of the OR expression.
abidiff_status
operator|(abidiff_status l, abidiff_status r)
{return static_cast<abidiff_status>(static_cast<unsigned>(l)
				     | static_cast<unsigned>(r));}

/// The bitwise 'AND' operator for abidiff_status bit masks.
///
/// @param l the left hand side operand of the AND operator.
///
/// @param r the right hand side operand of the AND operator.
///
/// @return the result of the AND expression.
abidiff_status
operator&(abidiff_status l, abidiff_status r)
{return static_cast<abidiff_status>(static_cast<unsigned>(l)
				     & static_cast<unsigned>(r));}

/// The |= operator.
///
/// @param l the left hand side operand of the operator.
///
/// @param r the right hand side operand of the operator.
///
/// @param the resulting bit mask.
abidiff_status&
operator|=(abidiff_status&l, abidiff_status r)
{
  l = static_cast<abidiff_status>(static_cast<unsigned>(l)
				  | static_cast<unsigned>(r));
  return l;
}

/// Test if an instance of @param abidiff_status bits mask represents
/// an error.
///
/// This functions tests if the @ref ABIDIFF_ERROR bit is set in the
/// given bits mask.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its ABIDIFF_ERROR bit set.
bool
abidiff_status_has_error(abidiff_status s)
{return s & ABIDIFF_ERROR;}

/// Test if an instance of @param abidiff_status bits mask represents
/// an abi change.
///
/// This functions tests if the @ref ABIDIFF_ABI_CHANGE bit is set in the
/// given bits mask.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its @ref ABIDIFF_ABI_CHANGE bit set.
bool
abidiff_status_has_abi_change(abidiff_status s)
{return s & ABIDIFF_ABI_CHANGE;}

/// Test if an instance of @param abidiff_status bits mask represents
/// an incompatible abi change.
///
/// This functions tests if the @ref ABIDIFF_INCOMPATIBLE_ABI_CHANGE
/// bit is set in the given bits mask.  Note that the this bit is set
/// then the bit @ref ABIDIFF_ABI_CHANGE must be set as well.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its @ref ABIDIFF_INCOMPATIBLE ABI_CHANGE
/// set.
bool
abidiff_status_has_incompatible_abi_change(abidiff_status s)
{return s & ABIDIFF_ABI_INCOMPATIBLE_CHANGE;}

#define DECLARE_STAT(st) \
  struct stat st; \
  memset(&st, 0, sizeof(st))

// <class timer stuff>

/// The private data type of the @ref timer class.
struct timer::priv
{
  timer::kind timer_kind;
  struct timeval begin_timeval;
  struct timeval end_timeval;

  priv(timer::kind k)
    : timer_kind(k),
      begin_timeval(),
      end_timeval()
  {}
}; // end struct timer::priv

/// Constructor of the @ref timer type.
///
/// @param k the kind of timer to instantiate.
timer::timer(timer::kind k)
  : priv_(new timer::priv(k))
{
  if (priv_->timer_kind == START_ON_INSTANTIATION_TIMER_KIND)
    start();
}

/// Start the timer.
///
/// To stop the timer (and record the time elapsed since the timer was
/// started), call the timer::stop member function.
///
/// @return true upon successful completion.
bool
timer::start()
{
  if (gettimeofday(&priv_->begin_timeval, 0))
    return false;
  return true;
}

/// Stop the timer.
///
/// This records the time elapsed since the timer was started using
/// the timer::start member function.
///
/// @return true upon successful completion.
bool
timer::stop()
{
  if (gettimeofday(&priv_->end_timeval, 0))
    return false;
  return true;
}

/// Get the elapsed time in seconds.
///
/// @return the time elapsed between the invocation of the methods
/// timer::start() and timer::stop, in seconds.
time_t
timer::value_in_seconds() const
{return priv_->end_timeval.tv_sec - priv_->begin_timeval.tv_sec;}

/// Get the elapsed time in hour:minutes:seconds:milliseconds.
///
/// @param hours out parameter. This is set to the number of hours elapsed.
///
/// @param minutes out parameter.  This is set to the number of minutes
/// (passed the number of hours) elapsed.
///
/// @param seconds out parameter.  This is se to the number of
/// seconds (passed the number of hours and minutes) elapsed.
///
/// @param milliseconds.  This is set ot the number of milliseconds
/// (passed the number of hours, minutes and seconds) elapsed.
///
/// @return true upon successful completion.
bool
timer::value(time_t&		hours,
	     time_t&		minutes,
	     time_t&		seconds,
	     time_t&	milliseconds) const
{
  time_t elapsed_seconds =
    priv_->end_timeval.tv_sec - priv_->begin_timeval.tv_sec;
  suseconds_t elapsed_usecs =
    ((priv_->end_timeval.tv_sec * 1000000) + priv_->end_timeval.tv_usec)
    - ((priv_->begin_timeval.tv_sec * 1000000) + priv_->begin_timeval.tv_usec);

  milliseconds = 0;

  hours = elapsed_seconds / 3600;
  minutes = (elapsed_seconds % 3600) / 60;
  seconds = (elapsed_seconds % 3600) % 60;
  if (elapsed_seconds == 0)
    milliseconds = elapsed_usecs / 1000;

  return true;
}

/// Get the elapsed time as a human-readable string.
///
/// @return the elapsed time as a human-readable string.
string
timer::value_as_string() const
{
  time_t hours = 0, minutes = 0, seconds = 0;
  time_t msecs = 0;

  value(hours, minutes, seconds, msecs);

  std::ostringstream o;

  if (hours)
    o << hours << "h";

  if (minutes)
    o << minutes << "m";

  o << seconds << "s";

  if (msecs)
    o <<msecs <<"ms";

  return o.str();
}

/// Destructor of the @ref timer type.
timer::~timer()
{
}

/// Streaming operator for the @ref timer type.
///
/// Emit a string representing the elapsed time (in a human-readable
/// manner) to an output stream.
///
/// @param o the output stream to emit the elapsed time string to.
///
/// @param t the timer to consider.
///
/// @return the output stream considered.
ostream&
operator<<(ostream& o, const timer& t)
{
  o << t.value_as_string();
  return o;
}

/// Get the stat struct (as returned by the lstat() function of the C
/// library) of a file.  Note that the function uses lstat, so that
/// callers can detect symbolic links.
///
/// @param path the path to the function to stat.
///
/// @param s the resulting stat struct.
///
/// @return true iff the stat function completed successfully.
static bool
get_stat(const string& path,
	 struct stat* s)
{return (lstat(path.c_str(), s) == 0);}

/// Tests whether a path exists;
///
/// @param path the path to test for.
///
/// @return true iff the path at @p path exist.
bool
file_exists(const string& path)
{
  DECLARE_STAT(st);

  return get_stat(path, &st);
}

/// Test that a given directory exists.
///
/// @param path the path of the directory to consider.
///
/// @return true iff a directory exists with the name @p path
bool
dir_exists(const string &path)
{return file_exists(path) && is_dir(path);}

/// Test if a given directory exists and is empty.
///
/// @param path the path of the directory to consider
bool
dir_is_empty(const string &path)
{
  if (!dir_exists(path))
    return false;

  DIR* dir = opendir(path.c_str());
  if (!dir)
    return false;

  errno = 0;
  dirent *result = readdir(dir);
  if (result == NULL && errno != 0)
    return false;

  closedir(dir);

  return result == NULL;
}

/// Test if path is a path to a regular file or a symbolic link to a
/// regular file.
///
/// @param path the path to consider.
///
/// @return true iff path is a regular path.
bool
is_regular_file(const string& path)
{
  DECLARE_STAT(st);

  if (!get_stat(path, &st))
    return false;

  if (S_ISREG(st.st_mode))
    return true;

  string symlink_target_path;
  if (maybe_get_symlink_target_file_path(path, symlink_target_path))
    return is_regular_file(symlink_target_path);

  return false;
}

/// Tests if a given path is a directory or a symbolic link to a
/// directory.
///
/// @param path the path to test for.
///
/// @return true iff @p path is a directory.
bool
is_dir(const string& path)
{
  DECLARE_STAT(st);

  if (!get_stat(path, &st))
    return false;

  if (S_ISDIR(st.st_mode))
    return true;

  string symlink_target_path;
  if (maybe_get_symlink_target_file_path(path, symlink_target_path))
    return is_dir(symlink_target_path);

  return false;
}

static const char* ANONYMOUS_STRUCT_INTERNAL_NAME = "__anonymous_struct__";
static const char* ANONYMOUS_UNION_INTERNAL_NAME =  "__anonymous_union__";
static const char* ANONYMOUS_ENUM_INTERNAL_NAME =   "__anonymous_enum__";

static int ANONYMOUS_STRUCT_INTERNAL_NAME_LEN =
  strlen(ANONYMOUS_STRUCT_INTERNAL_NAME);

static int ANONYMOUS_UNION_INTERNAL_NAME_LEN =
  strlen(ANONYMOUS_UNION_INTERNAL_NAME);

static int ANONYMOUS_ENUM_INTERNAL_NAME_LEN =
  strlen(ANONYMOUS_ENUM_INTERNAL_NAME);

/// Getter of the prefix for the name of anonymous structs.
///
/// @reaturn the prefix for the name of anonymous structs.
const char*
get_anonymous_struct_internal_name_prefix()
{return ANONYMOUS_STRUCT_INTERNAL_NAME;}

/// Getter of the prefix for the name of anonymous unions.
///
/// @reaturn the prefix for the name of anonymous unions.
const char*
get_anonymous_union_internal_name_prefix()
{return ANONYMOUS_UNION_INTERNAL_NAME;}

/// Getter of the prefix for the name of anonymous enums.
///
/// @reaturn the prefix for the name of anonymous enums.
const char*
get_anonymous_enum_internal_name_prefix()
{return ANONYMOUS_ENUM_INTERNAL_NAME;}

/// Compare two fully qualified decl names by taking into account that
/// they might have compontents that are anonymous types/namespace names.
///
/// For instance:
///
/// __anonymous_struct__1::foo and __anonymous_struct__2::foo are
/// considered being equivalent qualified names because both are data
/// members that belong to anonymous structs.  The anonymous structs
/// are numbered so that we can tell them appart (and look them up)
/// where there are several of them in the same scope.  But during
/// comparison, for various purposes, we want to consider them as
/// equivalent.
///
/// Similarly, __anonymous_struct__1::foo::__anonymous_struct__2::bar
/// and __anonymous_struct__10::foo::__anonymous_struct__11::bar are
/// equivalent.
///
/// But __anonymous_struct__1::foo::__anonymous_struct__2::bar and
/// __anonymous_struct__10::foo::__anonymous_union__11::bar are not
/// equivalent because the former designates a member of an anonymous
/// struct and the latter designates a member of an anonymous union.
///
/// So this function handles those cases.
///
/// @param l the name of the first (left hand side) decl to consider.
///
/// @param r the name of the second (right hand side) decl to consider.
///
/// @return true iff @p l is equivalent to @p r when taking into
/// account the anonymous scopes that both might have and if they
/// might be anonymous themselves.
bool
decl_names_equal(const string& l, const string& r)
{
  string::size_type l_pos1 = 0, r_pos1 = 0;
  const string::size_type l_length = l.length(), r_length = r.length();

  while (l_pos1 < l_length && r_pos1 < r_length)
    {
      string::size_type l_pos2 = l.find("::", l_pos1);
      string::size_type r_pos2 = r.find("::", r_pos1);
      if (l_pos2 == string::npos)
	l_pos2 = l_length;
      if (r_pos2 == string::npos)
	r_pos2 = r_length;

      if (l.compare(l_pos1, l_pos2 - l_pos1, r,
		    r_pos1, r_pos2 - r_pos1)
	  && (l.compare(l_pos1,
			ANONYMOUS_STRUCT_INTERNAL_NAME_LEN,
			ANONYMOUS_STRUCT_INTERNAL_NAME)
	      || r.compare(r_pos1,
			   ANONYMOUS_STRUCT_INTERNAL_NAME_LEN,
			   ANONYMOUS_STRUCT_INTERNAL_NAME))
	  && (l.compare(l_pos1,
			ANONYMOUS_UNION_INTERNAL_NAME_LEN,
			ANONYMOUS_UNION_INTERNAL_NAME)
	      || r.compare(r_pos1,
			   ANONYMOUS_UNION_INTERNAL_NAME_LEN,
			   ANONYMOUS_UNION_INTERNAL_NAME))
	  && (l.compare(l_pos1,
			ANONYMOUS_ENUM_INTERNAL_NAME_LEN,
			ANONYMOUS_ENUM_INTERNAL_NAME)
	      || r.compare(r_pos1,
			   ANONYMOUS_ENUM_INTERNAL_NAME_LEN,
			   ANONYMOUS_ENUM_INTERNAL_NAME)))
	return false;

      l_pos1 = l_pos2 == l_length ? l_pos2 : l_pos2 + 2;
      r_pos1 = r_pos2 == r_length ? r_pos2 : r_pos2 + 2;
    }

  return (l_pos1 == l_length) == (r_pos1 == r_length);
}

/// If a given file is a symbolic link, get the canonicalized absolute
/// path to the target file.
///
/// @param file_path the path to the file to consider.
///
/// @param target_path this parameter is set by the function to the
/// canonicalized path to the target file, if @p file_path is a
/// symbolic link.  In that case, the function returns true.
///
/// @return true iff @p file_path is a symbolic link.  In that case,
/// the function sets @p target_path to the canonicalized absolute
/// path of the target file.
bool
maybe_get_symlink_target_file_path(const string& file_path,
				   string& target_path)
{
  DECLARE_STAT(st);

  if (!get_stat(file_path, &st))
    return false;

  if (!S_ISLNK(st.st_mode))
    return false;

  char *link_target_path = realpath(file_path.c_str(), NULL);
  if (!link_target_path)
    return false;

  target_path = link_target_path;
  free(link_target_path);
  return true;
}

/// Return the directory part of a file path.
///
/// @param path the file path to consider
///
/// @param dirnam the resulting directory part, or "." if the couldn't
/// figure out anything better (for now; maybe we should do something
/// better than this later ...).
///
/// @param keep_separator_at_end if true, then keep the separator at
/// the end of the resulting dir name.
///
/// @return true upon successful completion, false otherwise (okay,
/// for now it always return true, but that might change in the future).
bool
dir_name(string const& path,
	 string& dir_name,
	 bool keep_separator_at_end)
{
  if (path.empty())
    {
      dir_name = ".";
      return true;
    }

  char *p = strdup(path.c_str());
  char *r = ::dirname(p);
  dir_name = r;
  free(p);
  if (keep_separator_at_end
      && dir_name.length() < path.length())
    dir_name += "/";
  return true;
}

/// Return the file name part of a file part.
///
/// @param path the file path to consider.
///
/// @param file_name the name part of the file to consider.
///
///@return true upon successful completion, false otherwise (okay it
///always return true for now, but that might change in the future).
bool
base_name(string const &path,
	 string& file_name)
{
  if (path.empty())
    {
      file_name = ".";
      return true;
    }

  char *p = strdup(path.c_str());
  char *f = ::basename(p);
  file_name = f;
  free(p);
  return true;
}

/// Return the real path of a given path.
///
/// The real path of path 'foo_path' is the same path as foo_path, but
/// with symlinks and relative paths resolved.
///
/// @param path the path to consider.
///
/// @param result the computed real_path;
void
real_path(const string&path, string& result)
{
  if (path.empty())
    {
      result.clear();
      return;
    }

  char *realp = realpath(path.c_str(), NULL);
  if (realp)
    {
      result = realp;
      free(realp);
    }
}

/// Ensures #dir_path is a directory and is created.  If #dir_path is
/// not created, this function creates it.
///
/// \return true if #dir_path is a directory that is already present,
/// of if the function has successfuly created it.
bool
ensure_dir_path_created(const string& dir_path)
{
  struct stat st;
  memset(&st, 0, sizeof (st));

  int stat_result = 0;

  stat_result = stat(dir_path.c_str(), &st);
  if (stat_result == 0)
    {
      // A file or directory already exists with the same name.
      if (!S_ISDIR (st.st_mode))
	return false;
      return true;
    }

  string cmd;
  cmd = "mkdir -p " + dir_path;

  if (system(cmd.c_str()))
    return false;

  return true;
}

/// Ensures that the parent directory of #path is created.
///
/// \return true if the parent directory of #path is already present,
/// or if this function has successfuly created it.
bool
ensure_parent_dir_created(const string& path)
{
  bool is_ok = false;

  if (path.empty())
    return is_ok;

  string parent;
  if (dir_name(path, parent))
    is_ok = ensure_dir_path_created(parent);

  return is_ok;
}

/// Emit a prefix made of the name of the program which is emitting a
/// message to an output stream.
///
/// The prefix is a string which looks like:
///
///   "<program-name> : "
///
/// @param prog_name the name of the program to use in the prefix.
/// @param out the output stream where to emit the prefix.
///
/// @return the output stream where the prefix was emitted.
ostream&
emit_prefix(const string& prog_name, ostream& out)
{
  if (!prog_name.empty())
    out << prog_name << ": ";
  return out;
}

/// Check if a given path exists and is readable.
///
/// @param path the path to consider.
///
/// @param out the out stream to report errors to.
///
/// @return true iff path exists and is readable.
bool
check_file(const string& path, ostream& out, const string& prog_name)
{
  if (!file_exists(path))
    {
      emit_prefix(prog_name, out) << "file " << path << " does not exist\n";
      return false;
    }

  if (!is_regular_file(path))
    {
      emit_prefix(prog_name, out) << path << " is not a regular file\n";
      return false;
    }

  return true;
}

/// Check if a given path exists, is readable and is a directory.
///
/// @param path the path to consider.
///
/// @param out the out stream to report errors to.
///
/// @param prog_name the program name on behalf of which to report the
/// error, if any.
///
/// @return true iff @p path exists and is for a directory.
bool
check_dir(const string& path, ostream& out, const string& prog_name)
{
  if (!file_exists(path))
    {
      emit_prefix(prog_name, out) << "path " << path << " does not exist\n";
      return false;
    }

  if (!is_dir(path))
    {
      emit_prefix(prog_name, out) << path << " is not a directory\n";
      return false;
    }

  return true;
}

/// Test if a given string ends with a particular suffix.
///
/// @param str the string to consider.
///
/// @param suffix the suffix to test for.
///
/// @return true iff string @p str ends with suffix @p suffix.
bool
string_ends_with(const string& str, const string& suffix)
{
  string::size_type str_len = str.length(), suffix_len = suffix.length();

  if (str_len < suffix_len)
    return false;
  return str.compare(str_len - suffix_len, suffix_len, suffix) == 0;
}

/// Test if a given string begins with a particular prefix.
///
/// @param str the string consider.
///
/// @param prefix the prefix to look for.
///
/// @return true iff string @p str begins with prefix @p prefix.
bool
string_begins_with(const string& str, const string& prefix)
{
  if (str.empty())
    return false;

  if (prefix.empty())
    return true;

  string::size_type prefix_len = prefix.length();
  if (prefix_len > str.length())
    return false;

  return str.compare(0, prefix.length(), prefix) == 0;
}

/// Test if a string is made of ascii characters.
///
/// @param str the string to consider.
///
/// @return true iff @p str is made of ascii characters.
bool
string_is_ascii(const string& str)
{
  for (string::const_iterator i = str.begin(); i != str.end(); ++i)
    if (!isascii(*i))
      return false;

  return true;
}

/// Test if a string is made of ascii characters which are identifiers
/// acceptable in C or C++ programs.
///
///
/// In the C++ spec, [lex.charset]/2, we can read:
///
/// "if the hexadecimal value for a universal-character-name [...]  or
///  string literal corresponds to a control character (in either of
///  the ranges 0x00-0x1F or 0x7F-0x9F, both inclusive) [...] the
///  program is ill-formed."
///
/// @param str the string to consider.
///
/// @return true iff @p str is made of ascii characters, and is an
/// identifier.
bool
string_is_ascii_identifier(const string& str)
{
  for (string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
      unsigned char c = *i;
    if (!isascii(c)
	|| (c <= 0x1F) // Rule out control characters
	|| (c >= 0x7F && c <= 0x9F)) // Rule out special extended
				     // ascii characters.
      return false;
    }

  return true;
}

/// Split a given string into substrings, given some delimiters.
///
/// @param input_string the input string to split.
///
/// @param delims a string containing the delimiters to consider.
///
/// @param result a vector of strings containing the splitted result.
///
/// @return true iff the function found delimiters in the input string
/// and did split it as a result.  Note that if no delimiter was found
/// in the input string, then the input string is added at the end of
/// the output vector of strings.
bool
split_string(const string& input_string,
	     const string& delims,
	     vector<string>& result)
{
  size_t current = 0, next;
  bool did_split = false;

  do
    {
      // Trim leading white spaces
      while (current < input_string.size() && isspace(input_string[current]))
	++current;

      if (current >= input_string.size())
	break;

      next = input_string.find_first_of(delims, current);
      if (next == string::npos)
	{
	  string s = input_string.substr(current);
	  if (!s.empty())
	    result.push_back(input_string.substr(current));
	  did_split = (current != 0);
	  break;
	}
      string s = input_string.substr(current, next - current);
      if (!s.empty())
	{
	  result.push_back(input_string.substr(current, next - current));
	  did_split = true;
	}
      current = next + 1;
    }
  while (next != string::npos);

  return did_split;
}

/// Get the suffix of a string, given a prefix to consider.
///
/// @param input_string the input string to consider.
///
/// @param prefix the prefix of the input string to consider.
///
/// @param suffix output parameter. This is set by the function to the
/// the computed suffix iff a suffix was found for prefix @p prefix.
///
/// @return true iff the function could find a prefix for the suffix
/// @p suffix in the input string @p input_string.
bool
string_suffix(const string& input_string,
	      const string& prefix,
	      string& suffix)
{
  // Some basic sanity check before we start hostilities.
  if (prefix.length() >= input_string.length())
    return false;

  if (input_string.compare(0, prefix.length(), prefix) != 0)
    // The input string does not start with the string contained in
    // the prefix parameter.
    return false;

  suffix = input_string.substr(prefix.length());
  return true;
}

/// Return the prefix that is common to two strings.
///
/// @param s1 the first input string to consider.
///
/// @param s2 the second input string to consider.
///
/// @param result output parameter.  The resulting common prefix found
/// between @p s1 and @p s2.  This is set iff the function returns
/// true.
///
/// @return true iff @p result was set by this function with the
/// common prefix of @p s1 and @p s2.
static bool
common_prefix(const string& s1, const string& s2, string &result)
{
  if (s1.length() == 0 || s2.length() == 0)
    return false;

  result.clear();
  for (size_t i = 0; i < s1.length() && i< s2.length(); ++i)
    if (s1[i] == s2[i])
      result += s1[i];
    else
      break;

  return !result.empty();
}

/// Find the prefix common to a *SORTED* vector of strings.
///
/// @param input_strings a lexycographically sorted vector of
/// strings.  Please note that this vector absolutely needs to be
/// sorted for the function to work correctly.  Otherwise the results
/// are going to be wrong.
///
/// @param prefix output parameter.  This is set by this function with
/// the prefix common to the strings found in @p input_strings, iff
/// the function returns true.
///
/// @return true iff the function could find a common prefix to the
/// strings in @p input_strings.
bool
sorted_strings_common_prefix(vector<string>& input_strings, string& prefix)
{
  string prefix_candidate;
  bool found_prefix = false;

  if (input_strings.size() == 1)
    {
      if (dir_name(input_strings.front(), prefix,
		   /*keep_separator_at_end=*/true))
	return true;
      return false;
    }

  string cur_str;
  for (vector<string>::const_iterator i = input_strings.begin();
       i != input_strings.end();
       ++i)
    {
      dir_name(*i, cur_str, /*keep_separator_at_end=*/true);
      if (prefix_candidate.empty())
	{
	  prefix_candidate = cur_str;
	  continue;
	}

      string s;
      if (common_prefix(prefix_candidate, cur_str, s))
	{
	  ABG_ASSERT(!s.empty());
	  prefix_candidate = s;
	  found_prefix = true;
	}
    }

  if (found_prefix)
    {
      prefix = prefix_candidate;
      return true;
    }

  return false;
}

/// Return the version string of the library.
///
/// @return the version string of the library.
string
get_library_version_string()
{
  string major, minor, revision, version_string, suffix;
  abigail::abigail_get_library_version(major, minor, revision, suffix);
  version_string = major + "." + minor + "." + revision + suffix;
  return version_string;
}

/// Execute a shell command and returns its output.
///
/// @param cmd the shell command to execute.
///
/// @param lines output parameter.  This is set with the lines that
/// constitute the output of the process that executed the command @p
/// cmd.
///
/// @return true iff the command was executed properly and no error
/// was encountered.
bool
execute_command_and_get_output(const string& cmd, vector<string>& lines)
{
  if (cmd.empty())
    return false;

  FILE *stream=
    popen(cmd.c_str(),
	  /*open 'stream' in
	    read-only mode: type=*/"r");

  if (stream == NULL)
    return false;

  string result;

#define TMP_BUF_LEN 1024 + 1
  char tmp_buf[TMP_BUF_LEN];
  memset(tmp_buf, 0, TMP_BUF_LEN);

  while (fgets(tmp_buf, TMP_BUF_LEN, stream))
    {
      lines.push_back(tmp_buf);
      memset(tmp_buf, 0, TMP_BUF_LEN);
    }

  if (pclose(stream) == -1)
    return false;

  return true;
}

/// Get the SONAMEs of the DSOs advertised as being "provided" by a
/// given RPM.  That set can be considered as being the set of
/// "public" DSOs of the RPM.
///
/// This runs the command "rpm -qp --provides <rpm> | grep .so" and
/// filters its result.
///
/// @param rpm_path the path to the RPM to consider.
///
/// @param provided_dsos output parameter.  This is set to the set of
/// SONAMEs of the DSOs advertised as being provided by the RPM
/// designated by @p rpm_path.
///
/// @return true iff we could successfully query the RPM to see what
/// DSOs it provides.
bool
get_dsos_provided_by_rpm(const string& rpm_path, set<string>& provided_dsos)
{
  vector<string> query_output;
  // We don't check the return value of this command because on some
  // system, the command can issue errors but still emit a valid
  // output.  We'll rather rely on the fact that the command emits a
  // valid output or not.
  execute_command_and_get_output("rpm -qp --provides "
				 + rpm_path + " 2> /dev/null | grep .so",
				 query_output);

  for (vector<string>::const_iterator line = query_output.begin();
       line != query_output.end();
       ++line)
    {
      string dso = line->substr(0, line->find('('));
      dso = trim_white_space(dso);
      if (!dso.empty())
	provided_dsos.insert(dso);
    }
  return true;
}

/// Remove spaces at the beginning and at the end of a given string.
///
/// @param str the input string to consider.
///
/// @return the @p str string with leading and trailing white spaces removed.
string
trim_white_space(const string& str)
{
  if (str.empty())
    return "";

  string result;
  string::size_type start, end;
  for (start = 0; start < str.length(); ++start)
    if (!isspace(str[start]))
      break;

  for (end = str.length() - 1; end > 0; --end)
    if (!isspace(str[end]))
      break;

  result = str.substr(start, end - start + 1);
  return result;
}

/// Remove a string of pattern in front of a given string.
///
/// For instance, consider this string:
///    "../../../foo"
///
/// The pattern "../" is repeated three times in front of the
/// sub-string "foo".  Thus, the call:
///    trim_leading_string("../../../foo", "../")
/// will return the string "foo".
///
/// @param from the string to trim the leading repetition of pattern from.
///
/// @param to_trim the pattern to consider (and to trim).
///
/// @return the resulting string where the leading patter @p to_trim
/// has been removed from.
string
trim_leading_string(const string& from, const string& to_trim)
{
  string str = from;

  while (string_begins_with(str, to_trim))
    string_suffix(str, to_trim, str);
  return str;
}

/// Convert a vector<char*> into a vector<char**>.
///
/// @param char_stars the input vector.
///
/// @param char_star_stars the output vector.
void
convert_char_stars_to_char_star_stars(const vector<char*> &char_stars,
				      vector<char**>& char_star_stars)
{
  for (vector<char*>::const_iterator i = char_stars.begin();
       i != char_stars.end();
       ++i)
    char_star_stars.push_back(const_cast<char**>(&*i));
}

/// The private data of the @ref temp_file type.
struct temp_file::priv
{
  char*			   path_template_;
  int			   fd_;
  shared_ptr<std::fstream> fstream_;

  priv()
  {
    const char* templat = "/tmp/libabigail-tmp-file-XXXXXX";
    int s = strlen(templat);
    path_template_ = new char[s + 1];
    memset(path_template_, 0, s + 1);
    memcpy(path_template_, templat, s);

    fd_ = mkstemp(path_template_);
    if (fd_ == -1)
      return;

    fstream_.reset(new std::fstream(path_template_,
				    std::ios::trunc
				    | std::ios::in
				    | std::ios::out));
  }

  ~priv()
  {
    if (fd_ && fd_ != -1)
      {
	fstream_.reset();
	close(fd_);
	remove(path_template_);
      }
    delete [] path_template_;
  }
};

/// Default constructor of @ref temp_file.
///
/// It actually creates the temporary file.
temp_file::temp_file()
  : priv_(new priv)
{}

/// Test if the temporary file has been created and is usable.
///
/// @return true iff the temporary file has been created and is
/// useable.
bool
temp_file::is_good() const
{return priv_->fstream_->good();}

/// Return the path to the temporary file.
///
/// @return the path to the temporary file if it's usable, otherwise
/// return nil.
const char*
temp_file::get_path() const
{
  if (is_good())
    return priv_->path_template_;

  return 0;
}

/// Get the fstream to the temporary file.
///
/// Note that the current process is aborted if this member function
/// is invoked on an instance of @ref temp_file that is not usable.
/// So please test that the instance is usable by invoking the
/// temp_file::is_good() member function on it first.
///
/// @return the fstream to the temporary file.
std::fstream&
temp_file::get_stream()
{
  ABG_ASSERT(is_good());
  return *priv_->fstream_;
}

/// Create the temporary file and return it if it's usable.
///
/// @return the newly created temporary file if it's usable, nil
/// otherwise.
temp_file_sptr
temp_file::create()
{
  temp_file_sptr result(new temp_file);
  if (result->is_good())
    return result;

  return temp_file_sptr();
}

/// Get a pseudo random number.
///
/// @return a pseudo random number.
size_t
get_random_number()
{
  static __thread bool initialized = false;

  if (!initialized)
    {
      srand(time(NULL));
      initialized = true;
    }

  return rand();
}

/// Get a pseudo random number as string.
///
/// @return a pseudo random number as string.
string
get_random_number_as_string()
{
  std::ostringstream o;
  o << get_random_number();

  return o.str();
}

ostream&
operator<<(ostream& output,
	   file_type r)
{
  string repr;

  switch(r)
    {
    case FILE_TYPE_UNKNOWN:
      repr = "unknown file type";
      break;
    case FILE_TYPE_NATIVE_BI:
      repr = "native binary instrumentation file type";
      break;
    case FILE_TYPE_ELF:
      repr = "ELF file type";
      break;
    case FILE_TYPE_AR:
      repr = "archive file type";
      break;
    case FILE_TYPE_XML_CORPUS:
      repr = "native XML corpus file type";
      break;
    case FILE_TYPE_XML_CORPUS_GROUP:
      repr = "native XML corpus group file type";
      break;
    case FILE_TYPE_RPM:
      repr = "RPM file type";
      break;
    case FILE_TYPE_SRPM:
      repr = "SRPM file type";
      break;
    case FILE_TYPE_DEB:
      repr = "Debian binary file type";
      break;
    case FILE_TYPE_DIR:
      repr = "Directory type";
      break;
    case FILE_TYPE_TAR:
      repr = "GNU tar archive type";
      break;
    }

  output << repr;
  return output;
}

/// Guess the type of the content of an input stream.
///
/// @param in the input stream to guess the content type for.
///
/// @return the type of content guessed.
file_type
guess_file_type(istream& in)
{
  const unsigned BUF_LEN = 264;
  const unsigned NB_BYTES_TO_READ = 263;

  char buf[BUF_LEN];
  memset(buf, 0, BUF_LEN);

  std::streampos initial_pos = in.tellg();
  in.read(buf, NB_BYTES_TO_READ);
  in.seekg(initial_pos);

  if (in.gcount() < 4 || in.bad())
    return FILE_TYPE_UNKNOWN;

  if (buf[0] == 0x7f
      && buf[1] == 'E'
      && buf[2] == 'L'
      && buf[3] == 'F')
    return FILE_TYPE_ELF;

  if (buf[0] == '!'
      && buf[1] == '<'
      && buf[2] == 'a'
      && buf[3] == 'r'
      && buf[4] == 'c'
      && buf[5] == 'h'
      && buf[6] == '>')
    {
      if (strstr(buf, "debian-binary"))
	return FILE_TYPE_DEB;
      else
	return FILE_TYPE_AR;
    }

  if (buf[0] == '<'
      && buf[1] == 'a'
      && buf[2] == 'b'
      && buf[3] == 'i'
      && buf[4] == '-'
      && buf[5] == 'i'
      && buf[6] == 'n'
      && buf[7] == 's'
      && buf[8] == 't'
      && buf[9] == 'r'
      && buf[10] == ' ')
    return FILE_TYPE_NATIVE_BI;

  if (buf[0]     == '<'
      && buf[1]  == 'a'
      && buf[2]  == 'b'
      && buf[3]  == 'i'
      && buf[4]  == '-'
      && buf[5]  == 'c'
      && buf[6]  == 'o'
      && buf[7]  == 'r'
      && buf[8]  == 'p'
      && buf[9]  == 'u'
      && buf[10] == 's'
      && buf[11] == '-'
      && buf[12] == 'g'
      && buf[13] == 'r'
      && buf[14] == 'o'
      && buf[15] == 'u'
      && buf[16] == 'p'
      && buf[17] == ' ')
    return FILE_TYPE_XML_CORPUS_GROUP;

  if (buf[0]     == '<'
      && buf[1]  == 'a'
      && buf[2]  == 'b'
      && buf[3]  == 'i'
      && buf[4]  == '-'
      && buf[5]  == 'c'
      && buf[6]  == 'o'
      && buf[7]  == 'r'
      && buf[8]  == 'p'
      && buf[9]  == 'u'
      && buf[10] == 's'
      && buf[11] == ' ')
    return FILE_TYPE_XML_CORPUS;

  if ((unsigned char) buf[0]    == 0xed
      && (unsigned char) buf[1] == 0xab
      && (unsigned char) buf[2] == 0xee
      && (unsigned char) buf[3] == 0xdb)
    {
        if (buf[7] == 0x00)
          return FILE_TYPE_RPM;
        else if (buf[7] == 0x01)
          return FILE_TYPE_SRPM;
        else
          return FILE_TYPE_UNKNOWN;
    }

  if (buf[257]    == 'u'
      && buf[258] == 's'
      && buf[259] == 't'
      && buf[260] == 'a'
      && buf[261] == 'r')
    return FILE_TYPE_TAR;

  return FILE_TYPE_UNKNOWN;
}

/// Guess the type of the content of an file.
///
/// @param file_path the path to the file to consider.
///
/// @return the type of content guessed.
file_type
guess_file_type(const string& file_path)
{
  if (is_dir(file_path))
    return FILE_TYPE_DIR;

  if (string_ends_with(file_path, ".tar")
      || string_ends_with(file_path, ".tar.gz")
      || string_ends_with(file_path, ".tgz")
      || string_ends_with(file_path, ".tar.bz2")
      || string_ends_with(file_path, ".tbz2")
      || string_ends_with(file_path, ".tbz")
      || string_ends_with(file_path, ".tb2")
      || string_ends_with(file_path, ".tar.xz")
      || string_ends_with(file_path, ".txz")
      || string_ends_with(file_path, ".tar.lzma")
      || string_ends_with(file_path, ".tar.lz")
      || string_ends_with(file_path, ".tlz")
      || string_ends_with(file_path, ".tar.Z")
      || string_ends_with(file_path, ".taz")
      || string_ends_with(file_path, ".tz"))
    return FILE_TYPE_TAR;

  ifstream in(file_path.c_str(), ifstream::binary);
  file_type r = guess_file_type(in);
  in.close();
  return r;
}

/// Get the package name of a .deb package.
///
/// @param str the string containing the .deb NVR.
///
/// @param name output parameter.  This is set with the package name
/// of the .deb package iff the function returns true.
///
/// @return true iff the function successfully finds the .deb package
/// name.
bool
get_deb_name(const string& str, string& name)
{
  if (str.empty() || str[0] == '_')
    return false;

  string::size_type str_len = str.length(), i = 0 ;

  for (; i < str_len; ++i)
    {
      if (str[i] == '_')
	break;
    }

  if (i == str_len)
    return false;

  name = str.substr(0, i);
  return true;
}

/// Get the package name of an rpm package.
///
/// @param str the string containing the NVR of the rpm.
///
/// @param name output parameter.  This is set with the package name
/// of the rpm package iff the function returns true.
///
/// @return true iff the function successfully finds the rpm package
/// name.
bool
get_rpm_name(const string& str, string& name)
{
  if (str.empty() || str[0] == '-')
    return false;

  string::size_type str_len = str.length(), i = 0;
  string::value_type c;

  for (; i < str_len; ++i)
    {
      c = str[i];
      string::size_type next_index = i + 1;
      if ((next_index < str_len) && c == '-' && isdigit(str[next_index]))
	break;
    }

  if (i == str_len)
    return false;

  name = str.substr(0, i);

  return true;
}

/// Get the architecture string from the NVR of an rpm.
///
/// @param str the NVR to consider.
///
/// @param arch output parameter.  Is set to the resulting
/// archirecture string iff the function returns true.
///
/// @return true iff the function could find the architecture string
/// from the NVR.
bool
get_rpm_arch(const string& str, string& arch)
{
  if (str.empty())
    return false;

  if (!string_ends_with(str, ".rpm"))
    return false;

  string::size_type str_len = str.length(), i = 0;
  string::value_type c;
  string::size_type last_dot_index = 0, dot_before_last_index = 0;

  for (i = str_len - 1; i > 0; --i)
    {
      c = str[i];
      if (c == '.')
	{
	  last_dot_index = i;
	  break;
	}
    }

  if (i == 0)
    return false;

  for(--i; i > 0; --i)
    {
      c = str[i];
      if (c == '.')
	{
	  dot_before_last_index = i;
	  break;
	}
    }

  if (i == 0)
    return false;

  arch = str.substr(dot_before_last_index + 1,
		    last_dot_index - dot_before_last_index - 1);

  return true;
}

/// Tests if a given file name designates a kernel package.
///
/// @param file_name the file name to consider.
///
/// @param file_type the type of the file @p file_name.
///
/// @return true iff @p file_name of kind @p file_type designates a
/// kernel package.
bool
file_is_kernel_package(const string& file_name, file_type file_type)
{
  bool result = false;
  string package_name;

  if (file_type == FILE_TYPE_RPM)
    {
      if (!get_rpm_name(file_name, package_name))
	return false;
      result = (package_name == "kernel");
    }
  else if (file_type == FILE_TYPE_DEB)
    {
      if (!get_deb_name(file_name, package_name))
	return false;
      result = (string_begins_with(package_name, "linux-image"));
    }

  return result;
}

/// Tests if a given file name designates a kernel debuginfo package.
///
/// @param file_name the file name to consider.
///
/// @param file_type the type of the file @p file_name.
///
/// @return true iff @p file_name of kind @p file_type designates a
/// kernel debuginfo package.
bool
file_is_kernel_debuginfo_package(const string& file_name, file_type file_type)
{
  bool result = false;
  string package_name;

  if (file_type == FILE_TYPE_RPM)
    {
      if (!get_rpm_name(file_name, package_name))
	return false;
      result = (package_name == "kernel-debuginfo");
    }
  else if (file_type == FILE_TYPE_DEB)
    {
      if (!get_deb_name(file_name, package_name))
	return false;
      result = (string_begins_with(package_name, "linux-image")
		&& (string_ends_with(package_name, "-dbg")
		    || string_ends_with(package_name, "-dbgsyms")));
    }

  return result;
}

/// The delete functor of a char buffer that has been created using
/// malloc.
struct malloced_char_star_deleter
{
  void
  operator()(char* ptr)
  {free(ptr);}
};

/// Return a copy of the path given in argument, turning it into an
/// absolute path by prefixing it with the concatenation of the result
/// of get_current_dir_name() and the '/' character.
///
/// The result being an shared_ptr to char*, it should manage its
/// memory by itself and the user shouldn't need to wory too much for
/// that.
///
/// @param p the path to turn into an absolute path.
///
/// @return a shared pointer to the resulting absolute path.
std::shared_ptr<char>
make_path_absolute(const char*p)
{
  using std::shared_ptr;

  shared_ptr<char> result;

  if (p && p[0] != '/')
    {
      shared_ptr<char> pwd(get_current_dir_name(),
			   malloced_char_star_deleter());
      string s = string(pwd.get()) + "/" + p;
      result.reset(strdup(s.c_str()), malloced_char_star_deleter());
    }
  else
    result.reset(strdup(p), malloced_char_star_deleter());

  return result;
}

/// Return a copy of the path given in argument, turning it into an
/// absolute path by prefixing it with the concatenation of the result
/// of get_current_dir_name() and the '/' character.
///
/// The result being a pointer to an allocated memory region, it must
/// be freed by the caller.
///
/// @param p the path to turn into an absolute path.
///
/// @return a pointer to the resulting absolute path.  It must be
/// freed by the caller.
char*
make_path_absolute_to_be_freed(const char*p)
{
    char* result = 0;

  if (p && p[0] != '/')
    {
      char* pwd = get_current_dir_name();
      string s = string(pwd) + "/" + p;
      free(pwd);
      result = strdup(s.c_str());
    }
  else
    result = strdup(p);

  return result;
}

/// This is a sub-routine of gen_suppr_spec_from_headers and
/// handle_fts_entry.
///
/// It setups a type suppression which is meant to keep types defined
/// in a given file and suppress all other types.
///
/// @param file_path the path to the file that defines types that are
/// meant to be kept by the type suppression.  All other types defined
/// in other files are to be suppressed.  Note that this file path is
/// added to the vector returned by
/// type_suppression::get_source_locations_to_keep()
///
/// @param suppr the type suppression to setup.  If this smart pointer
/// is nil then a new instance @ref type_suppression is created and
/// this variable is made to point to it.
static void
handle_file_entry(const string& file_path,
		  type_suppression_sptr& suppr)
{
  if (!suppr)
    {
      suppr.reset(new type_suppression(get_private_types_suppr_spec_label(),
				       /*type_name_regexp=*/"",
				       /*type_name=*/""));

      // Types that are defined in system headers are usually
      // OK to be considered as public types.
      suppr->set_source_location_to_keep_regex_str("^/usr/include/");
      suppr->set_is_artificial(true);
    }

  // And types that are defined in header files that are under
  // the header directory file we are looking are to be
  // considered public types too.
  suppr->get_source_locations_to_keep().insert(file_path);
}

/// This is a sub-routine of gen_suppr_spec_from_headers.
///
/// @param entry if this file represents a regular (or symlink) file,
/// then its file name is going to be added to the vector returned by
/// type_suppression::get_source_locations_to_keep().
///
/// @param if @p entry represents a file, then its file name is going
/// to be added to the vector returned by the method
/// type_suppression::get_source_locations_to_keep of this instance.
/// If this smart pointer is nil then a new instance @ref
/// type_suppression is created and this variable is made to point to
/// it.
static void
handle_fts_entry(const FTSENT *entry,
		 type_suppression_sptr& suppr)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return;

  string fname = entry->fts_name;
  if (!fname.empty())
    {
      if (string_ends_with(fname, ".h")
	  || string_ends_with(fname, ".hpp")
	  || string_ends_with(fname, ".hxx"))
	handle_file_entry (fname, suppr);
    }
}

/// Populate a type_supression from header files found in a given
/// directory tree.
///
/// The suppression suppresses types defined in source files that are
/// *NOT* found in the directory tree.
///
/// This is a subroutine for gen_suppr_spect_from_headers.
///
/// @param headers_root_dir the directory tree to consider for header
/// files.
///
/// @param result the type_supression to populate from the content of
/// @p headers_root_dir.
static void
gen_suppr_spec_from_headers_root_dir(const string& headers_root_dir,
				     type_suppression_sptr &result)
{
  if (!headers_root_dir.empty())
    {
      char* paths[] = {const_cast<char*>(headers_root_dir.c_str()), 0};

      if (FTS *file_hierarchy = fts_open(paths, FTS_LOGICAL|FTS_NOCHDIR, NULL))
	{
	  FTSENT *entry;
	  while ((entry = fts_read(file_hierarchy)))
	    handle_fts_entry(entry, result);
	  fts_close(file_hierarchy);
	}
    }
}

/// Generate a type suppression specification that suppresses ABI
/// changes for types defined in source files that are neither in a
/// given set of header root directories nor in a set of header
/// files.
///
/// @param headers_root_dirs ABI changes in types defined in files
/// *NOT* found in these directory trees are going be suppressed.
///
/// @param header_files a set of additional header files that define
/// types that are to be kept (not supressed) by the returned type
/// suppression.
///
/// @return the resulting type suppression generated, if any file was
/// found in the directory tree @p headers_root_dir.
type_suppression_sptr
gen_suppr_spec_from_headers(const vector<string>& headers_root_dirs,
			    const vector<string>& header_files)
{
  type_suppression_sptr result;

  for (vector<string>::const_iterator root_dir = headers_root_dirs.begin();
       root_dir != headers_root_dirs.end();
       ++root_dir)
    gen_suppr_spec_from_headers_root_dir(*root_dir, result);

  for (vector<string>::const_iterator file = header_files.begin();
       file != header_files.end();
       ++file)
    handle_file_entry(*file, result);

  return result;
}

/// Generate a type suppression specification that suppresses ABI
/// changes for types defined in source files that are neither in a
/// given header root dir, not in a set of header files.
///
/// @param headers_root_dir ABI changes in types defined in files
/// *NOT* found in this directory tree are going be suppressed.
///
/// @param header_files a set of additional header files that define
/// types that are to be kept (not supressed) by the returned type
/// suppression.
///
/// @return the resulting type suppression generated, if any file was
/// found in the directory tree @p headers_root_dir.
type_suppression_sptr
gen_suppr_spec_from_headers(const string& headers_root_dir,
			    const vector<string>& header_files)
{
  type_suppression_sptr result;
  vector<string> root_dirs;

  if (!headers_root_dir.empty())
    root_dirs.push_back(headers_root_dir);

  return gen_suppr_spec_from_headers(root_dirs, header_files);
}

/// Generate a type suppression specification that suppresses ABI
/// changes for types defined in source files that are not in a given
/// header root dir.
///
/// @param headers_root_dir ABI changes in types defined in files
/// *NOT* found in this directory tree are going be suppressed.
///
/// @return the resulting type suppression generated, if any file was
/// found in the directory tree @p headers_root_dir.
type_suppression_sptr
gen_suppr_spec_from_headers(const string& headers_root_dir)
{
  // We don't list individual files, just look under the headers_path.
  vector<string> header_files;
  return gen_suppr_spec_from_headers(headers_root_dir, header_files);
}

/// Generate a suppression specification from kernel abi whitelist
/// files.
///
/// A kernel ABI whitelist file is an INI file that usually has only
/// one section.  The name of the section is a string that ends up
/// with the sub-string "symbol_list" or "whitelist".  For instance
/// RHEL7_x86_64_symbol_list.
///
/// Then the content of the section is a set of function or variable
/// names, one name per line.  Each function or variable name is the
/// name of a function or a variable whose changes are to be keept.
///
/// A whitelist file can have multiple sections (adhering to the naming
/// conventions and multiple files can be passed. The suppression that
/// is created takes all whitelist sections from all files into account.
/// Symbols (or expression of such) are deduplicated in the final
/// suppression expression.
///
/// This function reads the white lists and generates a
/// function_suppression_sptr and variable_suppression_sptr and returns
/// a vector containing those.
///
/// @param abi_whitelist_paths a vector of KMI whitelist paths
///
/// @return a vector or suppressions
suppressions_type
gen_suppr_spec_from_kernel_abi_whitelists
   (const std::vector<std::string>& abi_whitelist_paths)
{

  std::vector<std::string> whitelisted_names;
  for (std::vector<std::string>::const_iterator
	   path_iter = abi_whitelist_paths.begin(),
	   path_end = abi_whitelist_paths.end();
       path_iter != path_end;
       ++path_iter)
    {

      abigail::ini::config whitelist;
      if (!read_config(*path_iter, whitelist))
	continue;

      const ini::config::sections_type& whitelist_sections =
	  whitelist.get_sections();

      for (ini::config::sections_type::const_iterator
	       section_iter = whitelist_sections.begin(),
	       section_end = whitelist_sections.end();
	   section_iter != section_end;
	   ++section_iter)
	{
	  std::string section_name = (*section_iter)->get_name();
	  if (!string_ends_with(section_name, "symbol_list")
	      && !string_ends_with(section_name, "whitelist"))
	    continue;
	  for (ini::config::properties_type::const_iterator
		   prop_iter = (*section_iter)->get_properties().begin(),
		   prop_end = (*section_iter)->get_properties().end();
	       prop_iter != prop_end;
	       ++prop_iter)
	    {
	      if (const simple_property_sptr& prop =
		      is_simple_property(*prop_iter))
		if (prop->has_empty_value())
		  {
		    const std::string& name = prop->get_name();
		    if (!name.empty())
		      whitelisted_names.push_back(name);
		  }
	    }
	}
    }

  suppressions_type result;
  if (!whitelisted_names.empty())
    {
      // Drop duplicates to simplify the regex we are generating
      std::sort(whitelisted_names.begin(), whitelisted_names.end());
      whitelisted_names.erase(std::unique(whitelisted_names.begin(),
					  whitelisted_names.end()),
			      whitelisted_names.end());

      // Build a regular expression representing the union of all
      // the function and variable names expressed in the white list.
      const std::string regex = regex::generate_from_strings(whitelisted_names);

      // Build a suppression specification which *keeps* functions
      // whose ELF symbols match the regular expression contained
      // in function_names_regexp.  This will also keep the ELF
      // symbols (not designated by any debug info) whose names
      // match this regexp.
      function_suppression_sptr fn_suppr(new function_suppression);
      fn_suppr->set_label("whitelist");
      fn_suppr->set_symbol_name_not_regex_str(regex);
      fn_suppr->set_drops_artifact_from_ir(true);
      result.push_back(fn_suppr);

      // Build a suppression specification which *keeps* variables
      // whose ELF symbols match the regular expression contained
      // in function_names_regexp.  This will also keep the ELF
      // symbols (not designated by any debug info) whose names
      // match this regexp.
      variable_suppression_sptr var_suppr(new variable_suppression);
      var_suppr->set_label("whitelist");
      var_suppr->set_symbol_name_not_regex_str(regex);
      var_suppr->set_drops_artifact_from_ir(true);
      result.push_back(var_suppr);
    }
  return result;
}

/// Get the path to the default system suppression file.
///
/// @return a copy of the default system suppression file.
string
get_default_system_suppression_file_path()
{
  string default_system_suppr_path;

  const char *s = getenv("LIBABIGAIL_DEFAULT_SYSTEM_SUPPRESSION_FILE");
  if (s)
    default_system_suppr_path = s;

  if (default_system_suppr_path.empty())
    default_system_suppr_path =
      get_system_libdir() + string("/libabigail/default.abignore");

  return default_system_suppr_path;
}

/// Get the path to the default user suppression file.
///
/// @return a copy of the default user suppression file.
string
get_default_user_suppression_file_path()
{
  string default_user_suppr_path;
  const char *s = getenv("LIBABIGAIL_DEFAULT_USER_SUPPRESSION_FILE");

  if (s == NULL)
    {
      s = getenv("HOME");
      if (s == NULL)
	return "";
      default_user_suppr_path  = s;
      if (default_user_suppr_path.empty())
	default_user_suppr_path = "~";
      default_user_suppr_path += "/.abignore";
    }
  else
    default_user_suppr_path = s;

  return default_user_suppr_path;
}

/// Load the default system suppression specification file and
/// populate a vector of @ref suppression_sptr with its content.
///
/// The default system suppression file is located at
/// $libdir/libabigail/default-libabigail.abignore.
///
/// @param supprs the vector to add the suppression specifications
/// read from the file to.
void
load_default_system_suppressions(suppr::suppressions_type& supprs)
{
  string default_system_suppr_path =
    get_default_system_suppression_file_path();

  read_suppressions(default_system_suppr_path, supprs);
}

/// Load the default user suppression specification file and populate
/// a vector of @ref suppression_sptr with its content.
///
/// The default user suppression file is located at $HOME~/.abignore.
///
/// @param supprs the vector to add the suppression specifications
/// read from the file to.
void
load_default_user_suppressions(suppr::suppressions_type& supprs)
{
  string default_user_suppr_path =
    get_default_user_suppression_file_path();

  read_suppressions(default_user_suppr_path, supprs);
}

/// Test if a given FTSENT* denotes a file with a given name.
///
/// @param entry the FTSENT* to consider.
///
/// @param fname the file name (or end of path) to consider.
///
/// @return true iff @p entry denotes a file which path ends with @p
/// fname.
static bool
entry_of_file_with_name(const FTSENT *entry,
			const string& fname)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return false;

  string fpath = entry->fts_path;
  if (string_ends_with(fpath, fname))
    return true;
  return false;
}

/// Find a given file under a root directory and return its absolute
/// path.
///
/// @param root_dir the root directory under which to look for.
///
/// @param file_path_to_look_for the file to look for under the
/// directory @p root_dir.
///
/// @param result the resulting path to @p file_path_to_look_for.
/// This is set iff the file has been found.
bool
find_file_under_dir(const string& root_dir,
		    const string& file_path_to_look_for,
		    string& result)
{
  char* paths[] = {const_cast<char*>(root_dir.c_str()), 0};

  FTS *file_hierarchy = fts_open(paths,
				 FTS_PHYSICAL|FTS_NOCHDIR|FTS_XDEV, 0);
  if (!file_hierarchy)
    return false;

  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    {
      // Skip descendents of symbolic links.
      if (entry->fts_info == FTS_SL || entry->fts_info == FTS_SLNONE)
	{
	  fts_set(file_hierarchy, entry, FTS_SKIP);
	  continue;
	}
      if (entry_of_file_with_name(entry, file_path_to_look_for))
	{
	  result = entry->fts_path;
	  return true;
	}
    }

  fts_close(file_hierarchy);
  return false;
}
/// If we were given suppression specification files or kabi whitelist
/// files, this function parses those, come up with suppression
/// specifications as a result, and set them to the read context.
///
/// @param read_ctxt the read context to consider.
///
/// @param suppr_paths paths to suppression specification files that
/// we were given.  If empty, it means we were not given any
/// suppression specification path.
///
/// @param kabi_whitelist_paths paths to kabi whitelist files that we
/// were given.  If empty, it means we were not given any kabi
/// whitelist.
///
/// @param supprs the suppressions specifications resulting from
/// parsing the suppression specification files at @p suppr_paths and
/// the kabi whitelist at @p kabi_whitelist_paths.
///
/// @param opts the options to consider.
static void
load_generate_apply_suppressions(dwarf_reader::read_context &read_ctxt,
				 vector<string>& suppr_paths,
				 vector<string>& kabi_whitelist_paths,
				 suppressions_type& supprs)
{
  if (supprs.empty())
    {
      for (vector<string>::const_iterator i = suppr_paths.begin();
	   i != suppr_paths.end();
	   ++i)
	read_suppressions(*i, supprs);

      const suppressions_type& wl_suppr =
	  gen_suppr_spec_from_kernel_abi_whitelists(kabi_whitelist_paths);

      supprs.insert(supprs.end(), wl_suppr.begin(), wl_suppr.end());
    }

  abigail::dwarf_reader::add_read_context_suppressions(read_ctxt, supprs);
}

/// Test if an FTSENT pointer (resulting from fts_read) represents the
/// vmlinux binary.
///
/// @param entry the FTSENT to consider.
///
/// @return true iff @p entry is for a vmlinux binary.
static bool
is_vmlinux(const FTSENT *entry)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return false;

  string fname = entry->fts_name;

  if (fname == "vmlinux")
    {
      string dirname;
      dir_name(entry->fts_path, dirname);
      if (string_ends_with(dirname, "compressed"))
	return false;

      return true;
    }

  return false;
}

/// Test if an FTSENT pointer (resulting from fts_read) represents a a
/// linux kernel module binary.
///
/// @param entry the FTSENT to consider.
///
/// @return true iff @p entry is for a linux kernel module binary.
static bool
is_kernel_module(const FTSENT *entry)
{
    if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return false;

  string fname = entry->fts_name;
  if (string_ends_with(fname, ".ko")
      || string_ends_with(fname, ".ko.xz")
      || string_ends_with(fname, ".ko.gz"))
    return true;

  return false;
}

/// Find a vmlinux and its kernel modules in a given directory tree.
///
/// @param from the directory tree to start looking from.
///
/// @param vmlinux_path output parameter.  This is set to the path
/// where the vmlinux binary is found.  This is set iff the returns
/// true and if this argument was empty to begin with.
///
/// @param module_paths output parameter.  This is set to the paths of
/// the linux kernel module binaries.
///
/// @return true iff at least the vmlinux binary was found.
static bool
find_vmlinux_and_module_paths(const string&	from,
			      string		&vmlinux_path,
			      vector<string>	&module_paths)
{
  char* path[] = {const_cast<char*>(from.c_str()), 0};

  FTS *file_hierarchy = fts_open(path, FTS_PHYSICAL|FTS_NOCHDIR|FTS_XDEV, 0);
  if (!file_hierarchy)
    return false;

  bool found_vmlinux = !vmlinux_path.empty();
  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    {
      // Skip descendents of symbolic links.
      if (entry->fts_info == FTS_SL || entry->fts_info == FTS_SLNONE)
	{
	  fts_set(file_hierarchy, entry, FTS_SKIP);
	  continue;
	}

      if (!found_vmlinux && is_vmlinux(entry))
	{
	  vmlinux_path = entry->fts_path;
	  found_vmlinux = true;
	}
      else if (is_kernel_module(entry))
	module_paths.push_back(entry->fts_path);
    }

  fts_close(file_hierarchy);

  return found_vmlinux;
}

/// Find a vmlinux binary in a given directory tree.
///
/// @param from the directory tree to start looking from.
///
/// @param vmlinux_path output parameter
///
/// return true iff the vmlinux binary was found
static bool
find_vmlinux_path(const string&	from,
		  string		&vmlinux_path)
{
  char* path[] = {const_cast<char*>(from.c_str()), 0};

  FTS *file_hierarchy = fts_open(path, FTS_PHYSICAL|FTS_NOCHDIR|FTS_XDEV, 0);
  if (!file_hierarchy)
    return false;

  bool found_vmlinux = false;
  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    {
      // Skip descendents of symbolic links.
      if (entry->fts_info == FTS_SL || entry->fts_info == FTS_SLNONE)
	{
	  fts_set(file_hierarchy, entry, FTS_SKIP);
	  continue;
	}

      if (!found_vmlinux && is_vmlinux(entry))
	{
	  vmlinux_path = entry->fts_path;
	  found_vmlinux = true;
	  break;
	}
    }

  fts_close(file_hierarchy);

  return found_vmlinux;
}

/// Get the paths of the vmlinux and kernel module binaries under
/// given directory.
///
/// @param dist_root the directory under which to look for.
///
/// @param debug_info_root_path the path to the directory under which
/// debug info is going to be found for binaries under @p dist_root.
///
/// @param vmlinux_path output parameter.  The path of the vmlinux
/// binary that was found.
///
/// @param module_paths output parameter.  The paths of the kernel
/// module binaries that were found, sorted to impose a deterministic
/// ordering.
///
/// @return true if at least the path to the vmlinux binary was found.
bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  const string&	debug_info_root_path,
				  string&		vmlinux_path,
				  vector<string>&	module_paths)
{
  if (!dir_exists(dist_root))
    return false;

  // For now, we assume either an Enterprise Linux or a Fedora kernel
  // distribution directory.
  //
  // We also take into account split debug info package for these.  In
  // this case, the content split debug info package is installed
  // under the 'debug_info_root_path' directory and its content is
  // accessible from <debug_info_root_path>/usr/lib/debug directory.

  string kernel_modules_root;
  string debug_info_root;
  if (dir_exists(dist_root + "/lib/modules"))
    {
      dist_root + "/lib/modules";
      debug_info_root = debug_info_root_path.empty()
	? dist_root
	: debug_info_root_path;
      debug_info_root += "/usr/lib/debug";
    }

  if (dir_is_empty(debug_info_root))
    debug_info_root.clear();

  bool found = false;
  string from = dist_root;
  if (find_vmlinux_and_module_paths(from, vmlinux_path, module_paths))
    found = true;

  std::sort(module_paths.begin(), module_paths.end());

  return found;
}

/// Get the path of the vmlinux binary under the given directory, that
/// must have been generated either from extracting a package.
///
/// @param from the directory under which to look for.
///
/// @param vmlinux_path output parameter.  The path of the vmlinux
/// binary that was found.
///
/// @return true if the path to the vmlinux binary was found.
bool
get_vmlinux_path_from_kernel_dist(const string&	from,
				  string&		vmlinux_path)
{
    if (!dir_exists(from))
    return false;

  // For now, we assume the possibility of having either an Enterprise
  // Linux or a Fedora kernel distribution directory.  In those cases,
  // the vmlinux binary is located under the /lib/modules
  // sub-directory.  So we might as well save some time by picking it
  // from there directly.

    string dist_root = from;
  if (dir_exists(dist_root + "/lib/modules"))
    dist_root += "/lib/modules";

  bool found = false;
  if (find_vmlinux_path(dist_root, vmlinux_path))
    found = true;

  return found;
}

/// Get the paths of the vmlinux and kernel module binaries under
/// given directory.
///
/// @param dist_root the directory under which to look for.
///
/// @param vmlinux_path output parameter.  The path of the vmlinux
/// binary that was found.
///
/// @param module_paths output parameter.  The paths of the kernel
/// module binaries that were found.
///
/// @return true if at least the path to the vmlinux binary was found.
bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  string&		vmlinux_path,
				  vector<string>&	module_paths)
{
  string debug_info_root_path;
  return get_binary_paths_from_kernel_dist(dist_root,
					   debug_info_root_path,
					   vmlinux_path,
					   module_paths);
}

/// Walk a given directory and build an instance of @ref corpus_group
/// from the vmlinux kernel binary and the linux kernel modules found
/// under that directory and under its sub-directories, recursively.
///
/// The main corpus of the @ref corpus_group is made of the vmlinux
/// binary.  The other corpora are made of the linux kernel binaries.
///
/// @param root the path of the directory under which the kernel
/// kernel modules are to be found.  The vmlinux can also be found
/// somewhere under that directory, but if it's not in there, its path
/// can be set to the @p vmlinux_path parameter.
///
/// @param debug_info_root the directory under which debug info is to
/// be found for binaries under director @p root.
///
/// @param vmlinux_path the path to the vmlinux binary, if that binary
/// is not under the @p root directory.  If this is empty, then it
/// means the vmlinux binary is to be found under the @p root
/// directory.
///
/// @param suppr_paths the paths to the suppression specifications to
/// apply while loading the binaries.
///
/// @param kabi_wl_path the paths to the kabi whitelist files to take
/// into account while loading the binaries.
///
/// @param supprs the suppressions resulting from parsing the
/// suppression specifications at @p suppr_paths.  This is set by this
/// function.
///
/// @param verbose true if the function has to emit some verbose
/// messages.
///
/// @param env the environment to create the corpus_group in.
corpus_group_sptr
build_corpus_group_from_kernel_dist_under(const string&	root,
					  const string		debug_info_root,
					  const string&	vmlinux_path,
					  vector<string>&	suppr_paths,
					  vector<string>&	kabi_wl_paths,
					  suppressions_type&	supprs,
					  bool			verbose,
					  environment_sptr&	env)
{
  string vmlinux = vmlinux_path;
  corpus_group_sptr result;
  vector<string> modules;

  if (verbose)
    std::cerr << "Analysing kernel dist root '"
	      << root << "' ... " << std::flush;

  timer t;

  t.start();
  bool got_binary_paths =
    get_binary_paths_from_kernel_dist(root, debug_info_root, vmlinux, modules);
  t.stop();

  if (verbose)
    std::cerr << "DONE: " << t << "\n";

  dwarf_reader::read_context_sptr ctxt;
  if (got_binary_paths)
    {
      shared_ptr<char> di_root =
	make_path_absolute(debug_info_root.c_str());
      char *di_root_ptr = di_root.get();
      vector<char**> di_roots;
      di_roots.push_back(&di_root_ptr);
      abigail::dwarf_reader::status status = abigail::dwarf_reader::STATUS_OK;
      corpus_group_sptr group;
      if (!vmlinux.empty())
	{
	  ctxt =
	    dwarf_reader::create_read_context(vmlinux, di_roots ,env.get(),
					      /*read_all_types=*/false,
					      /*linux_kernel_mode=*/true);
	  dwarf_reader::set_do_log(*ctxt, verbose);

	  t.start();
	  load_generate_apply_suppressions(*ctxt, suppr_paths,
					   kabi_wl_paths, supprs);
	  t.stop();

	  if (verbose)
	    std::cerr << "loaded white list and generated suppr spec in: "
		      << t
		      << "\n";

	  group.reset(new corpus_group(env.get(), root));

	  set_read_context_corpus_group(*ctxt, group);

	  if (verbose)
	    std::cerr << "reading kernel binary '"
		      << vmlinux << "' ...\n" << std::flush;

	  // Read the vmlinux corpus and add it to the group.
	  t.start();
	  read_and_add_corpus_to_group_from_elf(*ctxt, *group, status);
	  t.stop();

	  if (verbose)
	    std::cerr << vmlinux
		      << " reading DONE:"
		      << t << "\n";
	}

      if (!group->is_empty())
	{
	  // Now add the corpora of the modules to the corpus group.
	  int total_nb_modules = modules.size();
	  int cur_module_index = 1;
	  for (vector<string>::const_iterator m = modules.begin();
	       m != modules.end();
	       ++m, ++cur_module_index)
	    {
	      if (verbose)
		std::cerr << "reading module '"
			  << *m << "' ("
			  << cur_module_index
			  << "/" << total_nb_modules
			  << ") ... " << std::flush;

	      reset_read_context(ctxt, *m, di_roots, env.get(),
				 /*read_all_types=*/false,
				 /*linux_kernel_mode=*/true);

	      load_generate_apply_suppressions(*ctxt, suppr_paths,
					       kabi_wl_paths, supprs);

	      set_read_context_corpus_group(*ctxt, group);

	      t.start();
	      read_and_add_corpus_to_group_from_elf(*ctxt,
						    *group, status);
	      t.stop();
	      if (verbose)
		std::cerr << "module '"
			  << *m
			  << "' reading DONE: "
			  << t << "\n";
	    }

	  result = group;
	}
    }

  return result;
}

}//end namespace tools_utils

using abigail::ir::function_decl;

/// Dump (to the standard error stream) two sequences of strings where
/// each string represent one of the functions in the two sequences of
/// functions given in argument to this function.
///
/// @param a_begin the begin iterator for the first input sequence of
/// functions.
///
/// @parm a_end the end iterator for the first input sequence of
/// functions.
///
/// @param b_begin the begin iterator for the second input sequence of
/// functions.
///
/// @param b_end the end iterator for the second input sequence of functions.
void
dump_functions_as_string(std::vector<function_decl*>::const_iterator a_begin,
			 std::vector<function_decl*>::const_iterator a_end,
			 std::vector<function_decl*>::const_iterator b_begin,
			 std::vector<function_decl*>::const_iterator b_end)
{abigail::fns_to_str(a_begin, a_end, b_begin, b_end, std::cerr);}

/// Dump (to the standard error output stream) a pretty representation
/// of the signatures of two sequences of functions.
///
/// @param a_begin the start iterator of the first input sequence of functions.
///
/// @param a_end the end iterator of the first input sequence of functions.
///
/// @param b_begin the start iterator of the second input sequence of functions.
///
/// @param b_end the end iterator of the second input sequence of functions.
void
dump_function_names(std::vector<function_decl*>::const_iterator a_begin,
		    std::vector<function_decl*>::const_iterator a_end,
		    std::vector<function_decl*>::const_iterator b_begin,
		    std::vector<function_decl*>::const_iterator b_end)
{
  std::vector<function_decl*>::const_iterator i;
  std::ostream& o = std::cerr;
  for (i = a_begin; i != a_end; ++i)
    o << (*i)->get_pretty_representation() << "\n";

  o << "  ->|<-  \n";
  for (i = b_begin; i != b_end; ++i)
    o << (*i)->get_pretty_representation() << "\n";
  o << "\n";
}

/// Compare two functions that are in a vector of functions.
///
/// @param an iterator to the beginning of the the sequence of functions.
///
/// @param f1_index the index of the first function to compare.
///
/// @param f2_inde the index of the second function to compare
bool
compare_functions(vector<function_decl*>::const_iterator base,
		  unsigned f1_index, unsigned f2_index)
{
  function_decl* fn1 = base[f1_index];
  function_decl* fn2 = base[f2_index];

  return *fn1 == *fn2;
}

}//end namespace abigail
