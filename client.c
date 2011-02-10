#include <my_global.h>
#include <mysql.h>

int testfunc(MYSQL *conn)
{
    int num_fields;
    int i;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *b;

    mysql_query(conn, "SELECT test_id, test_string FROM tbltest WHERE test_id = 9");
    result = mysql_store_result(conn);

    num_fields = mysql_num_fields(result);

     while ((row = mysql_fetch_row(result)))
     {
         for(i = 0; i < num_fields; i++)
         {
             b = (row[i] ? row[i] : "NULL");
         }
         //printf("\n");
     }

     mysql_free_result(result);
}

int main(int argc, char **argv)
{
  printf("MySQL client version: %s\n", mysql_get_client_info());

  MYSQL *conn;
  int i;

   conn = mysql_init(NULL);

   if (conn == NULL) {
       printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
       exit(1);
   }

   if (mysql_real_connect(conn, "localhost", "root", argv[1], "concurrence_test", 0, NULL, 0) == NULL) {
       printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
       exit(1);
   }

   for(i = 0; i < 1000000; i++) {
       testfunc(conn);
   }

   mysql_close(conn);

}
