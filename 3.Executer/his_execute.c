/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// Prof. Joe Hummel
// Northwestern University
// CS 211, Winter 2023
//

// to eliminate warnings about stdlib in Visual Studio
#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>  // assert
#include <ctype.h>   // isdigit
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcpy, strcat

#include "ast.h"
#include "database.h"
#include "execute.h"
#include "resultset.h"
#include "util.h"

//
// getMetaDataColumnIndex
//
// Returns the ARRAY index of the given column in the meta-data,
// which is an integer in the range 0..numColumns.
//
static int getMetaDataColumnIndex(struct TableMeta *tablemeta,
                                  char *columnName) {
  for (int i = 0; i < tablemeta->numColumns; i++) {
    if (icmpStrings(columnName, tablemeta->columns[i].name) == 0)
      return i;
  }

  // if get here, something is wrong:
  panic("**INTERNAL ERROR: getMetaDataColumnIndex failed to find matching "
        "column");

  return 0; // never executed
}

//
// readDataIntoRS
//
// Opens the given .data file and reads each line of data into the resultset.
//
void readDataIntoRS(char *filename, struct TableMeta *tablemeta,
                    struct ResultSet *rs) {
  FILE *datafile = fopen(filename, "r");
  if (datafile == NULL) // unable to open:
  {
    printf("**INTERNAL ERROR: table's data file '%s' not found.\n", filename);
    panic("execution halted");
  }

  //
  // allocate buffer to hold one record of data:
  //
  int dataBufferSize =
      tablemeta->recordSize + 3; // ends with $\n + null terminator
  char *dataBuffer = (char *)malloc(sizeof(char) * dataBufferSize);
  if (dataBuffer == NULL)
    panic("out of memory");

  while (true) {
    //
    // input the next row of data:
    //
    fgets(dataBuffer, dataBufferSize, datafile);

    if (feof(datafile)) // end of the data file, we're done
      break;

    // printf("%s", dataBuffer);

    //
    // we have a row of data, so let's plan to store in result set:
    //
    int rowNum = resultset_addRow(rs);

    //
    // recall that every data value is followed by a space...
    //
    // let's turn each value into a "mini-string" by finding
    // this space and replacing with '\0' so that we can
    // easily get the value and work with it:
    //
    char *pos = dataBuffer;
    char *end = NULL;

    for (int c = 0; c < tablemeta->numColumns; c++) {
      int colNum = c + 1; // column #'s are 1-based, so +1

      if (tablemeta->columns[c].colType == COL_TYPE_INT) {
        assert(isdigit(*pos));

        end = strchr(pos, ' '); // find the next space
        assert(end != NULL);

        *end = '\0'; // turn that chunk of data into a mini-string:

        int value = atoi(pos);

        resultset_putInt(rs, rowNum, colNum, value);

        pos = end + 1; // advance to start of next value:
      } else if (tablemeta->columns[c].colType == COL_TYPE_REAL) {
        assert(isdigit(*pos));

        end = strchr(pos, ' '); // find the next space
        assert(end != NULL);

        *end = '\0'; // turn that chunk of data into a mini-string:

        double value = atof(pos);

        resultset_putReal(rs, rowNum, colNum, value);

        pos = end + 1; // advance to start of next value:
      } else {
        assert(tablemeta->columns[c].colType == COL_TYPE_STRING);

        //
        // strings start/end with either ' or "
        //
        char quote = *pos;

        assert(quote == '\'' || quote == '"');

        pos++; // advance past quote

        end = pos;

        while (*end != quote)
          end++;

        assert(*end == quote);
        assert(*(end + 1) == ' ');

        *end = '\0'; // over-write closing quote, turn into mini-string:

        resultset_putString(rs, rowNum, colNum, pos);

        pos = end + 2; // we have to skip over TWO chars: '\0' and ' '
      }
    } // for

    //
    // end of current record, loop around and process the next:
    //
  } // while

  free(dataBuffer); // we are done with the input, so we can free/close
  fclose(datafile);
}

