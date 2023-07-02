/*scanner.c*/

//
// Scanner for SimpleSQL programming language. The scanner
// reads the input stream and turns the characters into
// language Tokens, such as identifiers, keywords, and
// punctuation.
//
// Victor Agaba
// Northwestern University
// CS 211, Winter 2023
//
// Contributing author: Prof. Joe Hummel
//

#include <ctype.h>   // isspace, isdigit, isalpha
#include <stdbool.h> // true, false
#include <stdio.h>
#include <string.h> // stricmp
#include <strings.h>

#include "scanner.h"
#include "util.h"

//
// SimpleSQL keywords, in alphabetical order. Note that "static"
// means the array / variable is not accessible outside this file,
// which is intentional and good design.
//
static char *keywords[] = {"asc",  "avg",   "by",     "count",  "delete",
                           "desc", "from",  "inner",  "insert", "intersect",
                           "into", "join",  "like",   "limit",  "max",
                           "min",  "on",    "order",  "select", "set",
                           "sum",  "union", "update", "values", "where"};

static int numKeywords = sizeof(keywords) / sizeof(keywords[0]);

//
// returns token id if a keyword, otherwise returns SQL_IDENTIFIER:
//
static int isKeyword(char *value) {
  // linear search through array left to right, looking for match:
  for (int i = 0; i < numKeywords; i++) {
    if (strcasecmp(keywords[i], value) == 0) // match!
    {
      return SQL_KEYW_ASC + i; // token id of the keyword:
    }
  }
  // if get here, loop ended => not found:
  return SQL_IDENTIFIER;
}

// void consumeRestOfLine(FILE* input)
// {
//   int c = fgetc(input);
//   while (c != '\n' && c != EOF)
//     c = fgetc(input);
// }

//
// scanner_init
//
// Initializes line number, column number, and value before
// the start of the next input sequence.
//
void scanner_init(int *lineNumber, int *colNumber, char *value) {
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_init)");

  *lineNumber = 1;
  *colNumber = 1;
  value[0] = '\0'; // empty string ""
}

