/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// Victor Agaba
// Northwestern University
// CS 211, Winter 2023
//

#include <assert.h> // assert
#include <assert.h>
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

  struct ResultSet *rs = resultset_create(); // MALLOC

  // Add cols to tablemeta
  for (int i = 0; i < tablemeta->numColumns; i++) {
    int colNum = resultset_insertColumn(rs, i + 1, tablemeta->name,
                                        tablemeta->columns[i].name, NO_FUNCTION,
                                        tablemeta->columns[i].colType);
    rs->numCols += 1;
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
    // printf("%s\n", cp);

    /* NOW WHAT? */
    // int colNum = 1;
    // cp = dataBuffer;
    // struct RSColumn *curr = rs->columns;
    // int maxCol = rs->numCols;
    resultset_print(rs);
    int rowNum = resultset_addRow(rs);
    resultset_print(rs);
    while (curr != NULL) {
      printf("%s\n", cp);
      if (curr->coltype == COL_TYPE_INT) {
        resultset_putInt(rs, rowNum, colNum, atoi(cp));
        cp += strlen(cp) + 1;
      } else if (curr->coltype == COL_TYPE_INT) {
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
  // resultset_print(rs);
  fclose(datafile);
  free(dataBuffer);
  resultset_destroy(rs);

  //
  // done!
  //
}