//
// execute_where
//
// Assuming there's a where clause (e.g. "where ID < 1000"), we execute that
// where clause and delete any rows where the expression evaluates to false.
//
// NOTE: it's possible for the resultset to come back empty.
//
void execute_where(struct WHERE *where, struct TableMeta *tablemeta,
                   struct ResultSet *rs) {
  if (where == NULL) // missing, so nothing to do:
    return;

  //
  // which column is the where clause referencing?
  //
  int c = getMetaDataColumnIndex(tablemeta, where->expr->column->name);

  assert(0 <= c && c < tablemeta->numColumns);

  int colNum = c + 1; // column #'s are 1-based, so +1

  //
  // delete rows for which the where clause is false...  We
  // work backwards --- this is more efficient since we know
  // there's an array underneath and this won't cause any data
  // to be shifted. Also, row # doesn't change if we work
  // backwards...
  //
  int colType = tablemeta->columns[c].colType;

  if (colType == COL_TYPE_INT) {
    //
    // expr value should also be an integer:
    //
    assert(where->expr->litType == INTEGER_LITERAL);

    int rightValue = atoi(where->expr->value);

    for (int rowNum = rs->numRows; rowNum > 0; rowNum--) {
      int leftValue = resultset_getInt(rs, rowNum, colNum);

      bool result = false;

      switch (where->expr->Operator) {
      case EXPR_LT:
        result = leftValue < rightValue;
        break;
      case EXPR_LTE:
        result = leftValue <= rightValue;
        break;
      case EXPR_GT:
        result = leftValue > rightValue;
        break;
      case EXPR_GTE:
        result = leftValue >= rightValue;
        break;
      case EXPR_EQUAL:
        result = leftValue == rightValue;
        break;
      case EXPR_NOT_EQUAL:
        result = leftValue != rightValue;
        break;
      case EXPR_LIKE:
        panic("**INTERNAL ERROR: LIKE operator not valid with integers");
      default:
        panic("**INTERNAL ERROR: unknown expr operator");
      } // switch

      if (!result) // if false, then where is false, so delete this row from
                   // result:
        resultset_deleteRow(rs, rowNum);
    } // for
  }   // integers
  else if (colType == COL_TYPE_REAL) {
    //
    // expr value can be integer or real:
    //
    assert(where->expr->litType == INTEGER_LITERAL ||
           where->expr->litType == REAL_LITERAL);

    double rightValue = atof(where->expr->value);

    for (int rowNum = rs->numRows; rowNum > 0; rowNum--) {
      double leftValue = resultset_getReal(rs, rowNum, colNum);

      bool result = false;

      switch (where->expr->Operator) {
      case EXPR_LT:
        result = leftValue < rightValue;
        break;
      case EXPR_LTE:
        result = leftValue <= rightValue;
        break;
      case EXPR_GT:
        result = leftValue > rightValue;
        break;
      case EXPR_GTE:
        result = leftValue >= rightValue;
        break;
      case EXPR_EQUAL:
        result = leftValue == rightValue;
        break;
      case EXPR_NOT_EQUAL:
        result = leftValue != rightValue;
        break;
      case EXPR_LIKE:
        panic("**INTERNAL ERROR: LIKE operator not valid with reals");
      default:
        panic("**INTERNAL ERROR: unknown expr operator");
      } // switch

      if (!result) // if false, then where is false, so delete this row from
                   // result:
        resultset_deleteRow(rs, rowNum);
    } // for
  }   // reals
  else {
    assert(colType == COL_TYPE_STRING);

    //
    // expr value must also be a string:
    //
    char *rightValue = where->expr->value;

    for (int rowNum = rs->numRows; rowNum > 0; rowNum--) {
      char *leftValue = resultset_getString(rs, rowNum, colNum);

      bool result = false;

      switch (where->expr->Operator) {
      case EXPR_LT:
        result = (strcmp(leftValue, rightValue) < 0);
        break;
      case EXPR_LTE:
        result = (strcmp(leftValue, rightValue) <= 0);
        break;
      case EXPR_GT:
        result = (strcmp(leftValue, rightValue) > 0);
        break;
      case EXPR_GTE:
        result = (strcmp(leftValue, rightValue) >= 0);
        break;
      case EXPR_EQUAL:
        result = (strcmp(leftValue, rightValue) == 0);
        break;
      case EXPR_NOT_EQUAL:
        result = (strcmp(leftValue, rightValue) != 0);
        break;
      case EXPR_LIKE:
        panic("**ERROR: LIKE operator not yet supported");
      default:
        panic("**INTERNAL ERROR: unknown expr operator");
      } // switch

      if (!result) // if false, then where is false, so delete this row from
                   // result:
        resultset_deleteRow(rs, rowNum);

      free(leftValue); // we are responsible for freeing this memory:
    }                  // for
  }                    // strings
}

//
// delete_unneeded_columns
//
// Since we initially add all table columns to the resulset, now
// we look through the actual select query and remove any columns
// that are not asked for.
//
void delete_unneeded_columns(struct SELECT *select, struct TableMeta *tablemeta,
                             struct ResultSet *rs) {
  for (int c = 0; c < tablemeta->numColumns; c++) {
    //
    // go through the AST select query and see if we need
    // this column, and if not, delete from result set:
    //
    struct COLUMN *column = select->columns;

    bool deleteCol = true;

    while (column != NULL) {
      if (icmpStrings(tablemeta->columns[c].name, column->name) == 0) {
        deleteCol = false;
        break;
      }

      column = column->next;
    }

    if (deleteCol) // delete column!
    {
      int position = resultset_findColumn(rs, 1 /*startPos*/, tablemeta->name,
                                          tablemeta->columns[c].name);

      assert(position > 0);

      resultset_deleteColumn(rs, position);
    }
  } // for
}

