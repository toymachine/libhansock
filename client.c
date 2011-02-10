#include <my_global.h>
#include <mysql.h>

int testfunc(MYSQL *conn, int pk)
{
    int num_fields;
    int i;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *b;
    char qry[1024];

    sprintf(qry, "SELECT test_id, test_string FROM tbltest WHERE test_id = %d", pk);

    mysql_query(conn, qry);

    result = mysql_store_result(conn);

    num_fields = mysql_num_fields(result);

    while ((row = mysql_fetch_row(result)))
    {
         for(i = 0; i < num_fields; i++)
         {
             b = (row[i] ? row[i] : "NULL");
             //printf("%s", b);
         }
         //printf("\n");
     }

     mysql_free_result(result);
}

int testor(char *passwd)
{
  MYSQL *conn;
  int i;

   conn = mysql_init(NULL);

   if (conn == NULL) {
       printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
       exit(1);
   }

   if (mysql_real_connect(conn, "localhost", "root", passwd, "concurrence_test", 0, NULL, 0) == NULL) {
       printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
       exit(1);
   }

   for(i = 0; i < 100000; i++) {
       testfunc(conn, i % 300000);
   }

   mysql_close(conn);

   return 0;
}

#define N 10

int main(int argc, char *argv[])
{

    int i;
    int pids[N];

    for(i = 0; i < N; i++) {
        int pid = fork();
        if(pid == 0) {
            //child
            testor(argv[1]);
            return 0;
        }
        else {
            //parent
            pids[i] = pid;
        }
    }
    //parent
    int status;
    for(i = 0; i < N; i++) {
        wait(&status);
    }

    return 0;
}
