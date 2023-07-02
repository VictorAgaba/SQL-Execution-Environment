## SQL-Execution-Environment

The overall project is to build an SQL execution environment called SimpleSQL, imported from Replit.

"1.Scanner" is the first step which involves building a scanner that turns SQL-based input into
tokens that are more easily stored and processed. For instance, if the input string is

    Select * from Movies where Title like '%titanic%';

the output is a sequence of tokens: 36, 4, 24, 17, 42, 17, 30, 16, 1 (referencing a list in the
header file token.h)

"2.Analyzer" creates a tool to enforce semantic rules of an input query. For instance if a
query wants to filter via a string in an column of integers, the analyzer should be able to
spot the error. In case of no errors, an abstract syntax tree is produced representing the
structure of the query before it's executed.

"3.Executer" uses an unoptimized algorithm to execute SQL select queries and output a corresponding
result set. Queries with the following features are executed:
- Select any number of columns without duplicates
- Apply a where clause to filter the results, supporting operators <, <=, >, >=, = and <>
- Apply functions like MIN, MAX, AVG, SUM, COUNT to the columns
- Apply a limit clause

"4.Testing" is for unit tests to make sure that the different components are working cohesively.
