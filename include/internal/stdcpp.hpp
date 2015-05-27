#ifndef TRIK_VIDTRANSCODE_CV_INTERNAL_STDCPP_H_
#define TRIK_VIDTRANSCODE_CV_INTERNAL_STDCPP_H_

#ifndef __cplusplus
#error C++-only header
#endif

#include <cassert>


/* **** **** **** **** **** */ namespace trik /* **** **** **** **** **** */ {


class noncopyable
{
  public:
    noncopyable() {}

  private:
    noncopyable(const noncopyable&);
    noncopyable& operator=(const noncopyable&);
};


template <bool _expr>
class assert_inst // kind of static_assert for TI compiler
{
  public:
    assert_inst()
    {
      char should_fail_on_false[_expr?1:-1]; // should not compile if _expr is false
      (void)should_fail_on_false; // warn prevention
    }

};


template <typename _T>
inline _T range(_T _min, _T _val, _T _max)
{
  if (_val < _min) return _min;
  else if (_val > _max) return _max;
  else return _val;
}


} /* **** **** **** **** **** * namespace trik * **** **** **** **** **** */


#endif // !TRIK_VIDTRANSCODE_CV_INTERNAL_STDCPP_H_
