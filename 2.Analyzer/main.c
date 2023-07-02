/*main.c*/

// Victor Agaba
// Northwestern University
// CS 211 Project02, Winter 2023

// #include files
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "parser.h"
#include "scanner.h"
#include "util.h"

// your functions
static char *colTypes[] = {"int", "real", "string"};
static int numColTypes = sizeof(colTypes) / sizeof(colTypes[0]);

static char *indexTypes[] = {"non-indexed", "indexed", "unique indexed"};
static int numIndexTypes = sizeof(indexTypes) / sizeof(indexTypes[0]);

/* print_schema
Loops through database structure,
printing the appropriate data structure at each iteration */
void print_schema(struct Database *db) {
  printf("**DATABASE SCHEMA**\n");
  printf("Database: ");
  printf("%s\n", db->name);
  int flag1 = db->numTables;
  struct TableMeta *tables = db->tables;
  for (int i = 0; i < flag1; i++) {
    printf("Table: ");
    printf("%s\n", tables[i].name);
    printf("  Record size: %d\n", tables[i].recordSize);
    int flag2 = tables[i].numColumns;
    struct ColumnMeta *columns = tables[i].columns;
    for (int j = 0; j < flag2; j++) {
      printf("  Column: ");
      printf("%s, %s, %s\n", columns[j].name, colTypes[columns[j].colType - 1],
             indexTypes[columns[j].indexType]);
    }
  }
  printf("**END OF DATABASE SCHEMA**\n");
}

static char *operators[] = {"<", "<=", ">", ">=", "=", "<>", "like"};
static int numOperators = sizeof(operators) / sizeof(operators[0]);

/* print_expr
Helper function for print_ast
Prints the appropriate expression if keyword 'where' is used */
void print_expr(struct EXPR *expr) {
  int operator= expr->operator;
  int literal = expr->litType;
  printf("%s.%s %s ", expr->column->table, expr->column->name, operators[operator]);
  if (operator!= EXPR_LIKE && literal != STRING_LITERAL) {
    printf("%s\n", expr->value);
  } else if (literal == STRING_LITERAL) {
    if (strchr(expr->value, '\'') != NULL) {
      // use double quotes
      printf("\"%s\"\n", expr->value);
    } else {
      // use single quotes
      printf("'%s'\n", expr->value);
    }
  } else {
    printf("Illegal expression under 'where'\n");
    exit(-1);
  }
}

/* print_ast
Inputs a query from the user to print an Abstract Syntax Tree
Outputs the table name included after keyword 'from' for future use */
char *print_ast(struct QUERY *q) {
  printf("**QUERY AST**\n");
  struct SELECT *select = q->q.select;
  printf("Table: ");
  printf("%s\n", select->table);
  struct COLUMN *column = select->columns;
  while (column != NULL) {
    printf("Select column: ");
    if (column->function == MIN_FUNCTION) {
      printf("MIN(%s.%s)\n", column->table, column->name);
    } else if (column->function == MAX_FUNCTION) {
      printf("MAX(%s.%s)\n", column->table, column->name);
    } else if (column->function == SUM_FUNCTION) {
      printf("SUM(%s.%s)\n", column->table, column->name);
    } else if (column->function == AVG_FUNCTION) {
      printf("AVG(%s.%s)\n", column->table, column->name);
    } else if (column->function == COUNT_FUNCTION) {
      printf("COUNT(%s.%s)\n", column->table, column->name);
    } else { // no function
      printf("%s.%s\n", column->table, column->name);
    }
    column = column->next;
  }
  struct JOIN *join = select->join;
  printf("Join ");
  if (join == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%s On %s.%s = %s.%s\n", join->table, join->left->table,
           join->left->name, join->right->table, join->right->name);
  }
  struct WHERE *where = select->where;
  printf("Where ");
  if (where == NULL) {
    printf("(NULL)\n");
  } else {
    print_expr(where->expr);
  }
  struct ORDERBY *orderby = select->orderby;
  printf("Order By ");
  if (orderby == NULL) {
    printf("(NULL)\n");
  } else {
    if (orderby->column->function == MIN_FUNCTION) {
      printf("MIN(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == MAX_FUNCTION) {
      printf("MAX(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == SUM_FUNCTION) {
      printf("SUM(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == AVG_FUNCTION) {
      printf("AVG(%s.%s) ", orderby->column->table, orderby->column->name);
    } else if (orderby->column->function == COUNT_FUNCTION) {
      printf("COUNT(%s.%s) ", orderby->column->table, orderby->column->name);
    } else { // no function
      printf("%s.%s ", orderby->column->table, orderby->column->name);
    }
    if (orderby->ascending) {
      printf("ASC\n");
    } else {
      printf("DESC\n");
    }
  }
  struct LIMIT *limit = select->limit;
  printf("Limit ");
  if (limit == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%d\n", limit->N);
  }
  struct INTO *into = select->into;
  printf("Into ");
  if (into == NULL) {
    printf("(NULL)\n");
  } else {
    printf("%s\n", into->table);
  }
  printf("**END OF QUERY AST**\n");
  return select->table;
}

/* execute_query
Prints the first 5 lines of the table included after keyword 'from' */
void execute_query(FILE *input, int recSize) {
  int count = 0;
  int bufferSize = recSize + 3;
  char *buffer = (char *)malloc(sizeof(char) * bufferSize);
  if (buffer == NULL)
    panic("Out of memory\n");
  while (!feof(input) && count < 5) {
    fgets(buffer, bufferSize, input);
    printf("%s", buffer);
    count += 1;
  }
}

/* main function, calling the defined functions in sequence */
int main() {

  char database[DATABASE_MAX_ID_LENGTH + 1];
  printf("database? ");
  scanf("%s", database);

  struct Database *db = database_open(database);
  if (db == NULL) {
    printf("**Error: unable to open database '%s'\n", database);
    exit(-1);
  }

  print_schema(db);

  parser_init();
  while (true) {
    printf("query? ");
    struct TokenQueue *tq = parser_parse(stdin);
    if (tq == NULL) { // syntax error or EOF
      if (parser_eof()) {
        break;
      } else {
        continue;
      }
    } else { // success, call analyzer
      struct QUERY *q = analyzer_build(db, tq);
      if (q != NULL) { // no semantic error

        char *myTable = print_ast(q);
        // saved output to print first 5 lines of myTable in execute_query

        char filename[2 * DATABASE_MAX_ID_LENGTH + 7];
        strcpy(filename, db->name);
        strcat(filename, "/");
        // Find the table position
        struct TableMeta *tables = db->tables;
        int desired = -1;
        for (int i = 0; i < db->numTables; i++) {
          if (strcasecmp(tables[i].name, myTable) == 0) {
            desired = i;
            break;
          }
        }
        if (desired == -1) {
          printf("**Error: unexpected mismatch between table names\n");
          exit(-1);
        }
        strcat(filename, tables[desired].name);
        strcat(filename, ".data");
        FILE *input = fopen(filename, "r");
        if (input == NULL) {
          printf("**Error: unable to open '%s'", filename);
        } // if we get here, we have a file called input

        execute_query(input, tables[desired].recordSize);
      }
    }
  }

  database_close(db);
  return 0;
}