//
// scanner_nextToken
//
// Returns the next token in the given input stream, advancing the line
// number and column number as appropriate. The token's string-based
// value is returned via the "value" parameter. For example, if the
// token returned is an integer literal, then the value returned is
// the actual literal in string form, e.g. "123". For an identifer,
// the value is the identifer itself, e.g. "ID" or "title". For a
// string literal, the value is the contents of the string literal
// without the quotes.
//
struct Token scanner_nextToken(FILE *input, int *lineNumber, int *colNumber,
                               char *value) {
  if (input == NULL)
    panic("input stream is NULL (scanner_nextToken)");
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_nextToken)");

  struct Token T;

  //
  // repeatedly input characters one by one until a token is found:
  //
  while (true) {
    int c = fgetc(input);

    if (c == EOF) // no more input, return EOS:
    {
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;
      *colNumber += 1;

      value[0] = '$';
      value[1] = '\0';

      return T;
    } else if (c == '$') // this is also EOF / EOS
    {
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == ';') {
      T.id = SQL_SEMI_COLON;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '>') // could be > or >=
    {
      //
      // peek ahead to the next char:
      //
      c = fgetc(input);

      if (c == '=') {
        T.id = SQL_GTE;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '>';
        value[1] = '=';
        value[2] = '\0';
        *colNumber += 2;

        return T;
      }
      //
      // if we get here, then next char did not form a token, so
      // we need to put char back to be processed on next call:
      //
      ungetc(c, input);

      T.id = SQL_GT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '>';
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '<') {
      c = fgetc(input);

      if (c == '=') {
        T.id = SQL_LTE;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '<';
        value[1] = '=';
        value[2] = '\0';
        *colNumber += 2;

        return T;
      } else if (c == '>') {
        T.id = SQL_NOT_EQUAL;
        T.line = *lineNumber;
        T.col = *colNumber;

        value[0] = '<';
        value[1] = '>';
        value[2] = '\0';
        *colNumber += 2;

        return T;
      }
      ungetc(c, input);

      T.id = SQL_LT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '<';
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '(') {
      T.id = SQL_LEFT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == ')') {
      T.id = SQL_RIGHT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '*') {
      T.id = SQL_ASTERISK;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '.') {
      T.id = SQL_DOT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '#') {
      T.id = SQL_HASH;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == ',') {
      T.id = SQL_COMMA;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '=') {
      T.id = SQL_EQUAL;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    } else if (c == '"' || c == '\'') {
      T.id = SQL_STR_LITERAL;
      T.line = *lineNumber;
      T.col = *colNumber;
      int i = 0;
      bool good_end = false;
      bool bad_end = false;

      if (c == '"') {
        c = fgetc(input);
        while (!good_end && !bad_end) {
          if (c == '"') {
            good_end = true;
            break;
          }
          value[i] = (char)c;
          c = fgetc(input);
          i += 1;
          if (c == '"') {
            good_end = true;
          } else if (c == '\n' || c == EOF) {
            if (c == '\n') {
              printf("**WARNING: string literal @ (%d %d) not terminated "
                     "properly.\n",
                     *lineNumber, *colNumber);
              bad_end = true;
              *colNumber = 1;
              *lineNumber += 1;
              // Need to check for EOF alternative?
            } else {
              printf("**WARNING: string literal @ (%d %d) not terminated "
                     "properly.\n",
                     *lineNumber, *colNumber);
            }
          }
        }

        value[i] = '\0';
        if (good_end) {
          *colNumber += (i + 2);
        }

        return T;
      } else {
        c = fgetc(input);
        while (!good_end && !bad_end) {
          if (c == '\'') {
            good_end = true;
            break;
          }
          value[i] = (char)c;
          c = fgetc(input);
          i += 1;
          if (c == '\'') {
            good_end = true;
          } else if (c == '\n' || c == EOF) {
            if (c == '\n') {
              printf("**WARNING: string literal @ (%d %d) not terminated "
                     "properly.\n",
                     *lineNumber, *colNumber);
              bad_end = true;
              *colNumber = 1;
              *lineNumber += 1;
              // Need to check for EOF alternative?
            } else {
              printf("**WARNING: string literal @ (%d %d) not terminated "
                     "properly.\n",
                     *lineNumber, *colNumber);
            }
          }
        }

        value[i] = '\0';
        if (good_end) {
          *colNumber += (i + 2);
        }

        return T;
      }
    } else if (isdigit(c)) {

      T.line = *lineNumber;
      T.col = *colNumber;

      int i = 0;
      bool real = false;

      while (isdigit(c)) {
        value[i] = (char)c;
        c = fgetc(input);
        i += 1;
        if (c == '.' && !real) {
          real = true;
          value[i] = (char)c;
          c = fgetc(input);
          i += 1;
        }
      }
      ungetc(c, input);
      value[i] = '\0';
      *colNumber += i;

      if (real) {
        T.id = SQL_REAL_LITERAL;
      } else {
        T.id = SQL_INT_LITERAL;
      }

      return T;
    } else if (c == '+' || c == '-') {
      if (c == '+') {
        c = fgetc(input);

        if (isdigit(c)) {
          T.line = *lineNumber;
          T.col = *colNumber;
          value[0] = '+';
          int i = 1;
          bool real = false;

          while (isdigit(c)) {
            value[i] = (char)c;
            c = fgetc(input);
            i += 1;
            if (c == '.' && !real) {
              real = true;
              value[i] = (char)c;
              c = fgetc(input);
              i += 1;
            }
          }
          ungetc(c, input);
          value[i] = '\0';
          *colNumber += i;

          if (real) {
            T.id = SQL_REAL_LITERAL;
          } else {
            T.id = SQL_INT_LITERAL;
          }

          return T;
        } else {
          ungetc(c, input);
          T.id = SQL_UNKNOWN;
          T.line = *lineNumber;
          T.col = *colNumber;
    
          value[0] = '+';
          value[1] = '\0';
          *colNumber += 1;
    
          return T;
        }
      } else {
        c = fgetc(input);

        if (isdigit(c)) {
          T.line = *lineNumber;
          T.col = *colNumber;
          value[0] = '-';
          int i = 1;
          bool real = false;

          while (isdigit(c)) {
            value[i] = (char)c;
            c = fgetc(input);
            i += 1;

            if (c == '.' && !real) {
              real = true;
              value[i] = (char)c;
              c = fgetc(input);
              i += 1;
            }
          }
          ungetc(c, input);
          value[i] = '\0';
          *colNumber += i;

          if (real) {
            T.id = SQL_REAL_LITERAL;
          } else {
            T.id = SQL_INT_LITERAL;
          }

          return T;
        } else if (c == '-') {
          while (c != '\n' && c != EOF)
            c = fgetc(input);
          *colNumber = 1;
          *lineNumber += 1;
        } else {
          ungetc(c, input);
          T.id = SQL_UNKNOWN;
          T.line = *lineNumber;
          T.col = *colNumber;
    
          value[0] = '-';
          value[1] = '\0';
          *colNumber += 1;
    
          return T;
        }
      }
    } else if (isalnum(c)) {
      T.line = *lineNumber;
      T.col = *colNumber;
      int i = 0;

      while (isalnum(c) || c == '_') {
        value[i] = (char)c;
        c = fgetc(input);
        i += 1;
      }
      ungetc(c, input);
      value[i] = '\0';
      *colNumber += i;

      T.id = isKeyword(value);

      return T;
    } else if (isspace(c)) {
      if (c == '\n') {
        *colNumber = 1;
        *lineNumber += 1;
      } else {
        *colNumber += 1;
      }
    }

    else {
      //
      // if we get here, then char denotes an UNKNOWN token:
      //
      T.id = SQL_UNKNOWN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';
      *colNumber += 1;

      return T;
    }

  } // while

  //
  // execution should never get here, return occurs
  // from within loop
  //
  printf("Unexpectedly reached end of while loop");
}
