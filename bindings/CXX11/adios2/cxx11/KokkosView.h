#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_KOKKOS_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_KOKKOS_H_

#include <Kokkos_Core.hpp>

namespace adios2
{
template <class T>
class AdiosView
{
    T *pointer;

public:
    template <class D, class... P>
    AdiosView(Kokkos::View<D, P...> v)
    {
        pointer = v.data();
    }

    T *data() { return pointer; }
    T *data() const { return pointer; }
};

}
#endif
