#include "expand.h"
// for strdup() and strtok()
#include <cstring>
// for random()
#include <cstdlib>
#include <stdio.h>

typedef char * PCHAR;

NamePattern::NamePattern(const char *match, const char *replace)
 : m_match(strdup(match))
 , m_replace(strdup(replace))
 , m_conv_len(strlen(m_replace))
 , m_convert(new PCHAR[m_conv_len])
 , m_conv_item_len(new int[m_conv_len])
 , m_next_value(new int[m_conv_len])
 , m_no_seq(false)
 , m_next(NULL)
{
  regcomp(&m_match_regex, m_match, REG_NOSUB);
  int i;
  for(i = 0; i < m_conv_len; i++)
    m_next_value[i] = 0;
  int pos = 0;
  bool paren = false;
  bool escape = false;
  bool hyphen = false;

  char buf[1024];
  int buf_pos = 0;

  for(i = 0; replace[i]; i++)
  {
    switch(replace[i])
    {
    case '.':
      if(!paren)
      {
        m_convert[pos] = NULL;
        m_conv_item_len[pos] = 0;
        pos++;
      }
      else
      {
        buf[buf_pos] = '.';
        buf_pos++;
      }
    break;
    case '[':
      if(paren)
      {
        if(!escape)
        {
          printf("Unmatched brackets in \"%s\".\n", replace);
          exit(1);
        }
        else
        {
          buf[buf_pos] = '[';
          buf_pos++;
        }
      }
      else
      {
        paren = true;
      }
    break;
    case ']':
      if(!paren)
      {
        printf("Unmatched brackets in \"%s\".\n", replace);
        exit(1);
      }
      if(!escape)
      {
        if(hyphen)
        {
          printf("Unfinished hyphen in \"%s\".\n", replace);
          exit(1);
        }
        buf[buf_pos] = '\0';
        m_convert[pos] = strdup(buf);
        m_conv_item_len[pos] = strlen(m_convert[pos]);
        pos++;
        buf_pos = 0;
        paren = false;
        break;
      } // if escape then fall-through
    case '-':
      if(!paren)
      {
        printf("Misplaced hyphen in \"%s\".\n", replace);
        exit(1);
      }
      hyphen = true;
    break;
    default:
      if(paren)
      {
        if(!hyphen)
        {
          buf[buf_pos] = replace[i];
          buf_pos++;
          if(buf_pos == sizeof(buf))
          {
            printf("Out of buffer space for \"%s\".\n", replace);
            exit(1);
          }
        }
        else
        {
          for(char c = buf[buf_pos - 1] + 1; c <= replace[i] && c < 127; c++)
          {
            buf[buf_pos] = c;
            buf_pos++;
            if(buf_pos == sizeof(buf))
            {
              printf("Out of buffer space for \"%s\".\n", replace);
              exit(1);
            }
          }
          hyphen = false;
        }
      }
      else
      {
        printf("Can't parse replace string \"%s\".\n", replace);
        exit(1);
      }
    } // end switch
  } // end for
  m_conv_len = pos;
}

NamePattern::~NamePattern()
{
  for(int i = 0; i < m_conv_len; i++)
    delete m_convert[i];
  delete m_convert;
  delete m_conv_item_len;
  delete m_next_value;
  regfree(&m_match_regex);
  delete m_match;
  delete m_replace;
}

int NamePattern::expand(string &output, const string &input, bool sequential)
{
  int i;
  if(sequential && m_no_seq)
  {
    for(i = 0; i < m_conv_len; i++)
      m_next_value[i] = 0;
    m_no_seq = false;
    return 1;
  }
  if(regexec(&m_match_regex, input.c_str(), 0, NULL, 0))
  {
    if(m_next)
      return m_next->expand(output, input, sequential);
    else
      return 1;
  }
  output = input;
  for(i = 0; i < m_conv_len; i++)
  {
    if(m_convert[i])
    {
      if(!sequential)
      {
        output[i] = m_convert[i][random()%m_conv_item_len[i]];
      }
      else
      {
        output[i] = m_convert[i][m_next_value[i]];
      }
    }
  }
  if(sequential)
  {
    for(i = m_conv_len - 1; i >= 0; i--)
    {
      if(m_convert[i])
      {
        m_next_value[i]++;
        if(m_next_value[i] >= m_conv_item_len[i])
          m_next_value[i] = 0;
        else
          return 0;
      }
    }
    m_no_seq = true;
  }
  return 0;
}

NameExpand::NameExpand(const char *filename)
 : m_no_seq(true)
 , m_names(NULL)
{
  if(!filename || strcmp(filename, "-") == 0)
  {
    return;
  }
  FILE *fp = fopen(filename, "r");
  if(!fp)
  {
    printf("Can't open config file \"%s\".  Doing no expansion.\n", filename);
    return;
  }
  char match[1024];
  NamePattern *np = NULL, *old_np = NULL;
  while(fgets(match, sizeof(match), fp))
  {
    match[sizeof(match) - 1] = '\0';
    strtok(match, "\n");
    unsigned int i;
    int blank_pos = -1;
    for(i = 0; match[i]; i++)
    {
      if(match[i] == ' ')
      {
        if(blank_pos != -1)
        {
          printf("Can't parse pattern line \"%s\".\n", match);
          exit(1);
        }
        blank_pos = i;
      }
    }
    if(blank_pos == -1)
    {
      printf("Can't parse pattern line \"%s\".\n", match);
      exit(1);
    }
    char *replace = &match[blank_pos + 1];
    match[blank_pos] = '\0';
    np = new NamePattern(match, replace);
    if(!m_names)
      m_names = np;
    if(old_np)
      old_np->AddNext(np);
    old_np = np;
  }
  fclose(fp);
}

int NameExpand::expand(string &output, const string &input, bool sequential)
{
  if(!m_names)
  {
    if(sequential)
    {
      if(m_no_seq)
      {
        m_no_seq = false;
        return 1;
      }
      else
      {
        output = input;
        m_no_seq = true;
        return 0;
      }
    }
    output = input;
    return 1;
  }
  return m_names->expand(output, input, sequential);
}
