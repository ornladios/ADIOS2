#ifndef ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_VIEW_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_VIEW_H_

#include <Kokkos_Core.hpp>

namespace adios2
{
namespace detail
{

#ifdef ADIOS2_HAVE_GPU_SUPPORT
template <typename T>
struct memspace_kokkos_to_adios2
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::GPU;
};

template <>
struct memspace_kokkos_to_adios2<Kokkos::HostSpace>
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::Host;
};
#else
template <typename T>
struct memspace_kokkos_to_adios2
{
    static constexpr adios2::MemorySpace value = adios2::MemorySpace::Host;
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
