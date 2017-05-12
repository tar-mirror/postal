#ifndef EXPAND_H
#define EXPAND_H

#include <string>
#include <regex.h>

class NamePattern;

// This class stores a set of patterns to match and the replacement data for
// each pattern.
class NameExpand
{
public:
  // Read the file with the specified name and parse it as a list of
  // expansions to perform.
  NameExpand(const char *filename);

  // expand the input to a random output based on our replacement patterns.
  int expand(string &output, const string &input, bool sequential = false);

private:
  // past the end of the sequence
  bool m_no_seq;
  NamePattern *m_names;
};

// This class stores a pattern to match and the replacement data.
// It also can have a pointer to another NamePattern, if so it will search
// recursively if it's pattern is not matched.
class NamePattern
{
public:
  // match is a regular expression that input string should match.
  // replace is the pattern for replacing.  An unescaped (escape is back-slash
  // '.' will mean not to change that character.  Otherwise it's either a
  // single character to replace it with, or a list of characters inside
  // '[' and ']'.  Ranges can be denoted with a hyphen as usual for globbing.
  NamePattern(const char *match, const char *replace);
  ~NamePattern();

  // expand the input to a random output based on our replacement pattern (if
  // our regex is matched).  If not then call recursively if we have a next
  // NamePattern.
  int expand(string &output, const string &input, bool sequential = false);

  // Add the next pattern to the list.
  void AddNext(NamePattern *np) { m_next = np; }

private:
  // The pattern we are matching.  Not actually used after startup.
  char *m_match;

  // The pattern we replace with.  Again not used other than diagnostics
  // after startup.
  char *m_replace;

  // The compiled regular expression for matching.
  regex_t m_match_regex;

  // The array of strings used for conversion.  For every character position
  // that is to be changed there will be a pointer in this array (NULL means
  // no change).  Each character in each string is a random character to be
  // used for replacement.  If a character happens to occur twice then it will
  // be twice as likely to occur in output.
  char **m_convert;

  // The length of the strings above.  Saves lots of strlen() calls.
  int *m_conv_item_len;

  // The next number to be used for each element
  int *m_next_value;

  // past the end of the sequence
  bool m_no_seq;

  // The number of elements in the above arrays.
  int m_conv_len;

  // Pointer to the next pattern (or NULL).
  NamePattern *m_next;
};

#endif
