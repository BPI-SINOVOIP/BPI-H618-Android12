// This test comes from upstream bug
// https://sourceware.org/bugzilla/show_bug.cgi?id=21296
//
// It has to be compiled once with clang++ and once with g++, like
// this:
//
// clang++ -std=c++14  -o test40-PR21296-libclang.so -shared -fPIC  test40-PR21296-clanggcc.cc  -g
//
// g++ -o test40-PR21296-libgcc.so -shared -fPIC test40-PR21296-clanggcc.cc -g
//
extern "C" void free(void*) throw ();
extern "C" char* strdup(const char*);

struct STR {
   STR(const char* S): C(strdup(S)) {}
   ~STR() { free(C); }
   STR(STR&& O): C(O.C) { O.C = 0; }
   char* C;
};

extern "C" int printf(const char*,...);

namespace std
{

typedef unsigned long size_t;

  struct true_type { static constexpr bool value = true; };
  struct false_type { static constexpr bool value = false; };

  // Reference transformations.

  /// remove_reference
  template<typename _Tp>
    struct remove_reference
    { typedef _Tp   type; };

  template<typename _Tp>
    struct remove_reference<_Tp&>
    { typedef _Tp   type; };

  template<typename _Tp>
    struct remove_reference<_Tp&&>
    { typedef _Tp   type; };


  /**
   *  @brief  Forward an lvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template<typename _Tp>
    constexpr _Tp&&
    forward(typename std::remove_reference<_Tp>::type& __t) noexcept
    { return static_cast<_Tp&&>(__t); }

  /**
   *  @brief  Forward an rvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template<typename _Tp>
    constexpr _Tp&&
    forward(typename std::remove_reference<_Tp>::type&& __t) noexcept
    {
      return static_cast<_Tp&&>(__t);
    }

  /**
   *  @brief  Convert a value to an rvalue.
   *  @param  __t  A thing of arbitrary type.
   *  @return The parameter cast to an rvalue-reference to allow moving it.
  */
  template<typename _Tp>
    constexpr typename std::remove_reference<_Tp>::type&&
    move(_Tp&& __t) noexcept
    { return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }


  // Adds a const reference to a non-reference type.
  template<typename _Tp>
    struct __add_c_ref
    { typedef const _Tp& type; };

  template<typename _Tp>
    struct __add_c_ref<_Tp&>
    { typedef _Tp& type; };

  template<std::size_t _Idx, typename _Head, bool _IsEmptyNotFinal>
    struct _Head_base;


  template<std::size_t _Idx, typename _Head>
    struct _Head_base<_Idx, _Head, false>
    {
      constexpr _Head_base()
      : _M_head_impl() { }

      constexpr _Head_base(const _Head& __h)
      : _M_head_impl(__h) { }

      template<typename _UHead, typename = true_type>
        constexpr _Head_base(_UHead&& __h)
	: _M_head_impl(std::forward<_UHead>(__h)) { }

      static constexpr const _Head&
      _M_head(const _Head_base& __b) noexcept { return __b._M_head_impl; }

      _Head _M_head_impl;
    };

  template<std::size_t _Idx, typename... _Elements>
    struct _Tuple_impl; 

  template<std::size_t _Idx>
    struct _Tuple_impl<_Idx>
    {
      template<std::size_t, typename...> friend class _Tuple_impl;

      _Tuple_impl() = default;
    };

   template<typename... _Elements>
   class tuple;

