/* Database driver - replace mysql/mariadb with your DB, if you like */

#include <stdbool.h>
#include <my_global.h>
#include <mysql.h>


static MYSQL *con;

static void finish_with_error(MYSQL *con)

{
  fprintf (stderr, "%s\n", mysql_error (con));
  mysql_close (con);
  exit (1);
}


void db_init (void)

{
  con = mysql_init(NULL);

  if (con == NULL)
    {
      fprintf(stderr, "mysql_init () failed\n");
      exit(1);
    }

  if (mysql_real_connect (con, "localhost", "gol", "GoL", "gol", 0, NULL, 0) == NULL)
    finish_with_error (con);
}


bool db_target_exists (const char *rle, int sizeX, int sizeY)

{
  bool res = false;
  char query [4096];
  snprintf (query, 4095, "SELECT tId FROM target WHERE rle = '%s'", rle);

  if (mysql_query (con, query))
    finish_with_error (con);

  MYSQL_RES *result = mysql_store_result (con);

  if (result && mysql_fetch_row (result))
    res = true;
  else if (mysql_errno (con))
    finish_with_error (con);

  mysql_free_result (result);

  return res;
}


void db_target_insert (const char *rle, int sizeX, int sizeY)

{
  bool res = false;
  char query [4096];
  snprintf (query, 4095, "INSERT INTO target (tId, rle, x, y) VALUES (NULL, '%s', %d, %d)", rle, sizeX, sizeY);

  if (mysql_query (con, query))
    finish_with_error (con);
}
