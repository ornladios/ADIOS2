#include <string>
#include <iostream>
#include "ASTDriver.h"

int
main (int argc, char *argv[])
{
  adios2::detail::ASTDriver drv;

  std::string str_expr = "x= sim1/points/x\n y = sim1/points/y\nz=sim_1/points-more/x[ , , ::5]\na=   sim_1/points-more/.extra\\a[0::2, :2:,::5,32]\na+(x*y)^curl(z[1, 2:3:],sqrt(a))";

  adios2::detail::ASTNode *result = drv.parse(str_expr);

  std::cout << "Parsed input:\n" << str_expr << std::endl;
  std::cout << "Result: " << std::endl;
  std::cout << result->printpretty();
  
  return 0;
}