  template<std::size_t _Idx, typename _Head, typename... _Tail>
    struct _Tuple_impl<_Idx, _Head, _Tail...>
    : public _Tuple_impl<_Idx + 1, _Tail...>,
      private _Head_base<_Idx, _Head, /*__empty_not_final<_Head>::value*/
                         0>
    {
      template<std::size_t, typename...> friend class _Tuple_impl;

      typedef _Tuple_impl<_Idx + 1, _Tail...> _Inherited;
      typedef _Head_base<_Idx, _Head, /*__empty_not_final<_Head>::value*/ 0> _Base;

      static constexpr const _Head&
      _M_head(const _Tuple_impl& __t) noexcept { return _Base::_M_head(__t); }

      static constexpr const _Inherited&
      _M_tail(const _Tuple_impl& __t) noexcept { return __t; }

      template<typename _UHead, typename... _UTail, typename = false_type> 
        explicit
        constexpr _Tuple_impl(_UHead&& __head, _UTail&&... __tail)
	: _Inherited(std::forward<_UTail>(__tail)...),
	  _Base(std::forward<_UHead>(__head)) { }

      constexpr
      _Tuple_impl(_Tuple_impl&& __in)
      noexcept(false)
      : _Inherited(std::move(_M_tail(__in))), 
	_Base(std::forward<_Head>(_M_head(__in))) { }

    };

  /// Primary class template, tuple
  template<typename... _Elements> 
    class tuple : public _Tuple_impl<0, _Elements...>
    {
      typedef _Tuple_impl<0, _Elements...> _Inherited;

    public:
      template<typename... _UElements, typename = true_type>
	explicit
        constexpr tuple(_UElements&&... __elements)
	: _Inherited(std::forward<_UElements>(__elements)...) {	}
    };


  /// Gives the type of the ith element of a given tuple type.
  template<std::size_t __i, typename _Tp>
    struct tuple_element;

  /**
   * Recursive case for tuple_element: strip off the first element in
   * the tuple and retrieve the (i-1)th element of the remaining tuple.
   */
  template<std::size_t __i, typename _Head, typename... _Tail>
    struct tuple_element<__i, tuple<_Head, _Tail...> >
    : tuple_element<__i - 1, tuple<_Tail...> > { };

  /**
   * Basis case for tuple_element: The first element is the one we're seeking.
   */
  template<typename _Head, typename... _Tail>
    struct tuple_element<0, tuple<_Head, _Tail...> >
    {
      typedef _Head type;
    };

  template<std::size_t __i, typename _Tp>
    struct tuple_element<__i, const _Tp>
    {
      typedef const typename tuple_element<__i, _Tp>::type type;
    };


  template<std::size_t __i, typename _Head, typename... _Tail>
    constexpr typename __add_c_ref<_Head>::type
    __get_helper(const _Tuple_impl<__i, _Head, _Tail...>& __t) noexcept
    { return _Tuple_impl<__i, _Head, _Tail...>::_M_head(__t); }

  // Return a reference (const reference, rvalue reference) to the ith element
  // of a tuple.  Any const or non-const ref elements are returned with their
  // original type.
  template<std::size_t __i, typename... _Elements>
    constexpr typename __add_c_ref<
                      typename tuple_element<__i, tuple<_Elements...>>::type
                    >::type
    get(const tuple<_Elements...>& __t) noexcept
    { return __get_helper<__i>(__t); }


  template<typename... _Elements>
    tuple<_Elements&&...>
    forward_as_tuple(_Elements&&... __args) noexcept
    { return tuple<_Elements&&...>(std::forward<_Elements>(__args)...); }

} // namespace std


using namespace std;

template <class T>
void tpl(T x) {
  printf("tpl: &C=%p\n", get<0>(x).C);
  printf("tpl:  C=%s\n", get<0>(x).C);
};

template<typename... _Elements>
    tuple<_Elements&&...>
my_forward_as_tuple(_Elements&&... __args) noexcept
{ return tuple<_Elements&&...>(std::forward<_Elements>(__args)...); }


namespace {
void call(const char* pass, STR&& __k) {
  printf("%s: &rval=%p\n", pass, &__k);
  printf("%s: &C=%p\n", pass, __k.C);
  tpl(my_forward_as_tuple(std::move(__k)));
}
}

void clang() {
  call("clang", "a");
}
