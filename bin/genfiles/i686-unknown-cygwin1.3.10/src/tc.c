/* This is a C header file to be used by the output of the Cyclone
   to C translator.  The corresponding definitions are in file lib/runtime_cyc.c
*/
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

#include <setjmp.h>

#ifdef NO_CYC_PREFIX
#define ADD_PREFIX(x) x
#else
#define ADD_PREFIX(x) Cyc_##x
#endif

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Tagged arrays */
struct _dynforward_ptr {
  unsigned char *curr;
  unsigned char *last_plus_one;
};

struct _dyneither_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Need one of these per thread (we don't have threads)
   The runtime maintains a stack that contains either _handler_cons
   structs or _RegionHandle structs.  The tag is 0 for a handler_cons
   and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; /* 0 for an exception handler, 1 for a region handle */
  struct _RuntimeStack *next;
};

/* Regions */
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[0];
};

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#endif
};

struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};

extern struct _RegionHandle _new_region(const char *);
extern void * _region_malloc(struct _RegionHandle *, unsigned);
extern void * _region_calloc(struct _RegionHandle *, unsigned t, unsigned n);
extern void   _free_region(struct _RegionHandle *);
extern void   _reset_region(struct _RegionHandle *);
extern struct _RegionHandle *_open_dynregion(struct _DynRegionFrame *f,
                                             struct _DynRegionHandle *h);
extern void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
extern void _push_handler(struct _handler_cons *);
extern void _push_region(struct _RegionHandle *);
extern void _npop_handler(int);
extern void _pop_handler();
extern void _pop_region();

#ifndef _throw
extern int _throw_null();
extern int _throw_arraybounds();
extern int _throw_badalloc();
extern int _throw(void* e);
#endif

extern struct _xtunion_struct *_exn_thrown;

/* Built-in Exceptions */
extern struct _xtunion_struct ADD_PREFIX(Null_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Null_Exception);
extern struct _xtunion_struct ADD_PREFIX(Array_bounds_struct);
extern struct _xtunion_struct * ADD_PREFIX(Array_bounds);
extern struct _xtunion_struct ADD_PREFIX(Match_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Match_Exception);
extern struct _xtunion_struct ADD_PREFIX(Bad_alloc_struct);
extern struct _xtunion_struct * ADD_PREFIX(Bad_alloc);

/* Built-in Run-time Checks and company */
#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#ifdef _INLINE_FUNCTIONS
static inline void *
_check_null(void *ptr) {
  void*_check_null_temp = (void*)(ptr);
  if (!_check_null_temp) _throw_null();
  return _check_null_temp;
}
#else
#define _check_null(ptr) \
  ({ void*_check_null_temp = (void*)(ptr); \
     if (!_check_null_temp) _throw_null(); \
     _check_null_temp; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  ((char *)ptr) + (elt_sz)*(index); })
#else
#ifdef _INLINE_FUNCTIONS
static inline char *
_check_known_subscript_null(void *ptr, unsigned bound, unsigned elt_sz, unsigned index) {
  void*_cks_ptr = (void*)(ptr);
  unsigned _cks_bound = (bound);
  unsigned _cks_elt_sz = (elt_sz);
  unsigned _cks_index = (index);
  if (!_cks_ptr) _throw_null();
  if (_cks_index >= _cks_bound) _throw_arraybounds();
  return ((char *)_cks_ptr) + _cks_elt_sz*_cks_index;
}
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  void*_cks_ptr = (void*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  ((char *)_cks_ptr) + _cks_elt_sz*_cks_index; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(bound,index) (index)
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned
_check_known_subscript_notnull(unsigned bound,unsigned index) { 
  unsigned _cksnn_bound = (bound); 
  unsigned _cksnn_index = (index); 
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); 
  return _cksnn_index;
}
#else
#define _check_known_subscript_notnull(bound,index) ({ \
  unsigned _cksnn_bound = (bound); \
  unsigned _cksnn_index = (index); \
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); \
  _cksnn_index; })
#endif
#endif

/* Add i to zero-terminated pointer x.  Checks for x being null and
   ensures that x[0..i-1] are not 0. */
#ifdef NO_CYC_BOUNDS_CHECK
#define _zero_arr_plus(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#else
#define _zero_arr_plus(orig_x,orig_sz,orig_i) ({ \
  typedef _czs_tx = (*orig_x); \
  _czs_tx *_czs_x = (_czs_tx *)(orig_x); \
  unsigned int _czs_sz = (orig_sz); \
  int _czs_i = (orig_i); \
  unsigned int _czs_temp; \
  if ((_czs_x) == 0) _throw_null(); \
  if (_czs_i < 0) _throw_arraybounds(); \
  for (_czs_temp=_czs_sz; _czs_temp < _czs_i; _czs_temp++) \
    if (_czs_x[_czs_temp] == 0) _throw_arraybounds(); \
  _czs_x+_czs_i; })
#endif

/* Calculates the number of elements in a zero-terminated, thin array.
   If non-null, the array is guaranteed to have orig_offset elements. */
#define _get_zero_arr_size(orig_x,orig_offset) ({ \
  typedef _gres_tx = (*orig_x); \
  _gres_tx *_gres_x = (_gres_tx *)(orig_x); \
  unsigned int _gres_offset = (orig_offset); \
  unsigned int _gres = 0; \
  if (_gres_x != 0) { \
     _gres = _gres_offset; \
     _gres_x += _gres_offset - 1; \
     while (*_gres_x != 0) { _gres_x++; _gres++; } \
  } _gres; })

/* Does in-place addition of a zero-terminated pointer (x += e and ++x).  
   Note that this expands to call _zero_arr_plus. */
#define _zero_arr_inplace_plus(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  *_zap_x = _zero_arr_plus(*_zap_x,1,(orig_i)); })

/* Does in-place increment of a zero-terminated pointer (e.g., x++).
   Note that this expands to call _zero_arr_plus. */
#define _zero_arr_inplace_plus_post(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  _zap_tx *_zap_res = *_zap_x; \
  *_zap_x = _zero_arr_plus(_zap_res,1,(orig_i)); \
  _zap_res; })
  


