/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// Victor Agaba
// Northwestern University
// CS 211, Winter 2023
//

#include <assert.h> // assert
#include <ctype.h>
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcpy, strcat
#include <strings.h>
//
// #include any other system <.h> files?
//

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "execute.h"
#include "parser.h"
#include "resultset.h"
#include "scanner.h"
#include "tokenqueue.h"
#include "util.h"
//
// #include any other of our ".h" files?
//

//
// implementation of function(s), both private and public
//

// Helper function to compare ints in where expr
static bool expr_int(int basis, int op, int value) {
  if (op == EXPR_LT && value < basis)
    return true;
  else if (op == EXPR_LTE && value <= basis)
    return true;
  else if (op == EXPR_GT && value > basis)
    return true;
  else if (op == EXPR_GTE && value >= basis)
    return true;
  else if (op == EXPR_EQUAL && value == basis)
    return true;
  else if (op == EXPR_NOT_EQUAL && value != basis)
    return true;
  return false;
}

// Helper function to compare reals in where expr
static bool expr_real(double basis, int op, double value) {
  if (op == EXPR_LT && value < basis)
    return true;
  else if (op == EXPR_LTE && value <= basis)
    return true;
  else if (op == EXPR_GT && value > basis)
    return true;
  else if (op == EXPR_GTE && value >= basis)
    return true;
  else if (op == EXPR_EQUAL && value == basis)
    return true;
  else if (op == EXPR_NOT_EQUAL && value != basis)
    return true;
  return false;
}

// Helper function to compare strings in where expr
static bool expr_str(char *basis, int op, char *value) {
  if (op == EXPR_LT && strcasecmp(value, basis) < 0)
    return true;
  else if (op == EXPR_LTE && strcasecmp(value, basis) <= 0)
    return true;
  else if (op == EXPR_GT && strcasecmp(value, basis) > 0)
    return true;
  else if (op == EXPR_GTE && strcasecmp(value, basis) >= 0)
    return true;
  else if (op == EXPR_EQUAL && strcasecmp(value, basis) == 0)
    return true;
  else if (op == EXPR_NOT_EQUAL && strcasecmp(value, basis) != 0)
    return true;
  return false;
}

