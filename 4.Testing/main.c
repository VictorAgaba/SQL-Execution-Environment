/*main.c*/

//
// Unit tests for SimpleSQL execute_query component.
// These tests are written using the Google Test 
// framework.
//
// Victor Agaba
// Northwestern University
// CS 211, Winter 2023
//
// Initial version: Prof. Joe Hummel
//

#include <assert.h>  // assert
#include <ctype.h>   // isdigit
#include <math.h>    // fabs
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcpy, strcat

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "execute.h"
#include "parser.h"
#include "resultset.h"
#include "scanner.h"
#include "tokenqueue.h"
#include "util.h"

#include "gtest/gtest.h"

//
// Test case: Select * From Movies2 limit 3;
//
// NOTE: this was presented in class on Tues, Jan 31.
// This test is *NOT* complete and considered an
// example of how to structure a test, but it needs
// to be extended with more checking:
//    - correct # of columns, in correct order / types?
//    - correct # of rows?
//    - is the data in each row correct?
//
TEST(execute_query, basic_select) {
  //
  // Write our test query to a file, then open that
  // file for parsing:
  //
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "Select * From Movies2 limit 3;\n$\n");
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  //
  // open the database the query needs, and then
  // parse, analyze, and execute the query:
  //
  struct Database *db = database_open("MovieLens2");
  parser_init();

  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);

  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);

  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  //
  // TODO: add more checks to this test?
  //

  // check that we have the correct # rows/cols
  ASSERT_TRUE(rs->numRows == 3);
  ASSERT_TRUE(rs->numCols == 4);
  // check that columns exist as a linked list and have right peoperties
  struct RSColumn* column1 = rs->columns;
  ASSERT_TRUE(column1 != NULL);
  ASSERT_TRUE(column1->coltype == COL_TYPE_INT);
  ASSERT_TRUE(strcmp(column1->colName, "ID") == 0);
  struct RSColumn* column2 = column1->next;
  ASSERT_TRUE(column2 != NULL);
  ASSERT_TRUE(column2->coltype == COL_TYPE_STRING);
  ASSERT_TRUE(strcmp(column2->colName, "Title") == 0);
  struct RSColumn* column3 = column2->next;
  ASSERT_TRUE(column3 != NULL);
  ASSERT_TRUE(column3->coltype == COL_TYPE_INT);
  ASSERT_TRUE(strcmp(column3->colName, "Year") == 0);
  struct RSColumn* column4 = column3->next;
  ASSERT_TRUE(column4 != NULL);
  ASSERT_TRUE(column4->coltype == COL_TYPE_REAL);
  ASSERT_TRUE(strcmp(column4->colName, "Revenue") == 0);
  ASSERT_TRUE(column4->next == NULL); // column 4 should be the last column

  struct RSColumn* col = rs->columns;
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(strcmp(col->tableName, "Movies2") == 0);
    ASSERT_TRUE(col->function == NO_FUNCTION);
    col = col->next;
  }
  ASSERT_TRUE(col == NULL);

  //
  // check the data that should be in row 2:
  //
  ASSERT_TRUE(rs->numRows == 3);
  ASSERT_TRUE(rs->numCols == 4);
  ASSERT_TRUE(resultset_getInt(rs, 2, 1) == 111);
  ASSERT_TRUE(strcmp(resultset_getString(rs, 2, 2), "Scarface") == 0);
  ASSERT_TRUE(resultset_getInt(rs, 2, 3) == 1983);
  ASSERT_TRUE(fabs(resultset_getReal(rs, 2, 4) - 65884703.0) < 0.00001);
  
  fclose(input);
}

//
// TODO: add more tests!
//

TEST(execute_query, switch_select) {
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "select title, revenue from movies where ID < 100;\n$\n");
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  struct Database *db = database_open("MovieLens");
  parser_init();
  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  char* titles[] = {"Title", "Revenue"};
  int coltypes[] = {COL_TYPE_STRING, COL_TYPE_REAL};
  struct RSColumn* col = rs->columns;
  
  ASSERT_TRUE(rs->numCols == 2);
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(strcmp(col->colName, titles[i]) == 0);
    ASSERT_TRUE(col->coltype == coltypes[i]);

    col = col->next;
  }
  ASSERT_TRUE(col == NULL);
  ASSERT_TRUE(rs->numCols >= 2);
  ASSERT_TRUE(fabs(resultset_getReal(rs, 2, 2) - 168840000.0) < 0.00001);
  ASSERT_TRUE(strcmp(resultset_getString(rs, 2, 1), "Twelve Monkeys") == 0);

  fclose(input);

  //switch case
  output = fopen("input.txt", "w");
  fprintf(output, "select title, id from movies where ID < 100;\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("MovieLens");
  parser_init();
  tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  char* titles2[] = {"Title", "ID"};
  int coltypes2[] = {COL_TYPE_STRING, COL_TYPE_INT};
  col = rs->columns;
  
  ASSERT_TRUE(rs->numCols == 2);
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(strcmp(col->colName, titles2[i]) == 0);
    ASSERT_TRUE(col->coltype == coltypes2[i]);

    col = col->next;
  }
  ASSERT_TRUE(col == NULL);
  
  fclose(input);

  // control case
  output = fopen("input.txt", "w");
  fprintf(output, "select id, title from movies where ID < 100;\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("MovieLens");
  parser_init();
  tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  char* titles3[] = {"ID", "Title"};
  int coltypes3[] = {COL_TYPE_INT, COL_TYPE_STRING};
  col = rs->columns;
  
  ASSERT_TRUE(rs->numCols == 2);
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(strcmp(col->colName, titles3[i]) == 0);
    ASSERT_TRUE(col->coltype == coltypes3[i]);

    col = col->next;
  }
  ASSERT_TRUE(col == NULL);
  
  fclose(input);
}