//
// reorder_columns
//
// Since we initially add all table columns to the resulset, now
// we look through the actual select query and reorder the columns
// in the order specified in the select query.
//
void reorder_columns(struct SELECT *select, struct ResultSet *rs) {
  struct COLUMN *column = select->columns;

  int curPos = 1; // current position in the list of select columns (1, 2, ...)

  while (column != NULL) {
    //
    // NOTE: this does not handle duplicate columns, so queries like
    //
    //   select min(date), max(date) ...
    //
    // do not work. Only one of the columns will remain.
    //

    int position =
        resultset_findColumn(rs, 1 /*startPos*/, column->table, column->name);

    assert(position > 0);

    resultset_moveColumn(rs, position /*from*/, curPos /*to*/);

    //
    // advance to next column:
    //
    column = column->next;
    curPos++;
  }
}

//
// apply_functions
//
// applies any aggregate functions such as MAX or AVG.
//
// NOTE: after applying an aggregate function, the # of rows
// in the resultset will be at most 1.
//
void apply_functions(struct SELECT *select, struct ResultSet *rs) {
  struct COLUMN *column = select->columns;

  int colNum = 1;

  while (column != NULL) {
    if (column->function != NO_FUNCTION) {
      resultset_applyFunction(rs, column->function, colNum);
    }

    colNum++;
    column = column->next;
  }
}

//
// apply_limit
//
// If the query specified a limit, e.g."select ... limit 10;" then
// we apply that limit by deleting all rows past the limit.
//
void apply_limit(struct SELECT *select, struct ResultSet *rs) {
  if (select->limit != NULL) {
    int limit = select->limit->N;

    assert(limit >= 0);

    //
    // delete the rows backwards, this is more efficient since we know
    // there's an array underneath and this won't cause any data to be
    // shifted. Also, row # doesn't change if we go backwards...
    //
    for (int rowNum = rs->numRows; rowNum > limit; rowNum--) {
      resultset_deleteRow(rs, rowNum);
    }
  }
}

//
// execute_query
//
// execute a select query
//
struct ResultSet *execute_query(struct Database *db, struct QUERY *query) {
  if (db == NULL)
    panic("db is NULL (execute)");
  if (query == NULL)
    panic("query is NULL (execute)");

  if (query->queryType != SELECT_QUERY) {
    printf("**INTERNAL ERROR: execute() only supports SELECT queries.\n");
    return NULL;
  }

  struct SELECT *select = query->q.select; // alias for less typing:

  //
  // (1) we need a pointer to the table's meta data, so find it:
  //
  struct TableMeta *tablemeta = NULL;

  for (int t = 0; t < db->numTables; t++) {
    if (icmpStrings(db->tables[t].name, select->table) == 0) // found it:
    {
      tablemeta = &db->tables[t];
      break;
    }
  }

  assert(tablemeta != NULL);

  //
  // (2) create a result set and setup the columns in the result set to
  //     match the columns in the meta-data, in the same order. This way
  //     when we read the file, we insert the values in the same order
  //     as they appear in each line, left-to-right:
  //
  struct ResultSet *rs = resultset_create();

  for (int c = 0; c < tablemeta->numColumns; c++) {
    //
    // columns are 1-based, so insert @ position c+1 so
    // that we are appending each new column:
    //
    int position = resultset_insertColumn(
        rs, c + 1, tablemeta->name, tablemeta->columns[c].name,
        NO_FUNCTION /*for now*/, tablemeta->columns[c].colType);

    assert(position == (c + 1));
  }

  //
  // (3) open the table's data file
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

  //
  // (4) read all the data from .data file into the resultset:
  //
  readDataIntoRS(path, tablemeta, rs);

  // resultset_print(rs);

  //
  // At this point all the data is loaded into the resultset, now
  // we filter the data based on the where clause, if present:
  //
  execute_where(select->where, tablemeta, rs);

  //
  // next we delete the columns we don't need --- i.e. delete
  // the columns that the user didn't select:
  //
  delete_unneeded_columns(select, tablemeta, rs);

  //
  // we have to reorder the columns to match the order
  // in the select query...
  //
  reorder_columns(select, rs);

  //
  // Next step is to apply any functions applied
  // to the select columns...
  //
  // NOTE: when a function is applied, the # of rows in
  // the result set will drop to 1. The data in other
  // columns is NOT deleted however, so that additional
  // functions can be applied. But when printing the
  // result set, at most one row will be printed.
  //
  apply_functions(select, rs);

  //
  // if we have a limit, delete the unnecessary rows:
  //
  apply_limit(select, rs);

  //
  // Done! Return the final resultset to the caller:
  //
  return rs;
}
