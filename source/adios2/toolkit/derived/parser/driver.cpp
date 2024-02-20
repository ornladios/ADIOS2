#include <string>
#include <iostream>
#include "ASTDriver.h"

int
main (int argc, char *argv[])
{
  ASTDriver drv;
  // char* str_expr = "one := 1\ntwo := 2\nthree := 3\n(one + two * three) * two * three";
  char* str_expr = "var x:= sim1/points/x\nvar y := sim1/points/y\nvar z:=sim_1/points-more/x[ , , ::5]\nvar a:=sim_1/points-more/.extra\\a[0::2, :2:,::5,32]\na+(x*y)^curl(z[1, 2:3:],sqrt(a))";
  std::cout << "Parse input: " << str_expr << std::endl;

  ASTNode *result = drv.parse(str_expr);

  std::cout << "Result: " << std::endl;
  std::cout << result->printpretty("");
  
  return 0;
}
