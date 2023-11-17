source_dir="/opt/adios2/source/examples/basics/variablesShapes"
build_dir=$(readlink -f ${PWD})/build
install_dir=$(readlink -f ${PWD})/install
test_dir=$(readlink -f ${PWD})/test

echo "source_dir  = \"${source_dir}\""
echo "build_dir   = \"${build_dir}\""
echo "install_dir = \"${install_dir}\""
echo "test_dir    = \"${test_dir}\""

module load tau
