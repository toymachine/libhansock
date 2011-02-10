import MySQLdb
import sys
import string
import random

def randstr(n):
    s = []
    for i in range(n):
        s.append(random.choice(string.letters))
    return ''.join(s)


cnn = MySQLdb.connect(host = 'localhost', user = 'root', passwd = sys.argv[1], db = 'concurrence_test')
cur = cnn.cursor()
for i in range(1000000):
    cur.execute("insert into tbltest values (%d, '%s', '%s')" % (i, randstr(20), randstr(20)))
    if i % 10 == 0:
        cnn.commit()
cnn.commit()
cur.close()
cnn.close()