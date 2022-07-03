
#ifndef ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_INTEROP_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_KOKKOS_INTEROP_H_

#include <Kokkos_Core.hpp>
#include <adios2.h>

namespace adios2
{

namespace detail
{

template <typename T>
struct is_kokkos_view : std::false_type
{
};

template <typename... Ts>
struct is_kokkos_view<Kokkos::View<Ts...>> : std::true_type
{
};

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
    static constexpr adios2::MemorySpace space = adios2::MemorySpace::CUDA;
};

#endif

} // namespace detail

template <typename T>
struct ndarray_traits<T, typename std::enable_if<detail::is_kokkos_view<
                             typename std::remove_cv<T>::type>::value>::type>
: std::true_type
{
    using value_type = typename T::value_type;
    using size_type = typename T::size_type;
    using pointer = typename T::pointer_type;
    static constexpr auto memory_space =
        detail::memspace_kokkos_to_adios2<typename T::memory_space>::value;

    static pointer data(const T &c) { return c.data(); }
    static size_type size(const T &c) { return c.size(); }
};

} // namespace adios2

#endif