TEST(execute_query, where_select_string) {
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "select title from movies where title < 'A Close Shave';\n$\n");
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  struct Database *db = database_open("MovieLens");
  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numRows == 7);
  ASSERT_TRUE(rs->numCols == 1);
  for (int i=0; i<rs->numRows; i++) {
    ASSERT_TRUE(strcmp(resultset_getString(rs, i+1, 1), "A Close Shave") != 0);
  }

  fclose(input);
  
  output = fopen("input.txt", "w");
  fprintf(output, "select title from movies where title <= 'A Close Shave';\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("MovieLens");
  tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numCols == 1);
  ASSERT_TRUE(rs->numRows == 8);
  ASSERT_TRUE(strcmp(resultset_getString(rs, 4, 1), "A Close Shave") == 0);

  fclose(input);
}


TEST(execute_query, where_select_int) {
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "select * from movies where ID < 15;\n$\n");
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  struct Database *db = database_open("MovieLens");
  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numCols == 4);
  ASSERT_TRUE(rs->numRows == 4);
  int values[] = {5, 11, 13, 6};
  for (int i=0; i<rs->numRows; i++) {
    ASSERT_TRUE(resultset_getInt(rs, i+1, 1) == values[i]);
  }
  struct RSColumn* col = rs->columns;
  int coltypes[] = {COL_TYPE_INT, COL_TYPE_STRING, COL_TYPE_INT, COL_TYPE_REAL};
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(col->coltype = coltypes[i]);
    col = col->next;
  }

  fclose(input);

  output = fopen("input.txt", "w");
  fprintf(output, "select * from movies where ID <= 15;\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("MovieLens");
  tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numCols == 4);
  ASSERT_TRUE(rs->numRows == 5);
  int values2[] = {5, 11, 13, 6, 15};
  for (int i=0; i<rs->numRows; i++) {
    ASSERT_TRUE(resultset_getInt(rs, i+1, 1) == values2[i]);
  }
  col = rs->columns;
  int coltypes2[] = {COL_TYPE_INT, COL_TYPE_STRING, COL_TYPE_INT, COL_TYPE_REAL};
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(col->coltype = coltypes2[i]);
    col = col->next;
  }

  fclose(input);
}


TEST(execute_query, edgeCase_select) {
  FILE *input = fopen("Fred.txt", "r");
  ASSERT_TRUE(input == NULL);
  
  struct Database *db = database_open("Fred");
  ASSERT_TRUE(db == NULL);

  // make a NULL rs?
}


TEST(execute_query, limit_select) {
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "select * from movies limit 999;\n$\n");
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  struct Database *db = database_open("MovieLens");
  parser_init();
  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);
  
  ASSERT_TRUE(rs->numRows == 999);

  fclose(input);
}


TEST(execute_query, function_select) {
  FILE *output = fopen("input.txt", "w");
  fprintf(output, "select avg(riders), max(date), min(ID) from ridership;\n$\n"); // **continue from here
  fclose(output);
  FILE *input = fopen("input.txt", "r");

  struct Database *db = database_open("CTA");
  parser_init();
  struct TokenQueue *tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  struct QUERY *query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  struct ResultSet *rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numCols == 3);
  struct RSColumn* col = rs->columns;
  int functions[] = {AVG_FUNCTION, MAX_FUNCTION, MIN_FUNCTION};
  for (int i=0; i<rs->numCols; i++) {
    ASSERT_TRUE(col != NULL);
    ASSERT_TRUE(col->function == functions[i]);
    col = col->next;
  }
  ASSERT_TRUE(col == NULL);
  ASSERT_TRUE(rs->numRows >= 1);
  ASSERT_TRUE(fabs(resultset_getReal(rs, 1, 1) - 3188.58160) < 0.00001);
  ASSERT_TRUE(resultset_getInt(rs, 1, 3) == 40010);
  fclose(input);
  
  output = fopen("input.txt", "w");
  fprintf(output, "select max(station) from stations;\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("CTA");
  tq = parser_parse(input);
  query = analyzer_build(db, tq);
  rs = execute_query(db, query);
  
  ASSERT_TRUE(rs->numCols == 1);
  ASSERT_TRUE(rs->numRows >= 1);
  ASSERT_TRUE(strcmp(resultset_getString(rs, 1, 1), "Wilson") == 0);
  col = rs->columns;
  ASSERT_TRUE(col->function == MAX_FUNCTION);

  fclose(input);
  
  output = fopen("input.txt", "w");
  fprintf(output, "select min(station) from stations;\n$\n");
  fclose(output);
  input = fopen("input.txt", "r");

  db = database_open("CTA");
  tq = parser_parse(input);
  ASSERT_TRUE(tq != NULL);
  query = analyzer_build(db, tq);
  ASSERT_TRUE(query != NULL);
  rs = execute_query(db, query);
  ASSERT_TRUE(rs != NULL);

  ASSERT_TRUE(rs->numCols == 1);
  ASSERT_TRUE(rs->numRows >= 1);
  ASSERT_TRUE(strcmp(resultset_getString(rs, 1, 1), "18th") == 0);
  col = rs->columns;
  ASSERT_TRUE(col->function == MIN_FUNCTION);

  fclose(input);
}



//
// main program: do not change this, google test will find all your
// tests and run them.  So leave the code below as is!
//
int main() {
  ::testing::InitGoogleTest();

  //
  // run all the tests, returns 0 if they
  // all pass and 1 if any fail:
  //
  int result = RUN_ALL_TESTS();

  return result; // 0 => all passed
}
