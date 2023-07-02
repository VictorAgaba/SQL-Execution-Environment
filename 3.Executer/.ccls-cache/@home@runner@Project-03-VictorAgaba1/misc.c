/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// Victor Agaba
// Northwestern University
// CS 211, Winter 2023
//

#include <assert.h>  // assert
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcpy, strcat
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

struct ResultSet *resultset_create(void) {
  struct ResultSet *rs = (struct ResultSet *)malloc(sizeof(struct ResultSet));
  if (rs == NULL) panic("Out of memory! (resultset)");
  rs->columns = NULL;
  rs->numRows = 0; // why am I calling it 0 tho?
  rs->numCols = 0;
  return rs;
}

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

  //
  // (3) allocate a buffer for input, and start reading the data:
  //
  int dataBufferSize =
      tablemeta->recordSize + 3; // ends with $\n + null terminator
  char *dataBuffer = (char *)malloc(sizeof(char) * dataBufferSize);
  if (dataBuffer == NULL)
    panic("out of memory");

  for (int record = 0; record < 5; record++) {
    fgets(dataBuffer, dataBufferSize, datafile);

    if (feof(datafile)) // end of the data file, we're done
      break;

    printf("%s", dataBuffer);
  }

  //
  // done!
  //
}




bool quote1 = false;
    bool quote2 = false;
    // bool end1 = false;
    // bool end2 = false;
    int t_curr = 0;
    int terminators[tablemeta->numColumns];
  // printf("Orig %d\n", tablemeta->numColumns);
    for (int i = 0; i < dataBufferSize; i++) {
      if (!quote1 && !quote2) { // non-string case
        if (isspace(dataBuffer[i])) {
          // dataBuffer[i] = '\0';
          terminators[t_curr] = i;
          // printf("%d\n", i);
          t_curr += 1;
        } else if (dataBuffer[i] == '\'')
          quote1 = true;
        else if (dataBuffer[i] == '"')
          quote2 = true;
      } else if (quote1) {
        if (dataBuffer[i] == '\'') {
          // dataBuffer[i + 1] = '\0';
          quote1 = false;
          // terminators[t_curr] = i; ////
          // printf("%d\n", i);
          // t_curr += 1;
        }
      } else {
        if (dataBuffer[i] == '"') {
          // dataBuffer[i + 1] = '\0';
          quote2 = false;
          // terminators[t_curr] = i; ////
          // printf("%d\n", i);
          // t_curr += 1;
        }
      }
      if (t_curr == tablemeta->numColumns)
        break; // gut check
    }          // test this



char word[dataBufferSize];
      if (t_curr == 0) {
        for (int n = 0; n < terminators[0] + 1; n++)
          word[n] = dataBuffer[n];
      } else {
        int length = terminators[t_curr] - terminators[t_curr - 1];
        char word[length];
        for (int n = 0; n < length; n++)
          word[n] = dataBuffer[terminators[t_curr - 1] + n + 1];
      }
      printf("%s\n", word);