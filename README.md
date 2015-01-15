*Glucose transportation simulation* by Zdeněk Janeček 2015
use: glucose DATABASE CONFIG METHOD GENERATIONS METRIC
  METHOD:
    -s serial version
    -p pthread version
    -c opencl version
  METRIC:
    -a absolute value
    -s squared value
    -m maximum difference
  GENERATIONS means count of computed generations

example: glucose db.sqlite3 config.ini -p 500 -s

Install SQLite3, OpenCL SDK and compile with make

