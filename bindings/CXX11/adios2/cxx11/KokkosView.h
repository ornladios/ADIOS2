#ifndef ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_VIEW_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_VIEW_H_

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

#if defined(KOKKOS_ENABLE_CUDA) && defined(ADIOS2_HAVE_CUDA)

template <>
struct memspace_kokkos_to_adios2<Kokkos::CudaSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::CUDA;
};

template <>
struct memspace_kokkos_to_adios2<Kokkos::CudaUVMSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::CUDA;
};

template <>
struct memspace_kokkos_to_adios2<Kokkos::CudaHostPinnedSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::CUDA;
};

#endif

} // namespace detail

template <class T, class... Parameters>
class AdiosView<Kokkos::View<T, Parameters...>>
{
    using data_type = typename Kokkos::View<T, Parameters...>::value_type;
    data_type *pointer;
    adios2::MemorySpace mem_space;

public:
    template <class... P>
    AdiosView(Kokkos::View<T, P...> v)
    {
        pointer = v.data();
        mem_space = detail::memspace_kokkos_to_adios2<
            typename Kokkos::View<T, P...>::memory_space>::value;
    }

    data_type const *data() const { return pointer; }
    data_type *data() { return pointer; }
    adios2::MemorySpace memory_space() const { return mem_space; }
};

}
#endif