void execute_query(struct Database *db, struct QUERY *query) {
  if (db == NULL)
    panic("db is NULL (execute)");
  if (query == NULL)
    panic("query is NULL (execute)");

  if (query->queryType != SELECT_QUERY) {
    printf("**INTERNAL ERROR: execute() only supports SELECT queries.\n");
    return;
  }

  struct SELECT *select = query->q.select; // alias for less typing:

  //
  // the query has been analyzed and so we know it's correct: the
  // database exists, the table(s) exist, the column(s) exist, etc.
  //

  //
  // (1) we need a pointer to the table meta data, so find it:
  //
  struct TableMeta *tablemeta = NULL; // WHAT I WANT

  for (int t = 0; t < db->numTables; t++) {
    if (icmpStrings(db->tables[t].name, select->table) == 0) // found it:
    {
      tablemeta = &db->tables[t];
      break;
    }
  }

  assert(tablemeta != NULL);

  //
  // (2) open the table's data file
  //
  // the table exists within a sub-directory under the executable
  // where the directory has the same name as the database, and with
  // a "TABLE-NAME.data" filename within that sub-directory:
  //
  char path[(2 * DATABASE_MAX_ID_LENGTH) + 10];

  strcpy(path, db->name); // name/name.data
  strcat(path, "/");
  strcat(path, tablemeta->name);
  strcat(path, ".data");

  FILE *datafile = fopen(path, "r");
  if (datafile == NULL) // unable to open:
  {
    printf("**INTERNAL ERROR: table's data file '%s' not found.\n", path);
    panic("execution halted");
  }

  struct ResultSet *rs = resultset_create();

  // Add cols to tablemeta
  for (int i = 0; i < tablemeta->numColumns; i++) {
    int colNum = resultset_insertColumn(rs, i + 1, tablemeta->name,
                                        tablemeta->columns[i].name, NO_FUNCTION,
                                        tablemeta->columns[i].colType);
    // rs->numCols += 1; **THIS BUG NEARLY KILLED ME
  }

  //
  // (3) allocate a buffer for input, and start reading the data:
  //
  //
  int dataBufferSize =
      tablemeta->recordSize + 3; // ends with $\n + null terminator
  char *dataBuffer = (char *)malloc(sizeof(char) * dataBufferSize);
  if (dataBuffer == NULL)
    panic("out of memory");

  while (true) { // each line
    fgets(dataBuffer, dataBufferSize, datafile);
    if (feof(datafile))
      break;
    // printf("%s", dataBuffer);

    bool quote1 = false;
    bool quote2 = false;
    char *cp = dataBuffer;

    for (int i = 0; i < dataBufferSize; i++) { // each character
      if (!quote1 && !quote2) {                // non-string case
        if (*cp == ' ') {
          *cp = '\0';
        } else if (dataBuffer[i] == '\'')
          quote1 = true;
        else if (dataBuffer[i] == '"')
          quote2 = true;
      } else if (quote1) {
        if (*cp == '\'')
          quote1 = false;
      } else {
        if (*cp == '"')
          quote2 = false;
      }
      cp++;
    }

    // Adding mini-strings to the resultset
    int colNum = 1;
    cp = dataBuffer;
    struct RSColumn *curr = rs->columns;
    int rowNum = resultset_addRow(rs);
    while (curr != NULL) {
      if (curr->coltype == COL_TYPE_INT) {
        resultset_putInt(rs, rowNum, colNum, atoi(cp));
        cp += strlen(cp) + 1;
      } else if (curr->coltype == COL_TYPE_REAL) {
        resultset_putReal(rs, rowNum, colNum, atof(cp));
        cp += strlen(cp) + 1;
      } else if (curr->coltype == COL_TYPE_STRING) {
        char *last = cp + strlen(cp) - 1;
        *last = '\0';
        cp++;
        resultset_putString(rs, rowNum, colNum, cp);
        cp += strlen(cp) + 2;
      }

      colNum++;
      curr = curr->next;
    }
  }
  free(dataBuffer);

  struct WHERE *where = select->where;
  if (where != NULL) {
    // Find the column and traverse the rows
    // Where contains a pointer to an expr (operator, litType, value)
    struct EXPR *expr = where->expr;
    int operator= expr->operator;
    char *colName = expr->column->name;
    char *tabName = expr->column->table;
    int colNum = resultset_findColumn(rs, 1, tabName, colName);

    // Go through resultset to column represented by pos
    struct RSColumn *col = rs->columns;
    for (int i = 0; i < colNum - 1; i++) {
      col = col->next;
      if (col == NULL) {
        printf("**Didn't find column (execute-where)\n");
        break;
      }
    }

    // Loop through each row of column to evaluate where expr
    int op = expr->operator;
    for (int rowNum = rs->numRows; rowNum > 0; rowNum--) {
      if (col->coltype == COL_TYPE_INT) {
        int value = resultset_getInt(rs, rowNum, colNum);
        int basis = atoi(expr->value);
        if (!expr_int(basis, op, value)) {
          resultset_deleteRow(rs, rowNum);
        }
      } else if (col->coltype == COL_TYPE_REAL) {
        double value = resultset_getReal(rs, rowNum, colNum);
        double basis = atof(expr->value);
        if (!expr_real(basis, op, value)) {
          resultset_deleteRow(rs, rowNum);
        }
      } else if (col->coltype == COL_TYPE_STRING) {
        char *value = resultset_getString(rs, rowNum, colNum);
        char *basis = expr->value;
        if (!expr_str(basis, op, value)) {
          resultset_deleteRow(rs, rowNum);
        }
        free(value);
      }
    }
  }

  // Loop through tablemeta columns to delete
  char *tabName = tablemeta->name;
  for (int i = 0; i < tablemeta->numColumns; i++) {
    struct COLUMN *astCol = select->columns;
    char *currName = tablemeta->columns[i].name;
    bool its_in = false;
    while (astCol != NULL) {
      if (strcasecmp(currName, astCol->name) == 0) {
        its_in = true;
      }
      astCol = astCol->next;
    }
    if (!its_in) {
      int colPos = resultset_findColumn(rs, 1, tabName, currName);
      resultset_deleteColumn(rs, colPos);
    }
  }

  // Loop through AST columns to reorder
  int i = 1;
  struct COLUMN *astCol = select->columns;
  while (astCol != NULL) {
    int colNum = resultset_findColumn(rs, 1, tabName, astCol->name);
    resultset_moveColumn(rs, colNum, i);
    i++;
    astCol = astCol->next;
  }

  // Loop through AST columns to apply functions
  i = 1;
  astCol = select->columns;
  while (astCol != NULL) {
    int func = astCol->function;
    if (func != NO_FUNCTION) {
      resultset_applyFunction(rs, func, i);
    }
    astCol = astCol->next;
    i++;
  }

  // Delete all rows after limit
  struct LIMIT *limit = select->limit;
  if (limit != NULL) {
    assert(limit->N >= 0);
    while (rs->numRows > limit->N) {
      resultset_deleteRow(rs, rs->numRows);
    }
  }

  resultset_print(rs);
  resultset_destroy(rs);
  fclose(datafile);
  //
  // done!
  //
}