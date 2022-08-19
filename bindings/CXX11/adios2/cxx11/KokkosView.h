#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_KOKKOS_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_KOKKOS_H_

#include <Kokkos_Core.hpp>

namespace adios2
{
namespace detail
{

template <typename T>
struct memspace_kokkos_to_adios2;

template <>
struct memspace_kokkos_to_adios2<Kokkos::HostSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::Host;
};

#ifdef KOKKOS_ENABLE_CUDA

template <>
struct memspace_kokkos_to_adios2<Kokkos::CudaSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::CUDA;
};

#endif

} // namespace detail

template <class T>
class AdiosView
{
    T *pointer;
    adios2::MemorySpace mem_space;

public:
    template <class D, class... P>
    AdiosView(Kokkos::View<D, P...> v)
    {
        pointer = v.data();
        mem_space = detail::memspace_kokkos_to_adios2<
            typename Kokkos::View<D, P...>::memory_space>::value;
    }

    T *data() { return pointer; }
    T *data() const { return pointer; }
    adios2::MemorySpace memory_space() { return mem_space; }
    adios2::MemorySpace memory_space() const { return mem_space; }
};

}
#endif
