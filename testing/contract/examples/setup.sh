source_dir=$(readlink -f ${PWD})/source
build_dir=$(readlink -f ${PWD})/build
install_dir=$(readlink -f ${PWD})/install

echo "source_dir  = \"${source_dir}\""
echo "build_dir   = \"${build_dir}\""
echo "install_dir = \"${install_dir}\""
