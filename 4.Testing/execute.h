/*execute.h*/

//
// Project: Execution of queries for SimpleSQL
//
// Prof. Joe Hummel
// Northwestern University
// CS 211, Winter 2023
//

#pragma once

#include "database.h"
#include "ast.h"
#include "resultset.h"

//
// functions:
//
struct ResultSet* execute_query(struct Database* db, struct QUERY* query);