/* functions for dealing with dynamically sized pointers */
#ifdef NO_CYC_BOUNDS_CHECKS
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  return _cus_ans;
}
static inline unsigned char *
_check_dynforward_subscript(struct _dynforward_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dynforward_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  return _cus_ans;
}
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#define _check_dynforward_subscript(arr,elt_sz,index) ({ \
  struct _dynforward_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#endif
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  if (!_cus_arr.base) _throw_null();
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds();
  return _cus_ans;
}
static inline unsigned char *
_check_dynforward_subscript(struct _dynforward_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dynforward_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  if (!_cus_arr.last_plus_one) _throw_null();
  if (_cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds();
  return _cus_ans;
}
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  if (!_cus_arr.base) _throw_null(); \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _check_dynforward_subscript(arr,elt_sz,index) ({ \
  struct _dynforward_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  if (!_cus_arr.last_plus_one) _throw_null(); \
  if (_cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _dyneither_ptr
_tag_dyneither(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _tag_arr_ans;
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
static inline struct _dynforward_ptr
_tag_dynforward(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _dynforward_ptr _tag_arr_ans;
  _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.curr + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
#else
#define _tag_dyneither(tcurr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#define _tag_dynforward(tcurr,elt_sz,num_elts) ({ \
  struct _dynforward_ptr _tag_arr_ans; \
  _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.curr + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _dyneither_ptr *
_init_dyneither_ptr(struct _dyneither_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
static inline struct _dynforward_ptr *
_init_dynforward_ptr(struct _dynforward_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dynforward_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
#else
#define _init_dyneither_ptr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#define _init_dynforward_ptr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _dynforward_ptr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_dynforward_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_untag_dyneither_ptr(struct _dyneither_ptr arr, 
                     unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
    _throw_arraybounds();
  return _curr;
}
static inline unsigned char *
_untag_dynforward_ptr(struct _dynforward_ptr arr, 
                      unsigned elt_sz,unsigned num_elts) {
  struct _dynforward_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
    _throw_arraybounds();
  return _curr;
}
#else
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)\
    _throw_arraybounds(); \
  _curr; })
#define _untag_dynforward_ptr(arr,elt_sz,num_elts) ({ \
  struct _dynforward_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if (_curr + (elt_sz) * (num_elts) > _arr.last_plus_one)\
    _throw_arraybounds(); \
  _curr; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static inline unsigned
_get_dyneither_size(struct _dyneither_ptr arr,unsigned elt_sz) {
  struct _dyneither_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
static inline unsigned
_get_dynforward_size(struct _dynforward_ptr arr,unsigned elt_sz) {
  struct _dynforward_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
#else
#define _get_dyneither_size(arr,elt_sz) \
  ({struct _dyneither_ptr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})
#define _get_dynforward_size(arr,elt_sz) \
  ({struct _dynforward_ptr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _dyneither_ptr
_dyneither_ptr_plus(struct _dyneither_ptr arr,unsigned elt_sz,int change) {
  struct _dyneither_ptr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
/* Here we have to worry about wrapping around, so if we go past the
 * end, we set the end to 0. */
static inline struct _dynforward_ptr
_dynforward_ptr_plus(struct _dynforward_ptr arr,unsigned elt_sz,int change) {
  struct _dynforward_ptr _ans = (arr);
  unsigned int _dfpp_elts = (((unsigned)_ans.last_plus_one) - 
                             ((unsigned)_ans.curr)) / elt_sz;
  if (change < 0 || ((unsigned)change) > _dfpp_elts)
    _ans.last_plus_one = 0;
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_plus(arr,elt_sz,change) ({ \
  struct _dyneither_ptr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })
#define _dynforward_ptr_plus(arr,elt_sz,change) ({ \
  struct _dynforward_ptr _ans = (arr); \
  unsigned _dfpp_elt_sz = (elt_sz); \
  int _dfpp_change = (change); \
  unsigned int _dfpp_elts = (((unsigned)_ans.last_plus_one) - \
                            ((unsigned)_ans.curr)) / _dfpp_elt_sz; \
  if (_dfpp_change < 0 || ((unsigned)_dfpp_change) > _dfpp_elts) \
    _ans.last_plus_one = 0; \
  _ans.curr += ((int)(_dfpp_elt_sz))*(_dfpp_change); \
  _ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _dyneither_ptr
_dyneither_ptr_inplace_plus(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,
                            int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
static inline struct _dynforward_ptr
_dynforward_ptr_inplace_plus(struct _dynforward_ptr *arr_ptr,unsigned elt_sz,
                             int change) {
  struct _dynforward_ptr * _arr_ptr = (arr_ptr);
  unsigned int _dfpp_elts = (((unsigned)_arr_ptr->last_plus_one) - 
                             ((unsigned)_arr_ptr->curr)) / elt_sz;
  if (change < 0 || ((unsigned)change) > _dfpp_elts) 
    _arr_ptr->last_plus_one = 0;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
#else
#define _dyneither_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })
#define _dynforward_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dynforward_ptr * _arr_ptr = (arr_ptr); \
  unsigned _dfpp_elt_sz = (elt_sz); \
  int _dfpp_change = (change); \
  unsigned int _dfpp_elts = (((unsigned)_arr_ptr->last_plus_one) - \
                            ((unsigned)_arr_ptr->curr)) / _dfpp_elt_sz; \
  if (_dfpp_change < 0 || ((unsigned)_dfpp_change) > _dfpp_elts) \
    _arr_ptr->last_plus_one = 0; \
  _arr_ptr->curr += ((int)(_dfpp_elt_sz))*(_dfpp_change); \
  *_arr_ptr; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _dyneither_ptr
_dyneither_ptr_inplace_plus_post(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  struct _dyneither_ptr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
static inline struct _dynforward_ptr
_dynforward_ptr_inplace_plus_post(struct _dynforward_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dynforward_ptr * _arr_ptr = (arr_ptr);
  struct _dynforward_ptr _ans = *_arr_ptr;
  unsigned int _dfpp_elts = (((unsigned)_arr_ptr->last_plus_one) - 
                            ((unsigned)_arr_ptr->curr)) / elt_sz; 
  if (change < 0 || ((unsigned)change) > _dfpp_elts) 
    _arr_ptr->last_plus_one = 0; 
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  struct _dyneither_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#define _dynforward_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dynforward_ptr * _arr_ptr = (arr_ptr); \
  struct _dynforward_ptr _ans = *_arr_ptr; \
  unsigned _dfpp_elt_sz = (elt_sz); \
  int _dfpp_change = (change); \
  unsigned int _dfpp_elts = (((unsigned)_arr_ptr->last_plus_one) - \
                            ((unsigned)_arr_ptr->curr)) / _dfpp_elt_sz; \
  if (_dfpp_change < 0 || ((unsigned)_dfpp_change) > _dfpp_elts) \
    _arr_ptr->last_plus_one = 0; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

// Decrease the upper bound on a fat pointer by numelts where sz is
// the size of the pointer's type.  Note that this can't be a macro
// if we're to get initializers right.
static struct 
_dyneither_ptr _dyneither_ptr_decrease_size(struct _dyneither_ptr x,
                                            unsigned int sz,
                                            unsigned int numelts) {
  x.last_plus_one -= sz * numelts; 
  return x; 
}
static struct 
_dynforward_ptr _dynforward_ptr_decrease_size(struct _dynforward_ptr x,
                                            unsigned int sz,
                                            unsigned int numelts) {
  if (x.last_plus_one != 0)
    x.last_plus_one -= sz * numelts; 
  return x; 
}

/* Convert between the two forms of dynamic pointers */
#ifdef _INLINE_FUNCTIONS 
static struct _dynforward_ptr
_dyneither_to_dynforward(struct _dyneither_ptr p) {
  struct _dynforward_ptr res;
  res.curr = p.curr;
  res.last_plus_one = (p.base == 0) ? 0 : p.last_plus_one;
  return res;
}
static struct _dyneither_ptr
_dynforward_to_dyneither(struct _dynforward_ptr p) {
  struct _dyneither_ptr res;
  res.base = res.curr = p.curr;
  res.last_plus_one = p.last_plus_one;
  if (p.last_plus_one == 0) 
    res.base = 0;
  return res;
}
#else 
#define _dyneither_to_dynforward(_dnfptr) ({ \
  struct _dyneither_ptr _dnfp = (_dnfptr); \
  struct _dynforward_ptr _dnfpres; \
  _dnfpres.curr = _dnfp.curr; \
  _dnfpres.last_plus_one = (_dnfp.base == 0) ? 0 : _dnfp.last_plus_one; \
  _dnfpres; })
#define _dynforward_to_dyneither(_dfnptr) ({ \
  struct _dynforward_ptr _dfnp = (_dfnptr); \
  struct _dyneither_ptr _dfnres; \
  _dfnres.base = _dfnres.curr = _dfnp.curr; \
  _dfnres.last_plus_one = _dfnp.last_plus_one; \
  if (_dfnp.last_plus_one == 0) \
    _dfnres.base = 0; \
  _dfnres; })
#endif 


//// Allocation
extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);

static inline void* _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cyccalloc(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cyccalloc_atomic(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static inline unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long)x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,char *file,int lineno);
extern void* _profile_GC_malloc_atomic(int,char *file,int lineno);
extern void* _profile_region_malloc(struct _RegionHandle *, unsigned,
                                     char *file,int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						char *file,int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 char *file,int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__ ":" __FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__ ":" __FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__ ":" __FUNCTION__,__LINE__)
#endif
#endif

/* the next three routines swap [x] and [y]; not thread safe! */
static inline void _swap_word(void *x, void *y) {
  unsigned long *lx = (unsigned long *)x, *ly = (unsigned long *)y, tmp;
  tmp = *lx;
  *lx = *ly;
  *ly = tmp;
}
static inline void _swap_dynforward(struct _dynforward_ptr *x, 
				    struct _dynforward_ptr *y) {
  struct _dynforward_ptr tmp = *x;
  *x = *y;
  *y = tmp;
}
static inline void _swap_dyneither(struct _dyneither_ptr *x, 
				   struct _dyneither_ptr *y) {
  struct _dyneither_ptr tmp = *x;
  *x = *y;
  *y = tmp;
}
 struct Cyc_Core_NewRegion{struct _DynRegionHandle*dynregion;};struct Cyc_Core_Opt{
void*v;};extern char Cyc_Core_Invalid_argument[21];struct Cyc_Core_Invalid_argument_struct{
char*tag;struct _dynforward_ptr f1;};extern char Cyc_Core_Failure[12];struct Cyc_Core_Failure_struct{
char*tag;struct _dynforward_ptr f1;};extern char Cyc_Core_Impossible[15];struct Cyc_Core_Impossible_struct{
char*tag;struct _dynforward_ptr f1;};extern char Cyc_Core_Not_found[14];extern char
Cyc_Core_Unreachable[16];struct Cyc_Core_Unreachable_struct{char*tag;struct
_dynforward_ptr f1;};extern struct _RegionHandle*Cyc_Core_heap_region;extern char Cyc_Core_Open_Region[
16];extern char Cyc_Core_Free_Region[16];struct Cyc_List_List{void*hd;struct Cyc_List_List*
tl;};int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[
18];void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);void Cyc_List_iter_c(
void(*f)(void*,void*),void*env,struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_append(
struct Cyc_List_List*x,struct Cyc_List_List*y);struct Cyc_List_List*Cyc_List_imp_append(
struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[8];int Cyc_List_mem(
int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc_List_List*Cyc_List_filter_c(
int(*f)(void*,void*),void*env,struct Cyc_List_List*x);int Cyc_strcmp(struct
_dynforward_ptr s1,struct _dynforward_ptr s2);int Cyc_strptrcmp(struct
_dynforward_ptr*s1,struct _dynforward_ptr*s2);struct Cyc_Lineno_Pos{struct
_dynforward_ptr logical_file;struct _dynforward_ptr line;int line_no;int col;};extern
char Cyc_Position_Exit[9];struct Cyc_Position_Segment;struct Cyc_Position_Error{
struct _dynforward_ptr source;struct Cyc_Position_Segment*seg;void*kind;struct
_dynforward_ptr desc;};extern char Cyc_Position_Nocontext[14];struct Cyc_Absyn_Loc_n_struct{
int tag;};struct Cyc_Absyn_Rel_n_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{
int tag;struct Cyc_List_List*f1;};union Cyc_Absyn_Nmspace_union{struct Cyc_Absyn_Loc_n_struct
Loc_n;struct Cyc_Absyn_Rel_n_struct Rel_n;struct Cyc_Absyn_Abs_n_struct Abs_n;};
struct _tuple0{union Cyc_Absyn_Nmspace_union f1;struct _dynforward_ptr*f2;};struct
Cyc_Absyn_Conref;struct Cyc_Absyn_Tqual{int print_const;int q_volatile;int
q_restrict;int real_const;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Eq_constr_struct{
int tag;void*f1;};struct Cyc_Absyn_Forward_constr_struct{int tag;struct Cyc_Absyn_Conref*
f1;};struct Cyc_Absyn_No_constr_struct{int tag;};union Cyc_Absyn_Constraint_union{
struct Cyc_Absyn_Eq_constr_struct Eq_constr;struct Cyc_Absyn_Forward_constr_struct
Forward_constr;struct Cyc_Absyn_No_constr_struct No_constr;};struct Cyc_Absyn_Conref{
union Cyc_Absyn_Constraint_union v;};struct Cyc_Absyn_Eq_kb_struct{int tag;void*f1;}
;struct Cyc_Absyn_Unknown_kb_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_struct{
int tag;struct Cyc_Core_Opt*f1;void*f2;};struct Cyc_Absyn_Tvar{struct
_dynforward_ptr*name;int identity;void*kind;};struct Cyc_Absyn_Upper_b_struct{int
tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AbsUpper_b_struct{int tag;void*f1;};
struct Cyc_Absyn_PtrLoc{struct Cyc_Position_Segment*ptr_loc;struct Cyc_Position_Segment*
rgn_loc;struct Cyc_Position_Segment*zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;
struct Cyc_Absyn_Conref*nullable;struct Cyc_Absyn_Conref*bounds;struct Cyc_Absyn_Conref*
zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_typ;
struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{
struct Cyc_Core_Opt*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{
struct Cyc_List_List*tvars;struct Cyc_Core_Opt*effect;void*ret_typ;struct Cyc_List_List*
args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*
rgn_po;struct Cyc_List_List*attributes;};struct Cyc_Absyn_UnknownTunionInfo{struct
_tuple0*name;int is_xtunion;int is_flat;};struct Cyc_Absyn_UnknownTunion_struct{int
tag;struct Cyc_Absyn_UnknownTunionInfo f1;};struct Cyc_Absyn_KnownTunion_struct{int
tag;struct Cyc_Absyn_Tuniondecl**f1;};union Cyc_Absyn_TunionInfoU_union{struct Cyc_Absyn_UnknownTunion_struct
UnknownTunion;struct Cyc_Absyn_KnownTunion_struct KnownTunion;};struct Cyc_Absyn_TunionInfo{
union Cyc_Absyn_TunionInfoU_union tunion_info;struct Cyc_List_List*targs;struct Cyc_Core_Opt*
rgn;};struct Cyc_Absyn_UnknownTunionFieldInfo{struct _tuple0*tunion_name;struct
_tuple0*field_name;int is_xtunion;};struct Cyc_Absyn_UnknownTunionfield_struct{int
tag;struct Cyc_Absyn_UnknownTunionFieldInfo f1;};struct Cyc_Absyn_KnownTunionfield_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};union Cyc_Absyn_TunionFieldInfoU_union{
struct Cyc_Absyn_UnknownTunionfield_struct UnknownTunionfield;struct Cyc_Absyn_KnownTunionfield_struct
KnownTunionfield;};struct Cyc_Absyn_TunionFieldInfo{union Cyc_Absyn_TunionFieldInfoU_union
field_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownAggr_struct{int tag;
void*f1;struct _tuple0*f2;};struct Cyc_Absyn_KnownAggr_struct{int tag;struct Cyc_Absyn_Aggrdecl**
f1;};union Cyc_Absyn_AggrInfoU_union{struct Cyc_Absyn_UnknownAggr_struct
UnknownAggr;struct Cyc_Absyn_KnownAggr_struct KnownAggr;};struct Cyc_Absyn_AggrInfo{
union Cyc_Absyn_AggrInfoU_union aggr_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_ArrayInfo{
void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;struct Cyc_Absyn_Conref*
zero_term;struct Cyc_Position_Segment*zt_loc;};struct Cyc_Absyn_Evar_struct{int tag;
struct Cyc_Core_Opt*f1;struct Cyc_Core_Opt*f2;int f3;struct Cyc_Core_Opt*f4;};struct
Cyc_Absyn_VarType_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_TunionType_struct{
int tag;struct Cyc_Absyn_TunionInfo f1;};struct Cyc_Absyn_TunionFieldType_struct{int
tag;struct Cyc_Absyn_TunionFieldInfo f1;};struct Cyc_Absyn_PointerType_struct{int
tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_struct{int tag;void*f1;
void*f2;};struct Cyc_Absyn_DoubleType_struct{int tag;int f1;};struct Cyc_Absyn_ArrayType_struct{
int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_struct{int tag;struct
Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_AggrType_struct{int tag;struct Cyc_Absyn_AggrInfo f1;};struct
Cyc_Absyn_AnonAggrType_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_struct{
int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_SizeofType_struct{int tag;void*f1;
};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*f1;};struct Cyc_Absyn_DynRgnType_struct{
int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_struct{int tag;struct _tuple0*
f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void**f4;};struct Cyc_Absyn_TagType_struct{
int tag;void*f1;};struct Cyc_Absyn_TypeInt_struct{int tag;int f1;};struct Cyc_Absyn_AccessEff_struct{
int tag;void*f1;};struct Cyc_Absyn_JoinEff_struct{int tag;struct Cyc_List_List*f1;};
struct Cyc_Absyn_RgnsEff_struct{int tag;void*f1;};struct Cyc_Absyn_NoTypes_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*f2;};struct Cyc_Absyn_WithTypes_struct{
int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;struct Cyc_Core_Opt*
f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Regparm_att_struct{int tag;int f1;};
struct Cyc_Absyn_Aligned_att_struct{int tag;int f1;};struct Cyc_Absyn_Section_att_struct{
int tag;struct _dynforward_ptr f1;};struct Cyc_Absyn_Format_att_struct{int tag;void*
f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_struct{int tag;int f1;};struct Cyc_Absyn_Mode_att_struct{
int tag;struct _dynforward_ptr f1;};struct Cyc_Absyn_Carray_mod_struct{int tag;struct
Cyc_Absyn_Conref*f1;struct Cyc_Position_Segment*f2;};struct Cyc_Absyn_ConstArray_mod_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Conref*f2;struct Cyc_Position_Segment*
f3;};struct Cyc_Absyn_Pointer_mod_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct
Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_struct{int tag;void*f1;};struct
Cyc_Absyn_TypeParams_mod_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*
f2;int f3;};struct Cyc_Absyn_Attributes_mod_struct{int tag;struct Cyc_Position_Segment*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Char_c_struct{int tag;void*f1;char f2;
};struct Cyc_Absyn_Short_c_struct{int tag;void*f1;short f2;};struct Cyc_Absyn_Int_c_struct{
int tag;void*f1;int f2;};struct Cyc_Absyn_LongLong_c_struct{int tag;void*f1;
long long f2;};struct Cyc_Absyn_Float_c_struct{int tag;struct _dynforward_ptr f1;};
struct Cyc_Absyn_String_c_struct{int tag;struct _dynforward_ptr f1;};struct Cyc_Absyn_Null_c_struct{
int tag;};union Cyc_Absyn_Cnst_union{struct Cyc_Absyn_Char_c_struct Char_c;struct Cyc_Absyn_Short_c_struct
Short_c;struct Cyc_Absyn_Int_c_struct Int_c;struct Cyc_Absyn_LongLong_c_struct
LongLong_c;struct Cyc_Absyn_Float_c_struct Float_c;struct Cyc_Absyn_String_c_struct
String_c;struct Cyc_Absyn_Null_c_struct Null_c;};struct Cyc_Absyn_VarargCallInfo{
int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
struct Cyc_Absyn_StructField_struct{int tag;struct _dynforward_ptr*f1;};struct Cyc_Absyn_TupleIndex_struct{
int tag;unsigned int f1;};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*
rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;};struct Cyc_Absyn_Const_e_struct{
int tag;union Cyc_Absyn_Cnst_union f1;};struct Cyc_Absyn_Var_e_struct{int tag;struct
_tuple0*f1;void*f2;};struct Cyc_Absyn_UnknownId_e_struct{int tag;struct _tuple0*f1;
};struct Cyc_Absyn_Primop_e_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct
Cyc_Absyn_AssignOp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;
struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;void*f2;};struct Cyc_Absyn_Conditional_e_struct{int tag;struct Cyc_Absyn_Exp*f1;
struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_struct{int
tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnknownCall_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*
f3;};struct Cyc_Absyn_Throw_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoInstantiate_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_struct{
int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;void*f4;};struct Cyc_Absyn_Address_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_struct{int tag;void*f1;};
struct Cyc_Absyn_Sizeofexp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_struct{
int tag;void*f1;void*f2;};struct Cyc_Absyn_Gentyp_e_struct{int tag;struct Cyc_List_List*
f1;void*f2;};struct Cyc_Absyn_Deref_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_AggrMember_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
_dynforward_ptr*f2;};struct Cyc_Absyn_AggrArrow_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _dynforward_ptr*f2;};struct Cyc_Absyn_Subscript_e_struct{int tag;struct
Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_struct{int tag;
struct Cyc_List_List*f1;};struct _tuple1{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual
f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple1*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;
};struct Cyc_Absyn_Comprehension_e_struct{int tag;struct Cyc_Absyn_Vardecl*f1;
struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_Struct_e_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*
f4;};struct Cyc_Absyn_AnonStruct_e_struct{int tag;void*f1;struct Cyc_List_List*f2;}
;struct Cyc_Absyn_Tunion_e_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*
f2;struct Cyc_Absyn_Tunionfield*f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct
_tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{
int tag;struct _tuple0*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;
void*r;struct Cyc_Position_Segment*loc;void*annot;};struct Cyc_Absyn_Exp_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_IfThenElse_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple2{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_While_s_struct{int tag;struct _tuple2 f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Break_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct
_dynforward_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct _tuple2 f2;struct _tuple2 f3;struct Cyc_Absyn_Stmt*f4;}
;struct Cyc_Absyn_Switch_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Fallthru_s_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**
f2;};struct Cyc_Absyn_Decl_s_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Label_s_struct{int tag;struct _dynforward_ptr*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Do_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple2 f2;
};struct Cyc_Absyn_TryCatch_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Region_s_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*
f2;int f3;struct Cyc_Absyn_Exp*f4;struct Cyc_Absyn_Stmt*f5;};struct Cyc_Absyn_ResetRegion_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Alias_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Absyn_Tvar*f2;struct Cyc_Absyn_Vardecl*f3;struct Cyc_Absyn_Stmt*f4;};
struct Cyc_Absyn_Stmt{void*r;struct Cyc_Position_Segment*loc;struct Cyc_List_List*
non_local_preds;int try_depth;void*annot;};struct Cyc_Absyn_Var_p_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_Reference_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_struct{
int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_struct{
int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_struct{int tag;
struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_struct{int tag;struct Cyc_Absyn_AggrInfo
f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Tunion_p_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;struct Cyc_List_List*
f3;int f4;};struct Cyc_Absyn_Int_p_struct{int tag;void*f1;int f2;};struct Cyc_Absyn_Char_p_struct{
int tag;char f1;};struct Cyc_Absyn_Float_p_struct{int tag;struct _dynforward_ptr f1;};
struct Cyc_Absyn_Enum_p_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*
f2;};struct Cyc_Absyn_AnonEnum_p_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*
f2;};struct Cyc_Absyn_UnknownId_p_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;struct Cyc_Core_Opt*
topt;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*
pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*
body;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Global_b_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Param_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Local_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{void*sc;struct
_tuple0*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;
struct Cyc_Core_Opt*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{
void*sc;int is_inline;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
effect;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;struct Cyc_Core_Opt*
cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;
struct Cyc_List_List*attributes;};struct Cyc_Absyn_Aggrfield{struct _dynforward_ptr*
name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*
attributes;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct
Cyc_List_List*rgn_po;struct Cyc_List_List*fields;};struct Cyc_Absyn_Aggrdecl{void*
kind;void*sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*
impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Tunionfield{struct _tuple0*
name;struct Cyc_List_List*typs;struct Cyc_Position_Segment*loc;void*sc;};struct Cyc_Absyn_Tuniondecl{
void*sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int
is_xtunion;int is_flat;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*
tag;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Enumdecl{void*sc;struct
_tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct
_tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
kind;struct Cyc_Core_Opt*defn;struct Cyc_List_List*atts;};struct Cyc_Absyn_Var_d_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Let_d_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*
f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Letv_d_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_Aggr_d_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct
Cyc_Absyn_Tunion_d_struct{int tag;struct Cyc_Absyn_Tuniondecl*f1;};struct Cyc_Absyn_Enum_d_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_struct{int tag;
struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_struct{int tag;
struct _dynforward_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*r;
struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_dynforward_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];int Cyc_Absyn_qvar_cmp(
struct _tuple0*,struct _tuple0*);struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(struct
Cyc_Position_Segment*);struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(struct Cyc_Position_Segment*);
struct Cyc_Absyn_Conref*Cyc_Absyn_empty_conref();void*Cyc_Absyn_compress_kb(void*);
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);void*Cyc_Absyn_string_typ(
void*rgn);void*Cyc_Absyn_const_string_typ(void*rgn);void*Cyc_Absyn_dynforward_typ(
void*t,void*rgn,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Conref*zero_term);void*
Cyc_Absyn_array_typ(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*
num_elts,struct Cyc_Absyn_Conref*zero_term,struct Cyc_Position_Segment*ztloc);
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned int,struct Cyc_Position_Segment*);
struct _dynforward_ptr Cyc_Absyn_attribute2string(void*);struct Cyc___cycFILE;
struct Cyc_Cstdio___abstractFILE;struct Cyc_String_pa_struct{int tag;struct
_dynforward_ptr f1;};struct Cyc_Int_pa_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_struct{
int tag;double f1;};struct Cyc_LongDouble_pa_struct{int tag;long double f1;};struct
Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{int tag;
unsigned long*f1;};struct Cyc_ShortPtr_sa_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{
int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{
int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct
_dynforward_ptr f1;};struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dynforward_ptr f1;};
extern char Cyc_FileCloseError[19];extern char Cyc_FileOpenError[18];struct Cyc_FileOpenError_struct{
char*tag;struct _dynforward_ptr f1;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct
Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs: 1;int qvar_to_Cids: 1;int
add_cyc_prefix: 1;int to_VC: 1;int decls_first: 1;int rewrite_temp_tvars: 1;int
print_all_tvars: 1;int print_all_kinds: 1;int print_all_effects: 1;int
print_using_stmts: 1;int print_externC_stmts: 1;int print_full_evars: 1;int
print_zeroterm: 1;int generate_line_directives: 1;int use_curr_namespace: 1;struct Cyc_List_List*
curr_namespace;};void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);extern
struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;struct _dynforward_ptr Cyc_Absynpp_typ2string(
void*);struct _dynforward_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);struct Cyc_Iter_Iter{
void*env;int(*next)(void*env,void*dest);};int Cyc_Iter_next(struct Cyc_Iter_Iter,
void*);struct Cyc_Set_Set;struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*r,
int(*cmp)(void*,void*));struct Cyc_Set_Set*Cyc_Set_rinsert(struct _RegionHandle*r,
struct Cyc_Set_Set*s,void*elt);int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);
extern char Cyc_Set_Absent[11];struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,
void*);struct _RegionHandle*r;struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[12];
extern char Cyc_Dict_Absent[11];int Cyc_Dict_is_empty(struct Cyc_Dict_Dict d);int Cyc_Dict_member(
struct Cyc_Dict_Dict d,void*k);struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict
d,void*k,void*v);void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);void**Cyc_Dict_lookup_opt(
struct Cyc_Dict_Dict d,void*k);struct Cyc_Dict_Dict Cyc_Dict_rmap_c(struct
_RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d);struct
_tuple3{void*f1;void*f2;};struct _tuple3*Cyc_Dict_rchoose(struct _RegionHandle*r,
struct Cyc_Dict_Dict d);struct _tuple3*Cyc_Dict_rchoose(struct _RegionHandle*,struct
Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rfilter_c(struct _RegionHandle*,int(*
f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Iter_Iter Cyc_Dict_make_iter(
struct _RegionHandle*rgn,struct Cyc_Dict_Dict d);struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*
Cyc_RgnOrder_initial_fn_po(struct _RegionHandle*,struct Cyc_List_List*tvs,struct
Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,struct Cyc_Position_Segment*);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct
_RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn,struct Cyc_Position_Segment*
loc);struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct _RegionHandle*,
struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn,int resetable,int opened);int
Cyc_RgnOrder_is_region_resetable(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*
r);int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*
constraints,void*default_bound,int do_pin);int Cyc_RgnOrder_eff_outlives_eff(
struct Cyc_RgnOrder_RgnPO*po,void*eff1,void*eff2);void Cyc_RgnOrder_print_region_po(
struct Cyc_RgnOrder_RgnPO*po);struct Cyc_Tcenv_CList{void*hd;struct Cyc_Tcenv_CList*
tl;};struct Cyc_Tcenv_VarRes_struct{int tag;void*f1;};struct Cyc_Tcenv_AggrRes_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Tcenv_TunionRes_struct{int tag;
struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};struct Cyc_Tcenv_EnumRes_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_AnonEnumRes_struct{
int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_Genv{struct
_RegionHandle*grgn;struct Cyc_Set_Set*namespaces;struct Cyc_Dict_Dict aggrdecls;
struct Cyc_Dict_Dict tuniondecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict
typedefs;struct Cyc_Dict_Dict ordinaries;struct Cyc_List_List*availables;};struct
Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Stmt_j_struct{int tag;struct Cyc_Absyn_Stmt*f1;};
struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Dict_Dict ae;struct Cyc_Tcenv_Fenv*
le;};struct Cyc_Tcenv_Genv*Cyc_Tcenv_empty_genv(struct _RegionHandle*);struct Cyc_Tcenv_Fenv*
Cyc_Tcenv_new_fenv(struct _RegionHandle*,struct Cyc_Position_Segment*,struct Cyc_Absyn_Fndecl*);
struct Cyc_List_List*Cyc_Tcenv_resolve_namespace(struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,
struct _dynforward_ptr*,struct Cyc_List_List*);struct Cyc_Absyn_Tuniondecl***Cyc_Tcenv_lookup_xtuniondecl(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,struct
_tuple0*);int Cyc_Tcenv_all_labels_resolved(struct Cyc_Tcenv_Tenv*);void Cyc_Tcenv_check_delayed_effects(
struct Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*
te);void*Cyc_Tcutil_impos(struct _dynforward_ptr fmt,struct _dynforward_ptr ap);void
Cyc_Tcutil_terr(struct Cyc_Position_Segment*,struct _dynforward_ptr fmt,struct
_dynforward_ptr ap);void Cyc_Tcutil_warn(struct Cyc_Position_Segment*,struct
_dynforward_ptr fmt,struct _dynforward_ptr ap);void*Cyc_Tcutil_compress(void*t);int
Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*);int
Cyc_Tcutil_is_function_type(void*t);void*Cyc_Tcutil_kind_to_bound(void*k);void
Cyc_Tcutil_explain_failure();int Cyc_Tcutil_unify(void*,void*);void*Cyc_Tcutil_fndecl2typ(
struct Cyc_Absyn_Fndecl*);void Cyc_Tcutil_check_bitfield(struct Cyc_Position_Segment*
loc,struct Cyc_Tcenv_Tenv*te,void*field_typ,struct Cyc_Absyn_Exp*width,struct
_dynforward_ptr*fn);void Cyc_Tcutil_check_valid_toplevel_type(struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*,void*);void Cyc_Tcutil_check_fndecl_valid_type(struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Fndecl*);void Cyc_Tcutil_check_type(struct
Cyc_Position_Segment*,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,void*
k,int allow_evars,void*);void Cyc_Tcutil_check_unique_tvars(struct Cyc_Position_Segment*,
struct Cyc_List_List*);void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*);
int Cyc_Tcutil_bits_only(void*t);int Cyc_Tcutil_is_const_exp(struct Cyc_Tcenv_Tenv*
te,struct Cyc_Absyn_Exp*e);int Cyc_Tcutil_supports_default(void*);int Cyc_Tcutil_extract_const_from_typedef(
struct Cyc_Position_Segment*,int declared_const,void*);struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(
void*t,struct Cyc_List_List*atts);void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,
void**,struct Cyc_Absyn_Exp*);void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Absyn_Stmt*s,int new_block);struct _tuple4{unsigned int f1;int f2;};struct
_tuple4 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);void Cyc_Tc_tc(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*
ds);struct Cyc_List_List*Cyc_Tc_treeshake(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*);
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*,struct Cyc_Tcenv_Genv*,struct Cyc_Position_Segment*,
struct Cyc_Absyn_Aggrdecl*);void Cyc_Tc_tcTuniondecl(struct Cyc_Tcenv_Tenv*,struct
Cyc_Tcenv_Genv*,struct Cyc_Position_Segment*,struct Cyc_Absyn_Tuniondecl*);void Cyc_Tc_tcEnumdecl(
struct Cyc_Tcenv_Tenv*,struct Cyc_Tcenv_Genv*,struct Cyc_Position_Segment*,struct
Cyc_Absyn_Enumdecl*);extern char Cyc_Tcdecl_Incompatible[17];struct Cyc_Tcdecl_Xtunionfielddecl{
struct Cyc_Absyn_Tuniondecl*base;struct Cyc_Absyn_Tunionfield*field;};struct Cyc_Absyn_Aggrdecl*
Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*d0,struct Cyc_Absyn_Aggrdecl*
d1,struct Cyc_Position_Segment*loc,struct _dynforward_ptr*msg);struct Cyc_Absyn_Tuniondecl*
Cyc_Tcdecl_merge_tuniondecl(struct Cyc_Absyn_Tuniondecl*d0,struct Cyc_Absyn_Tuniondecl*
d1,struct Cyc_Position_Segment*loc,struct _dynforward_ptr*msg);struct Cyc_Absyn_Enumdecl*
Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*d0,struct Cyc_Absyn_Enumdecl*
d1,struct Cyc_Position_Segment*loc,struct _dynforward_ptr*msg);void*Cyc_Tcdecl_merge_binding(
void*d0,void*d1,struct Cyc_Position_Segment*loc,struct _dynforward_ptr*msg);struct
Cyc_List_List*Cyc_Tcdecl_sort_xtunion_fields(struct Cyc_List_List*f,int*res,
struct _dynforward_ptr*v,struct Cyc_Position_Segment*loc,struct _dynforward_ptr*msg);
struct Cyc_Tcgenrep_RepInfo;struct Cyc_Dict_Dict Cyc_Tcgenrep_empty_typerep_dict();
struct _tuple5{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Exp*
f3;};struct _tuple5 Cyc_Tcgenrep_tcGenrep(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*
ge,struct Cyc_Position_Segment*loc,void*type,struct Cyc_Dict_Dict dict);static char
_tmp0[1]="";static struct _dynforward_ptr Cyc_Tc_tc_msg_c={_tmp0,_tmp0 + 1};static
struct _dynforward_ptr*Cyc_Tc_tc_msg=(struct _dynforward_ptr*)& Cyc_Tc_tc_msg_c;
struct _tuple6{struct Cyc_Position_Segment*f1;struct _tuple0*f2;int f3;};static int
Cyc_Tc_export_member(struct _tuple0*x,struct Cyc_List_List*exports){for(0;exports
!= 0;exports=exports->tl){struct _tuple6*_tmp1=(struct _tuple6*)exports->hd;if(Cyc_Absyn_qvar_cmp(
x,(*_tmp1).f2)== 0){(*_tmp1).f3=1;return 1;}}return 0;}struct _tuple7{void*f1;int f2;
};static void Cyc_Tc_tcVardecl(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,
struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Vardecl*vd,int check_var_init,int
ignore_init,struct Cyc_List_List**exports){struct Cyc_Absyn_Vardecl _tmp3;void*
_tmp4;struct _tuple0*_tmp5;struct _tuple0 _tmp6;union Cyc_Absyn_Nmspace_union _tmp7;
struct _dynforward_ptr*_tmp8;void*_tmp9;struct Cyc_Absyn_Exp*_tmpA;struct Cyc_List_List*
_tmpB;struct Cyc_Absyn_Vardecl*_tmp2=vd;_tmp3=*_tmp2;_tmp4=(void*)_tmp3.sc;_tmp5=
_tmp3.name;_tmp6=*_tmp5;_tmp7=_tmp6.f1;_tmp8=_tmp6.f2;_tmp9=(void*)_tmp3.type;
_tmpA=_tmp3.initializer;_tmpB=_tmp3.attributes;{union Cyc_Absyn_Nmspace_union
_tmpC=_tmp7;struct Cyc_List_List*_tmpD;struct Cyc_List_List*_tmpE;_LL1: if((_tmpC.Rel_n).tag
!= 1)goto _LL3;_tmpD=(_tmpC.Rel_n).f1;if(_tmpD != 0)goto _LL3;_LL2: goto _LL4;_LL3:
if((_tmpC.Abs_n).tag != 2)goto _LL5;_tmpE=(_tmpC.Abs_n).f1;if(_tmpE != 0)goto _LL5;
_LL4: goto _LL0;_LL5:;_LL6:({struct Cyc_String_pa_struct _tmp11;_tmp11.tag=0;_tmp11.f1=(
struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(vd->name));{
void*_tmpF[1]={& _tmp11};Cyc_Tcutil_terr(loc,({const char*_tmp10="qualified variable declarations are not implemented (%s)";
_tag_dynforward(_tmp10,sizeof(char),_get_zero_arr_size(_tmp10,57));}),
_tag_dynforward(_tmpF,sizeof(void*),1));}});return;_LL0:;}(*vd->name).f1=(union
Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp12;(_tmp12.Abs_n).tag=
2;(_tmp12.Abs_n).f1=te->ns;_tmp12;});{void*_tmp13=Cyc_Tcutil_compress(_tmp9);
struct Cyc_Absyn_ArrayInfo _tmp14;void*_tmp15;struct Cyc_Absyn_Tqual _tmp16;struct
Cyc_Absyn_Exp*_tmp17;struct Cyc_Absyn_Conref*_tmp18;struct Cyc_Position_Segment*
_tmp19;_LL8: if(_tmp13 <= (void*)4)goto _LLA;if(*((int*)_tmp13)!= 7)goto _LLA;_tmp14=((
struct Cyc_Absyn_ArrayType_struct*)_tmp13)->f1;_tmp15=(void*)_tmp14.elt_type;
_tmp16=_tmp14.tq;_tmp17=_tmp14.num_elts;if(_tmp17 != 0)goto _LLA;_tmp18=_tmp14.zero_term;
_tmp19=_tmp14.zt_loc;if(!(_tmpA != 0))goto _LLA;_LL9:{void*_tmp1A=(void*)_tmpA->r;
union Cyc_Absyn_Cnst_union _tmp1B;struct _dynforward_ptr _tmp1C;struct Cyc_Absyn_Exp*
_tmp1D;struct Cyc_List_List*_tmp1E;struct Cyc_List_List*_tmp1F;_LLD: if(*((int*)
_tmp1A)!= 0)goto _LLF;_tmp1B=((struct Cyc_Absyn_Const_e_struct*)_tmp1A)->f1;if(((((
struct Cyc_Absyn_Const_e_struct*)_tmp1A)->f1).String_c).tag != 5)goto _LLF;_tmp1C=(
_tmp1B.String_c).f1;_LLE: _tmp9=(void*)(vd->type=(void*)Cyc_Absyn_array_typ(
_tmp15,_tmp16,(struct Cyc_Absyn_Exp*)Cyc_Absyn_uint_exp(_get_dynforward_size(
_tmp1C,sizeof(char)),0),_tmp18,_tmp19));goto _LLC;_LLF: if(*((int*)_tmp1A)!= 29)
goto _LL11;_tmp1D=((struct Cyc_Absyn_Comprehension_e_struct*)_tmp1A)->f2;_LL10:
_tmp9=(void*)(vd->type=(void*)Cyc_Absyn_array_typ(_tmp15,_tmp16,(struct Cyc_Absyn_Exp*)
_tmp1D,_tmp18,_tmp19));goto _LLC;_LL11: if(*((int*)_tmp1A)!= 37)goto _LL13;_tmp1E=((
struct Cyc_Absyn_UnresolvedMem_e_struct*)_tmp1A)->f2;_LL12: _tmp1F=_tmp1E;goto
_LL14;_LL13: if(*((int*)_tmp1A)!= 28)goto _LL15;_tmp1F=((struct Cyc_Absyn_Array_e_struct*)
_tmp1A)->f1;_LL14: _tmp9=(void*)(vd->type=(void*)Cyc_Absyn_array_typ(_tmp15,
_tmp16,(struct Cyc_Absyn_Exp*)Cyc_Absyn_uint_exp((unsigned int)((int(*)(struct Cyc_List_List*
x))Cyc_List_length)(_tmp1F),0),_tmp18,_tmp19));goto _LLC;_LL15:;_LL16: goto _LLC;
_LLC:;}goto _LL7;_LLA:;_LLB: goto _LL7;_LL7:;}Cyc_Tcutil_check_valid_toplevel_type(
loc,te,_tmp9);(vd->tq).real_const=Cyc_Tcutil_extract_const_from_typedef(loc,(vd->tq).print_const,
_tmp9);{void*_tmp20=Cyc_Tcutil_compress(_tmp9);_LL18: if(_tmp20 <= (void*)4)goto
_LL1A;if(*((int*)_tmp20)!= 7)goto _LL1A;_LL19: vd->escapes=0;goto _LL17;_LL1A:;
_LL1B: vd->escapes=1;goto _LL17;_LL17:;}if(Cyc_Tcutil_is_function_type(_tmp9))
_tmpB=Cyc_Tcutil_transfer_fn_type_atts(_tmp9,_tmpB);if(_tmp4 == (void*)3  || _tmp4
== (void*)4){if(_tmpA != 0)({void*_tmp21[0]={};Cyc_Tcutil_terr(loc,({const char*
_tmp22="extern declaration should not have initializer";_tag_dynforward(_tmp22,
sizeof(char),_get_zero_arr_size(_tmp22,47));}),_tag_dynforward(_tmp21,sizeof(
void*),0));});}else{if(!Cyc_Tcutil_is_function_type(_tmp9)){for(0;_tmpB != 0;
_tmpB=_tmpB->tl){void*_tmp23=(void*)_tmpB->hd;_LL1D: if(_tmp23 <= (void*)17)goto
_LL21;if(*((int*)_tmp23)!= 1)goto _LL1F;_LL1E: goto _LL20;_LL1F: if(*((int*)_tmp23)
!= 2)goto _LL21;_LL20: goto _LL22;_LL21: if((int)_tmp23 != 6)goto _LL23;_LL22: goto
_LL24;_LL23: if((int)_tmp23 != 7)goto _LL25;_LL24: goto _LL26;_LL25: if((int)_tmp23 != 
8)goto _LL27;_LL26: goto _LL28;_LL27: if((int)_tmp23 != 9)goto _LL29;_LL28: goto _LL2A;
_LL29: if((int)_tmp23 != 10)goto _LL2B;_LL2A: goto _LL2C;_LL2B: if((int)_tmp23 != 11)
goto _LL2D;_LL2C: continue;_LL2D:;_LL2E:({struct Cyc_String_pa_struct _tmp27;_tmp27.tag=
0;_tmp27.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(
vd->name));{struct Cyc_String_pa_struct _tmp26;_tmp26.tag=0;_tmp26.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)Cyc_Absyn_attribute2string((void*)_tmpB->hd));{
void*_tmp24[2]={& _tmp26,& _tmp27};Cyc_Tcutil_terr(loc,({const char*_tmp25="bad attribute %s for variable %s";
_tag_dynforward(_tmp25,sizeof(char),_get_zero_arr_size(_tmp25,33));}),
_tag_dynforward(_tmp24,sizeof(void*),2));}}});goto _LL1C;_LL1C:;}if(_tmpA == 0  || 
ignore_init){if(check_var_init  && !Cyc_Tcutil_supports_default(_tmp9))({struct
Cyc_String_pa_struct _tmp2B;_tmp2B.tag=0;_tmp2B.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)Cyc_Absynpp_typ2string(_tmp9));{struct Cyc_String_pa_struct
_tmp2A;_tmp2A.tag=0;_tmp2A.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(
vd->name));{void*_tmp28[2]={& _tmp2A,& _tmp2B};Cyc_Tcutil_terr(loc,({const char*
_tmp29="initializer required for variable %s of type %s";_tag_dynforward(_tmp29,
sizeof(char),_get_zero_arr_size(_tmp29,48));}),_tag_dynforward(_tmp28,sizeof(
void*),2));}}});}else{struct Cyc_Absyn_Exp*_tmp2C=(struct Cyc_Absyn_Exp*)_tmpA;
void*_tmp2D=Cyc_Tcexp_tcExpInitializer(te,(void**)& _tmp9,_tmp2C);if(!Cyc_Tcutil_coerce_assign(
te,_tmp2C,_tmp9)){({struct Cyc_String_pa_struct _tmp32;_tmp32.tag=0;_tmp32.f1=(
struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_typ2string(_tmp2D));{
struct Cyc_String_pa_struct _tmp31;_tmp31.tag=0;_tmp31.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)Cyc_Absynpp_typ2string(_tmp9));{struct Cyc_String_pa_struct
_tmp30;_tmp30.tag=0;_tmp30.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(
vd->name));{void*_tmp2E[3]={& _tmp30,& _tmp31,& _tmp32};Cyc_Tcutil_terr(loc,({const
char*_tmp2F="%s declared with type \n%s\n but initialized with type \n%s";
_tag_dynforward(_tmp2F,sizeof(char),_get_zero_arr_size(_tmp2F,57));}),
_tag_dynforward(_tmp2E,sizeof(void*),3));}}}});Cyc_Tcutil_explain_failure();}if(
!Cyc_Tcutil_is_const_exp(te,_tmp2C))({void*_tmp33[0]={};Cyc_Tcutil_terr(loc,({
const char*_tmp34="initializer is not a constant expression";_tag_dynforward(
_tmp34,sizeof(char),_get_zero_arr_size(_tmp34,41));}),_tag_dynforward(_tmp33,
sizeof(void*),0));});}}else{for(0;_tmpB != 0;_tmpB=_tmpB->tl){void*_tmp35=(void*)
_tmpB->hd;_LL30: if(_tmp35 <= (void*)17)goto _LL32;if(*((int*)_tmp35)!= 0)goto _LL32;
_LL31: goto _LL33;_LL32: if((int)_tmp35 != 0)goto _LL34;_LL33: goto _LL35;_LL34: if((int)
_tmp35 != 1)goto _LL36;_LL35: goto _LL37;_LL36: if((int)_tmp35 != 2)goto _LL38;_LL37:
goto _LL39;_LL38: if((int)_tmp35 != 3)goto _LL3A;_LL39: goto _LL3B;_LL3A: if(_tmp35 <= (
void*)17)goto _LL3E;if(*((int*)_tmp35)!= 3)goto _LL3C;_LL3B: goto _LL3D;_LL3C: if(*((
int*)_tmp35)!= 4)goto _LL3E;_LL3D: goto _LL3F;_LL3E: if((int)_tmp35 != 16)goto _LL40;
_LL3F: goto _LL41;_LL40: if((int)_tmp35 != 4)goto _LL42;_LL41:({void*_tmp36[0]={};((
int(*)(struct _dynforward_ptr fmt,struct _dynforward_ptr ap))Cyc_Tcutil_impos)(({
const char*_tmp37="tcVardecl: fn type atts in function var decl";_tag_dynforward(
_tmp37,sizeof(char),_get_zero_arr_size(_tmp37,45));}),_tag_dynforward(_tmp36,
sizeof(void*),0));});_LL42: if(_tmp35 <= (void*)17)goto _LL44;if(*((int*)_tmp35)!= 
1)goto _LL44;_LL43: goto _LL45;_LL44: if((int)_tmp35 != 5)goto _LL46;_LL45:({struct Cyc_String_pa_struct
_tmp3A;_tmp3A.tag=0;_tmp3A.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absyn_attribute2string((
void*)_tmpB->hd));{void*_tmp38[1]={& _tmp3A};Cyc_Tcutil_terr(loc,({const char*
_tmp39="bad attribute %s in function declaration";_tag_dynforward(_tmp39,sizeof(
char),_get_zero_arr_size(_tmp39,41));}),_tag_dynforward(_tmp38,sizeof(void*),1));}});
goto _LL2F;_LL46:;_LL47: continue;_LL2F:;}}}{struct _handler_cons _tmp3B;
_push_handler(& _tmp3B);{int _tmp3D=0;if(setjmp(_tmp3B.handler))_tmp3D=1;if(!
_tmp3D){{struct _tuple7*_tmp3E=((struct _tuple7*(*)(struct Cyc_Dict_Dict d,struct
_dynforward_ptr*k))Cyc_Dict_lookup)(ge->ordinaries,_tmp8);void*_tmp3F=(*_tmp3E).f1;
void*_tmp40;_LL49: if(*((int*)_tmp3F)!= 0)goto _LL4B;_tmp40=(void*)((struct Cyc_Tcenv_VarRes_struct*)
_tmp3F)->f1;_LL4A: {struct Cyc_Absyn_Global_b_struct*_tmp41=({struct Cyc_Absyn_Global_b_struct*
_tmp46=_cycalloc(sizeof(*_tmp46));_tmp46[0]=({struct Cyc_Absyn_Global_b_struct
_tmp47;_tmp47.tag=0;_tmp47.f1=vd;_tmp47;});_tmp46;});void*_tmp42=Cyc_Tcdecl_merge_binding(
_tmp40,(void*)_tmp41,loc,Cyc_Tc_tc_msg);if(_tmp42 == (void*)0){_npop_handler(0);
return;}if(_tmp42 == _tmp40  && (*_tmp3E).f2){_npop_handler(0);return;}if(exports
== 0  || Cyc_Tc_export_member(vd->name,*exports))ge->ordinaries=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct _tuple7*v))Cyc_Dict_insert)(
ge->ordinaries,_tmp8,(struct _tuple7*)({struct _tuple7*_tmp43=_cycalloc(sizeof(*
_tmp43));_tmp43->f1=(void*)({struct Cyc_Tcenv_VarRes_struct*_tmp44=_cycalloc(
sizeof(*_tmp44));_tmp44[0]=({struct Cyc_Tcenv_VarRes_struct _tmp45;_tmp45.tag=0;
_tmp45.f1=(void*)_tmp42;_tmp45;});_tmp44;});_tmp43->f2=1;_tmp43;}));
_npop_handler(0);return;}_LL4B: if(*((int*)_tmp3F)!= 1)goto _LL4D;_LL4C:({void*
_tmp48[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp49="variable declaration shadows previous struct declaration";
_tag_dynforward(_tmp49,sizeof(char),_get_zero_arr_size(_tmp49,57));}),
_tag_dynforward(_tmp48,sizeof(void*),0));});goto _LL48;_LL4D: if(*((int*)_tmp3F)!= 
2)goto _LL4F;_LL4E:({void*_tmp4A[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp4B="variable declaration shadows previous [x]tunion constructor";
_tag_dynforward(_tmp4B,sizeof(char),_get_zero_arr_size(_tmp4B,60));}),
_tag_dynforward(_tmp4A,sizeof(void*),0));});goto _LL48;_LL4F: if(*((int*)_tmp3F)!= 
4)goto _LL51;_LL50: goto _LL52;_LL51: if(*((int*)_tmp3F)!= 3)goto _LL48;_LL52:({void*
_tmp4C[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp4D="variable declaration shadows previous enum tag";
_tag_dynforward(_tmp4D,sizeof(char),_get_zero_arr_size(_tmp4D,47));}),
_tag_dynforward(_tmp4C,sizeof(void*),0));});goto _LL48;_LL48:;};_pop_handler();}
else{void*_tmp3C=(void*)_exn_thrown;void*_tmp4F=_tmp3C;_LL54: if(_tmp4F != Cyc_Dict_Absent)
goto _LL56;_LL55: goto _LL53;_LL56:;_LL57:(void)_throw(_tmp4F);_LL53:;}}}if(exports
== 0  || Cyc_Tc_export_member(vd->name,*exports))ge->ordinaries=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct _tuple7*v))Cyc_Dict_insert)(
ge->ordinaries,_tmp8,(struct _tuple7*)({struct _tuple7*_tmp50=_cycalloc(sizeof(*
_tmp50));_tmp50->f1=(void*)({struct Cyc_Tcenv_VarRes_struct*_tmp51=_cycalloc(
sizeof(*_tmp51));_tmp51[0]=({struct Cyc_Tcenv_VarRes_struct _tmp52;_tmp52.tag=0;
_tmp52.f1=(void*)((void*)({struct Cyc_Absyn_Global_b_struct*_tmp53=_cycalloc(
sizeof(*_tmp53));_tmp53[0]=({struct Cyc_Absyn_Global_b_struct _tmp54;_tmp54.tag=0;
_tmp54.f1=vd;_tmp54;});_tmp53;}));_tmp52;});_tmp51;});_tmp50->f2=0;_tmp50;}));}
static int Cyc_Tc_is_main(struct _tuple0*n){struct _tuple0 _tmp56;union Cyc_Absyn_Nmspace_union
_tmp57;struct _dynforward_ptr*_tmp58;struct _tuple0*_tmp55=n;_tmp56=*_tmp55;_tmp57=
_tmp56.f1;_tmp58=_tmp56.f2;{union Cyc_Absyn_Nmspace_union _tmp59=_tmp57;struct Cyc_List_List*
_tmp5A;_LL59: if((_tmp59.Abs_n).tag != 2)goto _LL5B;_tmp5A=(_tmp59.Abs_n).f1;if(
_tmp5A != 0)goto _LL5B;_LL5A: return Cyc_strcmp((struct _dynforward_ptr)*_tmp58,({
const char*_tmp5B="main";_tag_dynforward(_tmp5B,sizeof(char),_get_zero_arr_size(
_tmp5B,5));}))== 0;_LL5B:;_LL5C: return 0;_LL58:;}}struct _tuple8{struct
_dynforward_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};static void Cyc_Tc_tcFndecl(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*loc,
struct Cyc_Absyn_Fndecl*fd,int ignore_body,struct Cyc_List_List**exports){struct
_dynforward_ptr*v=(*fd->name).f2;if((void*)fd->sc == (void*)4  && !ignore_body)({
void*_tmp5C[0]={};Cyc_Tcutil_terr(loc,({const char*_tmp5D="extern \"C\" functions cannot be implemented in Cyclone";
_tag_dynforward(_tmp5D,sizeof(char),_get_zero_arr_size(_tmp5D,54));}),
_tag_dynforward(_tmp5C,sizeof(void*),0));});{union Cyc_Absyn_Nmspace_union _tmp5E=(*
fd->name).f1;struct Cyc_List_List*_tmp5F;struct Cyc_List_List*_tmp60;_LL5E: if((
_tmp5E.Rel_n).tag != 1)goto _LL60;_tmp5F=(_tmp5E.Rel_n).f1;if(_tmp5F != 0)goto _LL60;
_LL5F: goto _LL5D;_LL60: if((_tmp5E.Abs_n).tag != 2)goto _LL62;_tmp60=(_tmp5E.Abs_n).f1;
_LL61:({void*_tmp61[0]={};((int(*)(struct _dynforward_ptr fmt,struct
_dynforward_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp62="tc: Abs_n in tcFndecl";
_tag_dynforward(_tmp62,sizeof(char),_get_zero_arr_size(_tmp62,22));}),
_tag_dynforward(_tmp61,sizeof(void*),0));});_LL62:;_LL63:({struct Cyc_String_pa_struct
_tmp65;_tmp65.tag=0;_tmp65.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(
fd->name));{void*_tmp63[1]={& _tmp65};Cyc_Tcutil_terr(loc,({const char*_tmp64="qualified function declarations are not implemented (%s)";
_tag_dynforward(_tmp64,sizeof(char),_get_zero_arr_size(_tmp64,57));}),
_tag_dynforward(_tmp63,sizeof(void*),1));}});return;_LL5D:;}(*fd->name).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp66;(_tmp66.Abs_n).tag=
2;(_tmp66.Abs_n).f1=te->ns;_tmp66;});Cyc_Tcutil_check_fndecl_valid_type(loc,te,
fd);{void*t=Cyc_Tcutil_fndecl2typ(fd);fd->attributes=Cyc_Tcutil_transfer_fn_type_atts(
t,fd->attributes);{struct Cyc_List_List*atts=fd->attributes;for(0;atts != 0;atts=
atts->tl){void*_tmp67=(void*)atts->hd;_LL65: if((int)_tmp67 != 5)goto _LL67;_LL66:
goto _LL68;_LL67: if(_tmp67 <= (void*)17)goto _LL69;if(*((int*)_tmp67)!= 1)goto _LL69;
_LL68:({struct Cyc_String_pa_struct _tmp6A;_tmp6A.tag=0;_tmp6A.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)Cyc_Absyn_attribute2string((void*)atts->hd));{
void*_tmp68[1]={& _tmp6A};Cyc_Tcutil_terr(loc,({const char*_tmp69="bad attribute %s for function";
_tag_dynforward(_tmp69,sizeof(char),_get_zero_arr_size(_tmp69,30));}),
_tag_dynforward(_tmp68,sizeof(void*),1));}});goto _LL64;_LL69:;_LL6A: goto _LL64;
_LL64:;}}{struct _handler_cons _tmp6B;_push_handler(& _tmp6B);{int _tmp6D=0;if(
setjmp(_tmp6B.handler))_tmp6D=1;if(!_tmp6D){{struct _tuple7*_tmp6E=((struct
_tuple7*(*)(struct Cyc_Dict_Dict d,struct _dynforward_ptr*k))Cyc_Dict_lookup)(ge->ordinaries,
v);void*_tmp6F=(*_tmp6E).f1;void*_tmp70;_LL6C: if(*((int*)_tmp6F)!= 0)goto _LL6E;
_tmp70=(void*)((struct Cyc_Tcenv_VarRes_struct*)_tmp6F)->f1;_LL6D: {struct Cyc_Absyn_Funname_b_struct*
_tmp71=({struct Cyc_Absyn_Funname_b_struct*_tmp76=_cycalloc(sizeof(*_tmp76));
_tmp76[0]=({struct Cyc_Absyn_Funname_b_struct _tmp77;_tmp77.tag=1;_tmp77.f1=fd;
_tmp77;});_tmp76;});void*_tmp72=Cyc_Tcdecl_merge_binding(_tmp70,(void*)_tmp71,
loc,Cyc_Tc_tc_msg);if(_tmp72 == (void*)0)goto _LL6B;if(_tmp72 == _tmp70  && (*_tmp6E).f2)
goto _LL6B;if(exports == 0  || Cyc_Tc_export_member(fd->name,*exports))ge->ordinaries=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct
_tuple7*v))Cyc_Dict_insert)(ge->ordinaries,v,(struct _tuple7*)({struct _tuple7*
_tmp73=_cycalloc(sizeof(*_tmp73));_tmp73->f1=(void*)({struct Cyc_Tcenv_VarRes_struct*
_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74[0]=({struct Cyc_Tcenv_VarRes_struct
_tmp75;_tmp75.tag=0;_tmp75.f1=(void*)_tmp72;_tmp75;});_tmp74;});_tmp73->f2=1;
_tmp73;}));goto _LL6B;}_LL6E: if(*((int*)_tmp6F)!= 1)goto _LL70;_LL6F:({void*_tmp78[
0]={};Cyc_Tcutil_warn(loc,({const char*_tmp79="function declaration shadows previous type declaration";
_tag_dynforward(_tmp79,sizeof(char),_get_zero_arr_size(_tmp79,55));}),
_tag_dynforward(_tmp78,sizeof(void*),0));});goto _LL6B;_LL70: if(*((int*)_tmp6F)!= 
2)goto _LL72;_LL71:({void*_tmp7A[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp7B="function declaration shadows previous [x]tunion constructor";
_tag_dynforward(_tmp7B,sizeof(char),_get_zero_arr_size(_tmp7B,60));}),
_tag_dynforward(_tmp7A,sizeof(void*),0));});goto _LL6B;_LL72: if(*((int*)_tmp6F)!= 
4)goto _LL74;_LL73: goto _LL75;_LL74: if(*((int*)_tmp6F)!= 3)goto _LL6B;_LL75:({void*
_tmp7C[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp7D="function declaration shadows previous enum tag";
_tag_dynforward(_tmp7D,sizeof(char),_get_zero_arr_size(_tmp7D,47));}),
_tag_dynforward(_tmp7C,sizeof(void*),0));});goto _LL6B;_LL6B:;};_pop_handler();}
else{void*_tmp6C=(void*)_exn_thrown;void*_tmp7F=_tmp6C;_LL77: if(_tmp7F != Cyc_Dict_Absent)
goto _LL79;_LL78: if(exports == 0  || Cyc_Tc_export_member(fd->name,*exports))ge->ordinaries=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct
_tuple7*v))Cyc_Dict_insert)(ge->ordinaries,v,(struct _tuple7*)({struct _tuple7*
_tmp80=_cycalloc(sizeof(*_tmp80));_tmp80->f1=(void*)({struct Cyc_Tcenv_VarRes_struct*
_tmp81=_cycalloc(sizeof(*_tmp81));_tmp81[0]=({struct Cyc_Tcenv_VarRes_struct
_tmp82;_tmp82.tag=0;_tmp82.f1=(void*)((void*)({struct Cyc_Absyn_Funname_b_struct*
_tmp83=_cycalloc(sizeof(*_tmp83));_tmp83[0]=({struct Cyc_Absyn_Funname_b_struct
_tmp84;_tmp84.tag=1;_tmp84.f1=fd;_tmp84;});_tmp83;}));_tmp82;});_tmp81;});_tmp80->f2=
0;_tmp80;}));goto _LL76;_LL79:;_LL7A:(void)_throw(_tmp7F);_LL76:;}}}if(
ignore_body)return;{struct _RegionHandle _tmp85=_new_region("fnrgn");struct
_RegionHandle*fnrgn=& _tmp85;_push_region(fnrgn);{struct Cyc_Tcenv_Fenv*_tmp86=Cyc_Tcenv_new_fenv(
fnrgn,loc,fd);struct Cyc_Tcenv_Tenv*_tmp87=({struct Cyc_Tcenv_Tenv*_tmp8A=
_region_malloc(fnrgn,sizeof(*_tmp8A));_tmp8A->ns=te->ns;_tmp8A->ae=te->ae;_tmp8A->le=(
struct Cyc_Tcenv_Fenv*)_tmp86;_tmp8A;});Cyc_Tcstmt_tcStmt(_tmp87,fd->body,0);Cyc_Tcenv_check_delayed_effects(
_tmp87);Cyc_Tcenv_check_delayed_constraints(_tmp87);if(!Cyc_Tcenv_all_labels_resolved(
_tmp87))({void*_tmp88[0]={};Cyc_Tcutil_terr(loc,({const char*_tmp89="function has goto statements to undefined labels";
_tag_dynforward(_tmp89,sizeof(char),_get_zero_arr_size(_tmp89,49));}),
_tag_dynforward(_tmp88,sizeof(void*),0));});};_pop_region(fnrgn);}if(Cyc_Tc_is_main(
fd->name)){{void*_tmp8B=Cyc_Tcutil_compress((void*)fd->ret_type);void*_tmp8C;
_LL7C: if((int)_tmp8B != 0)goto _LL7E;_LL7D:({void*_tmp8D[0]={};Cyc_Tcutil_warn(loc,({
const char*_tmp8E="main declared with return type void";_tag_dynforward(_tmp8E,
sizeof(char),_get_zero_arr_size(_tmp8E,36));}),_tag_dynforward(_tmp8D,sizeof(
void*),0));});goto _LL7B;_LL7E: if(_tmp8B <= (void*)4)goto _LL80;if(*((int*)_tmp8B)
!= 5)goto _LL80;_tmp8C=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp8B)->f2;_LL7F:
goto _LL7B;_LL80:;_LL81:({struct Cyc_String_pa_struct _tmp91;_tmp91.tag=0;_tmp91.f1=(
struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_typ2string((void*)fd->ret_type));{
void*_tmp8F[1]={& _tmp91};Cyc_Tcutil_terr(loc,({const char*_tmp90="main declared with return type %s instead of int or void";
_tag_dynforward(_tmp90,sizeof(char),_get_zero_arr_size(_tmp90,57));}),
_tag_dynforward(_tmp8F,sizeof(void*),1));}});goto _LL7B;_LL7B:;}if(fd->c_varargs
 || fd->cyc_varargs != 0)({void*_tmp92[0]={};Cyc_Tcutil_terr(loc,({const char*
_tmp93="main declared with varargs";_tag_dynforward(_tmp93,sizeof(char),
_get_zero_arr_size(_tmp93,27));}),_tag_dynforward(_tmp92,sizeof(void*),0));});{
struct Cyc_List_List*_tmp94=fd->args;if(_tmp94 != 0){struct _tuple8 _tmp96;void*
_tmp97;struct _tuple8*_tmp95=(struct _tuple8*)_tmp94->hd;_tmp96=*_tmp95;_tmp97=
_tmp96.f3;{void*_tmp98=Cyc_Tcutil_compress(_tmp97);void*_tmp99;_LL83: if(_tmp98 <= (
void*)4)goto _LL85;if(*((int*)_tmp98)!= 5)goto _LL85;_tmp99=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp98)->f2;_LL84: goto _LL82;_LL85:;_LL86:({struct Cyc_String_pa_struct _tmp9C;
_tmp9C.tag=0;_tmp9C.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_typ2string(
_tmp97));{void*_tmp9A[1]={& _tmp9C};Cyc_Tcutil_terr(loc,({const char*_tmp9B="main declared with first argument of type %s instead of int";
_tag_dynforward(_tmp9B,sizeof(char),_get_zero_arr_size(_tmp9B,60));}),
_tag_dynforward(_tmp9A,sizeof(void*),1));}});goto _LL82;_LL82:;}_tmp94=_tmp94->tl;
if(_tmp94 != 0){struct _tuple8 _tmp9E;void*_tmp9F;struct _tuple8*_tmp9D=(struct
_tuple8*)_tmp94->hd;_tmp9E=*_tmp9D;_tmp9F=_tmp9E.f3;_tmp94=_tmp94->tl;if(_tmp94
!= 0)({void*_tmpA0[0]={};Cyc_Tcutil_terr(loc,({const char*_tmpA1="main declared with too many arguments";
_tag_dynforward(_tmpA1,sizeof(char),_get_zero_arr_size(_tmpA1,38));}),
_tag_dynforward(_tmpA0,sizeof(void*),0));});{struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*
_tmpAD=_cycalloc(sizeof(*_tmpAD));_tmpAD->v=fd->tvs;_tmpAD;});if(((!Cyc_Tcutil_unify(
_tmp9F,Cyc_Absyn_dynforward_typ(Cyc_Absyn_string_typ(Cyc_Absyn_new_evar(({struct
Cyc_Core_Opt*_tmpA2=_cycalloc(sizeof(*_tmpA2));_tmpA2->v=(void*)((void*)3);
_tmpA2;}),tvs)),Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmpA3=_cycalloc(sizeof(*
_tmpA3));_tmpA3->v=(void*)((void*)3);_tmpA3;}),tvs),Cyc_Absyn_empty_tqual(0),((
struct Cyc_Absyn_Conref*(*)())Cyc_Absyn_empty_conref)())) && !Cyc_Tcutil_unify(
_tmp9F,Cyc_Absyn_dynforward_typ(Cyc_Absyn_const_string_typ(Cyc_Absyn_new_evar(({
struct Cyc_Core_Opt*_tmpA4=_cycalloc(sizeof(*_tmpA4));_tmpA4->v=(void*)((void*)3);
_tmpA4;}),tvs)),Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmpA5=_cycalloc(sizeof(*
_tmpA5));_tmpA5->v=(void*)((void*)3);_tmpA5;}),tvs),Cyc_Absyn_empty_tqual(0),((
struct Cyc_Absyn_Conref*(*)())Cyc_Absyn_empty_conref)()))) && !Cyc_Tcutil_unify(
_tmp9F,Cyc_Absyn_dynforward_typ(Cyc_Absyn_string_typ(Cyc_Absyn_new_evar(({struct
Cyc_Core_Opt*_tmpA6=_cycalloc(sizeof(*_tmpA6));_tmpA6->v=(void*)((void*)3);
_tmpA6;}),tvs)),Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmpA7=_cycalloc(sizeof(*
_tmpA7));_tmpA7->v=(void*)((void*)3);_tmpA7;}),tvs),Cyc_Absyn_const_tqual(0),((
struct Cyc_Absyn_Conref*(*)())Cyc_Absyn_empty_conref)()))) && !Cyc_Tcutil_unify(
_tmp9F,Cyc_Absyn_dynforward_typ(Cyc_Absyn_const_string_typ(Cyc_Absyn_new_evar(({
struct Cyc_Core_Opt*_tmpA8=_cycalloc(sizeof(*_tmpA8));_tmpA8->v=(void*)((void*)3);
_tmpA8;}),tvs)),Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmpA9=_cycalloc(sizeof(*
_tmpA9));_tmpA9->v=(void*)((void*)3);_tmpA9;}),tvs),Cyc_Absyn_const_tqual(0),((
struct Cyc_Absyn_Conref*(*)())Cyc_Absyn_empty_conref)())))({struct Cyc_String_pa_struct
_tmpAC;_tmpAC.tag=0;_tmpAC.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_typ2string(
_tmp9F));{void*_tmpAA[1]={& _tmpAC};Cyc_Tcutil_terr(loc,({const char*_tmpAB="second argument of main has type %s instead of char??";
_tag_dynforward(_tmpAB,sizeof(char),_get_zero_arr_size(_tmpAB,54));}),
_tag_dynforward(_tmpAA,sizeof(void*),1));}});}}}}}}}static void Cyc_Tc_tcTypedefdecl(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*loc,
struct Cyc_Absyn_Typedefdecl*td){struct _dynforward_ptr*v=(*td->name).f2;{union Cyc_Absyn_Nmspace_union
_tmpAE=(*td->name).f1;struct Cyc_List_List*_tmpAF;struct Cyc_List_List*_tmpB0;
_LL88: if((_tmpAE.Rel_n).tag != 1)goto _LL8A;_tmpAF=(_tmpAE.Rel_n).f1;if(_tmpAF != 0)
goto _LL8A;_LL89: goto _LL8B;_LL8A: if((_tmpAE.Abs_n).tag != 2)goto _LL8C;_tmpB0=(
_tmpAE.Abs_n).f1;if(_tmpB0 != 0)goto _LL8C;_LL8B: goto _LL87;_LL8C:;_LL8D:({struct
Cyc_String_pa_struct _tmpB3;_tmpB3.tag=0;_tmpB3.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)Cyc_Absynpp_qvar2string(td->name));{void*_tmpB1[1]={&
_tmpB3};Cyc_Tcutil_terr(loc,({const char*_tmpB2="qualified typedef declarations are not implemented (%s)";
_tag_dynforward(_tmpB2,sizeof(char),_get_zero_arr_size(_tmpB2,56));}),
_tag_dynforward(_tmpB1,sizeof(void*),1));}});return;_LL87:;}if(((int(*)(struct
Cyc_Dict_Dict d,struct _dynforward_ptr*k))Cyc_Dict_member)(ge->typedefs,v)){({
struct Cyc_String_pa_struct _tmpB6;_tmpB6.tag=0;_tmpB6.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)*v);{void*_tmpB4[1]={& _tmpB6};Cyc_Tcutil_terr(loc,({const
char*_tmpB5="redeclaration of typedef %s";_tag_dynforward(_tmpB5,sizeof(char),
_get_zero_arr_size(_tmpB5,28));}),_tag_dynforward(_tmpB4,sizeof(void*),1));}});
return;}(*td->name).f1=(union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union
_tmpB7;(_tmpB7.Abs_n).tag=2;(_tmpB7.Abs_n).f1=te->ns;_tmpB7;});Cyc_Tcutil_check_unique_tvars(
loc,td->tvs);Cyc_Tcutil_add_tvar_identities(td->tvs);if(td->defn != 0){Cyc_Tcutil_check_type(
loc,te,td->tvs,(void*)0,0,(void*)((struct Cyc_Core_Opt*)_check_null(td->defn))->v);(
td->tq).real_const=Cyc_Tcutil_extract_const_from_typedef(loc,(td->tq).print_const,(
void*)((struct Cyc_Core_Opt*)_check_null(td->defn))->v);}{struct Cyc_List_List*tvs=
td->tvs;for(0;tvs != 0;tvs=tvs->tl){void*_tmpB8=Cyc_Absyn_compress_kb((void*)((
struct Cyc_Absyn_Tvar*)tvs->hd)->kind);struct Cyc_Core_Opt*_tmpB9;struct Cyc_Core_Opt**
_tmpBA;struct Cyc_Core_Opt*_tmpBB;struct Cyc_Core_Opt**_tmpBC;void*_tmpBD;_LL8F:
if(*((int*)_tmpB8)!= 1)goto _LL91;_tmpB9=((struct Cyc_Absyn_Unknown_kb_struct*)
_tmpB8)->f1;_tmpBA=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_struct*)
_tmpB8)->f1;_LL90: if(td->defn != 0)({struct Cyc_String_pa_struct _tmpC0;_tmpC0.tag=
0;_tmpC0.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*((struct Cyc_Absyn_Tvar*)
tvs->hd)->name);{void*_tmpBE[1]={& _tmpC0};Cyc_Tcutil_warn(loc,({const char*_tmpBF="type variable %s is not used in typedef definition";
_tag_dynforward(_tmpBF,sizeof(char),_get_zero_arr_size(_tmpBF,51));}),
_tag_dynforward(_tmpBE,sizeof(void*),1));}});*_tmpBA=({struct Cyc_Core_Opt*_tmpC1=
_cycalloc(sizeof(*_tmpC1));_tmpC1->v=(void*)Cyc_Tcutil_kind_to_bound((void*)2);
_tmpC1;});goto _LL8E;_LL91: if(*((int*)_tmpB8)!= 2)goto _LL93;_tmpBB=((struct Cyc_Absyn_Less_kb_struct*)
_tmpB8)->f1;_tmpBC=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_struct*)
_tmpB8)->f1;_tmpBD=(void*)((struct Cyc_Absyn_Less_kb_struct*)_tmpB8)->f2;_LL92:*
_tmpBC=({struct Cyc_Core_Opt*_tmpC2=_cycalloc(sizeof(*_tmpC2));_tmpC2->v=(void*)
Cyc_Tcutil_kind_to_bound(_tmpBD);_tmpC2;});goto _LL8E;_LL93:;_LL94: continue;_LL8E:;}}
ge->typedefs=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _dynforward_ptr*
k,struct Cyc_Absyn_Typedefdecl*v))Cyc_Dict_insert)(ge->typedefs,v,td);}static void
Cyc_Tc_tcAggrImpl(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*
loc,struct Cyc_List_List*tvs,struct Cyc_List_List*rpo,struct Cyc_List_List*fields){
struct _RegionHandle _tmpC3=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=&
_tmpC3;_push_region(uprev_rgn);for(0;rpo != 0;rpo=rpo->tl){struct _tuple3 _tmpC5;
void*_tmpC6;void*_tmpC7;struct _tuple3*_tmpC4=(struct _tuple3*)rpo->hd;_tmpC5=*
_tmpC4;_tmpC6=_tmpC5.f1;_tmpC7=_tmpC5.f2;Cyc_Tcutil_check_type(loc,te,tvs,(void*)
6,0,_tmpC6);Cyc_Tcutil_check_type(loc,te,tvs,(void*)5,0,_tmpC7);}{struct Cyc_List_List*
prev_fields=0;struct Cyc_List_List*_tmpC8=fields;for(0;_tmpC8 != 0;_tmpC8=_tmpC8->tl){
struct Cyc_Absyn_Aggrfield _tmpCA;struct _dynforward_ptr*_tmpCB;struct Cyc_Absyn_Tqual
_tmpCC;void*_tmpCD;struct Cyc_Absyn_Exp*_tmpCE;struct Cyc_List_List*_tmpCF;struct
Cyc_Absyn_Aggrfield*_tmpC9=(struct Cyc_Absyn_Aggrfield*)_tmpC8->hd;_tmpCA=*_tmpC9;
_tmpCB=_tmpCA.name;_tmpCC=_tmpCA.tq;_tmpCD=(void*)_tmpCA.type;_tmpCE=_tmpCA.width;
_tmpCF=_tmpCA.attributes;if(((int(*)(int(*compare)(struct _dynforward_ptr*,struct
_dynforward_ptr*),struct Cyc_List_List*l,struct _dynforward_ptr*x))Cyc_List_mem)(
Cyc_strptrcmp,prev_fields,_tmpCB))({struct Cyc_String_pa_struct _tmpD2;_tmpD2.tag=
0;_tmpD2.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*_tmpCB);{void*_tmpD0[
1]={& _tmpD2};Cyc_Tcutil_terr(loc,({const char*_tmpD1="duplicate field %s";
_tag_dynforward(_tmpD1,sizeof(char),_get_zero_arr_size(_tmpD1,19));}),
_tag_dynforward(_tmpD0,sizeof(void*),1));}});if(Cyc_strcmp((struct
_dynforward_ptr)*_tmpCB,({const char*_tmpD3="";_tag_dynforward(_tmpD3,sizeof(char),
_get_zero_arr_size(_tmpD3,1));}))!= 0)prev_fields=({struct Cyc_List_List*_tmpD4=
_region_malloc(uprev_rgn,sizeof(*_tmpD4));_tmpD4->hd=_tmpCB;_tmpD4->tl=
prev_fields;_tmpD4;});Cyc_Tcutil_check_type(loc,te,tvs,(void*)1,0,_tmpCD);(((
struct Cyc_Absyn_Aggrfield*)_tmpC8->hd)->tq).real_const=Cyc_Tcutil_extract_const_from_typedef(
loc,(((struct Cyc_Absyn_Aggrfield*)_tmpC8->hd)->tq).print_const,_tmpCD);Cyc_Tcutil_check_bitfield(
loc,te,_tmpCD,_tmpCE,_tmpCB);}};_pop_region(uprev_rgn);}struct _tuple9{struct Cyc_Absyn_AggrdeclImpl*
f1;struct Cyc_Absyn_Aggrdecl***f2;};void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Aggrdecl*
ad){struct _dynforward_ptr*_tmpD5=(*ad->name).f2;{struct Cyc_List_List*atts=ad->attributes;
for(0;atts != 0;atts=atts->tl){void*_tmpD6=(void*)atts->hd;_LL96: if((int)_tmpD6 != 
5)goto _LL98;_LL97: goto _LL99;_LL98: if(_tmpD6 <= (void*)17)goto _LL9A;if(*((int*)
_tmpD6)!= 1)goto _LL9A;_LL99: continue;_LL9A:;_LL9B:({struct Cyc_String_pa_struct
_tmpDA;_tmpDA.tag=0;_tmpDA.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*
_tmpD5);{struct Cyc_String_pa_struct _tmpD9;_tmpD9.tag=0;_tmpD9.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)Cyc_Absyn_attribute2string((void*)atts->hd));{
void*_tmpD7[2]={& _tmpD9,& _tmpDA};Cyc_Tcutil_terr(loc,({const char*_tmpD8="bad attribute %s in  %s definition";
_tag_dynforward(_tmpD8,sizeof(char),_get_zero_arr_size(_tmpD8,35));}),
_tag_dynforward(_tmpD7,sizeof(void*),2));}}});goto _LL95;_LL95:;}}{struct Cyc_List_List*
_tmpDB=ad->tvs;{struct Cyc_List_List*tvs2=_tmpDB;for(0;tvs2 != 0;tvs2=tvs2->tl){
void*_tmpDC=Cyc_Absyn_compress_kb((void*)((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);
struct Cyc_Core_Opt*_tmpDD;struct Cyc_Core_Opt**_tmpDE;struct Cyc_Core_Opt*_tmpDF;
struct Cyc_Core_Opt**_tmpE0;void*_tmpE1;struct Cyc_Core_Opt*_tmpE2;struct Cyc_Core_Opt**
_tmpE3;void*_tmpE4;void*_tmpE5;_LL9D: if(*((int*)_tmpDC)!= 1)goto _LL9F;_tmpDD=((
struct Cyc_Absyn_Unknown_kb_struct*)_tmpDC)->f1;_tmpDE=(struct Cyc_Core_Opt**)&((
struct Cyc_Absyn_Unknown_kb_struct*)_tmpDC)->f1;_LL9E: _tmpE0=_tmpDE;goto _LLA0;
_LL9F: if(*((int*)_tmpDC)!= 2)goto _LLA1;_tmpDF=((struct Cyc_Absyn_Less_kb_struct*)
_tmpDC)->f1;_tmpE0=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_struct*)
_tmpDC)->f1;_tmpE1=(void*)((struct Cyc_Absyn_Less_kb_struct*)_tmpDC)->f2;if((int)
_tmpE1 != 1)goto _LLA1;_LLA0: _tmpE3=_tmpE0;goto _LLA2;_LLA1: if(*((int*)_tmpDC)!= 2)
goto _LLA3;_tmpE2=((struct Cyc_Absyn_Less_kb_struct*)_tmpDC)->f1;_tmpE3=(struct Cyc_Core_Opt**)&((
struct Cyc_Absyn_Less_kb_struct*)_tmpDC)->f1;_tmpE4=(void*)((struct Cyc_Absyn_Less_kb_struct*)
_tmpDC)->f2;if((int)_tmpE4 != 0)goto _LLA3;_LLA2:*_tmpE3=({struct Cyc_Core_Opt*
_tmpE6=_cycalloc(sizeof(*_tmpE6));_tmpE6->v=(void*)Cyc_Tcutil_kind_to_bound((
void*)2);_tmpE6;});continue;_LLA3: if(*((int*)_tmpDC)!= 0)goto _LLA5;_tmpE5=(void*)((
struct Cyc_Absyn_Eq_kb_struct*)_tmpDC)->f1;if((int)_tmpE5 != 1)goto _LLA5;_LLA4:({
struct Cyc_String_pa_struct _tmpEA;_tmpEA.tag=0;_tmpEA.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)*((struct Cyc_Absyn_Tvar*)tvs2->hd)->name);{struct Cyc_String_pa_struct
_tmpE9;_tmpE9.tag=0;_tmpE9.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*
_tmpD5);{void*_tmpE7[2]={& _tmpE9,& _tmpEA};Cyc_Tcutil_terr(loc,({const char*_tmpE8="type %s attempts to abstract type variable %s of kind M";
_tag_dynforward(_tmpE8,sizeof(char),_get_zero_arr_size(_tmpE8,56));}),
_tag_dynforward(_tmpE7,sizeof(void*),2));}}});continue;_LLA5:;_LLA6: continue;
_LL9C:;}}{union Cyc_Absyn_Nmspace_union _tmpEB=(*ad->name).f1;struct Cyc_List_List*
_tmpEC;struct Cyc_List_List*_tmpED;_LLA8: if((_tmpEB.Rel_n).tag != 1)goto _LLAA;
_tmpEC=(_tmpEB.Rel_n).f1;if(_tmpEC != 0)goto _LLAA;_LLA9: goto _LLAB;_LLAA: if((
_tmpEB.Abs_n).tag != 2)goto _LLAC;_tmpED=(_tmpEB.Abs_n).f1;if(_tmpED != 0)goto _LLAC;
_LLAB: goto _LLA7;_LLAC:;_LLAD:({struct Cyc_String_pa_struct _tmpF0;_tmpF0.tag=0;
_tmpF0.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string(
ad->name));{void*_tmpEE[1]={& _tmpF0};Cyc_Tcutil_terr(loc,({const char*_tmpEF="qualified struct declarations are not implemented (%s)";
_tag_dynforward(_tmpEF,sizeof(char),_get_zero_arr_size(_tmpEF,55));}),
_tag_dynforward(_tmpEE,sizeof(void*),1));}});return;_LLA7:;}(*ad->name).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmpF1;(_tmpF1.Abs_n).tag=
2;(_tmpF1.Abs_n).f1=te->ns;_tmpF1;});Cyc_Tcutil_check_unique_tvars(loc,ad->tvs);
Cyc_Tcutil_add_tvar_identities(ad->tvs);{struct _tuple9 _tmpF3=({struct _tuple9
_tmpF2;_tmpF2.f1=ad->impl;_tmpF2.f2=((struct Cyc_Absyn_Aggrdecl***(*)(struct Cyc_Dict_Dict
d,struct _dynforward_ptr*k))Cyc_Dict_lookup_opt)(ge->aggrdecls,_tmpD5);_tmpF2;});
struct Cyc_Absyn_AggrdeclImpl*_tmpF4;struct Cyc_Absyn_Aggrdecl***_tmpF5;struct Cyc_Absyn_AggrdeclImpl*
_tmpF6;struct Cyc_Absyn_AggrdeclImpl _tmpF7;struct Cyc_List_List*_tmpF8;struct Cyc_List_List*
_tmpF9;struct Cyc_List_List*_tmpFA;struct Cyc_Absyn_Aggrdecl***_tmpFB;struct Cyc_Absyn_AggrdeclImpl*
_tmpFC;struct Cyc_Absyn_AggrdeclImpl _tmpFD;struct Cyc_List_List*_tmpFE;struct Cyc_List_List*
_tmpFF;struct Cyc_List_List*_tmp100;struct Cyc_Absyn_Aggrdecl***_tmp101;struct Cyc_Absyn_Aggrdecl**
_tmp102;struct Cyc_Absyn_AggrdeclImpl*_tmp103;struct Cyc_Absyn_Aggrdecl***_tmp104;
struct Cyc_Absyn_Aggrdecl**_tmp105;_LLAF: _tmpF4=_tmpF3.f1;if(_tmpF4 != 0)goto _LLB1;
_tmpF5=_tmpF3.f2;if(_tmpF5 != 0)goto _LLB1;_LLB0: ge->aggrdecls=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct Cyc_Absyn_Aggrdecl**v))Cyc_Dict_insert)(
ge->aggrdecls,_tmpD5,({struct Cyc_Absyn_Aggrdecl**_tmp106=_cycalloc(sizeof(*
_tmp106));_tmp106[0]=ad;_tmp106;}));goto _LLAE;_LLB1: _tmpF6=_tmpF3.f1;if(_tmpF6 == 
0)goto _LLB3;_tmpF7=*_tmpF6;_tmpF8=_tmpF7.exist_vars;_tmpF9=_tmpF7.rgn_po;_tmpFA=
_tmpF7.fields;_tmpFB=_tmpF3.f2;if(_tmpFB != 0)goto _LLB3;_LLB2: {struct Cyc_Absyn_Aggrdecl**
_tmp107=({struct Cyc_Absyn_Aggrdecl**_tmp10D=_cycalloc(sizeof(*_tmp10D));_tmp10D[
0]=({struct Cyc_Absyn_Aggrdecl*_tmp10E=_cycalloc(sizeof(*_tmp10E));_tmp10E->kind=(
void*)((void*)ad->kind);_tmp10E->sc=(void*)((void*)3);_tmp10E->name=ad->name;
_tmp10E->tvs=_tmpDB;_tmp10E->impl=0;_tmp10E->attributes=ad->attributes;_tmp10E;});
_tmp10D;});ge->aggrdecls=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct
_dynforward_ptr*k,struct Cyc_Absyn_Aggrdecl**v))Cyc_Dict_insert)(ge->aggrdecls,
_tmpD5,_tmp107);Cyc_Tcutil_check_unique_tvars(loc,_tmpF8);Cyc_Tcutil_add_tvar_identities(
_tmpF8);Cyc_Tc_tcAggrImpl(te,ge,loc,((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_append)(_tmpDB,_tmpF8),_tmpF9,_tmpFA);if((void*)
ad->kind == (void*)1){struct Cyc_List_List*f=_tmpFA;for(0;f != 0;f=f->tl){if(!Cyc_Tcutil_bits_only((
void*)((struct Cyc_Absyn_Aggrfield*)f->hd)->type))({struct Cyc_String_pa_struct
_tmp10C;_tmp10C.tag=0;_tmp10C.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)
Cyc_Absynpp_typ2string((void*)((struct Cyc_Absyn_Aggrfield*)f->hd)->type));{
struct Cyc_String_pa_struct _tmp10B;_tmp10B.tag=0;_tmp10B.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)*_tmpD5);{struct Cyc_String_pa_struct
_tmp10A;_tmp10A.tag=0;_tmp10A.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*((
struct Cyc_Absyn_Aggrfield*)f->hd)->name);{void*_tmp108[3]={& _tmp10A,& _tmp10B,&
_tmp10C};Cyc_Tcutil_warn(loc,({const char*_tmp109="member %s of union %s has type %s which is not `bits-only' so it can only be written and not read";
_tag_dynforward(_tmp109,sizeof(char),_get_zero_arr_size(_tmp109,98));}),
_tag_dynforward(_tmp108,sizeof(void*),3));}}}});}}*_tmp107=ad;goto _LLAE;}_LLB3:
_tmpFC=_tmpF3.f1;if(_tmpFC == 0)goto _LLB5;_tmpFD=*_tmpFC;_tmpFE=_tmpFD.exist_vars;
_tmpFF=_tmpFD.rgn_po;_tmp100=_tmpFD.fields;_tmp101=_tmpF3.f2;if(_tmp101 == 0)goto
_LLB5;_tmp102=*_tmp101;_LLB4: if((void*)ad->kind != (void*)(*_tmp102)->kind)({void*
_tmp10F[0]={};Cyc_Tcutil_terr(loc,({const char*_tmp110="cannot reuse struct names for unions and vice-versa";
_tag_dynforward(_tmp110,sizeof(char),_get_zero_arr_size(_tmp110,52));}),
_tag_dynforward(_tmp10F,sizeof(void*),0));});{struct Cyc_Absyn_Aggrdecl*_tmp111=*
_tmp102;*_tmp102=({struct Cyc_Absyn_Aggrdecl*_tmp112=_cycalloc(sizeof(*_tmp112));
_tmp112->kind=(void*)((void*)ad->kind);_tmp112->sc=(void*)((void*)3);_tmp112->name=
ad->name;_tmp112->tvs=_tmpDB;_tmp112->impl=0;_tmp112->attributes=ad->attributes;
_tmp112;});Cyc_Tcutil_check_unique_tvars(loc,_tmpFE);Cyc_Tcutil_add_tvar_identities(
_tmpFE);Cyc_Tc_tcAggrImpl(te,ge,loc,((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_append)(_tmpDB,_tmpFE),_tmpFF,_tmp100);*
_tmp102=_tmp111;_tmp105=_tmp102;goto _LLB6;}_LLB5: _tmp103=_tmpF3.f1;if(_tmp103 != 
0)goto _LLAE;_tmp104=_tmpF3.f2;if(_tmp104 == 0)goto _LLAE;_tmp105=*_tmp104;_LLB6: {
struct Cyc_Absyn_Aggrdecl*_tmp113=Cyc_Tcdecl_merge_aggrdecl(*_tmp105,ad,loc,Cyc_Tc_tc_msg);
if(_tmp113 == 0)return;else{*_tmp105=(struct Cyc_Absyn_Aggrdecl*)_tmp113;ad=(
struct Cyc_Absyn_Aggrdecl*)_tmp113;goto _LLAE;}}_LLAE:;}ge->ordinaries=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct _tuple7*v))Cyc_Dict_insert)(
ge->ordinaries,_tmpD5,(struct _tuple7*)({struct _tuple7*_tmp114=_cycalloc(sizeof(*
_tmp114));_tmp114->f1=(void*)({struct Cyc_Tcenv_AggrRes_struct*_tmp115=_cycalloc(
sizeof(*_tmp115));_tmp115[0]=({struct Cyc_Tcenv_AggrRes_struct _tmp116;_tmp116.tag=
1;_tmp116.f1=ad;_tmp116;});_tmp115;});_tmp114->f2=1;_tmp114;}));}}struct _tuple10{
struct Cyc_Absyn_Tqual f1;void*f2;};static struct Cyc_List_List*Cyc_Tc_tcTunionFields(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*loc,
struct _dynforward_ptr obj,int is_xtunion,struct _tuple0*name,struct Cyc_List_List*
fields,struct Cyc_List_List*tvs,struct Cyc_Absyn_Tuniondecl*tudres){{struct Cyc_List_List*
_tmp117=fields;for(0;_tmp117 != 0;_tmp117=_tmp117->tl){struct Cyc_Absyn_Tunionfield*
_tmp118=(struct Cyc_Absyn_Tunionfield*)_tmp117->hd;{struct Cyc_List_List*typs=
_tmp118->typs;for(0;typs != 0;typs=typs->tl){Cyc_Tcutil_check_type(_tmp118->loc,
te,tvs,(void*)1,0,(*((struct _tuple10*)typs->hd)).f2);((*((struct _tuple10*)typs->hd)).f1).real_const=
Cyc_Tcutil_extract_const_from_typedef(_tmp118->loc,((*((struct _tuple10*)typs->hd)).f1).print_const,(*((
struct _tuple10*)typs->hd)).f2);}}{union Cyc_Absyn_Nmspace_union _tmp119=(*_tmp118->name).f1;
struct Cyc_List_List*_tmp11A;_LLB8: if((_tmp119.Rel_n).tag != 1)goto _LLBA;_tmp11A=(
_tmp119.Rel_n).f1;if(_tmp11A != 0)goto _LLBA;_LLB9: if(is_xtunion)(*_tmp118->name).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp11B;(_tmp11B.Abs_n).tag=
2;(_tmp11B.Abs_n).f1=te->ns;_tmp11B;});else{(*_tmp118->name).f1=(*name).f1;}goto
_LLB7;_LLBA: if((_tmp119.Rel_n).tag != 1)goto _LLBC;_LLBB:({struct Cyc_String_pa_struct
_tmp11E;_tmp11E.tag=0;_tmp11E.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)
Cyc_Absynpp_qvar2string(_tmp118->name));{void*_tmp11C[1]={& _tmp11E};Cyc_Tcutil_terr(
_tmp118->loc,({const char*_tmp11D="qualified tunionfield declarations are not allowed (%s)";
_tag_dynforward(_tmp11D,sizeof(char),_get_zero_arr_size(_tmp11D,56));}),
_tag_dynforward(_tmp11C,sizeof(void*),1));}});goto _LLB7;_LLBC: if((_tmp119.Abs_n).tag
!= 2)goto _LLBE;_LLBD: goto _LLB7;_LLBE: if((_tmp119.Loc_n).tag != 0)goto _LLB7;_LLBF:({
void*_tmp11F[0]={};((int(*)(struct _dynforward_ptr fmt,struct _dynforward_ptr ap))
Cyc_Tcutil_impos)(({const char*_tmp120="tcTunionFields: Loc_n";_tag_dynforward(
_tmp120,sizeof(char),_get_zero_arr_size(_tmp120,22));}),_tag_dynforward(_tmp11F,
sizeof(void*),0));});_LLB7:;}}}{struct Cyc_List_List*fields2;if(is_xtunion){int
_tmp121=1;struct Cyc_List_List*_tmp122=Cyc_Tcdecl_sort_xtunion_fields(fields,&
_tmp121,(*name).f2,loc,Cyc_Tc_tc_msg);if(_tmp121)fields2=_tmp122;else{fields2=0;}}
else{struct _RegionHandle _tmp123=_new_region("uprev_rgn");struct _RegionHandle*
uprev_rgn=& _tmp123;_push_region(uprev_rgn);{struct Cyc_List_List*prev_fields=0;{
struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){struct Cyc_Absyn_Tunionfield*
_tmp124=(struct Cyc_Absyn_Tunionfield*)fs->hd;if(((int(*)(int(*compare)(struct
_dynforward_ptr*,struct _dynforward_ptr*),struct Cyc_List_List*l,struct
_dynforward_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,(*_tmp124->name).f2))({
struct Cyc_String_pa_struct _tmp128;_tmp128.tag=0;_tmp128.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)obj);{struct Cyc_String_pa_struct _tmp127;
_tmp127.tag=0;_tmp127.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*(*
_tmp124->name).f2);{void*_tmp125[2]={& _tmp127,& _tmp128};Cyc_Tcutil_terr(_tmp124->loc,({
const char*_tmp126="duplicate field name %s in %s";_tag_dynforward(_tmp126,
sizeof(char),_get_zero_arr_size(_tmp126,30));}),_tag_dynforward(_tmp125,sizeof(
void*),2));}}});else{prev_fields=({struct Cyc_List_List*_tmp129=_region_malloc(
uprev_rgn,sizeof(*_tmp129));_tmp129->hd=(*_tmp124->name).f2;_tmp129->tl=
prev_fields;_tmp129;});}if((void*)_tmp124->sc != (void*)2){({struct Cyc_String_pa_struct
_tmp12C;_tmp12C.tag=0;_tmp12C.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*(*
_tmp124->name).f2);{void*_tmp12A[1]={& _tmp12C};Cyc_Tcutil_warn(loc,({const char*
_tmp12B="ignoring scope of field %s";_tag_dynforward(_tmp12B,sizeof(char),
_get_zero_arr_size(_tmp12B,27));}),_tag_dynforward(_tmp12A,sizeof(void*),1));}});(
void*)(_tmp124->sc=(void*)((void*)2));}}}fields2=fields;};_pop_region(uprev_rgn);}{
struct Cyc_List_List*_tmp12D=fields;for(0;_tmp12D != 0;_tmp12D=_tmp12D->tl){struct
Cyc_Absyn_Tunionfield*_tmp12E=(struct Cyc_Absyn_Tunionfield*)_tmp12D->hd;ge->ordinaries=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct
_tuple7*v))Cyc_Dict_insert)(ge->ordinaries,(*_tmp12E->name).f2,(struct _tuple7*)({
struct _tuple7*_tmp12F=_cycalloc(sizeof(*_tmp12F));_tmp12F->f1=(void*)({struct Cyc_Tcenv_TunionRes_struct*
_tmp130=_cycalloc(sizeof(*_tmp130));_tmp130[0]=({struct Cyc_Tcenv_TunionRes_struct
_tmp131;_tmp131.tag=2;_tmp131.f1=tudres;_tmp131.f2=_tmp12E;_tmp131;});_tmp130;});
_tmp12F->f2=1;_tmp12F;}));}}return fields2;}}struct _tuple11{struct Cyc_Core_Opt*f1;
struct Cyc_Absyn_Tuniondecl***f2;};void Cyc_Tc_tcTuniondecl(struct Cyc_Tcenv_Tenv*
te,struct Cyc_Tcenv_Genv*ge,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Tuniondecl*
tud){struct _dynforward_ptr*v=(*tud->name).f2;struct _dynforward_ptr obj=tud->is_xtunion?({
const char*_tmp16D="xtunion";_tag_dynforward(_tmp16D,sizeof(char),
_get_zero_arr_size(_tmp16D,8));}):({const char*_tmp16E="tunion";_tag_dynforward(
_tmp16E,sizeof(char),_get_zero_arr_size(_tmp16E,7));});struct Cyc_List_List*tvs=
tud->tvs;{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){void*
_tmp132=Cyc_Absyn_compress_kb((void*)((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);
struct Cyc_Core_Opt*_tmp133;struct Cyc_Core_Opt**_tmp134;struct Cyc_Core_Opt*
_tmp135;struct Cyc_Core_Opt**_tmp136;void*_tmp137;struct Cyc_Core_Opt*_tmp138;
struct Cyc_Core_Opt**_tmp139;void*_tmp13A;void*_tmp13B;_LLC1: if(*((int*)_tmp132)
!= 1)goto _LLC3;_tmp133=((struct Cyc_Absyn_Unknown_kb_struct*)_tmp132)->f1;_tmp134=(
struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_struct*)_tmp132)->f1;_LLC2:
_tmp136=_tmp134;goto _LLC4;_LLC3: if(*((int*)_tmp132)!= 2)goto _LLC5;_tmp135=((
struct Cyc_Absyn_Less_kb_struct*)_tmp132)->f1;_tmp136=(struct Cyc_Core_Opt**)&((
struct Cyc_Absyn_Less_kb_struct*)_tmp132)->f1;_tmp137=(void*)((struct Cyc_Absyn_Less_kb_struct*)
_tmp132)->f2;if((int)_tmp137 != 1)goto _LLC5;_LLC4: _tmp139=_tmp136;goto _LLC6;_LLC5:
if(*((int*)_tmp132)!= 2)goto _LLC7;_tmp138=((struct Cyc_Absyn_Less_kb_struct*)
_tmp132)->f1;_tmp139=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_struct*)
_tmp132)->f1;_tmp13A=(void*)((struct Cyc_Absyn_Less_kb_struct*)_tmp132)->f2;if((
int)_tmp13A != 0)goto _LLC7;_LLC6:*_tmp139=({struct Cyc_Core_Opt*_tmp13C=_cycalloc(
sizeof(*_tmp13C));_tmp13C->v=(void*)Cyc_Tcutil_kind_to_bound((void*)2);_tmp13C;});
goto _LLC0;_LLC7: if(*((int*)_tmp132)!= 0)goto _LLC9;_tmp13B=(void*)((struct Cyc_Absyn_Eq_kb_struct*)
_tmp132)->f1;if((int)_tmp13B != 1)goto _LLC9;_LLC8:({struct Cyc_String_pa_struct
_tmp141;_tmp141.tag=0;_tmp141.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*((
struct Cyc_Absyn_Tvar*)tvs2->hd)->name);{struct Cyc_String_pa_struct _tmp140;
_tmp140.tag=0;_tmp140.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*v);{
struct Cyc_String_pa_struct _tmp13F;_tmp13F.tag=0;_tmp13F.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)obj);{void*_tmp13D[3]={& _tmp13F,& _tmp140,&
_tmp141};Cyc_Tcutil_terr(loc,({const char*_tmp13E="%s %s attempts to abstract type variable %s of kind M";
_tag_dynforward(_tmp13E,sizeof(char),_get_zero_arr_size(_tmp13E,54));}),
_tag_dynforward(_tmp13D,sizeof(void*),3));}}}});goto _LLC0;_LLC9:;_LLCA: goto _LLC0;
_LLC0:;}}Cyc_Tcutil_check_unique_tvars(loc,tvs);Cyc_Tcutil_add_tvar_identities(
tvs);{struct _RegionHandle _tmp142=_new_region("temp");struct _RegionHandle*temp=&
_tmp142;_push_region(temp);{struct Cyc_Absyn_Tuniondecl***tud_opt;if(tud->is_xtunion){{
struct _handler_cons _tmp143;_push_handler(& _tmp143);{int _tmp145=0;if(setjmp(
_tmp143.handler))_tmp145=1;if(!_tmp145){tud_opt=Cyc_Tcenv_lookup_xtuniondecl(
temp,te,loc,tud->name);;_pop_handler();}else{void*_tmp144=(void*)_exn_thrown;
void*_tmp147=_tmp144;_LLCC: if(_tmp147 != Cyc_Dict_Absent)goto _LLCE;_LLCD:({struct
Cyc_String_pa_struct _tmp14A;_tmp14A.tag=0;_tmp14A.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)Cyc_Absynpp_qvar2string(tud->name));{void*_tmp148[1]={&
_tmp14A};Cyc_Tcutil_terr(loc,({const char*_tmp149="qualified xtunion declaration %s is not an existing xtunion";
_tag_dynforward(_tmp149,sizeof(char),_get_zero_arr_size(_tmp149,60));}),
_tag_dynforward(_tmp148,sizeof(void*),1));}});_npop_handler(0);return;_LLCE:;
_LLCF:(void)_throw(_tmp147);_LLCB:;}}}if(tud_opt != 0)tud->name=(*(*tud_opt))->name;
else{(*tud->name).f1=(union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union
_tmp14B;(_tmp14B.Abs_n).tag=2;(_tmp14B.Abs_n).f1=te->ns;_tmp14B;});}}else{{union
Cyc_Absyn_Nmspace_union _tmp14C=(*tud->name).f1;struct Cyc_List_List*_tmp14D;_LLD1:
if((_tmp14C.Rel_n).tag != 1)goto _LLD3;_tmp14D=(_tmp14C.Rel_n).f1;if(_tmp14D != 0)
goto _LLD3;_LLD2:(*tud->name).f1=(union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union
_tmp14E;(_tmp14E.Abs_n).tag=2;(_tmp14E.Abs_n).f1=te->ns;_tmp14E;});goto _LLD0;
_LLD3: if((_tmp14C.Abs_n).tag != 2)goto _LLD5;_LLD4: goto _LLD6;_LLD5:;_LLD6:({struct
Cyc_String_pa_struct _tmp151;_tmp151.tag=0;_tmp151.f1=(struct _dynforward_ptr)((
struct _dynforward_ptr)Cyc_Absynpp_qvar2string(tud->name));{void*_tmp14F[1]={&
_tmp151};Cyc_Tcutil_terr(loc,({const char*_tmp150="qualified tunion declarations are not implemented (%s)";
_tag_dynforward(_tmp150,sizeof(char),_get_zero_arr_size(_tmp150,55));}),
_tag_dynforward(_tmp14F,sizeof(void*),1));}});_npop_handler(0);return;_LLD0:;}{
struct Cyc_Absyn_Tuniondecl***_tmp152=((struct Cyc_Absyn_Tuniondecl***(*)(struct
Cyc_Dict_Dict d,struct _dynforward_ptr*k))Cyc_Dict_lookup_opt)(ge->tuniondecls,v);
tud_opt=(unsigned int)_tmp152?({struct Cyc_Absyn_Tuniondecl***_tmp153=
_region_malloc(temp,sizeof(*_tmp153));_tmp153[0]=*_tmp152;_tmp153;}): 0;}}{struct
_tuple11 _tmp155=({struct _tuple11 _tmp154;_tmp154.f1=tud->fields;_tmp154.f2=
tud_opt;_tmp154;});struct Cyc_Core_Opt*_tmp156;struct Cyc_Absyn_Tuniondecl***
_tmp157;struct Cyc_Core_Opt*_tmp158;struct Cyc_Core_Opt _tmp159;struct Cyc_List_List*
_tmp15A;struct Cyc_List_List**_tmp15B;struct Cyc_Absyn_Tuniondecl***_tmp15C;struct
Cyc_Core_Opt*_tmp15D;struct Cyc_Core_Opt _tmp15E;struct Cyc_List_List*_tmp15F;
struct Cyc_List_List**_tmp160;struct Cyc_Absyn_Tuniondecl***_tmp161;struct Cyc_Absyn_Tuniondecl**
_tmp162;struct Cyc_Core_Opt*_tmp163;struct Cyc_Absyn_Tuniondecl***_tmp164;struct
Cyc_Absyn_Tuniondecl**_tmp165;_LLD8: _tmp156=_tmp155.f1;if(_tmp156 != 0)goto _LLDA;
_tmp157=_tmp155.f2;if(_tmp157 != 0)goto _LLDA;_LLD9: ge->tuniondecls=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct Cyc_Absyn_Tuniondecl**v))Cyc_Dict_insert)(
ge->tuniondecls,v,({struct Cyc_Absyn_Tuniondecl**_tmp166=_cycalloc(sizeof(*
_tmp166));_tmp166[0]=tud;_tmp166;}));goto _LLD7;_LLDA: _tmp158=_tmp155.f1;if(
_tmp158 == 0)goto _LLDC;_tmp159=*_tmp158;_tmp15A=(struct Cyc_List_List*)_tmp159.v;
_tmp15B=(struct Cyc_List_List**)&(*_tmp155.f1).v;_tmp15C=_tmp155.f2;if(_tmp15C != 
0)goto _LLDC;_LLDB: {struct Cyc_Absyn_Tuniondecl**_tmp167=({struct Cyc_Absyn_Tuniondecl**
_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168[0]=({struct Cyc_Absyn_Tuniondecl*
_tmp169=_cycalloc(sizeof(*_tmp169));_tmp169->sc=(void*)((void*)3);_tmp169->name=
tud->name;_tmp169->tvs=tvs;_tmp169->fields=0;_tmp169->is_xtunion=tud->is_xtunion;
_tmp169->is_flat=tud->is_flat;_tmp169;});_tmp168;});ge->tuniondecls=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct Cyc_Absyn_Tuniondecl**v))Cyc_Dict_insert)(
ge->tuniondecls,v,_tmp167);*_tmp15B=Cyc_Tc_tcTunionFields(te,ge,loc,obj,tud->is_xtunion,
tud->name,*_tmp15B,tvs,tud);*_tmp167=tud;goto _LLD7;}_LLDC: _tmp15D=_tmp155.f1;if(
_tmp15D == 0)goto _LLDE;_tmp15E=*_tmp15D;_tmp15F=(struct Cyc_List_List*)_tmp15E.v;
_tmp160=(struct Cyc_List_List**)&(*_tmp155.f1).v;_tmp161=_tmp155.f2;if(_tmp161 == 
0)goto _LLDE;_tmp162=*_tmp161;_LLDD: {struct Cyc_Absyn_Tuniondecl*_tmp16A=*_tmp162;*
_tmp162=({struct Cyc_Absyn_Tuniondecl*_tmp16B=_cycalloc(sizeof(*_tmp16B));_tmp16B->sc=(
void*)((void*)3);_tmp16B->name=tud->name;_tmp16B->tvs=tvs;_tmp16B->fields=0;
_tmp16B->is_xtunion=tud->is_xtunion;_tmp16B->is_flat=tud->is_flat;_tmp16B;});*
_tmp160=Cyc_Tc_tcTunionFields(te,ge,loc,obj,tud->is_xtunion,tud->name,*_tmp160,
tvs,tud);*_tmp162=_tmp16A;_tmp165=_tmp162;goto _LLDF;}_LLDE: _tmp163=_tmp155.f1;
if(_tmp163 != 0)goto _LLD7;_tmp164=_tmp155.f2;if(_tmp164 == 0)goto _LLD7;_tmp165=*
_tmp164;_LLDF: {struct Cyc_Absyn_Tuniondecl*_tmp16C=Cyc_Tcdecl_merge_tuniondecl(*
_tmp165,tud,loc,Cyc_Tc_tc_msg);if(_tmp16C == 0){_npop_handler(0);return;}else{*
_tmp165=(struct Cyc_Absyn_Tuniondecl*)_tmp16C;goto _LLD7;}}_LLD7:;}};_pop_region(
temp);}}void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,
struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Enumdecl*ed){struct
_dynforward_ptr*v=(*ed->name).f2;{union Cyc_Absyn_Nmspace_union _tmp16F=(*ed->name).f1;
struct Cyc_List_List*_tmp170;struct Cyc_List_List*_tmp171;_LLE1: if((_tmp16F.Rel_n).tag
!= 1)goto _LLE3;_tmp170=(_tmp16F.Rel_n).f1;if(_tmp170 != 0)goto _LLE3;_LLE2: goto
_LLE4;_LLE3: if((_tmp16F.Abs_n).tag != 2)goto _LLE5;_tmp171=(_tmp16F.Abs_n).f1;if(
_tmp171 != 0)goto _LLE5;_LLE4: goto _LLE0;_LLE5:;_LLE6:({struct Cyc_String_pa_struct
_tmp174;_tmp174.tag=0;_tmp174.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)
Cyc_Absynpp_qvar2string(ed->name));{void*_tmp172[1]={& _tmp174};Cyc_Tcutil_terr(
loc,({const char*_tmp173="qualified enum declarations are not implemented (%s)";
_tag_dynforward(_tmp173,sizeof(char),_get_zero_arr_size(_tmp173,53));}),
_tag_dynforward(_tmp172,sizeof(void*),1));}});return;_LLE0:;}(*ed->name).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp175;(_tmp175.Abs_n).tag=
2;(_tmp175.Abs_n).f1=te->ns;_tmp175;});if(ed->fields != 0){struct _RegionHandle
_tmp176=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp176;
_push_region(uprev_rgn);{struct Cyc_List_List*prev_fields=0;unsigned int tag_count=
0;struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(
ed->fields))->v;for(0;fs != 0;fs=fs->tl){struct Cyc_Absyn_Enumfield*_tmp177=(
struct Cyc_Absyn_Enumfield*)fs->hd;if(((int(*)(int(*compare)(struct
_dynforward_ptr*,struct _dynforward_ptr*),struct Cyc_List_List*l,struct
_dynforward_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,(*_tmp177->name).f2))({
struct Cyc_String_pa_struct _tmp17A;_tmp17A.tag=0;_tmp17A.f1=(struct
_dynforward_ptr)((struct _dynforward_ptr)*(*_tmp177->name).f2);{void*_tmp178[1]={&
_tmp17A};Cyc_Tcutil_terr(_tmp177->loc,({const char*_tmp179="duplicate field name %s";
_tag_dynforward(_tmp179,sizeof(char),_get_zero_arr_size(_tmp179,24));}),
_tag_dynforward(_tmp178,sizeof(void*),1));}});else{prev_fields=({struct Cyc_List_List*
_tmp17B=_region_malloc(uprev_rgn,sizeof(*_tmp17B));_tmp17B->hd=(*_tmp177->name).f2;
_tmp17B->tl=prev_fields;_tmp17B;});}if(_tmp177->tag == 0)_tmp177->tag=(struct Cyc_Absyn_Exp*)
Cyc_Absyn_uint_exp(tag_count,_tmp177->loc);else{if(!Cyc_Tcutil_is_const_exp(te,(
struct Cyc_Absyn_Exp*)_check_null(_tmp177->tag)))({struct Cyc_String_pa_struct
_tmp17F;_tmp17F.tag=0;_tmp17F.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)*(*
_tmp177->name).f2);{struct Cyc_String_pa_struct _tmp17E;_tmp17E.tag=0;_tmp17E.f1=(
struct _dynforward_ptr)((struct _dynforward_ptr)*v);{void*_tmp17C[2]={& _tmp17E,&
_tmp17F};Cyc_Tcutil_terr(loc,({const char*_tmp17D="enum %s, field %s: expression is not constant";
_tag_dynforward(_tmp17D,sizeof(char),_get_zero_arr_size(_tmp17D,46));}),
_tag_dynforward(_tmp17C,sizeof(void*),2));}}});}{unsigned int _tmp181;int _tmp182;
struct _tuple4 _tmp180=Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)
_check_null(_tmp177->tag));_tmp181=_tmp180.f1;_tmp182=_tmp180.f2;if(!_tmp182)({
void*_tmp183[0]={};Cyc_Tcutil_terr(loc,({const char*_tmp184="Cyclone enum tags cannot use sizeof or offsetof";
_tag_dynforward(_tmp184,sizeof(char),_get_zero_arr_size(_tmp184,48));}),
_tag_dynforward(_tmp183,sizeof(void*),0));});tag_count=_tmp181 + 1;(*_tmp177->name).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp185;(_tmp185.Abs_n).tag=
2;(_tmp185.Abs_n).f1=te->ns;_tmp185;});}}};_pop_region(uprev_rgn);}{struct
_handler_cons _tmp186;_push_handler(& _tmp186);{int _tmp188=0;if(setjmp(_tmp186.handler))
_tmp188=1;if(!_tmp188){{struct Cyc_Absyn_Enumdecl**_tmp189=((struct Cyc_Absyn_Enumdecl**(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k))Cyc_Dict_lookup)(ge->enumdecls,v);
struct Cyc_Absyn_Enumdecl*_tmp18A=Cyc_Tcdecl_merge_enumdecl(*_tmp189,ed,loc,Cyc_Tc_tc_msg);
if(_tmp18A == 0){_npop_handler(0);return;}*_tmp189=(struct Cyc_Absyn_Enumdecl*)
_tmp18A;};_pop_handler();}else{void*_tmp187=(void*)_exn_thrown;void*_tmp18C=
_tmp187;_LLE8: if(_tmp18C != Cyc_Dict_Absent)goto _LLEA;_LLE9: {struct Cyc_Absyn_Enumdecl**
_tmp18D=({struct Cyc_Absyn_Enumdecl**_tmp18E=_cycalloc(sizeof(*_tmp18E));_tmp18E[
0]=ed;_tmp18E;});ge->enumdecls=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,
struct _dynforward_ptr*k,struct Cyc_Absyn_Enumdecl**v))Cyc_Dict_insert)(ge->enumdecls,
v,_tmp18D);goto _LLE7;}_LLEA:;_LLEB:(void)_throw(_tmp18C);_LLE7:;}}}if(ed->fields
!= 0){struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)
_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){struct Cyc_Absyn_Enumfield*
_tmp18F=(struct Cyc_Absyn_Enumfield*)fs->hd;ge->ordinaries=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dynforward_ptr*k,struct _tuple7*v))Cyc_Dict_insert)(
ge->ordinaries,(*_tmp18F->name).f2,(struct _tuple7*)({struct _tuple7*_tmp190=
_cycalloc(sizeof(*_tmp190));_tmp190->f1=(void*)({struct Cyc_Tcenv_EnumRes_struct*
_tmp191=_cycalloc(sizeof(*_tmp191));_tmp191[0]=({struct Cyc_Tcenv_EnumRes_struct
_tmp192;_tmp192.tag=3;_tmp192.f1=ed;_tmp192.f2=_tmp18F;_tmp192;});_tmp191;});
_tmp190->f2=1;_tmp190;}));}}}static int Cyc_Tc_okay_externC(struct Cyc_Position_Segment*
loc,void*sc){void*_tmp193=sc;_LLED: if((int)_tmp193 != 0)goto _LLEF;_LLEE:({void*
_tmp194[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp195="static declaration nested within extern \"C\"";
_tag_dynforward(_tmp195,sizeof(char),_get_zero_arr_size(_tmp195,44));}),
_tag_dynforward(_tmp194,sizeof(void*),0));});return 0;_LLEF: if((int)_tmp193 != 1)
goto _LLF1;_LLF0:({void*_tmp196[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp197="abstract declaration nested within extern \"C\"";
_tag_dynforward(_tmp197,sizeof(char),_get_zero_arr_size(_tmp197,46));}),
_tag_dynforward(_tmp196,sizeof(void*),0));});return 0;_LLF1: if((int)_tmp193 != 2)
goto _LLF3;_LLF2: goto _LLF4;_LLF3: if((int)_tmp193 != 5)goto _LLF5;_LLF4: goto _LLF6;
_LLF5: if((int)_tmp193 != 3)goto _LLF7;_LLF6: return 1;_LLF7: if((int)_tmp193 != 4)goto
_LLEC;_LLF8:({void*_tmp198[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp199="nested extern \"C\" declaration";
_tag_dynforward(_tmp199,sizeof(char),_get_zero_arr_size(_tmp199,30));}),
_tag_dynforward(_tmp198,sizeof(void*),0));});return 1;_LLEC:;}static void Cyc_Tc_resolve_export_namespace(
struct Cyc_Tcenv_Tenv*te,struct _tuple6*exp){struct Cyc_Position_Segment*_tmp19B;
struct _tuple0*_tmp19C;struct _tuple6 _tmp19A=*exp;_tmp19B=_tmp19A.f1;_tmp19C=
_tmp19A.f2;{struct _tuple0 _tmp19E;union Cyc_Absyn_Nmspace_union _tmp19F;struct
_dynforward_ptr*_tmp1A0;struct _tuple0*_tmp19D=_tmp19C;_tmp19E=*_tmp19D;_tmp19F=
_tmp19E.f1;_tmp1A0=_tmp19E.f2;{union Cyc_Absyn_Nmspace_union _tmp1A1=_tmp19F;
struct Cyc_List_List*_tmp1A2;struct Cyc_List_List*_tmp1A3;_LLFA: if((_tmp1A1.Rel_n).tag
!= 1)goto _LLFC;_tmp1A2=(_tmp1A1.Rel_n).f1;if(_tmp1A2 != 0)goto _LLFC;_LLFB: goto
_LLFD;_LLFC: if((_tmp1A1.Abs_n).tag != 2)goto _LLFE;_tmp1A3=(_tmp1A1.Abs_n).f1;if(
_tmp1A3 != 0)goto _LLFE;_LLFD: goto _LLF9;_LLFE:;_LLFF:({struct Cyc_String_pa_struct
_tmp1A6;_tmp1A6.tag=0;_tmp1A6.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)
Cyc_Absynpp_qvar2string(_tmp19C));{void*_tmp1A4[1]={& _tmp1A6};Cyc_Tcutil_terr(
_tmp19B,({const char*_tmp1A5="qualified export variables are not implemented (%s)";
_tag_dynforward(_tmp1A5,sizeof(char),_get_zero_arr_size(_tmp1A5,52));}),
_tag_dynforward(_tmp1A4,sizeof(void*),1));}});return;_LLF9:;}(*_tmp19C).f1=(
union Cyc_Absyn_Nmspace_union)({union Cyc_Absyn_Nmspace_union _tmp1A7;(_tmp1A7.Abs_n).tag=
2;(_tmp1A7.Abs_n).f1=te->ns;_tmp1A7;});}}static void Cyc_Tc_tc_decls(struct Cyc_Tcenv_Tenv*
te,struct Cyc_List_List*ds0,int in_externC,int in_externCinclude,int check_var_init,
struct _RegionHandle*grgn,struct Cyc_List_List**exports){struct Cyc_Tcenv_Genv*ge=((
struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,te->ns);struct Cyc_List_List*last=0;struct Cyc_Dict_Dict dict=Cyc_Tcgenrep_empty_typerep_dict();
struct Cyc_List_List*_tmp1A8=ds0;for(0;_tmp1A8 != 0;(last=_tmp1A8,_tmp1A8=_tmp1A8->tl)){
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)_tmp1A8->hd;struct Cyc_Position_Segment*
loc=d->loc;void*_tmp1A9=(void*)d->r;struct Cyc_Absyn_Vardecl*_tmp1AA;struct Cyc_Absyn_Fndecl*
_tmp1AB;struct Cyc_Absyn_Typedefdecl*_tmp1AC;struct Cyc_Absyn_Aggrdecl*_tmp1AD;
struct Cyc_Absyn_Tuniondecl*_tmp1AE;struct Cyc_Absyn_Enumdecl*_tmp1AF;struct
_dynforward_ptr*_tmp1B0;struct Cyc_List_List*_tmp1B1;struct _tuple0*_tmp1B2;struct
_tuple0 _tmp1B3;union Cyc_Absyn_Nmspace_union _tmp1B4;struct _dynforward_ptr*_tmp1B5;
struct Cyc_List_List*_tmp1B6;struct Cyc_List_List*_tmp1B7;struct Cyc_List_List*
_tmp1B8;struct Cyc_List_List*_tmp1B9;_LL101: if(_tmp1A9 <= (void*)2)goto _LL111;if(*((
int*)_tmp1A9)!= 2)goto _LL103;_LL102: goto _LL104;_LL103: if(*((int*)_tmp1A9)!= 3)
goto _LL105;_LL104:({void*_tmp1BA[0]={};Cyc_Tcutil_terr(loc,({const char*_tmp1BB="top level let-declarations are not implemented";
_tag_dynforward(_tmp1BB,sizeof(char),_get_zero_arr_size(_tmp1BB,47));}),
_tag_dynforward(_tmp1BA,sizeof(void*),0));});goto _LL100;_LL105: if(*((int*)
_tmp1A9)!= 0)goto _LL107;_tmp1AA=((struct Cyc_Absyn_Var_d_struct*)_tmp1A9)->f1;
_LL106: if(in_externC  && Cyc_Tc_okay_externC(d->loc,(void*)_tmp1AA->sc))(void*)(
_tmp1AA->sc=(void*)((void*)4));if(_tmp1AA->initializer != 0){void*_tmp1BC=(void*)((
struct Cyc_Absyn_Exp*)_check_null(_tmp1AA->initializer))->r;void*_tmp1BD;_LL11E:
if(*((int*)_tmp1BC)!= 21)goto _LL120;_tmp1BD=(void*)((struct Cyc_Absyn_Gentyp_e_struct*)
_tmp1BC)->f2;_LL11F: {struct Cyc_Dict_Dict _tmp1BF;struct Cyc_List_List*_tmp1C0;
struct Cyc_Absyn_Exp*_tmp1C1;struct _tuple5 _tmp1BE=Cyc_Tcgenrep_tcGenrep(te,ge,loc,
_tmp1BD,dict);_tmp1BF=_tmp1BE.f1;_tmp1C0=_tmp1BE.f2;_tmp1C1=_tmp1BE.f3;dict=
_tmp1BF;Cyc_Tc_tc_decls(te,_tmp1C0,in_externC,in_externCinclude,check_var_init,
grgn,exports);_tmp1AA->initializer=(struct Cyc_Absyn_Exp*)_tmp1C1;Cyc_Tc_tcVardecl(
te,ge,loc,_tmp1AA,check_var_init,in_externCinclude,exports);if(_tmp1C0 != 0){if(
last != 0){((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))
Cyc_List_imp_append)(_tmp1C0,_tmp1A8);last->tl=_tmp1C0;}else{struct Cyc_List_List
tmp=({struct Cyc_List_List _tmp1C2;_tmp1C2.hd=(struct Cyc_Absyn_Decl*)_tmp1A8->hd;
_tmp1C2.tl=_tmp1A8->tl;_tmp1C2;});(struct Cyc_Absyn_Decl*)(_tmp1A8->hd=(void*)((
struct Cyc_Absyn_Decl*)_tmp1C0->hd));_tmp1A8->tl=_tmp1C0->tl;(struct Cyc_Absyn_Decl*)(
_tmp1C0->hd=(void*)((struct Cyc_Absyn_Decl*)tmp.hd));_tmp1C0->tl=tmp.tl;((struct
Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(
_tmp1A8,_tmp1C0);}}continue;}_LL120:;_LL121: goto _LL11D;_LL11D:;}Cyc_Tc_tcVardecl(
te,ge,loc,_tmp1AA,check_var_init,in_externCinclude,exports);goto _LL100;_LL107:
if(*((int*)_tmp1A9)!= 1)goto _LL109;_tmp1AB=((struct Cyc_Absyn_Fn_d_struct*)
_tmp1A9)->f1;_LL108: if(in_externC  && Cyc_Tc_okay_externC(d->loc,(void*)_tmp1AB->sc))(
void*)(_tmp1AB->sc=(void*)((void*)4));Cyc_Tc_tcFndecl(te,ge,loc,_tmp1AB,
in_externCinclude,exports);goto _LL100;_LL109: if(*((int*)_tmp1A9)!= 7)goto _LL10B;
_tmp1AC=((struct Cyc_Absyn_Typedef_d_struct*)_tmp1A9)->f1;_LL10A: Cyc_Tc_tcTypedefdecl(
te,ge,loc,_tmp1AC);goto _LL100;_LL10B: if(*((int*)_tmp1A9)!= 4)goto _LL10D;_tmp1AD=((
struct Cyc_Absyn_Aggr_d_struct*)_tmp1A9)->f1;_LL10C: if(in_externC  && Cyc_Tc_okay_externC(
d->loc,(void*)_tmp1AD->sc))(void*)(_tmp1AD->sc=(void*)((void*)4));Cyc_Tc_tcAggrdecl(
te,ge,loc,_tmp1AD);goto _LL100;_LL10D: if(*((int*)_tmp1A9)!= 5)goto _LL10F;_tmp1AE=((
struct Cyc_Absyn_Tunion_d_struct*)_tmp1A9)->f1;_LL10E: if(in_externC  && Cyc_Tc_okay_externC(
d->loc,(void*)_tmp1AE->sc))(void*)(_tmp1AE->sc=(void*)((void*)4));Cyc_Tc_tcTuniondecl(
te,ge,loc,_tmp1AE);goto _LL100;_LL10F: if(*((int*)_tmp1A9)!= 6)goto _LL111;_tmp1AF=((
struct Cyc_Absyn_Enum_d_struct*)_tmp1A9)->f1;_LL110: if(in_externC  && Cyc_Tc_okay_externC(
d->loc,(void*)_tmp1AF->sc))(void*)(_tmp1AF->sc=(void*)((void*)4));Cyc_Tc_tcEnumdecl(
te,ge,loc,_tmp1AF);goto _LL100;_LL111: if((int)_tmp1A9 != 0)goto _LL113;_LL112:({
void*_tmp1C3[0]={};Cyc_Tcutil_warn(d->loc,({const char*_tmp1C4="spurious __cyclone_port_on__";
_tag_dynforward(_tmp1C4,sizeof(char),_get_zero_arr_size(_tmp1C4,29));}),
_tag_dynforward(_tmp1C3,sizeof(void*),0));});goto _LL100;_LL113: if((int)_tmp1A9 != 
1)goto _LL115;_LL114: goto _LL100;_LL115: if(_tmp1A9 <= (void*)2)goto _LL117;if(*((int*)
_tmp1A9)!= 8)goto _LL117;_tmp1B0=((struct Cyc_Absyn_Namespace_d_struct*)_tmp1A9)->f1;
_tmp1B1=((struct Cyc_Absyn_Namespace_d_struct*)_tmp1A9)->f2;_LL116: {struct Cyc_List_List*
_tmp1C5=te->ns;struct Cyc_List_List*_tmp1C6=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_append)(_tmp1C5,({struct Cyc_List_List*_tmp1C7=
_cycalloc(sizeof(*_tmp1C7));_tmp1C7->hd=_tmp1B0;_tmp1C7->tl=0;_tmp1C7;}));if(!((
int(*)(struct Cyc_Set_Set*s,struct _dynforward_ptr*elt))Cyc_Set_member)(ge->namespaces,
_tmp1B0)){ge->namespaces=((struct Cyc_Set_Set*(*)(struct _RegionHandle*r,struct Cyc_Set_Set*
s,struct _dynforward_ptr*elt))Cyc_Set_rinsert)(grgn,ge->namespaces,_tmp1B0);te->ae=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k,struct Cyc_Tcenv_Genv*
v))Cyc_Dict_insert)(te->ae,_tmp1C6,Cyc_Tcenv_empty_genv(grgn));}te->ns=_tmp1C6;
Cyc_Tc_tc_decls(te,_tmp1B1,in_externC,in_externCinclude,check_var_init,grgn,
exports);te->ns=_tmp1C5;goto _LL100;}_LL117: if(_tmp1A9 <= (void*)2)goto _LL119;if(*((
int*)_tmp1A9)!= 9)goto _LL119;_tmp1B2=((struct Cyc_Absyn_Using_d_struct*)_tmp1A9)->f1;
_tmp1B3=*_tmp1B2;_tmp1B4=_tmp1B3.f1;_tmp1B5=_tmp1B3.f2;_tmp1B6=((struct Cyc_Absyn_Using_d_struct*)
_tmp1A9)->f2;_LL118: {struct _dynforward_ptr*first;struct Cyc_List_List*rest;{
union Cyc_Absyn_Nmspace_union _tmp1C8=_tmp1B4;struct Cyc_List_List*_tmp1C9;struct
Cyc_List_List*_tmp1CA;struct Cyc_List_List*_tmp1CB;struct Cyc_List_List _tmp1CC;
struct _dynforward_ptr*_tmp1CD;struct Cyc_List_List*_tmp1CE;struct Cyc_List_List*
_tmp1CF;struct Cyc_List_List _tmp1D0;struct _dynforward_ptr*_tmp1D1;struct Cyc_List_List*
_tmp1D2;_LL123: if((_tmp1C8.Loc_n).tag != 0)goto _LL125;_LL124: goto _LL126;_LL125:
if((_tmp1C8.Rel_n).tag != 1)goto _LL127;_tmp1C9=(_tmp1C8.Rel_n).f1;if(_tmp1C9 != 0)
goto _LL127;_LL126: goto _LL128;_LL127: if((_tmp1C8.Abs_n).tag != 2)goto _LL129;
_tmp1CA=(_tmp1C8.Abs_n).f1;if(_tmp1CA != 0)goto _LL129;_LL128: first=_tmp1B5;rest=0;
goto _LL122;_LL129: if((_tmp1C8.Rel_n).tag != 1)goto _LL12B;_tmp1CB=(_tmp1C8.Rel_n).f1;
if(_tmp1CB == 0)goto _LL12B;_tmp1CC=*_tmp1CB;_tmp1CD=(struct _dynforward_ptr*)
_tmp1CC.hd;_tmp1CE=_tmp1CC.tl;_LL12A: _tmp1D1=_tmp1CD;_tmp1D2=_tmp1CE;goto _LL12C;
_LL12B: if((_tmp1C8.Abs_n).tag != 2)goto _LL122;_tmp1CF=(_tmp1C8.Abs_n).f1;if(
_tmp1CF == 0)goto _LL122;_tmp1D0=*_tmp1CF;_tmp1D1=(struct _dynforward_ptr*)_tmp1D0.hd;
_tmp1D2=_tmp1D0.tl;_LL12C: first=_tmp1D1;rest=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_append)(_tmp1D2,({struct Cyc_List_List*_tmp1D3=
_cycalloc(sizeof(*_tmp1D3));_tmp1D3->hd=_tmp1B5;_tmp1D3->tl=0;_tmp1D3;}));goto
_LL122;_LL122:;}{struct Cyc_List_List*_tmp1D4=Cyc_Tcenv_resolve_namespace(te,loc,
first,rest);ge->availables=(struct Cyc_List_List*)({struct Cyc_List_List*_tmp1D5=
_cycalloc(sizeof(*_tmp1D5));_tmp1D5->hd=_tmp1D4;_tmp1D5->tl=ge->availables;
_tmp1D5;});Cyc_Tc_tc_decls(te,_tmp1B6,in_externC,in_externCinclude,
check_var_init,grgn,exports);ge->availables=((struct Cyc_List_List*)_check_null(
ge->availables))->tl;goto _LL100;}}_LL119: if(_tmp1A9 <= (void*)2)goto _LL11B;if(*((
int*)_tmp1A9)!= 10)goto _LL11B;_tmp1B7=((struct Cyc_Absyn_ExternC_d_struct*)
_tmp1A9)->f1;_LL11A: Cyc_Tc_tc_decls(te,_tmp1B7,1,in_externCinclude,
check_var_init,grgn,exports);goto _LL100;_LL11B: if(_tmp1A9 <= (void*)2)goto _LL100;
if(*((int*)_tmp1A9)!= 11)goto _LL100;_tmp1B8=((struct Cyc_Absyn_ExternCinclude_d_struct*)
_tmp1A9)->f1;_tmp1B9=((struct Cyc_Absyn_ExternCinclude_d_struct*)_tmp1A9)->f2;
_LL11C:((void(*)(void(*f)(struct Cyc_Tcenv_Tenv*,struct _tuple6*),struct Cyc_Tcenv_Tenv*
env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_Tc_resolve_export_namespace,te,
_tmp1B9);{struct Cyc_List_List*newexs=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_append)(_tmp1B9,(unsigned int)exports?*exports:
0);Cyc_Tc_tc_decls(te,_tmp1B8,1,1,check_var_init,grgn,(struct Cyc_List_List**)&
newexs);for(0;_tmp1B9 != 0;_tmp1B9=_tmp1B9->tl){struct _tuple6*_tmp1D6=(struct
_tuple6*)_tmp1B9->hd;if(!(*_tmp1D6).f3)({struct Cyc_String_pa_struct _tmp1D9;
_tmp1D9.tag=0;_tmp1D9.f1=(struct _dynforward_ptr)((struct _dynforward_ptr)Cyc_Absynpp_qvar2string((*
_tmp1D6).f2));{void*_tmp1D7[1]={& _tmp1D9};Cyc_Tcutil_warn((*_tmp1D6).f1,({const
char*_tmp1D8="%s is exported but not defined";_tag_dynforward(_tmp1D8,sizeof(
char),_get_zero_arr_size(_tmp1D8,31));}),_tag_dynforward(_tmp1D7,sizeof(void*),1));}});}
goto _LL100;}_LL100:;}}void Cyc_Tc_tc(struct _RegionHandle*g,struct Cyc_Tcenv_Tenv*
te,int check_var_init,struct Cyc_List_List*ds){Cyc_Absynpp_set_params(& Cyc_Absynpp_tc_params_r);
Cyc_Tc_tc_decls(te,ds,0,0,check_var_init,g,0);}struct Cyc_Tc_TreeshakeEnv{struct
_RegionHandle*rgn;int in_cinclude;struct Cyc_Dict_Dict nsdict;};static int Cyc_Tc_vardecl_needed(
struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_Absyn_Decl*d);static struct Cyc_List_List*
Cyc_Tc_treeshake_f(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*ds){return((
struct Cyc_List_List*(*)(int(*f)(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*),
struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*x))Cyc_List_filter_c)(Cyc_Tc_vardecl_needed,
env,ds);}struct _tuple12{struct Cyc_Tcenv_Genv*f1;struct Cyc_Set_Set*f2;};static int
Cyc_Tc_vardecl_needed(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_Absyn_Decl*d){void*
_tmp1DA=(void*)d->r;struct Cyc_Absyn_Vardecl*_tmp1DB;struct Cyc_List_List*_tmp1DC;
struct Cyc_List_List**_tmp1DD;struct Cyc_List_List*_tmp1DE;struct Cyc_List_List**
_tmp1DF;struct Cyc_List_List*_tmp1E0;struct Cyc_List_List**_tmp1E1;struct Cyc_List_List*
_tmp1E2;struct Cyc_List_List**_tmp1E3;_LL12E: if(_tmp1DA <= (void*)2)goto _LL138;if(*((
int*)_tmp1DA)!= 0)goto _LL130;_tmp1DB=((struct Cyc_Absyn_Var_d_struct*)_tmp1DA)->f1;
_LL12F: if(env->in_cinclude  || (void*)_tmp1DB->sc != (void*)3  && (void*)_tmp1DB->sc
!= (void*)4)return 1;{struct _tuple0 _tmp1E5;union Cyc_Absyn_Nmspace_union _tmp1E6;
struct _dynforward_ptr*_tmp1E7;struct _tuple0*_tmp1E4=_tmp1DB->name;_tmp1E5=*
_tmp1E4;_tmp1E6=_tmp1E5.f1;_tmp1E7=_tmp1E5.f2;{struct Cyc_List_List*ns;{union Cyc_Absyn_Nmspace_union
_tmp1E8=_tmp1E6;struct Cyc_List_List*_tmp1E9;struct Cyc_List_List*_tmp1EA;_LL13B:
if((_tmp1E8.Loc_n).tag != 0)goto _LL13D;_LL13C: ns=0;goto _LL13A;_LL13D: if((_tmp1E8.Rel_n).tag
!= 1)goto _LL13F;_tmp1E9=(_tmp1E8.Rel_n).f1;_LL13E: ns=_tmp1E9;goto _LL13A;_LL13F:
if((_tmp1E8.Abs_n).tag != 2)goto _LL13A;_tmp1EA=(_tmp1E8.Abs_n).f1;_LL140: ns=
_tmp1EA;goto _LL13A;_LL13A:;}{struct _tuple12*_tmp1EB=((struct _tuple12*(*)(struct
Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(env->nsdict,ns);struct Cyc_Tcenv_Genv*
_tmp1EC=(*_tmp1EB).f1;int _tmp1ED=(*((struct _tuple7*(*)(struct Cyc_Dict_Dict d,
struct _dynforward_ptr*k))Cyc_Dict_lookup)(_tmp1EC->ordinaries,_tmp1E7)).f2;if(!
_tmp1ED)(*_tmp1EB).f2=((struct Cyc_Set_Set*(*)(struct _RegionHandle*r,struct Cyc_Set_Set*
s,struct _dynforward_ptr*elt))Cyc_Set_rinsert)(env->rgn,(*_tmp1EB).f2,_tmp1E7);
return _tmp1ED;}}}_LL130: if(*((int*)_tmp1DA)!= 10)goto _LL132;_tmp1DC=((struct Cyc_Absyn_ExternC_d_struct*)
_tmp1DA)->f1;_tmp1DD=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternC_d_struct*)
_tmp1DA)->f1;_LL131: _tmp1DF=_tmp1DD;goto _LL133;_LL132: if(*((int*)_tmp1DA)!= 9)
goto _LL134;_tmp1DE=((struct Cyc_Absyn_Using_d_struct*)_tmp1DA)->f2;_tmp1DF=(
struct Cyc_List_List**)&((struct Cyc_Absyn_Using_d_struct*)_tmp1DA)->f2;_LL133:
_tmp1E1=_tmp1DF;goto _LL135;_LL134: if(*((int*)_tmp1DA)!= 8)goto _LL136;_tmp1E0=((
struct Cyc_Absyn_Namespace_d_struct*)_tmp1DA)->f2;_tmp1E1=(struct Cyc_List_List**)&((
struct Cyc_Absyn_Namespace_d_struct*)_tmp1DA)->f2;_LL135:*_tmp1E1=Cyc_Tc_treeshake_f(
env,*_tmp1E1);return 1;_LL136: if(*((int*)_tmp1DA)!= 11)goto _LL138;_tmp1E2=((
struct Cyc_Absyn_ExternCinclude_d_struct*)_tmp1DA)->f1;_tmp1E3=(struct Cyc_List_List**)&((
struct Cyc_Absyn_ExternCinclude_d_struct*)_tmp1DA)->f1;_LL137: {int in_cinclude=
env->in_cinclude;env->in_cinclude=1;*_tmp1E3=Cyc_Tc_treeshake_f(env,*_tmp1E3);
env->in_cinclude=in_cinclude;return 1;}_LL138:;_LL139: return 1;_LL12D:;}static int
Cyc_Tc_treeshake_remove_f(struct Cyc_Set_Set*set,struct _dynforward_ptr*x,void*y){
return !((int(*)(struct Cyc_Set_Set*s,struct _dynforward_ptr*elt))Cyc_Set_member)(
set,x);}static struct _tuple12*Cyc_Tc_treeshake_make_env_f(struct _RegionHandle*rgn,
struct Cyc_Tcenv_Genv*ge){return({struct _tuple12*_tmp1EE=_region_malloc(rgn,
sizeof(*_tmp1EE));_tmp1EE->f1=ge;_tmp1EE->f2=((struct Cyc_Set_Set*(*)(struct
_RegionHandle*r,int(*cmp)(struct _dynforward_ptr*,struct _dynforward_ptr*)))Cyc_Set_rempty)(
rgn,Cyc_strptrcmp);_tmp1EE;});}struct _tuple13{struct Cyc_List_List*f1;struct
_tuple12*f2;};struct Cyc_List_List*Cyc_Tc_treeshake(struct Cyc_Tcenv_Tenv*te,
struct Cyc_List_List*ds){struct _RegionHandle _tmp1EF=_new_region("rgn");struct
_RegionHandle*rgn=& _tmp1EF;_push_region(rgn);{struct Cyc_Tc_TreeshakeEnv _tmp1F0=({
struct Cyc_Tc_TreeshakeEnv _tmp1FB;_tmp1FB.rgn=rgn;_tmp1FB.in_cinclude=0;_tmp1FB.nsdict=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,struct _tuple12*(*f)(struct
_RegionHandle*,struct Cyc_Tcenv_Genv*),struct _RegionHandle*env,struct Cyc_Dict_Dict
d))Cyc_Dict_rmap_c)(rgn,Cyc_Tc_treeshake_make_env_f,rgn,te->ae);_tmp1FB;});
struct Cyc_List_List*_tmp1F1=Cyc_Tc_treeshake_f(& _tmp1F0,ds);if(((int(*)(struct
Cyc_Dict_Dict d))Cyc_Dict_is_empty)(_tmp1F0.nsdict)){struct Cyc_List_List*_tmp1F2=
_tmp1F1;_npop_handler(0);return _tmp1F2;}{struct Cyc_Iter_Iter _tmp1F3=((struct Cyc_Iter_Iter(*)(
struct _RegionHandle*rgn,struct Cyc_Dict_Dict d))Cyc_Dict_make_iter)(Cyc_Core_heap_region,
_tmp1F0.nsdict);struct _tuple13 _tmp1F4=*((struct _tuple13*(*)(struct _RegionHandle*
r,struct Cyc_Dict_Dict d))Cyc_Dict_rchoose)(rgn,_tmp1F0.nsdict);while(((int(*)(
struct Cyc_Iter_Iter,struct _tuple13*))Cyc_Iter_next)(_tmp1F3,& _tmp1F4)){struct
_tuple12*_tmp1F6;struct _tuple12 _tmp1F7;struct Cyc_Tcenv_Genv*_tmp1F8;struct Cyc_Set_Set*
_tmp1F9;struct _tuple13 _tmp1F5=_tmp1F4;_tmp1F6=_tmp1F5.f2;_tmp1F7=*_tmp1F6;
_tmp1F8=_tmp1F7.f1;_tmp1F9=_tmp1F7.f2;_tmp1F8->ordinaries=((struct Cyc_Dict_Dict(*)(
struct _RegionHandle*,int(*f)(struct Cyc_Set_Set*,struct _dynforward_ptr*,struct
_tuple7*),struct Cyc_Set_Set*env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c)(
_tmp1F8->grgn,(int(*)(struct Cyc_Set_Set*set,struct _dynforward_ptr*x,struct
_tuple7*y))Cyc_Tc_treeshake_remove_f,_tmp1F9,_tmp1F8->ordinaries);}{struct Cyc_List_List*
_tmp1FA=_tmp1F1;_npop_handler(0);return _tmp1FA;}}};_pop_region(rgn);}
