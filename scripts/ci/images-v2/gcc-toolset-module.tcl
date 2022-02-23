#%Module1.0

set gcchome /opt/rh/gcc-toolset-TOOLSET/root

prepend-path PATH            $gcchome/usr/bin
prepend-path MANPATH         $gcchome/usr/share/man
prepend-path INFOPATH        $gcchome/usr/share/info
prepend-path PCP_DIR         $gcchome
prepend-path LD_LIBRARY_PATH $gcchome/usr/lib64:$gcchome/usr/lib:$gcchome/usr/lib64/dyninst:$gcchome/usr/lib/dyninst
prepend-path PKG_CONFIG_PATH $gcchome/usr/lib64/pkgconfig
