#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file
   lib/runtime_cyc.c
*/
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

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
  char data[1];  /*FJS: used to be size 0, but that's forbidden in ansi c*/
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
extern int _throw_match();
extern int _throw(void* e);
#endif

extern struct _xtunion_struct *_exn_thrown;

/* Built-in Exceptions */
struct Cyc_Null_Exception_struct { char *tag; };
struct Cyc_Array_bounds_struct { char *tag; };
struct Cyc_Match_Exception_struct { char *tag; };
struct Cyc_Bad_alloc_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef __APPLE__
#define _INLINE_FUNCTIONS
#endif

#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#define _INLINE_FUNCTIONS
#else
#define _INLINE inline
#endif

#ifdef VC_C
#define _CYC_U_LONG_LONG_T __int64
#else
#ifdef GCC_C
#define _CYC_U_LONG_LONG_T unsigned long long
#else
#define _CYC_U_LONG_LONG_T unsigned long long
#endif
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE void *
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
static _INLINE char *
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
static _INLINE unsigned
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
#define _zero_arr_plus_char(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_short(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_int(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_float(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_double(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_longdouble(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#define _zero_arr_plus_voidstar(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#else
static _INLINE char *
_zero_arr_plus_char(char *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE short *
_zero_arr_plus_short(short *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE int *
_zero_arr_plus_int(int *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE float *
_zero_arr_plus_float(float *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE double *
_zero_arr_plus_double(double *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE long double *
_zero_arr_plus_longdouble(long double *orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
static _INLINE void *
_zero_arr_plus_voidstar(void **orig_x, int orig_sz, int orig_i) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null();
  if (orig_i < 0) _throw_arraybounds();
  for (_czs_temp=orig_sz; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds();
  return orig_x + orig_i;
}
#endif


/* Calculates the number of elements in a zero-terminated, thin array.
   If non-null, the array is guaranteed to have orig_offset elements. */
static _INLINE int
_get_zero_arr_size_char(const char *orig_x, unsigned int orig_offset) {
  const char *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_short(const short *orig_x, unsigned int orig_offset) {
  const short *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_int(const int *orig_x, unsigned int orig_offset) {
  const int *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_float(const float *orig_x, unsigned int orig_offset) {
  const float *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_double(const double *orig_x, unsigned int orig_offset) {
  const double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_longdouble(const long double *orig_x, unsigned int orig_offset) {
  const long double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_voidstar(const void **orig_x, unsigned int orig_offset) {
  const void **_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}


/* Does in-place addition of a zero-terminated pointer (x += e and ++x).  
   Note that this expands to call _zero_arr_plus. */
/*#define _zero_arr_inplace_plus(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  *_zap_x = _zero_arr_plus(*_zap_x,1,(orig_i)); })
  */
static _INLINE void 
_zero_arr_inplace_plus_char(char **x, int orig_i) {
  *x = _zero_arr_plus_char(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_short(short **x, int orig_i) {
  *x = _zero_arr_plus_short(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_int(int **x, int orig_i) {
  *x = _zero_arr_plus_int(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_float(float **x, int orig_i) {
  *x = _zero_arr_plus_float(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_double(double **x, int orig_i) {
  *x = _zero_arr_plus_double(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_longdouble(long double **x, int orig_i) {
  *x = _zero_arr_plus_longdouble(*x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_voidstar(void ***x, int orig_i) {
  *x = _zero_arr_plus_voidstar(*x,1,orig_i);
}

/* Does in-place increment of a zero-terminated pointer (e.g., x++).
   Note that this expands to call _zero_arr_plus. */
/*#define _zero_arr_inplace_plus_post(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  _zap_tx *_zap_res = *_zap_x; \
  *_zap_x = _zero_arr_plus(_zap_res,1,(orig_i)); \
  _zap_res; })*/
  
static _INLINE char *
_zero_arr_inplace_plus_post_char(char **x, int orig_i){
  char * _zap_res = *x;
  *x = _zero_arr_plus_char(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE short *
_zero_arr_inplace_plus_post_short(short **x, int orig_i){
  short * _zap_res = *x;
  *x = _zero_arr_plus_short(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE int *
_zero_arr_inplace_plus_post_int(int **x, int orig_i){
  int * _zap_res = *x;
  *x = _zero_arr_plus_int(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE float *
_zero_arr_inplace_plus_post_float(float **x, int orig_i){
  float * _zap_res = *x;
  *x = _zero_arr_plus_float(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE double *
_zero_arr_inplace_plus_post_double(double **x, int orig_i){
  double * _zap_res = *x;
  *x = _zero_arr_plus_double(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE long double *
_zero_arr_inplace_plus_post_longdouble(long double **x, int orig_i){
  long double * _zap_res = *x;
  *x = _zero_arr_plus_longdouble(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE void **
_zero_arr_inplace_plus_post_voidstar(void ***x, int orig_i){
  void ** _zap_res = *x;
  *x = _zero_arr_plus_voidstar(_zap_res,1,orig_i);
  return _zap_res;
}



/* functions for dealing with dynamically sized pointers */
#ifdef NO_CYC_BOUNDS_CHECKS
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
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
#endif
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  /* JGM: not needed! if (!_cus_arr.base) _throw_null(); */ 
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds();
  return _cus_ans;
}
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_tag_dyneither(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _tag_arr_ans;
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
#else
#define _tag_dyneither(tcurr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr *
_init_dyneither_ptr(struct _dyneither_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((unsigned char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
#else
#define _init_dyneither_ptr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_untag_dyneither_ptr(struct _dyneither_ptr arr, 
                     unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
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
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned
_get_dyneither_size(struct _dyneither_ptr arr,unsigned elt_sz) {
  struct _dyneither_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
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
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_plus(struct _dyneither_ptr arr,unsigned elt_sz,int change) {
  struct _dyneither_ptr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_plus(arr,elt_sz,change) ({ \
  struct _dyneither_ptr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,
                            int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
#else
#define _dyneither_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus_post(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  struct _dyneither_ptr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  struct _dyneither_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

/* Decrease the upper bound on a fat pointer by numelts where sz is
   the size of the pointer's type.  Note that this can't be a macro
   if we're to get initializers right. */
static struct 
_dyneither_ptr _dyneither_ptr_decrease_size(struct _dyneither_ptr x,
                                            unsigned int sz,
                                            unsigned int numelts) {
  x.last_plus_one -= sz * numelts; 
  return x; 
}

/* Allocation */
extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);

static _INLINE void* _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc_atomic(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  _CYC_U_LONG_LONG_T whole_ans = 
    ((_CYC_U_LONG_LONG_T)x)*((_CYC_U_LONG_LONG_T)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,const char *file,const char *func,
                                int lineno);
extern void* _profile_GC_malloc_atomic(int,const char *file,
                                       const char *func,int lineno);
extern void* _profile_region_malloc(struct _RegionHandle *, unsigned,
                                    const char *file,
                                    const char *func,
                                    int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						const char *file,
						const char *func,
                                                int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 const char *file,
                                 const char *func,
                                 int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif

/* the next two routines swap [x] and [y]; not thread safe! */
static _INLINE void _swap_word(void *x, void *y) {
  unsigned long *lx = (unsigned long *)x, *ly = (unsigned long *)y, tmp;
  tmp = *lx;
  *lx = *ly;
  *ly = tmp;
}
static _INLINE void _swap_dyneither(struct _dyneither_ptr *x, 
				   struct _dyneither_ptr *y) {
  struct _dyneither_ptr tmp = *x;
  *x = *y;
  *y = tmp;
}
 typedef char*Cyc_Cstring;typedef char*Cyc_CstringNN;typedef struct _dyneither_ptr Cyc_string_t;
typedef struct _dyneither_ptr Cyc_mstring_t;typedef struct _dyneither_ptr*Cyc_stringptr_t;
typedef struct _dyneither_ptr*Cyc_mstringptr_t;typedef char*Cyc_Cbuffer_t;typedef
char*Cyc_CbufferNN_t;typedef struct _dyneither_ptr Cyc_buffer_t;typedef struct
_dyneither_ptr Cyc_mbuffer_t;typedef int Cyc_bool;struct Cyc_Core_NewRegion{struct
_DynRegionHandle*dynregion;};typedef unsigned long Cyc_size_t;typedef
unsigned short Cyc_mode_t;struct Cyc___cycFILE;typedef struct Cyc___cycFILE Cyc_FILE;
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_struct{int tag;struct
_dyneither_ptr f1;};struct Cyc_Int_pa_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_struct{
int tag;double f1;};struct Cyc_LongDouble_pa_struct{int tag;long double f1;};struct
Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{int tag;
unsigned long*f1;};typedef void*Cyc_parg_t;struct _dyneither_ptr Cyc_aprintf(struct
_dyneither_ptr,struct _dyneither_ptr);int Cyc_fflush(struct Cyc___cycFILE*);int Cyc_fprintf(
struct Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_struct{
int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{int tag;unsigned short*f1;};
struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{int tag;
unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
typedef void*Cyc_sarg_t;extern char Cyc_FileCloseError[15];struct Cyc_FileCloseError_struct{
char*tag;};extern char Cyc_FileOpenError[14];struct Cyc_FileOpenError_struct{char*
tag;struct _dyneither_ptr f1;};typedef unsigned int Cyc_Core_sizeof_t;struct Cyc_Core_Opt{
void*v;};typedef struct Cyc_Core_Opt*Cyc_Core_opt_t;extern char Cyc_Core_Invalid_argument[
17];struct Cyc_Core_Invalid_argument_struct{char*tag;struct _dyneither_ptr f1;};
extern char Cyc_Core_Failure[8];struct Cyc_Core_Failure_struct{char*tag;struct
_dyneither_ptr f1;};extern char Cyc_Core_Impossible[11];struct Cyc_Core_Impossible_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10];struct Cyc_Core_Not_found_struct{
char*tag;};extern char Cyc_Core_Unreachable[12];struct Cyc_Core_Unreachable_struct{
char*tag;struct _dyneither_ptr f1;};extern struct _RegionHandle*Cyc_Core_heap_region;
extern struct _RegionHandle*Cyc_Core_unique_region;extern char Cyc_Core_Open_Region[
12];struct Cyc_Core_Open_Region_struct{char*tag;};extern char Cyc_Core_Free_Region[
12];struct Cyc_Core_Free_Region_struct{char*tag;};inline static void* arrcast(
struct _dyneither_ptr dyn,unsigned int bd,unsigned int sz){if(bd >> 20  || sz >> 12)
return 0;{unsigned char*ptrbd=dyn.curr + bd * sz;if(((ptrbd < dyn.curr  || dyn.curr == 
0) || dyn.curr < dyn.base) || ptrbd > dyn.last_plus_one)return 0;return dyn.curr;};}
struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};typedef struct Cyc_List_List*
Cyc_List_list_t;typedef struct Cyc_List_List*Cyc_List_List_t;extern char Cyc_List_List_mismatch[
14];struct Cyc_List_List_mismatch_struct{char*tag;};struct Cyc_List_List*Cyc_List_rev(
struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*
x);struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*
y);extern char Cyc_List_Nth[4];struct Cyc_List_Nth_struct{char*tag;};struct Cyc_Iter_Iter{
void*env;int(*next)(void*env,void*dest);};typedef struct Cyc_Iter_Iter Cyc_Iter_iter_t;
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Dict_T;typedef const struct
Cyc_Dict_T*Cyc_Dict_tree;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct
_RegionHandle*r;const struct Cyc_Dict_T*t;};typedef struct Cyc_Dict_Dict Cyc_Dict_dict_t;
extern char Cyc_Dict_Present[8];struct Cyc_Dict_Present_struct{char*tag;};extern
char Cyc_Dict_Absent[7];struct Cyc_Dict_Absent_struct{char*tag;};struct Cyc_Dict_Dict
Cyc_Dict_rempty(struct _RegionHandle*,int(*cmp)(void*,void*));int Cyc_Dict_is_empty(
struct Cyc_Dict_Dict d);int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);struct Cyc_Dict_Dict
Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);void*Cyc_Dict_lookup(struct
Cyc_Dict_Dict d,void*k);void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);
struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct _RegionHandle*,struct Cyc_Dict_Dict,
void*);struct Cyc_Set_Set;typedef struct Cyc_Set_Set*Cyc_Set_set_t;struct Cyc_Set_Set*
Cyc_Set_empty(int(*cmp)(void*,void*));struct Cyc_Set_Set*Cyc_Set_rempty(struct
_RegionHandle*r,int(*cmp)(void*,void*));int Cyc_Set_member(struct Cyc_Set_Set*s,
void*elt);extern char Cyc_Set_Absent[7];struct Cyc_Set_Absent_struct{char*tag;};
struct Cyc_SlowDict_Dict;typedef struct Cyc_SlowDict_Dict*Cyc_SlowDict_dict_t;
extern char Cyc_SlowDict_Present[8];struct Cyc_SlowDict_Present_struct{char*tag;};
extern char Cyc_SlowDict_Absent[7];struct Cyc_SlowDict_Absent_struct{char*tag;};
struct Cyc_Lineno_Pos{struct _dyneither_ptr logical_file;struct _dyneither_ptr line;
int line_no;int col;};typedef struct Cyc_Lineno_Pos*Cyc_Lineno_pos_t;extern char Cyc_Position_Exit[
5];struct Cyc_Position_Exit_struct{char*tag;};typedef unsigned int Cyc_Position_seg_t;
struct Cyc_Position_Lex_struct{int tag;};struct Cyc_Position_Parse_struct{int tag;};
struct Cyc_Position_Elab_struct{int tag;};typedef void*Cyc_Position_error_kind_t;
struct Cyc_Position_Error{struct _dyneither_ptr source;unsigned int seg;void*kind;
struct _dyneither_ptr desc;};typedef struct Cyc_Position_Error*Cyc_Position_error_t;
extern char Cyc_Position_Nocontext[10];struct Cyc_Position_Nocontext_struct{char*
tag;};int Cyc_strptrcmp(struct _dyneither_ptr*s1,struct _dyneither_ptr*s2);struct
Cyc_PP_Ppstate;typedef struct Cyc_PP_Ppstate*Cyc_PP_ppstate_t;struct Cyc_PP_Out;
typedef struct Cyc_PP_Out*Cyc_PP_out_t;struct Cyc_PP_Doc;typedef struct Cyc_PP_Doc*
Cyc_PP_doc_t;typedef struct _dyneither_ptr*Cyc_Absyn_field_name_t;typedef struct
_dyneither_ptr*Cyc_Absyn_var_t;typedef struct _dyneither_ptr*Cyc_Absyn_tvarname_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_opt_t;struct _union_Nmspace_Rel_n{int
tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*
val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct
_union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct
_union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct
_union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};typedef union Cyc_Absyn_Nmspace
Cyc_Absyn_nmspace_t;union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;union Cyc_Absyn_Nmspace
Cyc_Absyn_Rel_n(struct Cyc_List_List*);union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(
struct Cyc_List_List*ns,int C_scope);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct
_dyneither_ptr*f2;};typedef struct _tuple0*Cyc_Absyn_qvar_t;typedef struct _tuple0*
Cyc_Absyn_qvar_opt_t;typedef struct _tuple0*Cyc_Absyn_typedef_name_t;typedef struct
_tuple0*Cyc_Absyn_typedef_name_opt_t;typedef enum Cyc_Absyn_Scope Cyc_Absyn_scope_t;
typedef struct Cyc_Absyn_Tqual Cyc_Absyn_tqual_t;typedef enum Cyc_Absyn_Size_of Cyc_Absyn_size_of_t;
typedef struct Cyc_Absyn_Kind*Cyc_Absyn_kind_t;typedef void*Cyc_Absyn_kindbound_t;
typedef struct Cyc_Absyn_Tvar*Cyc_Absyn_tvar_t;typedef enum Cyc_Absyn_Sign Cyc_Absyn_sign_t;
typedef enum Cyc_Absyn_AggrKind Cyc_Absyn_aggr_kind_t;typedef void*Cyc_Absyn_bounds_t;
typedef struct Cyc_Absyn_PtrAtts Cyc_Absyn_ptr_atts_t;typedef struct Cyc_Absyn_PtrInfo
Cyc_Absyn_ptr_info_t;typedef struct Cyc_Absyn_VarargInfo Cyc_Absyn_vararg_info_t;
typedef struct Cyc_Absyn_FnInfo Cyc_Absyn_fn_info_t;typedef struct Cyc_Absyn_DatatypeInfo
Cyc_Absyn_datatype_info_t;typedef struct Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_datatype_field_info_t;
typedef struct Cyc_Absyn_AggrInfo Cyc_Absyn_aggr_info_t;typedef struct Cyc_Absyn_ArrayInfo
Cyc_Absyn_array_info_t;typedef void*Cyc_Absyn_type_t;typedef void*Cyc_Absyn_rgntype_t;
typedef void*Cyc_Absyn_type_opt_t;typedef union Cyc_Absyn_Cnst Cyc_Absyn_cnst_t;
typedef enum Cyc_Absyn_Primop Cyc_Absyn_primop_t;typedef enum Cyc_Absyn_Incrementor
Cyc_Absyn_incrementor_t;typedef struct Cyc_Absyn_VarargCallInfo Cyc_Absyn_vararg_call_info_t;
typedef void*Cyc_Absyn_raw_exp_t;typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_t;
typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_opt_t;typedef void*Cyc_Absyn_raw_stmt_t;
typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_t;typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_opt_t;
typedef void*Cyc_Absyn_raw_pat_t;typedef struct Cyc_Absyn_Pat*Cyc_Absyn_pat_t;
typedef void*Cyc_Absyn_binding_t;typedef struct Cyc_Absyn_Switch_clause*Cyc_Absyn_switch_clause_t;
typedef struct Cyc_Absyn_Fndecl*Cyc_Absyn_fndecl_t;typedef struct Cyc_Absyn_Aggrdecl*
Cyc_Absyn_aggrdecl_t;typedef struct Cyc_Absyn_Datatypefield*Cyc_Absyn_datatypefield_t;
typedef struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_datatypedecl_t;typedef struct Cyc_Absyn_Typedefdecl*
Cyc_Absyn_typedefdecl_t;typedef struct Cyc_Absyn_Enumfield*Cyc_Absyn_enumfield_t;
typedef struct Cyc_Absyn_Enumdecl*Cyc_Absyn_enumdecl_t;typedef struct Cyc_Absyn_Vardecl*
Cyc_Absyn_vardecl_t;typedef void*Cyc_Absyn_raw_decl_t;typedef struct Cyc_Absyn_Decl*
Cyc_Absyn_decl_t;typedef void*Cyc_Absyn_designator_t;typedef void*Cyc_Absyn_absyn_annot_t;
typedef void*Cyc_Absyn_attribute_t;typedef struct Cyc_List_List*Cyc_Absyn_attributes_t;
typedef struct Cyc_Absyn_Aggrfield*Cyc_Absyn_aggrfield_t;typedef void*Cyc_Absyn_offsetof_field_t;
typedef struct Cyc_Absyn_MallocInfo Cyc_Absyn_malloc_info_t;typedef enum Cyc_Absyn_Coercion
Cyc_Absyn_coercion_t;typedef struct Cyc_Absyn_PtrLoc*Cyc_Absyn_ptrloc_t;enum Cyc_Absyn_Scope{
Cyc_Absyn_Static  = 0,Cyc_Absyn_Abstract  = 1,Cyc_Absyn_Public  = 2,Cyc_Absyn_Extern
 = 3,Cyc_Absyn_ExternC  = 4,Cyc_Absyn_Register  = 5};struct Cyc_Absyn_Tqual{int
print_const;int q_volatile;int q_restrict;int real_const;unsigned int loc;};enum Cyc_Absyn_Size_of{
Cyc_Absyn_Char_sz  = 0,Cyc_Absyn_Short_sz  = 1,Cyc_Absyn_Int_sz  = 2,Cyc_Absyn_Long_sz
 = 3,Cyc_Absyn_LongLong_sz  = 4};enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable  = 0,
Cyc_Absyn_Unique  = 1,Cyc_Absyn_Top  = 2};enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind
 = 0,Cyc_Absyn_MemKind  = 1,Cyc_Absyn_BoxKind  = 2,Cyc_Absyn_RgnKind  = 3,Cyc_Absyn_EffKind
 = 4,Cyc_Absyn_IntKind  = 5};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;
enum Cyc_Absyn_AliasQual aliasqual;};enum Cyc_Absyn_Sign{Cyc_Absyn_Signed  = 0,Cyc_Absyn_Unsigned
 = 1,Cyc_Absyn_None  = 2};enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA  = 0,Cyc_Absyn_UnionA
 = 1};struct _union_Constraint_Eq_constr{int tag;void*val;};struct
_union_Constraint_Forward_constr{int tag;union Cyc_Absyn_Constraint*val;};struct
_union_Constraint_No_constr{int tag;int val;};union Cyc_Absyn_Constraint{struct
_union_Constraint_Eq_constr Eq_constr;struct _union_Constraint_Forward_constr
Forward_constr;struct _union_Constraint_No_constr No_constr;};typedef union Cyc_Absyn_Constraint*
Cyc_Absyn_conref_t;struct Cyc_Absyn_Eq_kb_struct{int tag;struct Cyc_Absyn_Kind*f1;};
struct Cyc_Absyn_Unknown_kb_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{
struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_DynEither_b_struct{
int tag;};struct Cyc_Absyn_Upper_b_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct
Cyc_Absyn_PtrLoc{unsigned int ptr_loc;unsigned int rgn_loc;unsigned int zt_loc;};
struct Cyc_Absyn_PtrAtts{void*rgn;union Cyc_Absyn_Constraint*nullable;union Cyc_Absyn_Constraint*
bounds;union Cyc_Absyn_Constraint*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};
struct Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts
ptr_atts;};struct Cyc_Absyn_Numelts_ptrqual_struct{int tag;struct Cyc_Absyn_Exp*f1;
};struct Cyc_Absyn_Region_ptrqual_struct{int tag;void*f1;};struct Cyc_Absyn_Thin_ptrqual_struct{
int tag;};struct Cyc_Absyn_Fat_ptrqual_struct{int tag;};struct Cyc_Absyn_Zeroterm_ptrqual_struct{
int tag;};struct Cyc_Absyn_Nozeroterm_ptrqual_struct{int tag;};struct Cyc_Absyn_Notnull_ptrqual_struct{
int tag;};struct Cyc_Absyn_Nullable_ptrqual_struct{int tag;};typedef void*Cyc_Absyn_pointer_qual_t;
typedef struct Cyc_List_List*Cyc_Absyn_pointer_quals_t;struct Cyc_Absyn_VarargInfo{
struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct
Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual
ret_tqual;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;};struct
Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct
_union_DatatypeInfoU_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo
val;};struct _union_DatatypeInfoU_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**
val;};union Cyc_Absyn_DatatypeInfoU{struct _union_DatatypeInfoU_UnknownDatatype
UnknownDatatype;struct _union_DatatypeInfoU_KnownDatatype KnownDatatype;};struct
Cyc_Absyn_DatatypeInfo{union Cyc_Absyn_DatatypeInfoU datatype_info;struct Cyc_List_List*
targs;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;
struct _tuple0*field_name;int is_extensible;};struct
_union_DatatypeFieldInfoU_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo
val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*
f2;};struct _union_DatatypeFieldInfoU_KnownDatatypefield{int tag;struct _tuple1 val;
};union Cyc_Absyn_DatatypeFieldInfoU{struct
_union_DatatypeFieldInfoU_UnknownDatatypefield UnknownDatatypefield;struct
_union_DatatypeFieldInfoU_KnownDatatypefield KnownDatatypefield;};struct Cyc_Absyn_DatatypeFieldInfo{
union Cyc_Absyn_DatatypeFieldInfoU field_info;struct Cyc_List_List*targs;};struct
_tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};
struct _union_AggrInfoU_UnknownAggr{int tag;struct _tuple2 val;};struct
_union_AggrInfoU_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfoU{
struct _union_AggrInfoU_UnknownAggr UnknownAggr;struct _union_AggrInfoU_KnownAggr
KnownAggr;};struct Cyc_Absyn_AggrInfo{union Cyc_Absyn_AggrInfoU aggr_info;struct Cyc_List_List*
targs;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct
Cyc_Absyn_Exp*num_elts;union Cyc_Absyn_Constraint*zero_term;unsigned int zt_loc;};
struct Cyc_Absyn_Aggr_td_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_struct{int tag;
struct Cyc_Absyn_Datatypedecl*f1;};typedef void*Cyc_Absyn_raw_type_decl_t;struct
Cyc_Absyn_TypeDecl{void*r;unsigned int loc;};typedef struct Cyc_Absyn_TypeDecl*Cyc_Absyn_type_decl_t;
struct Cyc_Absyn_VoidType_struct{int tag;};struct Cyc_Absyn_Evar_struct{int tag;
struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_struct{
int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_DatatypeType_struct{int tag;
struct Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldType_struct{int tag;
struct Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_PointerType_struct{int tag;
struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_struct{int tag;enum Cyc_Absyn_Sign
f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatType_struct{int tag;int f1;};
struct Cyc_Absyn_ArrayType_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_struct{
int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_struct{int tag;struct
Cyc_List_List*f1;};struct Cyc_Absyn_AggrType_struct{int tag;struct Cyc_Absyn_AggrInfo
f1;};struct Cyc_Absyn_AnonAggrType_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_struct{int tag;struct _tuple0*f1;
struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{int tag;struct
Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*f1;};struct
Cyc_Absyn_DynRgnType_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
void*f4;};struct Cyc_Absyn_ValueofType_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_TagType_struct{int tag;void*f1;};struct Cyc_Absyn_HeapRgn_struct{
int tag;};struct Cyc_Absyn_UniqueRgn_struct{int tag;};struct Cyc_Absyn_RefCntRgn_struct{
int tag;};struct Cyc_Absyn_AccessEff_struct{int tag;void*f1;};struct Cyc_Absyn_JoinEff_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnsEff_struct{int tag;void*f1;};
struct Cyc_Absyn_TypeDeclType_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;
};extern struct Cyc_Absyn_HeapRgn_struct Cyc_Absyn_HeapRgn_val;extern struct Cyc_Absyn_UniqueRgn_struct
Cyc_Absyn_UniqueRgn_val;extern struct Cyc_Absyn_RefCntRgn_struct Cyc_Absyn_RefCntRgn_val;
struct Cyc_Absyn_NoTypes_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;};
struct Cyc_Absyn_WithTypes_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*
f3;void*f4;struct Cyc_List_List*f5;};typedef void*Cyc_Absyn_funcparams_t;enum Cyc_Absyn_Format_Type{
Cyc_Absyn_Printf_ft  = 0,Cyc_Absyn_Scanf_ft  = 1};struct Cyc_Absyn_Regparm_att_struct{
int tag;int f1;};struct Cyc_Absyn_Stdcall_att_struct{int tag;};struct Cyc_Absyn_Cdecl_att_struct{
int tag;};struct Cyc_Absyn_Fastcall_att_struct{int tag;};struct Cyc_Absyn_Noreturn_att_struct{
int tag;};struct Cyc_Absyn_Const_att_struct{int tag;};struct Cyc_Absyn_Aligned_att_struct{
int tag;int f1;};struct Cyc_Absyn_Packed_att_struct{int tag;};struct Cyc_Absyn_Section_att_struct{
int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Nocommon_att_struct{int tag;};
struct Cyc_Absyn_Shared_att_struct{int tag;};struct Cyc_Absyn_Unused_att_struct{int
tag;};struct Cyc_Absyn_Weak_att_struct{int tag;};struct Cyc_Absyn_Dllimport_att_struct{
int tag;};struct Cyc_Absyn_Dllexport_att_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_struct{
int tag;};struct Cyc_Absyn_Constructor_att_struct{int tag;};struct Cyc_Absyn_Destructor_att_struct{
int tag;};struct Cyc_Absyn_No_check_memory_usage_att_struct{int tag;};struct Cyc_Absyn_Format_att_struct{
int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_struct{
int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_struct{
int tag;};struct Cyc_Absyn_Mode_att_struct{int tag;struct _dyneither_ptr f1;};struct
Cyc_Absyn_Carray_mod_struct{int tag;union Cyc_Absyn_Constraint*f1;unsigned int f2;};
struct Cyc_Absyn_ConstArray_mod_struct{int tag;struct Cyc_Absyn_Exp*f1;union Cyc_Absyn_Constraint*
f2;unsigned int f3;};struct Cyc_Absyn_Pointer_mod_struct{int tag;struct Cyc_Absyn_PtrAtts
f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_struct{int tag;void*f1;
};struct Cyc_Absyn_TypeParams_mod_struct{int tag;struct Cyc_List_List*f1;
unsigned int f2;int f3;};struct Cyc_Absyn_Attributes_mod_struct{int tag;unsigned int
f1;struct Cyc_List_List*f2;};typedef void*Cyc_Absyn_type_modifier_t;struct
_union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};
struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int
tag;struct _dyneither_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};
struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign
f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum 
Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6
val;};struct _tuple7{struct _dyneither_ptr f1;int f2;};struct _union_Cnst_Float_c{int
tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _dyneither_ptr val;
};struct _union_Cnst_Wstring_c{int tag;struct _dyneither_ptr val;};union Cyc_Absyn_Cnst{
struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct
_union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct
_union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct
_union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct
_union_Cnst_Wstring_c Wstring_c;};enum Cyc_Absyn_Primop{Cyc_Absyn_Plus  = 0,Cyc_Absyn_Times
 = 1,Cyc_Absyn_Minus  = 2,Cyc_Absyn_Div  = 3,Cyc_Absyn_Mod  = 4,Cyc_Absyn_Eq  = 5,
Cyc_Absyn_Neq  = 6,Cyc_Absyn_Gt  = 7,Cyc_Absyn_Lt  = 8,Cyc_Absyn_Gte  = 9,Cyc_Absyn_Lte
 = 10,Cyc_Absyn_Not  = 11,Cyc_Absyn_Bitnot  = 12,Cyc_Absyn_Bitand  = 13,Cyc_Absyn_Bitor
 = 14,Cyc_Absyn_Bitxor  = 15,Cyc_Absyn_Bitlshift  = 16,Cyc_Absyn_Bitlrshift  = 17,
Cyc_Absyn_Bitarshift  = 18,Cyc_Absyn_Numelts  = 19};enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc
 = 0,Cyc_Absyn_PostInc  = 1,Cyc_Absyn_PreDec  = 2,Cyc_Absyn_PostDec  = 3};struct Cyc_Absyn_VarargCallInfo{
int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
struct Cyc_Absyn_StructField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_struct{
int tag;unsigned int f1;};enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion  = 0,
Cyc_Absyn_No_coercion  = 1,Cyc_Absyn_NonNull_to_Null  = 2,Cyc_Absyn_Other_coercion
 = 3};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**
elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;};struct Cyc_Absyn_Const_e_struct{
int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_struct{int tag;struct _tuple0*
f1;void*f2;};struct Cyc_Absyn_Primop_e_struct{int tag;enum Cyc_Absyn_Primop f1;
struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};
struct Cyc_Absyn_And_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_Or_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_SeqExp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_FnCall_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*
f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_struct{int
tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoInstantiate_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_struct{int tag;void*f1;struct
Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_struct{int tag;void*f1;};
struct Cyc_Absyn_Sizeofexp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_struct{
int tag;void*f1;void*f2;};struct Cyc_Absyn_Deref_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_AggrMember_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
_dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_struct{int tag;struct
Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_struct{
int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual
f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple8*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;
};struct Cyc_Absyn_Comprehension_e_struct{int tag;struct Cyc_Absyn_Vardecl*f1;
struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_Aggregate_e_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*
f4;};struct Cyc_Absyn_AnonStruct_e_struct{int tag;void*f1;struct Cyc_List_List*f2;};
struct Cyc_Absyn_Datatype_e_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*
f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_struct{int tag;
struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;struct Cyc_Absyn_Enumfield*f3;};
struct Cyc_Absyn_AnonEnum_e_struct{int tag;struct _tuple0*f1;void*f2;struct Cyc_Absyn_Enumfield*
f3;};struct Cyc_Absyn_Malloc_e_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};
struct Cyc_Absyn_Swap_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_UnresolvedMem_e_struct{int tag;struct Cyc_Core_Opt*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{int tag;struct Cyc_Absyn_Stmt*
f1;};struct Cyc_Absyn_Tagcheck_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
_dyneither_ptr*f2;};struct Cyc_Absyn_Valueof_e_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_struct{
int tag;int f1;struct _dyneither_ptr f2;};struct Cyc_Absyn_Exp{void*topt;void*r;
unsigned int loc;void*annot;};struct Cyc_Absyn_Skip_s_struct{int tag;};struct Cyc_Absyn_Exp_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_IfThenElse_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_While_s_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Break_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct
_dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};
struct Cyc_Absyn_Switch_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Fallthru_s_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**
f2;};struct Cyc_Absyn_Decl_s_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Label_s_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Do_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;
};struct Cyc_Absyn_TryCatch_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ResetRegion_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct
Cyc_Absyn_Stmt{void*r;unsigned int loc;struct Cyc_List_List*non_local_preds;int
try_depth;void*annot;};struct Cyc_Absyn_Wild_p_struct{int tag;};struct Cyc_Absyn_Var_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_Reference_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_struct{
int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_struct{
int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_struct{int tag;
struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_struct{int tag;struct Cyc_Absyn_AggrInfo*
f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_struct{
int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct
Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_struct{int tag;};struct Cyc_Absyn_Int_p_struct{
int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_struct{int tag;char f1;
};struct Cyc_Absyn_Float_p_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_struct{
int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_struct{
int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_struct{int tag;struct
_tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned int loc;};
struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*
pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned int
loc;};struct Cyc_Absyn_Unresolved_b_struct{int tag;};struct Cyc_Absyn_Global_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_struct{int tag;
struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_struct{int tag;struct Cyc_Absyn_Vardecl*
f1;};struct Cyc_Absyn_Local_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Pat_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};extern struct Cyc_Absyn_Unresolved_b_struct
Cyc_Absyn_Unresolved_b_val;struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct
_tuple0*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;
void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{
enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_List_List*tvs;
void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*
args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*
rgn_po;struct Cyc_Absyn_Stmt*body;void*cached_typ;struct Cyc_Core_Opt*
param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;struct Cyc_List_List*attributes;
};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;
void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;};struct Cyc_Absyn_AggrdeclImpl{
struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*
fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope
sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;
struct Cyc_List_List*attributes;};struct Cyc_Absyn_Datatypefield{struct _tuple0*
name;struct Cyc_List_List*typs;unsigned int loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{
enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*
tag;unsigned int loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct
_tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct
_tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
kind;void*defn;struct Cyc_List_List*atts;};struct Cyc_Absyn_Var_d_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Let_d_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*
f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Letv_d_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_Region_d_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*
f2;int f3;struct Cyc_Absyn_Exp*f4;};struct Cyc_Absyn_Alias_d_struct{int tag;struct
Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Tvar*f2;struct Cyc_Absyn_Vardecl*f3;};struct Cyc_Absyn_Aggr_d_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_struct{int tag;
struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_struct{int tag;struct Cyc_Absyn_Enumdecl*
f1;};struct Cyc_Absyn_Typedef_d_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};
struct Cyc_Absyn_Namespace_d_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Using_d_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ExternC_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Porton_d_struct{
int tag;};struct Cyc_Absyn_Portoff_d_struct{int tag;};struct Cyc_Absyn_Decl{void*r;
unsigned int loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct _dyneither_ptr*f1;};extern
char Cyc_Absyn_EmptyAnnot[11];struct Cyc_Absyn_EmptyAnnot_struct{char*tag;};int Cyc_Absyn_varlist_cmp(
struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(
unsigned int);extern union Cyc_Absyn_Constraint*Cyc_Absyn_false_conref;extern void*
Cyc_Absyn_empty_effect;extern struct _tuple0*Cyc_Absyn_exn_name;extern struct Cyc_Absyn_Datatypedecl*
Cyc_Absyn_exn_tud;void*Cyc_Absyn_dyneither_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual
tq,union Cyc_Absyn_Constraint*zero_term);struct Cyc_Absynpp_Params{int
expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int
rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;
int print_using_stmts;int print_externC_stmts;int print_full_evars;int
print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*
curr_namespace;};struct _dyneither_ptr Cyc_Absynpp_typ2string(void*);struct
_dyneither_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);struct Cyc_RgnOrder_RgnPO;
typedef struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_rgn_po_t;struct Cyc_RgnOrder_RgnPO*
Cyc_RgnOrder_initial_fn_po(struct _RegionHandle*,struct Cyc_List_List*tvs,struct
Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned int);struct
Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct _RegionHandle*,
struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn,unsigned int loc);struct Cyc_RgnOrder_RgnPO*
Cyc_RgnOrder_add_youngest(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,
struct Cyc_Absyn_Tvar*rgn,int resetable,int opened);int Cyc_RgnOrder_is_region_resetable(
struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*r);int Cyc_RgnOrder_effect_outlives(
struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);int Cyc_RgnOrder_satisfies_constraints(
struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*constraints,void*default_bound,
int do_pin);int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*po,void*
eff1,void*eff2);void Cyc_RgnOrder_print_region_po(struct Cyc_RgnOrder_RgnPO*po);
struct Cyc_Tcenv_CList{void*hd;const struct Cyc_Tcenv_CList*tl;};typedef const struct
Cyc_Tcenv_CList*Cyc_Tcenv_mclist_t;typedef const struct Cyc_Tcenv_CList*const Cyc_Tcenv_clist_t;
struct Cyc_Tcenv_VarRes_struct{int tag;void*f1;};struct Cyc_Tcenv_AggrRes_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Tcenv_DatatypeRes_struct{int tag;
struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcenv_EnumRes_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_AnonEnumRes_struct{
int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};typedef void*Cyc_Tcenv_resolved_t;
struct Cyc_Tcenv_Genv{struct _RegionHandle*grgn;struct Cyc_Set_Set*namespaces;
struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict
enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;struct Cyc_List_List*
availables;};typedef struct Cyc_Tcenv_Genv*Cyc_Tcenv_genv_t;struct Cyc_Tcenv_Fenv;
typedef struct Cyc_Tcenv_Fenv*Cyc_Tcenv_fenv_t;struct Cyc_Tcenv_NotLoop_j_struct{
int tag;};struct Cyc_Tcenv_CaseEnd_j_struct{int tag;};struct Cyc_Tcenv_FnEnd_j_struct{
int tag;};struct Cyc_Tcenv_Stmt_j_struct{int tag;struct Cyc_Absyn_Stmt*f1;};extern
struct Cyc_Tcenv_NotLoop_j_struct Cyc_Tcenv_NotLoop_j_val;extern struct Cyc_Tcenv_CaseEnd_j_struct
Cyc_Tcenv_CaseEnd_j_val;extern struct Cyc_Tcenv_FnEnd_j_struct Cyc_Tcenv_FnEnd_j_val;
typedef void*Cyc_Tcenv_jumpee_t;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;
struct Cyc_Dict_Dict ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof;};typedef struct
Cyc_Tcenv_Tenv*Cyc_Tcenv_tenv_t;void*Cyc_Tcenv_env_err(struct _dyneither_ptr msg);
struct _RegionHandle*Cyc_Tcenv_get_fnrgn(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_tc_init(struct _RegionHandle*);struct Cyc_Tcenv_Genv*Cyc_Tcenv_empty_genv(
struct _RegionHandle*);struct Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(struct
_RegionHandle*,unsigned int,struct Cyc_Absyn_Fndecl*);struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(
unsigned int,struct Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*new_fn);struct
Cyc_List_List*Cyc_Tcenv_resolve_namespace(struct Cyc_Tcenv_Tenv*,unsigned int,
struct _dyneither_ptr*,struct Cyc_List_List*);void*Cyc_Tcenv_lookup_ordinary(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple0*);struct
Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*,unsigned int,
struct _tuple0*);struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(
struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple0*);struct Cyc_Absyn_Datatypedecl***
Cyc_Tcenv_lookup_xdatatypedecl(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,
unsigned int,struct _tuple0*);struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(
struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple0*);struct Cyc_Absyn_Typedefdecl*
Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple0*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);
void*Cyc_Tcenv_return_typ(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_local_var(
struct _RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Vardecl*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_pat_var(struct _RegionHandle*,unsigned int,
struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Vardecl*);struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(
struct Cyc_Tcenv_Tenv*);struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct
Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(struct
_RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*);struct
Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_loop(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Absyn_Stmt*continue_dest);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_switch(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*new_tvs,struct
Cyc_List_List*vds,struct Cyc_Absyn_Switch_clause*clause);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_next(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,void*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_try(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_get_try_depth(struct
Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_in_notreadctxt(struct
Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_lhs(struct _RegionHandle*,
struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_lhs(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_in_lhs(struct Cyc_Tcenv_Tenv*
te);void Cyc_Tcenv_process_continue(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,
struct Cyc_Absyn_Stmt**);void Cyc_Tcenv_process_break(struct Cyc_Tcenv_Tenv*,struct
Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt**);void Cyc_Tcenv_process_goto(struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Stmt*,struct _dyneither_ptr*,struct Cyc_Absyn_Stmt**);struct
_tuple10{struct Cyc_Absyn_Switch_clause*f1;struct Cyc_List_List*f2;const struct Cyc_Tcenv_CList*
f3;};const struct _tuple10*const Cyc_Tcenv_process_fallthru(struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Switch_clause***);struct Cyc_Absyn_Stmt*Cyc_Tcenv_get_encloser(
struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_encloser(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_label(struct Cyc_Tcenv_Tenv*,struct _dyneither_ptr*,struct Cyc_Absyn_Stmt*);
int Cyc_Tcenv_all_labels_resolved(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(
struct _RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_new_named_block(struct _RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Tvar*name);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,unsigned int
loc);struct _tuple11{struct Cyc_Absyn_Tvar*f1;void*f2;};struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region_equality(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*r1,void*r2,struct _tuple11**
oldtv,unsigned int loc);void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_region(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,void*r,int
resetable,int opened);void Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*,
unsigned int,void*rgn);void Cyc_Tcenv_check_rgn_resetable(struct Cyc_Tcenv_Tenv*,
unsigned int,void*rgn);void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*
te,unsigned int loc,void*eff);int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*,
void*r1,void*r2);void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,
unsigned int loc,struct Cyc_List_List*po);void Cyc_Tcenv_check_delayed_effects(
struct Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*
te);void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);void
Cyc_Tcutil_terr(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
void Cyc_Tcutil_warn(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
extern struct Cyc_Core_Opt*Cyc_Tcutil_empty_var_set;int Cyc_Tcutil_kind_leq(struct
Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2);struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(
struct Cyc_Absyn_Tvar*t,struct Cyc_Absyn_Kind*def);struct Cyc_Absyn_Kind*Cyc_Tcutil_typ_kind(
void*t);void*Cyc_Tcutil_compress(void*t);extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*
k);struct _tuple11 Cyc_Tcutil_swap_kind(void*t,void*kb);int Cyc_Tcutil_subset_effect(
int may_constrain_evars,void*e1,void*e2);int Cyc_Tcutil_region_in_effect(int
constrain,void*r,void*e);void Cyc_Tcutil_check_unique_tvars(unsigned int,struct
Cyc_List_List*);struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k);int Cyc_Tcutil_new_tvar_id();
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*);void Cyc_Tcutil_add_tvar_identities(
struct Cyc_List_List*);void Cyc_Tcutil_check_no_qual(unsigned int loc,void*t);char
Cyc_Tcenv_Env_error[10]="Env_error";struct Cyc_Tcenv_Env_error_struct{char*tag;};
struct Cyc_Tcenv_Env_error_struct Cyc_Tcenv_Env_error_val={Cyc_Tcenv_Env_error};
void*Cyc_Tcenv_env_err(struct _dyneither_ptr msg){{const char*_tmp361;void*_tmp360[
1];struct Cyc_String_pa_struct _tmp35F;(_tmp35F.tag=0,((_tmp35F.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)msg),((_tmp360[0]=& _tmp35F,Cyc_fprintf(Cyc_stderr,((
_tmp361="Internal error in tcenv: %s\n",_tag_dyneither(_tmp361,sizeof(char),29))),
_tag_dyneither(_tmp360,sizeof(void*),1)))))));}Cyc_fflush((struct Cyc___cycFILE*)
Cyc_stderr);(int)_throw((void*)& Cyc_Tcenv_Env_error_val);}struct Cyc_Tcenv_Tenv;
struct Cyc_Tcenv_Genv;struct Cyc_Tcenv_NotLoop_j_struct Cyc_Tcenv_NotLoop_j_val={0};
struct Cyc_Tcenv_CaseEnd_j_struct Cyc_Tcenv_CaseEnd_j_val={1};struct Cyc_Tcenv_FnEnd_j_struct
Cyc_Tcenv_FnEnd_j_val={2};typedef const struct _tuple10*Cyc_Tcenv_ftclause_t;struct
Cyc_Tcenv_CtrlEnv{struct _RegionHandle*ctrl_rgn;void*continue_stmt;void*
break_stmt;const struct _tuple10*fallthru_clause;void*next_stmt;int try_depth;};
typedef struct Cyc_Tcenv_CtrlEnv*Cyc_Tcenv_ctrl_env_t;struct Cyc_Tcenv_SharedFenv{
struct _RegionHandle*frgn;void*return_typ;struct Cyc_Dict_Dict seen_labels;struct
Cyc_Dict_Dict needed_labels;struct Cyc_List_List*delayed_effect_checks;struct Cyc_List_List*
delayed_constraint_checks;};struct Cyc_Tcenv_Bindings{struct _dyneither_ptr*v;void*
b;const struct Cyc_Tcenv_Bindings*tl;};typedef const struct Cyc_Tcenv_Bindings*const
Cyc_Tcenv_bindings_t;typedef const struct Cyc_Tcenv_Bindings*Cyc_Tcenv_bnds_t;
struct Cyc_Tcenv_Fenv{struct Cyc_Tcenv_SharedFenv*shared;struct Cyc_List_List*
type_vars;struct Cyc_RgnOrder_RgnPO*region_order;const struct Cyc_Tcenv_Bindings*
locals;struct Cyc_Absyn_Stmt*encloser;struct Cyc_Tcenv_CtrlEnv*ctrl_env;void*
capability;void*curr_rgn;int in_notreadctxt;int in_lhs;struct _RegionHandle*fnrgn;};
char Cyc_Tcenv_NoBinding[10]="NoBinding";struct Cyc_Tcenv_NoBinding_struct{char*
tag;};struct Cyc_Tcenv_NoBinding_struct Cyc_Tcenv_NoBinding_val={Cyc_Tcenv_NoBinding};
void*Cyc_Tcenv_lookup_binding(const struct Cyc_Tcenv_Bindings*bs,struct
_dyneither_ptr*v){for(0;(unsigned int)bs;bs=bs->tl){if(Cyc_strptrcmp(v,bs->v)== 
0)return bs->b;}(int)_throw((void*)& Cyc_Tcenv_NoBinding_val);}struct Cyc_Tcenv_Genv*
Cyc_Tcenv_empty_genv(struct _RegionHandle*r){struct Cyc_Tcenv_Genv*_tmp362;return(
_tmp362=_region_malloc(r,sizeof(*_tmp362)),((_tmp362->grgn=r,((_tmp362->namespaces=((
struct Cyc_Set_Set*(*)(struct _RegionHandle*r,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Set_rempty)(r,Cyc_strptrcmp),((_tmp362->aggrdecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp362->datatypedecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp362->enumdecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp362->typedefs=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp362->ordinaries=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp362->availables=
0,_tmp362)))))))))))))))));}struct _tuple12{void*f1;int f2;};struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_tc_init(struct _RegionHandle*r){{struct Cyc_Core_Opt*_tmp363;Cyc_Tcutil_empty_var_set=((
_tmp363=_cycalloc(sizeof(*_tmp363)),((_tmp363->v=((struct Cyc_Set_Set*(*)(int(*
cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),
_tmp363))));}{struct Cyc_Tcenv_Genv*_tmpA=Cyc_Tcenv_empty_genv(r);{struct Cyc_Absyn_Datatypedecl**
_tmp364;_tmpA->datatypedecls=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,
struct _dyneither_ptr*k,struct Cyc_Absyn_Datatypedecl**v))Cyc_Dict_insert)(_tmpA->datatypedecls,(*
Cyc_Absyn_exn_name).f2,((_tmp364=_cycalloc(sizeof(*_tmp364)),((_tmp364[0]=Cyc_Absyn_exn_tud,
_tmp364)))));}{struct Cyc_List_List*_tmpC=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)
_check_null(Cyc_Absyn_exn_tud->fields))->v;for(0;_tmpC != 0;_tmpC=_tmpC->tl){
struct Cyc_Tcenv_DatatypeRes_struct*_tmp36A;struct Cyc_Tcenv_DatatypeRes_struct
_tmp369;struct _tuple12*_tmp368;_tmpA->ordinaries=((struct Cyc_Dict_Dict(*)(struct
Cyc_Dict_Dict d,struct _dyneither_ptr*k,struct _tuple12*v))Cyc_Dict_insert)(_tmpA->ordinaries,(*((
struct Cyc_Absyn_Datatypefield*)_tmpC->hd)->name).f2,((_tmp368=_region_malloc(r,
sizeof(*_tmp368)),((_tmp368->f1=(void*)((_tmp36A=_cycalloc(sizeof(*_tmp36A)),((
_tmp36A[0]=((_tmp369.tag=2,((_tmp369.f1=Cyc_Absyn_exn_tud,((_tmp369.f2=(struct
Cyc_Absyn_Datatypefield*)_tmpC->hd,_tmp369)))))),_tmp36A)))),((_tmp368->f2=1,
_tmp368)))))));}}{struct Cyc_Dict_Dict ae=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k,struct Cyc_Tcenv_Genv*v))Cyc_Dict_insert)(((struct Cyc_Dict_Dict(*)(
struct _RegionHandle*,int(*cmp)(struct Cyc_List_List*,struct Cyc_List_List*)))Cyc_Dict_rempty)(
r,Cyc_Absyn_varlist_cmp),0,_tmpA);struct Cyc_Tcenv_Tenv*_tmp36B;return(_tmp36B=
_region_malloc(r,sizeof(*_tmp36B)),((_tmp36B->ns=0,((_tmp36B->ae=ae,((_tmp36B->le=
0,((_tmp36B->allow_valueof=0,_tmp36B)))))))));};};}static struct Cyc_Tcenv_Genv*
Cyc_Tcenv_lookup_namespace(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct
_dyneither_ptr*n,struct Cyc_List_List*ns){return((struct Cyc_Tcenv_Genv*(*)(struct
Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,Cyc_Tcenv_resolve_namespace(
te,loc,n,ns));}static struct Cyc_List_List*Cyc_Tcenv_outer_namespace(struct Cyc_List_List*
ns){if(ns == 0){const char*_tmp36C;return((struct Cyc_List_List*(*)(struct
_dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp36C="outer_namespace",_tag_dyneither(
_tmp36C,sizeof(char),16))));}return((struct Cyc_List_List*(*)(struct Cyc_List_List*
x))Cyc_List_rev)(((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_rev)(ns)))->tl);}static int Cyc_Tcenv_same_namespace(
struct Cyc_List_List*n1,struct Cyc_List_List*n2){if(n1 == n2)return 1;for(0;n1 != 0;
n1=n1->tl){if(n2 == 0)return 0;if(Cyc_strptrcmp((struct _dyneither_ptr*)n1->hd,(
struct _dyneither_ptr*)n2->hd)!= 0)return 0;n2=n2->tl;}if(n2 != 0)return 0;return 1;}
static void Cyc_Tcenv_check_repeat(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*(*
lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v,
struct Cyc_List_List*cns,struct Cyc_List_List*nss){for(0;nss != 0;nss=nss->tl){if(!
Cyc_Tcenv_same_namespace(cns,(struct Cyc_List_List*)nss->hd)){struct Cyc_Tcenv_Genv*
ge2=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,(struct Cyc_List_List*)nss->hd);struct _handler_cons _tmp12;_push_handler(&
_tmp12);{int _tmp14=0;if(setjmp(_tmp12.handler))_tmp14=1;if(!_tmp14){lookup(ge2,v);{
const char*_tmp370;void*_tmp36F[1];struct Cyc_String_pa_struct _tmp36E;(_tmp36E.tag=
0,((_tmp36E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*v),((_tmp36F[0]=&
_tmp36E,Cyc_Tcutil_terr(loc,((_tmp370="%s is ambiguous",_tag_dyneither(_tmp370,
sizeof(char),16))),_tag_dyneither(_tmp36F,sizeof(void*),1)))))));};;_pop_handler();}
else{void*_tmp13=(void*)_exn_thrown;void*_tmp19=_tmp13;_LL1: {struct Cyc_Dict_Absent_struct*
_tmp1A=(struct Cyc_Dict_Absent_struct*)_tmp19;if(_tmp1A->tag != Cyc_Dict_Absent)
goto _LL3;}_LL2: goto _LL0;_LL3:;_LL4:(void)_throw(_tmp19);_LL0:;}};}}return;}
static void*Cyc_Tcenv_scoped_lookup(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*(*
lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v){
struct Cyc_List_List*cns=te->ns;while(1){struct Cyc_Tcenv_Genv*ge=((struct Cyc_Tcenv_Genv*(*)(
struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,cns);{struct
_handler_cons _tmp1B;_push_handler(& _tmp1B);{int _tmp1D=0;if(setjmp(_tmp1B.handler))
_tmp1D=1;if(!_tmp1D){{void*result=lookup(ge,v);Cyc_Tcenv_check_repeat(te,loc,
lookup,v,cns,ge->availables);{void*_tmp1E=result;_npop_handler(0);return _tmp1E;};};
_pop_handler();}else{void*_tmp1C=(void*)_exn_thrown;void*_tmp20=_tmp1C;_LL6: {
struct Cyc_Dict_Absent_struct*_tmp21=(struct Cyc_Dict_Absent_struct*)_tmp20;if(
_tmp21->tag != Cyc_Dict_Absent)goto _LL8;}_LL7: goto _LL5;_LL8:;_LL9:(void)_throw(
_tmp20);_LL5:;}};}{struct Cyc_List_List*_tmp22=ge->availables;for(0;_tmp22 != 0;
_tmp22=_tmp22->tl){struct Cyc_Tcenv_Genv*ge2=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,(struct Cyc_List_List*)_tmp22->hd);
struct _handler_cons _tmp23;_push_handler(& _tmp23);{int _tmp25=0;if(setjmp(_tmp23.handler))
_tmp25=1;if(!_tmp25){{void*result=lookup(ge2,v);Cyc_Tcenv_check_repeat(te,loc,
lookup,v,(struct Cyc_List_List*)_tmp22->hd,_tmp22->tl);{void*_tmp26=result;
_npop_handler(0);return _tmp26;};};_pop_handler();}else{void*_tmp24=(void*)
_exn_thrown;void*_tmp28=_tmp24;_LLB: {struct Cyc_Dict_Absent_struct*_tmp29=(
struct Cyc_Dict_Absent_struct*)_tmp28;if(_tmp29->tag != Cyc_Dict_Absent)goto _LLD;}
_LLC: goto _LLA;_LLD:;_LLE:(void)_throw(_tmp28);_LLA:;}};}}if(cns == 0){struct Cyc_Dict_Absent_struct
_tmp373;struct Cyc_Dict_Absent_struct*_tmp372;(int)_throw((void*)((_tmp372=
_cycalloc_atomic(sizeof(*_tmp372)),((_tmp372[0]=((_tmp373.tag=Cyc_Dict_Absent,
_tmp373)),_tmp372)))));}cns=Cyc_Tcenv_outer_namespace(cns);}}static void*Cyc_Tcenv_lookup_ordinary_global_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp2C=ge->ordinaries;
struct _tuple12*ans=((struct _tuple12*(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp2C,v);(*ans).f2=1;return(*ans).f1;}static
void*Cyc_Tcenv_lookup_ordinary_global(struct Cyc_Tcenv_Tenv*te,unsigned int loc,
struct _tuple0*q){struct _tuple0 _tmp2E;union Cyc_Absyn_Nmspace _tmp2F;struct
_dyneither_ptr*_tmp30;struct _tuple0*_tmp2D=q;_tmp2E=*_tmp2D;_tmp2F=_tmp2E.f1;
_tmp30=_tmp2E.f2;{union Cyc_Absyn_Nmspace _tmp31=_tmp2F;int _tmp32;struct Cyc_List_List*
_tmp33;struct Cyc_List_List*_tmp34;struct Cyc_List_List _tmp35;struct _dyneither_ptr*
_tmp36;struct Cyc_List_List*_tmp37;struct Cyc_List_List*_tmp38;struct Cyc_List_List*
_tmp39;_LL10: if((_tmp31.Loc_n).tag != 4)goto _LL12;_tmp32=(int)(_tmp31.Loc_n).val;
_LL11: goto _LL13;_LL12: if((_tmp31.Rel_n).tag != 1)goto _LL14;_tmp33=(struct Cyc_List_List*)(
_tmp31.Rel_n).val;if(_tmp33 != 0)goto _LL14;_LL13: return((void*(*)(struct Cyc_Tcenv_Tenv*
te,unsigned int loc,void*(*lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),
struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_ordinary_global_f,
_tmp30);_LL14: if((_tmp31.Rel_n).tag != 1)goto _LL16;_tmp34=(struct Cyc_List_List*)(
_tmp31.Rel_n).val;if(_tmp34 == 0)goto _LL16;_tmp35=*_tmp34;_tmp36=(struct
_dyneither_ptr*)_tmp35.hd;_tmp37=_tmp35.tl;_LL15: {struct Cyc_Tcenv_Genv*_tmp3A=
Cyc_Tcenv_lookup_namespace(te,loc,_tmp36,_tmp37);return Cyc_Tcenv_lookup_ordinary_global_f(
_tmp3A,_tmp30);}_LL16: if((_tmp31.C_n).tag != 3)goto _LL18;_tmp38=(struct Cyc_List_List*)(
_tmp31.C_n).val;_LL17: _tmp39=_tmp38;goto _LL19;_LL18: if((_tmp31.Abs_n).tag != 2)
goto _LLF;_tmp39=(struct Cyc_List_List*)(_tmp31.Abs_n).val;_LL19: return Cyc_Tcenv_lookup_ordinary_global_f(((
struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,_tmp39),_tmp30);_LLF:;};}struct Cyc_List_List*Cyc_Tcenv_resolve_namespace(
struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct _dyneither_ptr*n,struct Cyc_List_List*
ns){struct Cyc_List_List*_tmp3B=te->ns;struct _RegionHandle _tmp3C=_new_region("temp");
struct _RegionHandle*temp=& _tmp3C;_push_region(temp);{struct Cyc_List_List*_tmp3D=
0;while(1){struct Cyc_Tcenv_Genv*_tmp3E=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp3B);struct Cyc_List_List*
_tmp3F=_tmp3E->availables;struct Cyc_Set_Set*_tmp40=_tmp3E->namespaces;{struct Cyc_List_List*
_tmp41=_tmp3F;for(0;_tmp41 != 0;_tmp41=_tmp41->tl){struct Cyc_Set_Set*_tmp42=(((
struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,(struct Cyc_List_List*)_tmp41->hd))->namespaces;if(((int(*)(struct Cyc_Set_Set*
s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp42,n)){struct Cyc_List_List*
_tmp376;struct Cyc_List_List*_tmp375;_tmp3D=((_tmp375=_region_malloc(temp,sizeof(*
_tmp375)),((_tmp375->hd=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct
Cyc_List_List*y))Cyc_List_append)(_tmp3B,((_tmp376=_cycalloc(sizeof(*_tmp376)),((
_tmp376->hd=n,((_tmp376->tl=ns,_tmp376))))))),((_tmp375->tl=_tmp3D,_tmp375))))));}}}
if(((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp40,
n)){struct Cyc_List_List*_tmp379;struct Cyc_List_List*_tmp378;_tmp3D=((_tmp378=
_region_malloc(temp,sizeof(*_tmp378)),((_tmp378->hd=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(_tmp3B,((_tmp379=
_cycalloc(sizeof(*_tmp379)),((_tmp379->hd=n,((_tmp379->tl=ns,_tmp379))))))),((
_tmp378->tl=_tmp3D,_tmp378))))));}if(_tmp3D != 0){if(_tmp3D->tl != 0){const char*
_tmp37D;void*_tmp37C[1];struct Cyc_String_pa_struct _tmp37B;(_tmp37B.tag=0,((
_tmp37B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*n),((_tmp37C[0]=&
_tmp37B,Cyc_Tcutil_terr(loc,((_tmp37D="%s is ambiguous",_tag_dyneither(_tmp37D,
sizeof(char),16))),_tag_dyneither(_tmp37C,sizeof(void*),1)))))));}{struct Cyc_List_List*
_tmp4A=(struct Cyc_List_List*)_tmp3D->hd;_npop_handler(0);return _tmp4A;};}if(
_tmp3B == 0){struct Cyc_Dict_Absent_struct _tmp380;struct Cyc_Dict_Absent_struct*
_tmp37F;(int)_throw((void*)((_tmp37F=_cycalloc_atomic(sizeof(*_tmp37F)),((
_tmp37F[0]=((_tmp380.tag=Cyc_Dict_Absent,_tmp380)),_tmp37F)))));}_tmp3B=Cyc_Tcenv_outer_namespace(
_tmp3B);}};_pop_region(temp);}static struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp4D=ge->aggrdecls;
return((struct Cyc_Absyn_Aggrdecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*
k))Cyc_Dict_lookup)(_tmp4D,v);}struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(
struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct _tuple0*q){struct _tuple0 _tmp4F;
union Cyc_Absyn_Nmspace _tmp50;struct _dyneither_ptr*_tmp51;struct _tuple0*_tmp4E=q;
_tmp4F=*_tmp4E;_tmp50=_tmp4F.f1;_tmp51=_tmp4F.f2;{union Cyc_Absyn_Nmspace _tmp52=
_tmp50;int _tmp53;struct Cyc_List_List*_tmp54;struct Cyc_List_List*_tmp55;struct Cyc_List_List*
_tmp56;struct Cyc_List_List*_tmp57;struct Cyc_List_List _tmp58;struct _dyneither_ptr*
_tmp59;struct Cyc_List_List*_tmp5A;_LL1B: if((_tmp52.Loc_n).tag != 4)goto _LL1D;
_tmp53=(int)(_tmp52.Loc_n).val;_LL1C: goto _LL1E;_LL1D: if((_tmp52.Rel_n).tag != 1)
goto _LL1F;_tmp54=(struct Cyc_List_List*)(_tmp52.Rel_n).val;if(_tmp54 != 0)goto
_LL1F;_LL1E: return((struct Cyc_Absyn_Aggrdecl**(*)(struct Cyc_Tcenv_Tenv*te,
unsigned int loc,struct Cyc_Absyn_Aggrdecl**(*lookup)(struct Cyc_Tcenv_Genv*,struct
_dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_aggrdecl_f,
_tmp51);_LL1F: if((_tmp52.C_n).tag != 3)goto _LL21;_tmp55=(struct Cyc_List_List*)(
_tmp52.C_n).val;_LL20: _tmp56=_tmp55;goto _LL22;_LL21: if((_tmp52.Abs_n).tag != 2)
goto _LL23;_tmp56=(struct Cyc_List_List*)(_tmp52.Abs_n).val;_LL22: {struct Cyc_Dict_Dict
_tmp5B=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))
Cyc_Dict_lookup)(te->ae,_tmp56))->aggrdecls;return((struct Cyc_Absyn_Aggrdecl**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp5B,_tmp51);}
_LL23: if((_tmp52.Rel_n).tag != 1)goto _LL1A;_tmp57=(struct Cyc_List_List*)(_tmp52.Rel_n).val;
if(_tmp57 == 0)goto _LL1A;_tmp58=*_tmp57;_tmp59=(struct _dyneither_ptr*)_tmp58.hd;
_tmp5A=_tmp58.tl;_LL24: {struct Cyc_Dict_Dict _tmp5C=(Cyc_Tcenv_lookup_namespace(
te,loc,_tmp59,_tmp5A))->aggrdecls;return((struct Cyc_Absyn_Aggrdecl**(*)(struct
Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp5C,_tmp51);}_LL1A:;};}
static struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl_f(struct Cyc_Tcenv_Genv*
ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp5D=ge->datatypedecls;return((
struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))
Cyc_Dict_lookup)(_tmp5D,v);}struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(
struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct _tuple0*q){struct _tuple0 _tmp5F;
union Cyc_Absyn_Nmspace _tmp60;struct _dyneither_ptr*_tmp61;struct _tuple0*_tmp5E=q;
_tmp5F=*_tmp5E;_tmp60=_tmp5F.f1;_tmp61=_tmp5F.f2;{union Cyc_Absyn_Nmspace _tmp62=
_tmp60;int _tmp63;struct Cyc_List_List*_tmp64;struct Cyc_List_List*_tmp65;struct Cyc_List_List
_tmp66;struct _dyneither_ptr*_tmp67;struct Cyc_List_List*_tmp68;struct Cyc_List_List*
_tmp69;struct Cyc_List_List*_tmp6A;_LL26: if((_tmp62.Loc_n).tag != 4)goto _LL28;
_tmp63=(int)(_tmp62.Loc_n).val;_LL27: goto _LL29;_LL28: if((_tmp62.Rel_n).tag != 1)
goto _LL2A;_tmp64=(struct Cyc_List_List*)(_tmp62.Rel_n).val;if(_tmp64 != 0)goto
_LL2A;_LL29: return((struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Tcenv_Tenv*te,
unsigned int loc,struct Cyc_Absyn_Datatypedecl**(*lookup)(struct Cyc_Tcenv_Genv*,
struct _dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,
Cyc_Tcenv_lookup_datatypedecl_f,_tmp61);_LL2A: if((_tmp62.Rel_n).tag != 1)goto
_LL2C;_tmp65=(struct Cyc_List_List*)(_tmp62.Rel_n).val;if(_tmp65 == 0)goto _LL2C;
_tmp66=*_tmp65;_tmp67=(struct _dyneither_ptr*)_tmp66.hd;_tmp68=_tmp66.tl;_LL2B: {
struct Cyc_Dict_Dict _tmp6B=(Cyc_Tcenv_lookup_namespace(te,loc,_tmp67,_tmp68))->datatypedecls;
return((struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp6B,_tmp61);}_LL2C: if((_tmp62.C_n).tag != 3)
goto _LL2E;_tmp69=(struct Cyc_List_List*)(_tmp62.C_n).val;_LL2D: _tmp6A=_tmp69;goto
_LL2F;_LL2E: if((_tmp62.Abs_n).tag != 2)goto _LL25;_tmp6A=(struct Cyc_List_List*)(
_tmp62.Abs_n).val;_LL2F: {struct Cyc_Dict_Dict _tmp6C=(((struct Cyc_Tcenv_Genv*(*)(
struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp6A))->datatypedecls;
return((struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp6C,_tmp61);}_LL25:;};}static struct Cyc_Absyn_Datatypedecl**
Cyc_Tcenv_lookup_xdatatypedecl_f(struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){
return((struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(ge->datatypedecls,v);}struct Cyc_Absyn_Datatypedecl***
Cyc_Tcenv_lookup_xdatatypedecl(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,
unsigned int loc,struct _tuple0*q){struct _tuple0 _tmp6E;union Cyc_Absyn_Nmspace
_tmp6F;struct _dyneither_ptr*_tmp70;struct _tuple0*_tmp6D=q;_tmp6E=*_tmp6D;_tmp6F=
_tmp6E.f1;_tmp70=_tmp6E.f2;{union Cyc_Absyn_Nmspace _tmp71=_tmp6F;struct Cyc_List_List*
_tmp72;int _tmp73;struct Cyc_List_List*_tmp74;struct Cyc_List_List _tmp75;struct
_dyneither_ptr*_tmp76;struct Cyc_List_List*_tmp77;struct Cyc_List_List*_tmp78;
struct Cyc_List_List*_tmp79;_LL31: if((_tmp71.Rel_n).tag != 1)goto _LL33;_tmp72=(
struct Cyc_List_List*)(_tmp71.Rel_n).val;if(_tmp72 != 0)goto _LL33;_LL32: {struct
_handler_cons _tmp7A;_push_handler(& _tmp7A);{int _tmp7C=0;if(setjmp(_tmp7A.handler))
_tmp7C=1;if(!_tmp7C){{struct Cyc_Absyn_Datatypedecl***_tmp381;struct Cyc_Absyn_Datatypedecl***
_tmp7E=(_tmp381=_region_malloc(r,sizeof(*_tmp381)),((_tmp381[0]=((struct Cyc_Absyn_Datatypedecl**(*)(
struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Datatypedecl**(*lookup)(
struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(
te,loc,Cyc_Tcenv_lookup_xdatatypedecl_f,_tmp70),_tmp381)));_npop_handler(0);
return _tmp7E;};_pop_handler();}else{void*_tmp7B=(void*)_exn_thrown;void*_tmp80=
_tmp7B;_LL3C: {struct Cyc_Dict_Absent_struct*_tmp81=(struct Cyc_Dict_Absent_struct*)
_tmp80;if(_tmp81->tag != Cyc_Dict_Absent)goto _LL3E;}_LL3D: return 0;_LL3E:;_LL3F:(
void)_throw(_tmp80);_LL3B:;}};}_LL33: if((_tmp71.Loc_n).tag != 4)goto _LL35;_tmp73=(
int)(_tmp71.Loc_n).val;_LL34:{const char*_tmp384;void*_tmp383;(_tmp383=0,Cyc_Tcutil_terr(
loc,((_tmp384="lookup_xdatatypedecl: impossible",_tag_dyneither(_tmp384,sizeof(
char),33))),_tag_dyneither(_tmp383,sizeof(void*),0)));}return 0;_LL35: if((_tmp71.Rel_n).tag
!= 1)goto _LL37;_tmp74=(struct Cyc_List_List*)(_tmp71.Rel_n).val;if(_tmp74 == 0)
goto _LL37;_tmp75=*_tmp74;_tmp76=(struct _dyneither_ptr*)_tmp75.hd;_tmp77=_tmp75.tl;
_LL36: {struct Cyc_Tcenv_Genv*ge;{struct _handler_cons _tmp84;_push_handler(& _tmp84);{
int _tmp86=0;if(setjmp(_tmp84.handler))_tmp86=1;if(!_tmp86){ge=Cyc_Tcenv_lookup_namespace(
te,loc,_tmp76,_tmp77);;_pop_handler();}else{void*_tmp85=(void*)_exn_thrown;void*
_tmp88=_tmp85;_LL41: {struct Cyc_Dict_Absent_struct*_tmp89=(struct Cyc_Dict_Absent_struct*)
_tmp88;if(_tmp89->tag != Cyc_Dict_Absent)goto _LL43;}_LL42:{const char*_tmp387;void*
_tmp386;(_tmp386=0,Cyc_Tcutil_terr(loc,((_tmp387="bad qualified name for @extensible datatype",
_tag_dyneither(_tmp387,sizeof(char),44))),_tag_dyneither(_tmp386,sizeof(void*),0)));}{
struct Cyc_Dict_Absent_struct _tmp38A;struct Cyc_Dict_Absent_struct*_tmp389;(int)
_throw((void*)((_tmp389=_cycalloc_atomic(sizeof(*_tmp389)),((_tmp389[0]=((
_tmp38A.tag=Cyc_Dict_Absent,_tmp38A)),_tmp389)))));};_LL43:;_LL44:(void)_throw(
_tmp88);_LL40:;}};}{struct Cyc_Dict_Dict _tmp8E=ge->datatypedecls;struct Cyc_Absyn_Datatypedecl***
_tmp38B;return(_tmp38B=_region_malloc(r,sizeof(*_tmp38B)),((_tmp38B[0]=((struct
Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp8E,_tmp70),_tmp38B)));};}_LL37: if((_tmp71.C_n).tag != 3)goto _LL39;_tmp78=(
struct Cyc_List_List*)(_tmp71.C_n).val;_LL38: _tmp79=_tmp78;goto _LL3A;_LL39: if((
_tmp71.Abs_n).tag != 2)goto _LL30;_tmp79=(struct Cyc_List_List*)(_tmp71.Abs_n).val;
_LL3A: {struct Cyc_Dict_Dict _tmp90=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp79))->datatypedecls;struct
Cyc_Absyn_Datatypedecl***_tmp38C;return(_tmp38C=_region_malloc(r,sizeof(*_tmp38C)),((
_tmp38C[0]=((struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp90,_tmp70),_tmp38C)));}_LL30:;};}static
struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl_f(struct Cyc_Tcenv_Genv*ge,
struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp92=ge->enumdecls;return((struct
Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp92,v);}struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*
te,unsigned int loc,struct _tuple0*q){struct _tuple0 _tmp94;union Cyc_Absyn_Nmspace
_tmp95;struct _dyneither_ptr*_tmp96;struct _tuple0*_tmp93=q;_tmp94=*_tmp93;_tmp95=
_tmp94.f1;_tmp96=_tmp94.f2;{union Cyc_Absyn_Nmspace _tmp97=_tmp95;int _tmp98;struct
Cyc_List_List*_tmp99;struct Cyc_List_List*_tmp9A;struct Cyc_List_List _tmp9B;struct
_dyneither_ptr*_tmp9C;struct Cyc_List_List*_tmp9D;struct Cyc_List_List*_tmp9E;
struct Cyc_List_List*_tmp9F;_LL46: if((_tmp97.Loc_n).tag != 4)goto _LL48;_tmp98=(int)(
_tmp97.Loc_n).val;_LL47: goto _LL49;_LL48: if((_tmp97.Rel_n).tag != 1)goto _LL4A;
_tmp99=(struct Cyc_List_List*)(_tmp97.Rel_n).val;if(_tmp99 != 0)goto _LL4A;_LL49:
return((struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Tcenv_Tenv*te,unsigned int loc,
struct Cyc_Absyn_Enumdecl**(*lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),
struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_enumdecl_f,
_tmp96);_LL4A: if((_tmp97.Rel_n).tag != 1)goto _LL4C;_tmp9A=(struct Cyc_List_List*)(
_tmp97.Rel_n).val;if(_tmp9A == 0)goto _LL4C;_tmp9B=*_tmp9A;_tmp9C=(struct
_dyneither_ptr*)_tmp9B.hd;_tmp9D=_tmp9B.tl;_LL4B: {struct Cyc_Dict_Dict _tmpA0=(
Cyc_Tcenv_lookup_namespace(te,loc,_tmp9C,_tmp9D))->enumdecls;return((struct Cyc_Absyn_Enumdecl**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmpA0,_tmp96);}
_LL4C: if((_tmp97.C_n).tag != 3)goto _LL4E;_tmp9E=(struct Cyc_List_List*)(_tmp97.C_n).val;
_LL4D: _tmp9F=_tmp9E;goto _LL4F;_LL4E: if((_tmp97.Abs_n).tag != 2)goto _LL45;_tmp9F=(
struct Cyc_List_List*)(_tmp97.Abs_n).val;_LL4F: {struct Cyc_Dict_Dict _tmpA1=(((
struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,_tmp9F))->enumdecls;return((struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict
d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmpA1,_tmp96);}_LL45:;};}static
struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl_f(struct Cyc_Tcenv_Genv*
ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmpA2=ge->typedefs;return((struct
Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmpA2,v);}struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*
te,unsigned int loc,struct _tuple0*q){struct _tuple0 _tmpA4;union Cyc_Absyn_Nmspace
_tmpA5;struct _dyneither_ptr*_tmpA6;struct _tuple0*_tmpA3=q;_tmpA4=*_tmpA3;_tmpA5=
_tmpA4.f1;_tmpA6=_tmpA4.f2;{union Cyc_Absyn_Nmspace _tmpA7=_tmpA5;int _tmpA8;struct
Cyc_List_List*_tmpA9;struct Cyc_List_List*_tmpAA;struct Cyc_List_List _tmpAB;struct
_dyneither_ptr*_tmpAC;struct Cyc_List_List*_tmpAD;struct Cyc_List_List*_tmpAE;
struct Cyc_List_List*_tmpAF;_LL51: if((_tmpA7.Loc_n).tag != 4)goto _LL53;_tmpA8=(int)(
_tmpA7.Loc_n).val;_LL52: goto _LL54;_LL53: if((_tmpA7.Rel_n).tag != 1)goto _LL55;
_tmpA9=(struct Cyc_List_List*)(_tmpA7.Rel_n).val;if(_tmpA9 != 0)goto _LL55;_LL54:
return((struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Tcenv_Tenv*te,unsigned int loc,
struct Cyc_Absyn_Typedefdecl*(*lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),
struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_typedefdecl_f,
_tmpA6);_LL55: if((_tmpA7.Rel_n).tag != 1)goto _LL57;_tmpAA=(struct Cyc_List_List*)(
_tmpA7.Rel_n).val;if(_tmpAA == 0)goto _LL57;_tmpAB=*_tmpAA;_tmpAC=(struct
_dyneither_ptr*)_tmpAB.hd;_tmpAD=_tmpAB.tl;_LL56: {struct Cyc_Dict_Dict _tmpB0=(
Cyc_Tcenv_lookup_namespace(te,loc,_tmpAC,_tmpAD))->typedefs;return((struct Cyc_Absyn_Typedefdecl*(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmpB0,_tmpA6);}
_LL57: if((_tmpA7.C_n).tag != 3)goto _LL59;_tmpAE=(struct Cyc_List_List*)(_tmpA7.C_n).val;
_LL58: _tmpAF=_tmpAE;goto _LL5A;_LL59: if((_tmpA7.Abs_n).tag != 2)goto _LL50;_tmpAF=(
struct Cyc_List_List*)(_tmpA7.Abs_n).val;_LL5A: {struct Cyc_Dict_Dict _tmpB1=(((
struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,_tmpAF))->typedefs;return((struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict
d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmpB1,_tmpA6);}_LL50:;};}static
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_get_fenv(struct Cyc_Tcenv_Tenv*te,struct
_dyneither_ptr err_msg){struct Cyc_Tcenv_Fenv*_tmpB2=te->le;if((struct Cyc_Tcenv_Tenv*)
te == 0)((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(err_msg);return(
struct Cyc_Tcenv_Fenv*)_check_null(_tmpB2);}struct _RegionHandle*Cyc_Tcenv_coerce_heap_region(){
return(struct _RegionHandle*)Cyc_Core_heap_region;}struct _RegionHandle*Cyc_Tcenv_get_fnrgn(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpB3=te->le;if(_tmpB3 != 0){
struct Cyc_Tcenv_Fenv _tmpB5;struct _RegionHandle*_tmpB6;struct Cyc_Tcenv_Fenv*
_tmpB4=(struct Cyc_Tcenv_Fenv*)_tmpB3;_tmpB5=*_tmpB4;_tmpB6=_tmpB5.fnrgn;return
_tmpB6;}return Cyc_Tcenv_coerce_heap_region();}static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_fenv(
struct _RegionHandle*l,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Fenv*fe){if(te->le
== 0){const char*_tmp38D;((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((
_tmp38D="put_fenv",_tag_dyneither(_tmp38D,sizeof(char),9))));}{struct Cyc_Tcenv_Tenv*
_tmp38E;return(_tmp38E=_region_malloc(l,sizeof(*_tmp38E)),((_tmp38E->ns=te->ns,((
_tmp38E->ae=te->ae,((_tmp38E->le=(struct Cyc_Tcenv_Fenv*)fe,((_tmp38E->allow_valueof=
te->allow_valueof,_tmp38E)))))))));};}static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_emptyfenv(
struct _RegionHandle*l,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Tenv*_tmp38F;
return(_tmp38F=_region_malloc(l,sizeof(*_tmp38F)),((_tmp38F->ns=te->ns,((_tmp38F->ae=
te->ae,((_tmp38F->le=0,((_tmp38F->allow_valueof=te->allow_valueof,_tmp38F)))))))));}
static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_old_ctrl(struct _RegionHandle*l,
struct Cyc_Tcenv_Fenv*f){struct Cyc_Tcenv_SharedFenv*_tmpBB;struct Cyc_List_List*
_tmpBC;struct Cyc_RgnOrder_RgnPO*_tmpBD;const struct Cyc_Tcenv_Bindings*_tmpBE;
struct Cyc_Absyn_Stmt*_tmpBF;struct Cyc_Tcenv_CtrlEnv*_tmpC0;void*_tmpC1;void*
_tmpC2;int _tmpC3;int _tmpC4;struct _RegionHandle*_tmpC5;struct Cyc_Tcenv_Fenv _tmpBA=*
f;_tmpBB=_tmpBA.shared;_tmpBC=_tmpBA.type_vars;_tmpBD=_tmpBA.region_order;_tmpBE=
_tmpBA.locals;_tmpBF=_tmpBA.encloser;_tmpC0=_tmpBA.ctrl_env;_tmpC1=_tmpBA.capability;
_tmpC2=_tmpBA.curr_rgn;_tmpC3=_tmpBA.in_notreadctxt;_tmpC4=_tmpBA.in_lhs;_tmpC5=
_tmpBA.fnrgn;{struct Cyc_Tcenv_Fenv*_tmp390;return(_tmp390=_region_malloc(l,
sizeof(*_tmp390)),((_tmp390->shared=(struct Cyc_Tcenv_SharedFenv*)_tmpBB,((
_tmp390->type_vars=(struct Cyc_List_List*)_tmpBC,((_tmp390->region_order=(struct
Cyc_RgnOrder_RgnPO*)_tmpBD,((_tmp390->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmpBE),((_tmp390->encloser=(struct Cyc_Absyn_Stmt*)
_tmpBF,((_tmp390->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmpC0),((_tmp390->capability=(void*)_tmpC1,((_tmp390->curr_rgn=(void*)_tmpC2,((
_tmp390->in_notreadctxt=(int)_tmpC3,((_tmp390->in_lhs=(int)_tmpC4,((_tmp390->fnrgn=(
struct _RegionHandle*)l,(struct Cyc_Tcenv_Fenv*)_tmp390)))))))))))))))))))))));};}
static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_new_ctrl(struct _RegionHandle*l,
struct Cyc_Tcenv_Fenv*f){struct Cyc_Tcenv_SharedFenv*_tmpC8;struct Cyc_List_List*
_tmpC9;struct Cyc_RgnOrder_RgnPO*_tmpCA;const struct Cyc_Tcenv_Bindings*_tmpCB;
struct Cyc_Absyn_Stmt*_tmpCC;struct Cyc_Tcenv_CtrlEnv*_tmpCD;void*_tmpCE;void*
_tmpCF;int _tmpD0;int _tmpD1;struct Cyc_Tcenv_Fenv _tmpC7=*f;_tmpC8=_tmpC7.shared;
_tmpC9=_tmpC7.type_vars;_tmpCA=_tmpC7.region_order;_tmpCB=_tmpC7.locals;_tmpCC=
_tmpC7.encloser;_tmpCD=_tmpC7.ctrl_env;_tmpCE=_tmpC7.capability;_tmpCF=_tmpC7.curr_rgn;
_tmpD0=_tmpC7.in_notreadctxt;_tmpD1=_tmpC7.in_lhs;{struct _RegionHandle*_tmpD3;
void*_tmpD4;void*_tmpD5;const struct _tuple10*_tmpD6;void*_tmpD7;int _tmpD8;struct
Cyc_Tcenv_CtrlEnv _tmpD2=*_tmpCD;_tmpD3=_tmpD2.ctrl_rgn;_tmpD4=_tmpD2.continue_stmt;
_tmpD5=_tmpD2.break_stmt;_tmpD6=_tmpD2.fallthru_clause;_tmpD7=_tmpD2.next_stmt;
_tmpD8=_tmpD2.try_depth;{struct Cyc_Tcenv_CtrlEnv*_tmp391;struct Cyc_Tcenv_CtrlEnv*
_tmpD9=(_tmp391=_region_malloc(l,sizeof(*_tmp391)),((_tmp391->ctrl_rgn=_tmpD3,((
_tmp391->continue_stmt=_tmpD4,((_tmp391->break_stmt=_tmpD5,((_tmp391->fallthru_clause=
_tmpD6,((_tmp391->next_stmt=_tmpD7,((_tmp391->try_depth=_tmpD8,_tmp391)))))))))))));
struct Cyc_Tcenv_Fenv*_tmp392;return(_tmp392=_region_malloc(l,sizeof(*_tmp392)),((
_tmp392->shared=(struct Cyc_Tcenv_SharedFenv*)_tmpC8,((_tmp392->type_vars=(struct
Cyc_List_List*)_tmpC9,((_tmp392->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmpCA,((
_tmp392->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmpCB),((_tmp392->encloser=(struct Cyc_Absyn_Stmt*)_tmpCC,((_tmp392->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)_tmpD9,((_tmp392->capability=(void*)_tmpCE,((_tmp392->curr_rgn=(
void*)_tmpCF,((_tmp392->in_notreadctxt=(int)_tmpD0,((_tmp392->in_lhs=(int)_tmpD1,((
_tmp392->fnrgn=(struct _RegionHandle*)l,(struct Cyc_Tcenv_Fenv*)_tmp392)))))))))))))))))))))));};};}
void*Cyc_Tcenv_return_typ(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmpDE;
struct Cyc_Tcenv_SharedFenv*_tmpDF;const char*_tmp393;struct Cyc_Tcenv_Fenv*_tmpDD=
Cyc_Tcenv_get_fenv(te,((_tmp393="return_typ",_tag_dyneither(_tmp393,sizeof(char),
11))));_tmpDE=*_tmpDD;_tmpDF=_tmpDE.shared;{void*_tmpE1;struct Cyc_Tcenv_SharedFenv
_tmpE0=*_tmpDF;_tmpE1=_tmpE0.return_typ;return _tmpE1;};}struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpE2=te->le;if(te->le == 0)return
0;{struct Cyc_List_List*_tmpE4;struct Cyc_Tcenv_Fenv _tmpE3=*((struct Cyc_Tcenv_Fenv*)
_check_null(_tmpE2));_tmpE4=_tmpE3.type_vars;return _tmpE4;};}struct Cyc_Core_Opt*
Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*te){struct Cyc_Core_Opt*
_tmp394;return(_tmp394=_cycalloc(sizeof(*_tmp394)),((_tmp394->v=Cyc_Tcenv_lookup_type_vars(
te),_tmp394)));}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(struct _RegionHandle*
r,unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*tvs){struct Cyc_Tcenv_Fenv
_tmpE8;struct Cyc_Tcenv_SharedFenv*_tmpE9;struct Cyc_List_List*_tmpEA;struct Cyc_RgnOrder_RgnPO*
_tmpEB;const struct Cyc_Tcenv_Bindings*_tmpEC;struct Cyc_Absyn_Stmt*_tmpED;struct
Cyc_Tcenv_CtrlEnv*_tmpEE;void*_tmpEF;void*_tmpF0;int _tmpF1;int _tmpF2;const char*
_tmp395;struct Cyc_Tcenv_Fenv*_tmpE7=Cyc_Tcenv_get_fenv(te,((_tmp395="add_type_vars",
_tag_dyneither(_tmp395,sizeof(char),14))));_tmpE8=*_tmpE7;_tmpE9=_tmpE8.shared;
_tmpEA=_tmpE8.type_vars;_tmpEB=_tmpE8.region_order;_tmpEC=_tmpE8.locals;_tmpED=
_tmpE8.encloser;_tmpEE=_tmpE8.ctrl_env;_tmpEF=_tmpE8.capability;_tmpF0=_tmpE8.curr_rgn;
_tmpF1=_tmpE8.in_notreadctxt;_tmpF2=_tmpE8.in_lhs;Cyc_Tcutil_add_tvar_identities(
tvs);{struct Cyc_List_List*_tmpF3=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_append)(tvs,_tmpEA);Cyc_Tcutil_check_unique_tvars(
loc,_tmpF3);{struct Cyc_Tcenv_Fenv*_tmp396;struct Cyc_Tcenv_Fenv*_tmpF4=(_tmp396=
_region_malloc(r,sizeof(*_tmp396)),((_tmp396->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmpE9,((_tmp396->type_vars=(struct Cyc_List_List*)_tmpF3,((_tmp396->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmpEB,((_tmp396->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmpEC),((_tmp396->encloser=(struct Cyc_Absyn_Stmt*)
_tmpED,((_tmp396->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmpEE),((_tmp396->capability=(void*)_tmpEF,((_tmp396->curr_rgn=(void*)_tmpF0,((
_tmp396->in_notreadctxt=(int)_tmpF1,((_tmp396->in_lhs=(int)_tmpF2,((_tmp396->fnrgn=(
struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp396)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmpF4);};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpF6=te->le;
if(_tmpF6 == 0)return Cyc_Tcenv_put_emptyfenv(r,te);else{struct Cyc_Tcenv_Fenv*fe=
Cyc_Tcenv_copy_fenv_old_ctrl(r,(struct Cyc_Tcenv_Fenv*)_tmpF6);return Cyc_Tcenv_put_fenv(
r,te,fe);}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_local_var(struct _RegionHandle*r,
unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Vardecl*vd){struct
_dyneither_ptr*_tmpF7=(*vd->name).f2;{union Cyc_Absyn_Nmspace _tmpF8=(*vd->name).f1;
int _tmpF9;_LL5C: if((_tmpF8.Loc_n).tag != 4)goto _LL5E;_tmpF9=(int)(_tmpF8.Loc_n).val;
_LL5D: goto _LL5B;_LL5E:;_LL5F: {struct Cyc_Core_Impossible_struct _tmp39C;const char*
_tmp39B;struct Cyc_Core_Impossible_struct*_tmp39A;(int)_throw((void*)((_tmp39A=
_cycalloc(sizeof(*_tmp39A)),((_tmp39A[0]=((_tmp39C.tag=Cyc_Core_Impossible,((
_tmp39C.f1=((_tmp39B="add_local_var: called with Rel_n",_tag_dyneither(_tmp39B,
sizeof(char),33))),_tmp39C)))),_tmp39A)))));}_LL5B:;}{struct Cyc_Tcenv_Fenv _tmpFF;
struct Cyc_Tcenv_SharedFenv*_tmp100;struct Cyc_List_List*_tmp101;struct Cyc_RgnOrder_RgnPO*
_tmp102;const struct Cyc_Tcenv_Bindings*_tmp103;struct Cyc_Absyn_Stmt*_tmp104;
struct Cyc_Tcenv_CtrlEnv*_tmp105;void*_tmp106;void*_tmp107;int _tmp108;int _tmp109;
const char*_tmp39D;struct Cyc_Tcenv_Fenv*_tmpFE=Cyc_Tcenv_get_fenv(te,((_tmp39D="add_local_var",
_tag_dyneither(_tmp39D,sizeof(char),14))));_tmpFF=*_tmpFE;_tmp100=_tmpFF.shared;
_tmp101=_tmpFF.type_vars;_tmp102=_tmpFF.region_order;_tmp103=_tmpFF.locals;
_tmp104=_tmpFF.encloser;_tmp105=_tmpFF.ctrl_env;_tmp106=_tmpFF.capability;
_tmp107=_tmpFF.curr_rgn;_tmp108=_tmpFF.in_notreadctxt;_tmp109=_tmpFF.in_lhs;{
struct Cyc_Absyn_Local_b_struct _tmp3A0;struct Cyc_Absyn_Local_b_struct*_tmp39F;
struct Cyc_Absyn_Local_b_struct*_tmp10A=(_tmp39F=_cycalloc(sizeof(*_tmp39F)),((
_tmp39F[0]=((_tmp3A0.tag=4,((_tmp3A0.f1=vd,_tmp3A0)))),_tmp39F)));struct Cyc_Tcenv_Bindings*
_tmp3A1;struct Cyc_Tcenv_Bindings*_tmp10B=(_tmp3A1=_region_malloc(r,sizeof(*
_tmp3A1)),((_tmp3A1->v=_tmpF7,((_tmp3A1->b=(void*)_tmp10A,((_tmp3A1->tl=(const
struct Cyc_Tcenv_Bindings*)_tmp103,_tmp3A1)))))));struct Cyc_Tcenv_Fenv*_tmp3A2;
struct Cyc_Tcenv_Fenv*_tmp10C=(_tmp3A2=_region_malloc(r,sizeof(*_tmp3A2)),((
_tmp3A2->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp100,((_tmp3A2->type_vars=(
struct Cyc_List_List*)_tmp101,((_tmp3A2->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp102,((_tmp3A2->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp10B),((_tmp3A2->encloser=(struct Cyc_Absyn_Stmt*)_tmp104,((_tmp3A2->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp105),((_tmp3A2->capability=(
void*)_tmp106,((_tmp3A2->curr_rgn=(void*)_tmp107,((_tmp3A2->in_notreadctxt=(int)
_tmp108,((_tmp3A2->in_lhs=(int)_tmp109,((_tmp3A2->fnrgn=(struct _RegionHandle*)r,(
struct Cyc_Tcenv_Fenv*)_tmp3A2)))))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,
te,_tmp10C);};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(struct
_RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp111=te->le;if(
_tmp111 == 0)return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*
_tmp113;struct Cyc_List_List*_tmp114;struct Cyc_RgnOrder_RgnPO*_tmp115;const struct
Cyc_Tcenv_Bindings*_tmp116;struct Cyc_Absyn_Stmt*_tmp117;struct Cyc_Tcenv_CtrlEnv*
_tmp118;void*_tmp119;void*_tmp11A;int _tmp11B;int _tmp11C;struct Cyc_Tcenv_Fenv
_tmp112=*_tmp111;_tmp113=_tmp112.shared;_tmp114=_tmp112.type_vars;_tmp115=
_tmp112.region_order;_tmp116=_tmp112.locals;_tmp117=_tmp112.encloser;_tmp118=
_tmp112.ctrl_env;_tmp119=_tmp112.capability;_tmp11A=_tmp112.curr_rgn;_tmp11B=
_tmp112.in_notreadctxt;_tmp11C=_tmp112.in_lhs;{struct Cyc_Tcenv_Fenv*_tmp3A3;
struct Cyc_Tcenv_Fenv*_tmp11D=(_tmp3A3=_region_malloc(r,sizeof(*_tmp3A3)),((
_tmp3A3->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp113,((_tmp3A3->type_vars=(
struct Cyc_List_List*)_tmp114,((_tmp3A3->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp115,((_tmp3A3->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp116),((_tmp3A3->encloser=(struct Cyc_Absyn_Stmt*)_tmp117,((_tmp3A3->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp118),((_tmp3A3->capability=(
void*)_tmp119,((_tmp3A3->curr_rgn=(void*)_tmp11A,((_tmp3A3->in_notreadctxt=(int)
1,((_tmp3A3->in_lhs=(int)_tmp11C,((_tmp3A3->fnrgn=(struct _RegionHandle*)r,(
struct Cyc_Tcenv_Fenv*)_tmp3A3)))))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,
te,_tmp11D);};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(struct
_RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp11F=te->le;if(
_tmp11F == 0)return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*
_tmp121;struct Cyc_List_List*_tmp122;struct Cyc_RgnOrder_RgnPO*_tmp123;const struct
Cyc_Tcenv_Bindings*_tmp124;struct Cyc_Absyn_Stmt*_tmp125;struct Cyc_Tcenv_CtrlEnv*
_tmp126;void*_tmp127;void*_tmp128;int _tmp129;int _tmp12A;struct Cyc_Tcenv_Fenv
_tmp120=*_tmp11F;_tmp121=_tmp120.shared;_tmp122=_tmp120.type_vars;_tmp123=
_tmp120.region_order;_tmp124=_tmp120.locals;_tmp125=_tmp120.encloser;_tmp126=
_tmp120.ctrl_env;_tmp127=_tmp120.capability;_tmp128=_tmp120.curr_rgn;_tmp129=
_tmp120.in_notreadctxt;_tmp12A=_tmp120.in_lhs;{struct Cyc_Tcenv_Fenv*_tmp3A4;
struct Cyc_Tcenv_Fenv*_tmp12B=(_tmp3A4=_region_malloc(r,sizeof(*_tmp3A4)),((
_tmp3A4->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp121,((_tmp3A4->type_vars=(
struct Cyc_List_List*)_tmp122,((_tmp3A4->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp123,((_tmp3A4->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp124),((_tmp3A4->encloser=(struct Cyc_Absyn_Stmt*)_tmp125,((_tmp3A4->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp126),((_tmp3A4->capability=(
void*)_tmp127,((_tmp3A4->curr_rgn=(void*)_tmp128,((_tmp3A4->in_notreadctxt=(int)
0,((_tmp3A4->in_lhs=(int)_tmp12A,((_tmp3A4->fnrgn=(struct _RegionHandle*)r,(
struct Cyc_Tcenv_Fenv*)_tmp3A4)))))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,
te,_tmp12B);};};}int Cyc_Tcenv_in_notreadctxt(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*
_tmp12D=te->le;if(_tmp12D == 0)return 0;{struct Cyc_Tcenv_Fenv _tmp12F;int _tmp130;
struct Cyc_Tcenv_Fenv*_tmp12E=(struct Cyc_Tcenv_Fenv*)_tmp12D;_tmp12F=*_tmp12E;
_tmp130=_tmp12F.in_notreadctxt;return _tmp130;};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_lhs(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp131=te->le;
if(_tmp131 == 0)return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*
_tmp133;struct Cyc_List_List*_tmp134;struct Cyc_RgnOrder_RgnPO*_tmp135;const struct
Cyc_Tcenv_Bindings*_tmp136;struct Cyc_Absyn_Stmt*_tmp137;struct Cyc_Tcenv_CtrlEnv*
_tmp138;void*_tmp139;void*_tmp13A;int _tmp13B;int _tmp13C;struct Cyc_Tcenv_Fenv
_tmp132=*_tmp131;_tmp133=_tmp132.shared;_tmp134=_tmp132.type_vars;_tmp135=
_tmp132.region_order;_tmp136=_tmp132.locals;_tmp137=_tmp132.encloser;_tmp138=
_tmp132.ctrl_env;_tmp139=_tmp132.capability;_tmp13A=_tmp132.curr_rgn;_tmp13B=
_tmp132.in_notreadctxt;_tmp13C=_tmp132.in_lhs;{struct Cyc_Tcenv_Fenv*_tmp3A5;
struct Cyc_Tcenv_Fenv*_tmp13D=(_tmp3A5=_region_malloc(r,sizeof(*_tmp3A5)),((
_tmp3A5->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp133,((_tmp3A5->type_vars=(
struct Cyc_List_List*)_tmp134,((_tmp3A5->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp135,((_tmp3A5->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp136),((_tmp3A5->encloser=(struct Cyc_Absyn_Stmt*)_tmp137,((_tmp3A5->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp138),((_tmp3A5->capability=(
void*)_tmp139,((_tmp3A5->curr_rgn=(void*)_tmp13A,((_tmp3A5->in_notreadctxt=(int)
_tmp13B,((_tmp3A5->in_lhs=(int)1,((_tmp3A5->fnrgn=(struct _RegionHandle*)r,(
struct Cyc_Tcenv_Fenv*)_tmp3A5)))))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,
te,_tmp13D);};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_lhs(struct _RegionHandle*r,
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp13F=te->le;if(_tmp13F == 0)
return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*_tmp141;struct
Cyc_List_List*_tmp142;struct Cyc_RgnOrder_RgnPO*_tmp143;const struct Cyc_Tcenv_Bindings*
_tmp144;struct Cyc_Absyn_Stmt*_tmp145;struct Cyc_Tcenv_CtrlEnv*_tmp146;void*
_tmp147;void*_tmp148;int _tmp149;int _tmp14A;struct Cyc_Tcenv_Fenv _tmp140=*_tmp13F;
_tmp141=_tmp140.shared;_tmp142=_tmp140.type_vars;_tmp143=_tmp140.region_order;
_tmp144=_tmp140.locals;_tmp145=_tmp140.encloser;_tmp146=_tmp140.ctrl_env;_tmp147=
_tmp140.capability;_tmp148=_tmp140.curr_rgn;_tmp149=_tmp140.in_notreadctxt;
_tmp14A=_tmp140.in_lhs;{struct Cyc_Tcenv_Fenv*_tmp3A6;struct Cyc_Tcenv_Fenv*
_tmp14B=(_tmp3A6=_region_malloc(r,sizeof(*_tmp3A6)),((_tmp3A6->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp141,((_tmp3A6->type_vars=(struct Cyc_List_List*)_tmp142,((_tmp3A6->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp143,((_tmp3A6->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmp144),((_tmp3A6->encloser=(struct Cyc_Absyn_Stmt*)
_tmp145,((_tmp3A6->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp146),((_tmp3A6->capability=(void*)_tmp147,((_tmp3A6->curr_rgn=(void*)_tmp148,((
_tmp3A6->in_notreadctxt=(int)_tmp149,((_tmp3A6->in_lhs=(int)0,((_tmp3A6->fnrgn=(
struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp3A6)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp14B);};};}int Cyc_Tcenv_in_lhs(struct Cyc_Tcenv_Tenv*
te){struct Cyc_Tcenv_Fenv*_tmp14D=te->le;if(_tmp14D == 0)return 0;{struct Cyc_Tcenv_Fenv
_tmp14F;int _tmp150;struct Cyc_Tcenv_Fenv*_tmp14E=(struct Cyc_Tcenv_Fenv*)_tmp14D;
_tmp14F=*_tmp14E;_tmp150=_tmp14F.in_lhs;return _tmp150;};}struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_pat_var(struct _RegionHandle*r,unsigned int loc,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Absyn_Vardecl*vd){struct _dyneither_ptr*_tmp151=(*vd->name).f2;
struct Cyc_Tcenv_Fenv _tmp154;struct Cyc_Tcenv_SharedFenv*_tmp155;struct Cyc_List_List*
_tmp156;struct Cyc_RgnOrder_RgnPO*_tmp157;const struct Cyc_Tcenv_Bindings*_tmp158;
struct Cyc_Absyn_Stmt*_tmp159;struct Cyc_Tcenv_CtrlEnv*_tmp15A;void*_tmp15B;void*
_tmp15C;int _tmp15D;int _tmp15E;const char*_tmp3A7;struct Cyc_Tcenv_Fenv*_tmp153=Cyc_Tcenv_get_fenv(
te,((_tmp3A7="add_pat_var",_tag_dyneither(_tmp3A7,sizeof(char),12))));_tmp154=*
_tmp153;_tmp155=_tmp154.shared;_tmp156=_tmp154.type_vars;_tmp157=_tmp154.region_order;
_tmp158=_tmp154.locals;_tmp159=_tmp154.encloser;_tmp15A=_tmp154.ctrl_env;_tmp15B=
_tmp154.capability;_tmp15C=_tmp154.curr_rgn;_tmp15D=_tmp154.in_notreadctxt;
_tmp15E=_tmp154.in_lhs;{struct Cyc_Absyn_Pat_b_struct _tmp3AA;struct Cyc_Absyn_Pat_b_struct*
_tmp3A9;struct Cyc_Absyn_Pat_b_struct*_tmp15F=(_tmp3A9=_cycalloc(sizeof(*_tmp3A9)),((
_tmp3A9[0]=((_tmp3AA.tag=5,((_tmp3AA.f1=vd,_tmp3AA)))),_tmp3A9)));struct Cyc_Tcenv_Bindings*
_tmp3AB;struct Cyc_Tcenv_Bindings*_tmp160=(_tmp3AB=_region_malloc(r,sizeof(*
_tmp3AB)),((_tmp3AB->v=_tmp151,((_tmp3AB->b=(void*)_tmp15F,((_tmp3AB->tl=(const
struct Cyc_Tcenv_Bindings*)_tmp158,_tmp3AB)))))));struct Cyc_Tcenv_Fenv*_tmp3AC;
struct Cyc_Tcenv_Fenv*_tmp161=(_tmp3AC=_region_malloc(r,sizeof(*_tmp3AC)),((
_tmp3AC->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp155,((_tmp3AC->type_vars=(
struct Cyc_List_List*)_tmp156,((_tmp3AC->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp157,((_tmp3AC->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp160),((_tmp3AC->encloser=(struct Cyc_Absyn_Stmt*)_tmp159,((_tmp3AC->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp15A),((_tmp3AC->capability=(
void*)_tmp15B,((_tmp3AC->curr_rgn=(void*)_tmp15C,((_tmp3AC->in_notreadctxt=(int)
_tmp15D,((_tmp3AC->in_lhs=(int)_tmp15E,((_tmp3AC->fnrgn=(struct _RegionHandle*)r,(
struct Cyc_Tcenv_Fenv*)_tmp3AC)))))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,
te,_tmp161);};}void*Cyc_Tcenv_lookup_ordinary(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te,unsigned int loc,struct _tuple0*q){struct Cyc_Tcenv_Fenv*_tmp166=te->le;struct
_tuple0 _tmp168;union Cyc_Absyn_Nmspace _tmp169;struct _dyneither_ptr*_tmp16A;struct
_tuple0*_tmp167=q;_tmp168=*_tmp167;_tmp169=_tmp168.f1;_tmp16A=_tmp168.f2;{union
Cyc_Absyn_Nmspace _tmp16B=_tmp169;int _tmp16C;struct Cyc_List_List*_tmp16D;_LL61:
if((_tmp16B.Loc_n).tag != 4)goto _LL63;_tmp16C=(int)(_tmp16B.Loc_n).val;_LL62: if(
_tmp166 == 0){struct Cyc_Dict_Absent_struct _tmp3AF;struct Cyc_Dict_Absent_struct*
_tmp3AE;(int)_throw((void*)((_tmp3AE=_cycalloc_atomic(sizeof(*_tmp3AE)),((
_tmp3AE[0]=((_tmp3AF.tag=Cyc_Dict_Absent,_tmp3AF)),_tmp3AE)))));}goto _LL64;_LL63:
if((_tmp16B.Rel_n).tag != 1)goto _LL65;_tmp16D=(struct Cyc_List_List*)(_tmp16B.Rel_n).val;
if(_tmp16D != 0)goto _LL65;if(!(_tmp166 != 0))goto _LL65;_LL64: {struct Cyc_Tcenv_Fenv
_tmp171;const struct Cyc_Tcenv_Bindings*_tmp172;struct Cyc_Tcenv_Fenv*_tmp170=(
struct Cyc_Tcenv_Fenv*)_tmp166;_tmp171=*_tmp170;_tmp172=_tmp171.locals;{struct
_handler_cons _tmp173;_push_handler(& _tmp173);{int _tmp175=0;if(setjmp(_tmp173.handler))
_tmp175=1;if(!_tmp175){{struct Cyc_Tcenv_VarRes_struct _tmp3B2;struct Cyc_Tcenv_VarRes_struct*
_tmp3B1;void*_tmp178=(void*)((_tmp3B1=_region_malloc(r,sizeof(*_tmp3B1)),((
_tmp3B1[0]=((_tmp3B2.tag=0,((_tmp3B2.f1=(void*)Cyc_Tcenv_lookup_binding(_tmp172,
_tmp16A),_tmp3B2)))),_tmp3B1))));_npop_handler(0);return _tmp178;};_pop_handler();}
else{void*_tmp174=(void*)_exn_thrown;void*_tmp17A=_tmp174;_LL68: {struct Cyc_Tcenv_NoBinding_struct*
_tmp17B=(struct Cyc_Tcenv_NoBinding_struct*)_tmp17A;if(_tmp17B->tag != Cyc_Tcenv_NoBinding)
goto _LL6A;}_LL69: return(void*)Cyc_Tcenv_lookup_ordinary_global(te,loc,q);_LL6A:;
_LL6B:(void)_throw(_tmp17A);_LL67:;}};};}_LL65:;_LL66: {struct _handler_cons
_tmp17C;_push_handler(& _tmp17C);{int _tmp17E=0;if(setjmp(_tmp17C.handler))_tmp17E=
1;if(!_tmp17E){{void*_tmp17F=(void*)Cyc_Tcenv_lookup_ordinary_global(te,loc,q);
_npop_handler(0);return _tmp17F;};_pop_handler();}else{void*_tmp17D=(void*)
_exn_thrown;void*_tmp181=_tmp17D;_LL6D: {struct Cyc_Dict_Absent_struct*_tmp182=(
struct Cyc_Dict_Absent_struct*)_tmp181;if(_tmp182->tag != Cyc_Dict_Absent)goto
_LL6F;}_LL6E: {struct Cyc_Tcenv_VarRes_struct _tmp3B5;struct Cyc_Tcenv_VarRes_struct*
_tmp3B4;return(void*)((_tmp3B4=_region_malloc(r,sizeof(*_tmp3B4)),((_tmp3B4[0]=((
_tmp3B5.tag=0,((_tmp3B5.f1=(void*)((void*)& Cyc_Absyn_Unresolved_b_val),_tmp3B5)))),
_tmp3B4))));}_LL6F:;_LL70:(void)_throw(_tmp181);_LL6C:;}};}_LL60:;};}void Cyc_Tcenv_process_continue(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt**sopt){
struct Cyc_Tcenv_Fenv _tmp187;struct Cyc_Tcenv_CtrlEnv*_tmp188;const char*_tmp3B6;
struct Cyc_Tcenv_Fenv*_tmp186=Cyc_Tcenv_get_fenv(te,((_tmp3B6="process_continue",
_tag_dyneither(_tmp3B6,sizeof(char),17))));_tmp187=*_tmp186;_tmp188=_tmp187.ctrl_env;{
void*_tmp189=_tmp188->continue_stmt;struct Cyc_Absyn_Stmt*_tmp18B;_LL72: {struct
Cyc_Tcenv_Stmt_j_struct*_tmp18A=(struct Cyc_Tcenv_Stmt_j_struct*)_tmp189;if(
_tmp18A->tag != 3)goto _LL74;else{_tmp18B=_tmp18A->f1;}}_LL73:{struct Cyc_List_List*
_tmp3B7;_tmp18B->non_local_preds=((_tmp3B7=_cycalloc(sizeof(*_tmp3B7)),((_tmp3B7->hd=
s,((_tmp3B7->tl=_tmp18B->non_local_preds,_tmp3B7))))));}*sopt=(struct Cyc_Absyn_Stmt*)
_tmp18B;return;_LL74: {struct Cyc_Tcenv_NotLoop_j_struct*_tmp18C=(struct Cyc_Tcenv_NotLoop_j_struct*)
_tmp189;if(_tmp18C->tag != 0)goto _LL76;}_LL75:{const char*_tmp3BA;void*_tmp3B9;(
_tmp3B9=0,Cyc_Tcutil_terr(s->loc,((_tmp3BA="continue not in a loop",
_tag_dyneither(_tmp3BA,sizeof(char),23))),_tag_dyneither(_tmp3B9,sizeof(void*),0)));}
return;_LL76: {struct Cyc_Tcenv_CaseEnd_j_struct*_tmp18D=(struct Cyc_Tcenv_CaseEnd_j_struct*)
_tmp189;if(_tmp18D->tag != 1)goto _LL78;}_LL77: goto _LL79;_LL78: {struct Cyc_Tcenv_FnEnd_j_struct*
_tmp18E=(struct Cyc_Tcenv_FnEnd_j_struct*)_tmp189;if(_tmp18E->tag != 2)goto _LL71;}
_LL79: {const char*_tmp3BB;((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((
_tmp3BB="bad continue destination",_tag_dyneither(_tmp3BB,sizeof(char),25))));}
_LL71:;};}void Cyc_Tcenv_process_break(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*
s,struct Cyc_Absyn_Stmt**sopt){struct Cyc_Tcenv_Fenv _tmp195;struct Cyc_Tcenv_CtrlEnv*
_tmp196;struct Cyc_Tcenv_SharedFenv*_tmp197;const char*_tmp3BC;struct Cyc_Tcenv_Fenv*
_tmp194=Cyc_Tcenv_get_fenv(te,((_tmp3BC="process_break",_tag_dyneither(_tmp3BC,
sizeof(char),14))));_tmp195=*_tmp194;_tmp196=_tmp195.ctrl_env;_tmp197=_tmp195.shared;{
void*_tmp198=_tmp196->break_stmt;struct Cyc_Absyn_Stmt*_tmp19A;_LL7B: {struct Cyc_Tcenv_Stmt_j_struct*
_tmp199=(struct Cyc_Tcenv_Stmt_j_struct*)_tmp198;if(_tmp199->tag != 3)goto _LL7D;
else{_tmp19A=_tmp199->f1;}}_LL7C:{struct Cyc_List_List*_tmp3BD;_tmp19A->non_local_preds=((
_tmp3BD=_cycalloc(sizeof(*_tmp3BD)),((_tmp3BD->hd=s,((_tmp3BD->tl=_tmp19A->non_local_preds,
_tmp3BD))))));}*sopt=(struct Cyc_Absyn_Stmt*)_tmp19A;return;_LL7D: {struct Cyc_Tcenv_NotLoop_j_struct*
_tmp19B=(struct Cyc_Tcenv_NotLoop_j_struct*)_tmp198;if(_tmp19B->tag != 0)goto _LL7F;}
_LL7E:{const char*_tmp3C0;void*_tmp3BF;(_tmp3BF=0,Cyc_Tcutil_terr(s->loc,((
_tmp3C0="break not in a loop or switch",_tag_dyneither(_tmp3C0,sizeof(char),30))),
_tag_dyneither(_tmp3BF,sizeof(void*),0)));}return;_LL7F: {struct Cyc_Tcenv_FnEnd_j_struct*
_tmp19C=(struct Cyc_Tcenv_FnEnd_j_struct*)_tmp198;if(_tmp19C->tag != 2)goto _LL81;}
_LL80:{void*_tmp1A1=Cyc_Tcutil_compress(_tmp197->return_typ);_LL84: {struct Cyc_Absyn_VoidType_struct*
_tmp1A2=(struct Cyc_Absyn_VoidType_struct*)_tmp1A1;if(_tmp1A2->tag != 0)goto _LL86;}
_LL85: goto _LL83;_LL86: {struct Cyc_Absyn_FloatType_struct*_tmp1A3=(struct Cyc_Absyn_FloatType_struct*)
_tmp1A1;if(_tmp1A3->tag != 7)goto _LL88;}_LL87: goto _LL89;_LL88: {struct Cyc_Absyn_IntType_struct*
_tmp1A4=(struct Cyc_Absyn_IntType_struct*)_tmp1A1;if(_tmp1A4->tag != 6)goto _LL8A;}
_LL89:{const char*_tmp3C3;void*_tmp3C2;(_tmp3C2=0,Cyc_Tcutil_warn(s->loc,((
_tmp3C3="break may cause function not to return a value",_tag_dyneither(_tmp3C3,
sizeof(char),47))),_tag_dyneither(_tmp3C2,sizeof(void*),0)));}goto _LL83;_LL8A:;
_LL8B:{const char*_tmp3C6;void*_tmp3C5;(_tmp3C5=0,Cyc_Tcutil_terr(s->loc,((
_tmp3C6="break may cause function not to return a value",_tag_dyneither(_tmp3C6,
sizeof(char),47))),_tag_dyneither(_tmp3C5,sizeof(void*),0)));}goto _LL83;_LL83:;}
return;_LL81: {struct Cyc_Tcenv_CaseEnd_j_struct*_tmp19D=(struct Cyc_Tcenv_CaseEnd_j_struct*)
_tmp198;if(_tmp19D->tag != 1)goto _LL7A;}_LL82:{const char*_tmp3C9;void*_tmp3C8;(
_tmp3C8=0,Cyc_Tcutil_terr(s->loc,((_tmp3C9="break causes outer switch clause to implicitly fallthru",
_tag_dyneither(_tmp3C9,sizeof(char),56))),_tag_dyneither(_tmp3C8,sizeof(void*),0)));}
return;_LL7A:;};}void Cyc_Tcenv_process_goto(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*
s,struct _dyneither_ptr*l,struct Cyc_Absyn_Stmt**sopt){struct Cyc_Tcenv_Fenv _tmp1AD;
struct Cyc_Tcenv_SharedFenv*_tmp1AE;const char*_tmp3CA;struct Cyc_Tcenv_Fenv*
_tmp1AC=Cyc_Tcenv_get_fenv(te,((_tmp3CA="process_goto",_tag_dyneither(_tmp3CA,
sizeof(char),13))));_tmp1AD=*_tmp1AC;_tmp1AE=_tmp1AD.shared;{struct Cyc_Absyn_Stmt**
sopt2=((struct Cyc_Absyn_Stmt**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))
Cyc_Dict_lookup_opt)(_tmp1AE->seen_labels,l);if(sopt2 == 0){struct Cyc_Dict_Dict
_tmp1AF=_tmp1AE->needed_labels;struct Cyc_List_List**slopt=((struct Cyc_List_List**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup_opt)(_tmp1AF,l);
struct _RegionHandle*frgn=_tmp1AE->frgn;if(slopt == 0){struct Cyc_List_List**
_tmp3CB;slopt=((_tmp3CB=_region_malloc(frgn,sizeof(*_tmp3CB)),((_tmp3CB[0]=0,
_tmp3CB))));}{struct Cyc_List_List*_tmp3CC;struct Cyc_List_List*new_needed=(
_tmp3CC=_cycalloc(sizeof(*_tmp3CC)),((_tmp3CC->hd=s,((_tmp3CC->tl=*slopt,_tmp3CC)))));
_tmp1AE->needed_labels=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k,struct Cyc_List_List*v))Cyc_Dict_insert)(_tmp1AF,l,new_needed);};}
else{struct Cyc_Absyn_Stmt*s=*sopt2;{struct Cyc_List_List*_tmp3CD;s->non_local_preds=((
_tmp3CD=_cycalloc(sizeof(*_tmp3CD)),((_tmp3CD->hd=s,((_tmp3CD->tl=s->non_local_preds,
_tmp3CD))))));}*sopt=(struct Cyc_Absyn_Stmt*)s;}};}const struct _tuple10*const Cyc_Tcenv_process_fallthru(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Switch_clause***
clauseopt){struct Cyc_Tcenv_Fenv _tmp1B5;struct Cyc_Tcenv_CtrlEnv*_tmp1B6;const char*
_tmp3CE;struct Cyc_Tcenv_Fenv*_tmp1B4=Cyc_Tcenv_get_fenv(te,((_tmp3CE="process_fallthru",
_tag_dyneither(_tmp3CE,sizeof(char),17))));_tmp1B5=*_tmp1B4;_tmp1B6=_tmp1B5.ctrl_env;{
const struct _tuple10*_tmp1B7=(const struct _tuple10*)_tmp1B6->fallthru_clause;if(
_tmp1B7 != (const struct _tuple10*)0){{struct Cyc_List_List*_tmp3CF;(((*_tmp1B7).f1)->body)->non_local_preds=((
_tmp3CF=_cycalloc(sizeof(*_tmp3CF)),((_tmp3CF->hd=s,((_tmp3CF->tl=(((*_tmp1B7).f1)->body)->non_local_preds,
_tmp3CF))))));}{struct Cyc_Absyn_Switch_clause**_tmp3D0;*clauseopt=(struct Cyc_Absyn_Switch_clause**)((
_tmp3D0=_cycalloc(sizeof(*_tmp3D0)),((_tmp3D0[0]=(*_tmp1B7).f1,_tmp3D0))));};}
return _tmp1B7;};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(struct _RegionHandle*
r,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*new_tvs,struct Cyc_List_List*vds,
struct Cyc_Absyn_Switch_clause*clause){struct Cyc_Tcenv_Fenv _tmp1BC;struct Cyc_Tcenv_SharedFenv*
_tmp1BD;struct Cyc_List_List*_tmp1BE;struct Cyc_RgnOrder_RgnPO*_tmp1BF;const struct
Cyc_Tcenv_Bindings*_tmp1C0;struct Cyc_Absyn_Stmt*_tmp1C1;struct Cyc_Tcenv_CtrlEnv*
_tmp1C2;void*_tmp1C3;void*_tmp1C4;int _tmp1C5;int _tmp1C6;const char*_tmp3D1;struct
Cyc_Tcenv_Fenv*_tmp1BB=Cyc_Tcenv_get_fenv(te,((_tmp3D1="set_fallthru",
_tag_dyneither(_tmp3D1,sizeof(char),13))));_tmp1BC=*_tmp1BB;_tmp1BD=_tmp1BC.shared;
_tmp1BE=_tmp1BC.type_vars;_tmp1BF=_tmp1BC.region_order;_tmp1C0=_tmp1BC.locals;
_tmp1C1=_tmp1BC.encloser;_tmp1C2=_tmp1BC.ctrl_env;_tmp1C3=_tmp1BC.capability;
_tmp1C4=_tmp1BC.curr_rgn;_tmp1C5=_tmp1BC.in_notreadctxt;_tmp1C6=_tmp1BC.in_lhs;{
struct Cyc_Tcenv_CtrlEnv _tmp1C8;struct _RegionHandle*_tmp1C9;void*_tmp1CA;void*
_tmp1CB;const struct _tuple10*_tmp1CC;void*_tmp1CD;int _tmp1CE;struct Cyc_Tcenv_CtrlEnv*
_tmp1C7=_tmp1C2;_tmp1C8=*_tmp1C7;_tmp1C9=_tmp1C8.ctrl_rgn;_tmp1CA=_tmp1C8.continue_stmt;
_tmp1CB=_tmp1C8.break_stmt;_tmp1CC=_tmp1C8.fallthru_clause;_tmp1CD=_tmp1C8.next_stmt;
_tmp1CE=_tmp1C8.try_depth;{struct Cyc_List_List*ft_typ=0;for(0;vds != 0;vds=vds->tl){
struct Cyc_List_List*_tmp3D2;ft_typ=((_tmp3D2=_region_malloc(_tmp1C9,sizeof(*
_tmp3D2)),((_tmp3D2->hd=((struct Cyc_Absyn_Vardecl*)vds->hd)->type,((_tmp3D2->tl=
ft_typ,_tmp3D2))))));}{const struct Cyc_Tcenv_CList*_tmp1D0=(const struct Cyc_Tcenv_CList*)((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(ft_typ);struct
_tuple10*_tmp3D5;struct Cyc_Tcenv_CtrlEnv*_tmp3D4;struct Cyc_Tcenv_CtrlEnv*_tmp1D1=(
_tmp3D4=_region_malloc(r,sizeof(*_tmp3D4)),((_tmp3D4->ctrl_rgn=_tmp1C9,((_tmp3D4->continue_stmt=
_tmp1CA,((_tmp3D4->break_stmt=_tmp1CB,((_tmp3D4->fallthru_clause=(const struct
_tuple10*)((_tmp3D5=_region_malloc(_tmp1C9,sizeof(*_tmp3D5)),((_tmp3D5->f1=
clause,((_tmp3D5->f2=new_tvs,((_tmp3D5->f3=_tmp1D0,_tmp3D5)))))))),((_tmp3D4->next_stmt=
_tmp1CD,((_tmp3D4->try_depth=_tmp1CE,_tmp3D4)))))))))))));struct Cyc_Tcenv_Fenv*
_tmp3D6;struct Cyc_Tcenv_Fenv*_tmp1D2=(_tmp3D6=_region_malloc(r,sizeof(*_tmp3D6)),((
_tmp3D6->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp1BD,((_tmp3D6->type_vars=(
struct Cyc_List_List*)_tmp1BE,((_tmp3D6->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp1BF,((_tmp3D6->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp1C0),((_tmp3D6->encloser=(struct Cyc_Absyn_Stmt*)_tmp1C1,((_tmp3D6->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)_tmp1D1,((_tmp3D6->capability=(void*)_tmp1C3,((_tmp3D6->curr_rgn=(
void*)_tmp1C4,((_tmp3D6->in_notreadctxt=(int)_tmp1C5,((_tmp3D6->in_lhs=(int)
_tmp1C6,((_tmp3D6->fnrgn=(struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp3D6)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp1D2);};};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){const char*_tmp3D7;struct Cyc_Tcenv_Fenv*
fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(te,((_tmp3D7="clear_fallthru",
_tag_dyneither(_tmp3D7,sizeof(char),15)))));struct Cyc_Tcenv_Fenv _tmp1D7;struct
Cyc_Tcenv_CtrlEnv*_tmp1D8;struct Cyc_Tcenv_Fenv*_tmp1D6=fe;_tmp1D7=*_tmp1D6;
_tmp1D8=_tmp1D7.ctrl_env;_tmp1D8->fallthru_clause=0;return Cyc_Tcenv_put_fenv(r,
te,fe);}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_loop(struct _RegionHandle*r,struct
Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*continue_dest){struct Cyc_Tcenv_Fenv
_tmp1DC;struct Cyc_Tcenv_SharedFenv*_tmp1DD;struct Cyc_List_List*_tmp1DE;struct Cyc_RgnOrder_RgnPO*
_tmp1DF;const struct Cyc_Tcenv_Bindings*_tmp1E0;struct Cyc_Absyn_Stmt*_tmp1E1;
struct Cyc_Tcenv_CtrlEnv*_tmp1E2;void*_tmp1E3;void*_tmp1E4;int _tmp1E5;int _tmp1E6;
const char*_tmp3D8;struct Cyc_Tcenv_Fenv*_tmp1DB=Cyc_Tcenv_get_fenv(te,((_tmp3D8="set_in_loop",
_tag_dyneither(_tmp3D8,sizeof(char),12))));_tmp1DC=*_tmp1DB;_tmp1DD=_tmp1DC.shared;
_tmp1DE=_tmp1DC.type_vars;_tmp1DF=_tmp1DC.region_order;_tmp1E0=_tmp1DC.locals;
_tmp1E1=_tmp1DC.encloser;_tmp1E2=_tmp1DC.ctrl_env;_tmp1E3=_tmp1DC.capability;
_tmp1E4=_tmp1DC.curr_rgn;_tmp1E5=_tmp1DC.in_notreadctxt;_tmp1E6=_tmp1DC.in_lhs;{
struct Cyc_Tcenv_Stmt_j_struct*_tmp3E3;struct Cyc_Tcenv_Stmt_j_struct _tmp3E2;
struct Cyc_Tcenv_Stmt_j_struct*_tmp3E1;struct Cyc_Tcenv_Stmt_j_struct _tmp3E0;
struct Cyc_Tcenv_CtrlEnv*_tmp3DF;struct Cyc_Tcenv_CtrlEnv*new_cenv=(_tmp3DF=
_region_malloc(r,sizeof(*_tmp3DF)),((_tmp3DF->ctrl_rgn=r,((_tmp3DF->continue_stmt=(
void*)((_tmp3E1=_region_malloc(r,sizeof(*_tmp3E1)),((_tmp3E1[0]=((_tmp3E0.tag=3,((
_tmp3E0.f1=continue_dest,_tmp3E0)))),_tmp3E1)))),((_tmp3DF->break_stmt=(void*)
_tmp1E2->next_stmt,((_tmp3DF->next_stmt=(void*)((_tmp3E3=_region_malloc(r,
sizeof(*_tmp3E3)),((_tmp3E3[0]=((_tmp3E2.tag=3,((_tmp3E2.f1=continue_dest,
_tmp3E2)))),_tmp3E3)))),((_tmp3DF->fallthru_clause=(const struct _tuple10*)_tmp1E2->fallthru_clause,((
_tmp3DF->try_depth=_tmp1E2->try_depth,_tmp3DF)))))))))))));struct Cyc_Tcenv_Fenv*
_tmp3E4;struct Cyc_Tcenv_Fenv*new_fenv=(_tmp3E4=_region_malloc(r,sizeof(*_tmp3E4)),((
_tmp3E4->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp1DD,((_tmp3E4->type_vars=(
struct Cyc_List_List*)_tmp1DE,((_tmp3E4->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp1DF,((_tmp3E4->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp1E0),((_tmp3E4->encloser=(struct Cyc_Absyn_Stmt*)_tmp1E1,((_tmp3E4->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)new_cenv,((_tmp3E4->capability=(void*)_tmp1E3,((_tmp3E4->curr_rgn=(
void*)_tmp1E4,((_tmp3E4->in_notreadctxt=(int)_tmp1E5,((_tmp3E4->in_lhs=(int)
_tmp1E6,((_tmp3E4->fnrgn=(struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp3E4)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,new_fenv);};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_try(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){const char*_tmp3E5;struct Cyc_Tcenv_Fenv*
fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(te,((_tmp3E5="enter_try",
_tag_dyneither(_tmp3E5,sizeof(char),10)))));struct Cyc_Tcenv_Fenv _tmp1EE;struct
Cyc_Tcenv_CtrlEnv*_tmp1EF;struct Cyc_Tcenv_Fenv*_tmp1ED=fe;_tmp1EE=*_tmp1ED;
_tmp1EF=_tmp1EE.ctrl_env;++ _tmp1EF->try_depth;return Cyc_Tcenv_put_fenv(r,te,fe);}
int Cyc_Tcenv_get_try_depth(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmp1F3;
struct Cyc_Tcenv_CtrlEnv*_tmp1F4;const char*_tmp3E6;struct Cyc_Tcenv_Fenv*_tmp1F2=
Cyc_Tcenv_get_fenv(te,((_tmp3E6="get_try_depth",_tag_dyneither(_tmp3E6,sizeof(
char),14))));_tmp1F3=*_tmp1F2;_tmp1F4=_tmp1F3.ctrl_env;return _tmp1F4->try_depth;}
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_switch(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te){const char*_tmp3E7;struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(
te,((_tmp3E7="set_in_switch",_tag_dyneither(_tmp3E7,sizeof(char),14)))));struct
Cyc_Tcenv_Fenv _tmp1F6;struct Cyc_Tcenv_CtrlEnv*_tmp1F7;struct Cyc_Tcenv_Fenv*
_tmp1F5=fe;_tmp1F6=*_tmp1F5;_tmp1F7=_tmp1F6.ctrl_env;_tmp1F7->break_stmt=_tmp1F7->next_stmt;
_tmp1F7->next_stmt=(void*)& Cyc_Tcenv_CaseEnd_j_val;return Cyc_Tcenv_put_fenv(r,te,
fe);}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_next(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te,void*j){struct Cyc_Tcenv_SharedFenv*_tmp1FB;struct Cyc_List_List*_tmp1FC;struct
Cyc_RgnOrder_RgnPO*_tmp1FD;const struct Cyc_Tcenv_Bindings*_tmp1FE;struct Cyc_Absyn_Stmt*
_tmp1FF;struct Cyc_Tcenv_CtrlEnv*_tmp200;void*_tmp201;void*_tmp202;int _tmp203;int
_tmp204;const char*_tmp3E8;struct Cyc_Tcenv_Fenv _tmp1FA=*Cyc_Tcenv_get_fenv(te,((
_tmp3E8="set_next",_tag_dyneither(_tmp3E8,sizeof(char),9))));_tmp1FB=_tmp1FA.shared;
_tmp1FC=_tmp1FA.type_vars;_tmp1FD=_tmp1FA.region_order;_tmp1FE=_tmp1FA.locals;
_tmp1FF=_tmp1FA.encloser;_tmp200=_tmp1FA.ctrl_env;_tmp201=_tmp1FA.capability;
_tmp202=_tmp1FA.curr_rgn;_tmp203=_tmp1FA.in_notreadctxt;_tmp204=_tmp1FA.in_lhs;{
struct Cyc_Tcenv_CtrlEnv*_tmp3E9;struct Cyc_Tcenv_CtrlEnv*new_cenv=(_tmp3E9=
_region_malloc(r,sizeof(*_tmp3E9)),((_tmp3E9->ctrl_rgn=r,((_tmp3E9->continue_stmt=(
void*)_tmp200->continue_stmt,((_tmp3E9->break_stmt=(void*)_tmp200->break_stmt,((
_tmp3E9->next_stmt=j,((_tmp3E9->fallthru_clause=(const struct _tuple10*)_tmp200->fallthru_clause,((
_tmp3E9->try_depth=_tmp200->try_depth,_tmp3E9)))))))))))));struct Cyc_Tcenv_Fenv*
_tmp3EA;struct Cyc_Tcenv_Fenv*new_fenv=(_tmp3EA=_region_malloc(r,sizeof(*_tmp3EA)),((
_tmp3EA->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp1FB,((_tmp3EA->type_vars=(
struct Cyc_List_List*)_tmp1FC,((_tmp3EA->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp1FD,((_tmp3EA->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp1FE),((_tmp3EA->encloser=(struct Cyc_Absyn_Stmt*)_tmp1FF,((_tmp3EA->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)new_cenv,((_tmp3EA->capability=(void*)_tmp201,((_tmp3EA->curr_rgn=(
void*)_tmp202,((_tmp3EA->in_notreadctxt=(int)_tmp203,((_tmp3EA->in_lhs=(int)
_tmp204,((_tmp3EA->fnrgn=(struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp3EA)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,new_fenv);};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_label(
struct Cyc_Tcenv_Tenv*te,struct _dyneither_ptr*v,struct Cyc_Absyn_Stmt*s){struct Cyc_Tcenv_Fenv
_tmp209;struct Cyc_Tcenv_SharedFenv*_tmp20A;const char*_tmp3EB;struct Cyc_Tcenv_Fenv*
_tmp208=Cyc_Tcenv_get_fenv(te,((_tmp3EB="add_label",_tag_dyneither(_tmp3EB,
sizeof(char),10))));_tmp209=*_tmp208;_tmp20A=_tmp209.shared;{struct Cyc_Dict_Dict
needed=_tmp20A->needed_labels;struct Cyc_List_List**sl_opt=((struct Cyc_List_List**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup_opt)(needed,v);
struct _RegionHandle*frgn=_tmp20A->frgn;if(sl_opt != 0){_tmp20A->needed_labels=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,struct Cyc_Dict_Dict,struct
_dyneither_ptr*))Cyc_Dict_rdelete)(frgn,needed,v);{struct Cyc_List_List*_tmp20B=*
sl_opt;s->non_local_preds=_tmp20B;for(0;_tmp20B != 0;_tmp20B=_tmp20B->tl){void*
_tmp20C=((struct Cyc_Absyn_Stmt*)_tmp20B->hd)->r;struct Cyc_Absyn_Stmt*_tmp20E;
struct Cyc_Absyn_Stmt**_tmp20F;_LL8D: {struct Cyc_Absyn_Goto_s_struct*_tmp20D=(
struct Cyc_Absyn_Goto_s_struct*)_tmp20C;if(_tmp20D->tag != 8)goto _LL8F;else{
_tmp20E=_tmp20D->f2;_tmp20F=(struct Cyc_Absyn_Stmt**)& _tmp20D->f2;}}_LL8E:*
_tmp20F=(struct Cyc_Absyn_Stmt*)s;goto _LL8C;_LL8F:;_LL90:{const char*_tmp3EC;((int(*)(
struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp3EC="Tcenv: add_label backpatching of non-goto",
_tag_dyneither(_tmp3EC,sizeof(char),42))));}goto _LL8C;_LL8C:;}};}if(((int(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_member)(_tmp20A->seen_labels,
v)){const char*_tmp3F0;void*_tmp3EF[1];struct Cyc_String_pa_struct _tmp3EE;(_tmp3EE.tag=
0,((_tmp3EE.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*v),((_tmp3EF[0]=&
_tmp3EE,Cyc_Tcutil_terr(s->loc,((_tmp3F0="Repeated label: %s",_tag_dyneither(
_tmp3F0,sizeof(char),19))),_tag_dyneither(_tmp3EF,sizeof(void*),1)))))));}
_tmp20A->seen_labels=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k,struct Cyc_Absyn_Stmt*v))Cyc_Dict_insert)(_tmp20A->seen_labels,v,
s);return te;};}int Cyc_Tcenv_all_labels_resolved(struct Cyc_Tcenv_Tenv*te){struct
Cyc_Tcenv_Fenv _tmp216;struct Cyc_Tcenv_SharedFenv*_tmp217;const char*_tmp3F1;
struct Cyc_Tcenv_Fenv*_tmp215=Cyc_Tcenv_get_fenv(te,((_tmp3F1="all_labels_resolved",
_tag_dyneither(_tmp3F1,sizeof(char),20))));_tmp216=*_tmp215;_tmp217=_tmp216.shared;
return((int(*)(struct Cyc_Dict_Dict d))Cyc_Dict_is_empty)(_tmp217->needed_labels);}
struct Cyc_Absyn_Stmt*Cyc_Tcenv_get_encloser(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv
_tmp21A;struct Cyc_Absyn_Stmt*_tmp21B;const char*_tmp3F2;struct Cyc_Tcenv_Fenv*
_tmp219=Cyc_Tcenv_get_fenv(te,((_tmp3F2="get_encloser",_tag_dyneither(_tmp3F2,
sizeof(char),13))));_tmp21A=*_tmp219;_tmp21B=_tmp21A.encloser;return _tmp21B;}
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct _RegionHandle*r2,struct Cyc_Tcenv_Tenv*
te){if(te->le == 0){struct Cyc_Tcenv_Tenv*_tmp3F3;return(_tmp3F3=_region_malloc(r2,
sizeof(*_tmp3F3)),((_tmp3F3->ns=te->ns,((_tmp3F3->ae=te->ae,((_tmp3F3->le=0,((
_tmp3F3->allow_valueof=1,_tmp3F3)))))))));}{struct Cyc_Tcenv_SharedFenv*_tmp21E;
struct Cyc_List_List*_tmp21F;struct Cyc_RgnOrder_RgnPO*_tmp220;const struct Cyc_Tcenv_Bindings*
_tmp221;struct Cyc_Absyn_Stmt*_tmp222;struct Cyc_Tcenv_CtrlEnv*_tmp223;void*
_tmp224;void*_tmp225;int _tmp226;int _tmp227;struct Cyc_Tcenv_Fenv _tmp21D=*((struct
Cyc_Tcenv_Fenv*)_check_null(te->le));_tmp21E=_tmp21D.shared;_tmp21F=_tmp21D.type_vars;
_tmp220=_tmp21D.region_order;_tmp221=_tmp21D.locals;_tmp222=_tmp21D.encloser;
_tmp223=_tmp21D.ctrl_env;_tmp224=_tmp21D.capability;_tmp225=_tmp21D.curr_rgn;
_tmp226=_tmp21D.in_notreadctxt;_tmp227=_tmp21D.in_lhs;{struct Cyc_Tcenv_Fenv*
_tmp3F4;struct Cyc_Tcenv_Fenv*_tmp228=(_tmp3F4=_region_malloc(r2,sizeof(*_tmp3F4)),((
_tmp3F4->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp21E,((_tmp3F4->type_vars=(
struct Cyc_List_List*)_tmp21F,((_tmp3F4->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp220,((_tmp3F4->locals=(const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)
_tmp221),((_tmp3F4->encloser=(struct Cyc_Absyn_Stmt*)_tmp222,((_tmp3F4->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp223),((_tmp3F4->capability=(
void*)_tmp224,((_tmp3F4->curr_rgn=(void*)_tmp225,((_tmp3F4->in_notreadctxt=(int)
_tmp226,((_tmp3F4->in_lhs=(int)_tmp227,((_tmp3F4->fnrgn=(struct _RegionHandle*)r2,(
struct Cyc_Tcenv_Fenv*)_tmp3F4)))))))))))))))))))))));struct Cyc_Tcenv_Tenv*
_tmp3F5;return(_tmp3F5=_region_malloc(r2,sizeof(*_tmp3F5)),((_tmp3F5->ns=te->ns,((
_tmp3F5->ae=te->ae,((_tmp3F5->le=_tmp228,((_tmp3F5->allow_valueof=1,_tmp3F5)))))))));};};}
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_encloser(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Absyn_Stmt*s){struct Cyc_Tcenv_SharedFenv*_tmp22D;struct Cyc_List_List*
_tmp22E;struct Cyc_RgnOrder_RgnPO*_tmp22F;const struct Cyc_Tcenv_Bindings*_tmp230;
struct Cyc_Absyn_Stmt*_tmp231;struct Cyc_Tcenv_CtrlEnv*_tmp232;void*_tmp233;void*
_tmp234;int _tmp235;int _tmp236;const char*_tmp3F6;struct Cyc_Tcenv_Fenv _tmp22C=*Cyc_Tcenv_get_fenv(
te,((_tmp3F6="set_encloser",_tag_dyneither(_tmp3F6,sizeof(char),13))));_tmp22D=
_tmp22C.shared;_tmp22E=_tmp22C.type_vars;_tmp22F=_tmp22C.region_order;_tmp230=
_tmp22C.locals;_tmp231=_tmp22C.encloser;_tmp232=_tmp22C.ctrl_env;_tmp233=_tmp22C.capability;
_tmp234=_tmp22C.curr_rgn;_tmp235=_tmp22C.in_notreadctxt;_tmp236=_tmp22C.in_lhs;{
struct Cyc_Tcenv_Fenv*_tmp3F7;struct Cyc_Tcenv_Fenv*_tmp237=(_tmp3F7=
_region_malloc(r,sizeof(*_tmp3F7)),((_tmp3F7->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp22D,((_tmp3F7->type_vars=(struct Cyc_List_List*)_tmp22E,((_tmp3F7->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp22F,((_tmp3F7->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmp230),((_tmp3F7->encloser=(struct Cyc_Absyn_Stmt*)
s,((_tmp3F7->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp232),((_tmp3F7->capability=(void*)_tmp233,((_tmp3F7->curr_rgn=(void*)_tmp234,((
_tmp3F7->in_notreadctxt=(int)_tmp235,((_tmp3F7->in_lhs=(int)_tmp236,((_tmp3F7->fnrgn=(
struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp3F7)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp237);};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*rgn,int resetable,int opened){
struct Cyc_Absyn_Tvar*tv;{void*_tmp239=Cyc_Tcutil_compress(rgn);struct Cyc_Absyn_Tvar*
_tmp23B;_LL92: {struct Cyc_Absyn_VarType_struct*_tmp23A=(struct Cyc_Absyn_VarType_struct*)
_tmp239;if(_tmp23A->tag != 2)goto _LL94;else{_tmp23B=_tmp23A->f1;}}_LL93: tv=
_tmp23B;goto _LL91;_LL94:;_LL95:{const char*_tmp3F8;tv=((struct Cyc_Absyn_Tvar*(*)(
struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp3F8="bad add region",
_tag_dyneither(_tmp3F8,sizeof(char),15))));}goto _LL91;_LL91:;}{struct Cyc_Tcenv_SharedFenv*
_tmp23F;struct Cyc_List_List*_tmp240;struct Cyc_RgnOrder_RgnPO*_tmp241;const struct
Cyc_Tcenv_Bindings*_tmp242;struct Cyc_Absyn_Stmt*_tmp243;struct Cyc_Tcenv_CtrlEnv*
_tmp244;void*_tmp245;void*_tmp246;int _tmp247;int _tmp248;const char*_tmp3F9;struct
Cyc_Tcenv_Fenv _tmp23E=*Cyc_Tcenv_get_fenv(te,((_tmp3F9="add_region",
_tag_dyneither(_tmp3F9,sizeof(char),11))));_tmp23F=_tmp23E.shared;_tmp240=
_tmp23E.type_vars;_tmp241=_tmp23E.region_order;_tmp242=_tmp23E.locals;_tmp243=
_tmp23E.encloser;_tmp244=_tmp23E.ctrl_env;_tmp245=_tmp23E.capability;_tmp246=
_tmp23E.curr_rgn;_tmp247=_tmp23E.in_notreadctxt;_tmp248=_tmp23E.in_lhs;_tmp241=
Cyc_RgnOrder_add_youngest(_tmp23F->frgn,_tmp241,tv,resetable,opened);{struct Cyc_Absyn_JoinEff_struct
_tmp40C;struct Cyc_List_List*_tmp40B;struct Cyc_Absyn_AccessEff_struct*_tmp40A;
struct Cyc_Absyn_AccessEff_struct _tmp409;struct Cyc_List_List*_tmp408;struct Cyc_Absyn_JoinEff_struct*
_tmp407;_tmp245=(void*)((_tmp407=_cycalloc(sizeof(*_tmp407)),((_tmp407[0]=((
_tmp40C.tag=24,((_tmp40C.f1=((_tmp408=_cycalloc(sizeof(*_tmp408)),((_tmp408->hd=(
void*)((_tmp40A=_cycalloc(sizeof(*_tmp40A)),((_tmp40A[0]=((_tmp409.tag=23,((
_tmp409.f1=(void*)rgn,_tmp409)))),_tmp40A)))),((_tmp408->tl=((_tmp40B=_cycalloc(
sizeof(*_tmp40B)),((_tmp40B->hd=_tmp245,((_tmp40B->tl=0,_tmp40B)))))),_tmp408)))))),
_tmp40C)))),_tmp407))));}{struct Cyc_Tcenv_Fenv*_tmp40D;struct Cyc_Tcenv_Fenv*
_tmp24F=(_tmp40D=_region_malloc(r,sizeof(*_tmp40D)),((_tmp40D->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp23F,((_tmp40D->type_vars=(struct Cyc_List_List*)_tmp240,((_tmp40D->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp241,((_tmp40D->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmp242),((_tmp40D->encloser=(struct Cyc_Absyn_Stmt*)
_tmp243,((_tmp40D->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp244),((_tmp40D->capability=(void*)_tmp245,((_tmp40D->curr_rgn=(void*)_tmp246,((
_tmp40D->in_notreadctxt=(int)_tmp247,((_tmp40D->in_lhs=(int)_tmp248,((_tmp40D->fnrgn=(
struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp40D)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp24F);};};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(
struct _RegionHandle*r,unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Tvar*
block_rgn){struct Cyc_Tcenv_SharedFenv*_tmp253;struct Cyc_List_List*_tmp254;struct
Cyc_RgnOrder_RgnPO*_tmp255;const struct Cyc_Tcenv_Bindings*_tmp256;struct Cyc_Absyn_Stmt*
_tmp257;struct Cyc_Tcenv_CtrlEnv*_tmp258;void*_tmp259;void*_tmp25A;int _tmp25B;int
_tmp25C;const char*_tmp40E;struct Cyc_Tcenv_Fenv _tmp252=*Cyc_Tcenv_get_fenv(te,((
_tmp40E="new_named_block",_tag_dyneither(_tmp40E,sizeof(char),16))));_tmp253=
_tmp252.shared;_tmp254=_tmp252.type_vars;_tmp255=_tmp252.region_order;_tmp256=
_tmp252.locals;_tmp257=_tmp252.encloser;_tmp258=_tmp252.ctrl_env;_tmp259=_tmp252.capability;
_tmp25A=_tmp252.curr_rgn;_tmp25B=_tmp252.in_notreadctxt;_tmp25C=_tmp252.in_lhs;{
const char*_tmp40F;struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_copy_fenv_old_ctrl(r,Cyc_Tcenv_get_fenv(
te,((_tmp40F="new_block",_tag_dyneither(_tmp40F,sizeof(char),10)))));struct Cyc_Absyn_VarType_struct
_tmp412;struct Cyc_Absyn_VarType_struct*_tmp411;void*block_typ=(void*)((_tmp411=
_cycalloc(sizeof(*_tmp411)),((_tmp411[0]=((_tmp412.tag=2,((_tmp412.f1=block_rgn,
_tmp412)))),_tmp411))));{struct Cyc_List_List*_tmp413;_tmp254=((_tmp413=_cycalloc(
sizeof(*_tmp413)),((_tmp413->hd=block_rgn,((_tmp413->tl=_tmp254,_tmp413))))));}
Cyc_Tcutil_check_unique_tvars(loc,_tmp254);_tmp255=Cyc_RgnOrder_add_youngest(
_tmp253->frgn,_tmp255,block_rgn,0,0);{struct Cyc_Absyn_JoinEff_struct _tmp426;
struct Cyc_List_List*_tmp425;struct Cyc_Absyn_AccessEff_struct*_tmp424;struct Cyc_Absyn_AccessEff_struct
_tmp423;struct Cyc_List_List*_tmp422;struct Cyc_Absyn_JoinEff_struct*_tmp421;
_tmp259=(void*)((_tmp421=_cycalloc(sizeof(*_tmp421)),((_tmp421[0]=((_tmp426.tag=
24,((_tmp426.f1=((_tmp422=_cycalloc(sizeof(*_tmp422)),((_tmp422->hd=(void*)((
_tmp424=_cycalloc(sizeof(*_tmp424)),((_tmp424[0]=((_tmp423.tag=23,((_tmp423.f1=(
void*)block_typ,_tmp423)))),_tmp424)))),((_tmp422->tl=((_tmp425=_cycalloc(
sizeof(*_tmp425)),((_tmp425->hd=_tmp259,((_tmp425->tl=0,_tmp425)))))),_tmp422)))))),
_tmp426)))),_tmp421))));}_tmp25A=block_typ;{struct Cyc_Tcenv_Fenv*_tmp427;struct
Cyc_Tcenv_Fenv*_tmp264=(_tmp427=_region_malloc(r,sizeof(*_tmp427)),((_tmp427->shared=(
struct Cyc_Tcenv_SharedFenv*)_tmp253,((_tmp427->type_vars=(struct Cyc_List_List*)
_tmp254,((_tmp427->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmp255,((_tmp427->locals=(
const struct Cyc_Tcenv_Bindings*)((const struct Cyc_Tcenv_Bindings*)_tmp256),((
_tmp427->encloser=(struct Cyc_Absyn_Stmt*)_tmp257,((_tmp427->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
struct Cyc_Tcenv_CtrlEnv*)_tmp258),((_tmp427->capability=(void*)_tmp259,((_tmp427->curr_rgn=(
void*)_tmp25A,((_tmp427->in_notreadctxt=(int)_tmp25B,((_tmp427->in_lhs=(int)
_tmp25C,((_tmp427->fnrgn=(struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp427)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp264);};};}static struct Cyc_Absyn_Eq_kb_struct
Cyc_Tcenv_rgn_kb={0,& Cyc_Tcutil_rk};struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(
struct _RegionHandle*r,unsigned int loc,struct Cyc_Tcenv_Tenv*te){struct Cyc_Absyn_Tvar*
t=Cyc_Tcutil_new_tvar((void*)& Cyc_Tcenv_rgn_kb);Cyc_Tcutil_add_tvar_identity(t);
return Cyc_Tcenv_new_named_block(r,loc,te,t);}struct _tuple13{void*f1;void*f2;};
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(struct _RegionHandle*r,
struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,unsigned int loc){struct Cyc_Tcenv_SharedFenv*
_tmp26C;struct Cyc_List_List*_tmp26D;struct Cyc_RgnOrder_RgnPO*_tmp26E;const struct
Cyc_Tcenv_Bindings*_tmp26F;struct Cyc_Absyn_Stmt*_tmp270;struct Cyc_Tcenv_CtrlEnv*
_tmp271;void*_tmp272;void*_tmp273;int _tmp274;int _tmp275;const char*_tmp428;struct
Cyc_Tcenv_Fenv _tmp26B=*Cyc_Tcenv_get_fenv(te,((_tmp428="new_outlives_constraints",
_tag_dyneither(_tmp428,sizeof(char),25))));_tmp26C=_tmp26B.shared;_tmp26D=
_tmp26B.type_vars;_tmp26E=_tmp26B.region_order;_tmp26F=_tmp26B.locals;_tmp270=
_tmp26B.encloser;_tmp271=_tmp26B.ctrl_env;_tmp272=_tmp26B.capability;_tmp273=
_tmp26B.curr_rgn;_tmp274=_tmp26B.in_notreadctxt;_tmp275=_tmp26B.in_lhs;for(0;cs
!= 0;cs=cs->tl){_tmp26E=Cyc_RgnOrder_add_outlives_constraint(_tmp26C->frgn,
_tmp26E,(*((struct _tuple13*)cs->hd)).f1,(*((struct _tuple13*)cs->hd)).f2,loc);}{
struct Cyc_Tcenv_Fenv*_tmp429;struct Cyc_Tcenv_Fenv*_tmp276=(_tmp429=
_region_malloc(r,sizeof(*_tmp429)),((_tmp429->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp26C,((_tmp429->type_vars=(struct Cyc_List_List*)_tmp26D,((_tmp429->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp26E,((_tmp429->locals=(const struct Cyc_Tcenv_Bindings*)((
const struct Cyc_Tcenv_Bindings*)_tmp26F),((_tmp429->encloser=(struct Cyc_Absyn_Stmt*)
_tmp270,((_tmp429->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp271),((_tmp429->capability=(void*)_tmp272,((_tmp429->curr_rgn=(void*)_tmp273,((
_tmp429->in_notreadctxt=(int)_tmp274,((_tmp429->in_lhs=(int)_tmp275,((_tmp429->fnrgn=(
struct _RegionHandle*)r,(struct Cyc_Tcenv_Fenv*)_tmp429)))))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp276);};}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region_equality(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*r1,void*r2,struct _tuple11**
oldtv,unsigned int loc){void*_tmp278=Cyc_Tcutil_compress(r1);void*_tmp279=Cyc_Tcutil_compress(
r2);struct Cyc_Absyn_Kind*_tmp27A=Cyc_Tcutil_typ_kind(_tmp278);struct Cyc_Absyn_Kind*
_tmp27B=Cyc_Tcutil_typ_kind(_tmp279);int r1_le_r2=Cyc_Tcutil_kind_leq(_tmp27A,
_tmp27B);int r2_le_r1=Cyc_Tcutil_kind_leq(_tmp27B,_tmp27A);if(!r1_le_r2  && !
r2_le_r1){{const char*_tmp430;void*_tmp42F[4];struct Cyc_String_pa_struct _tmp42E;
struct Cyc_String_pa_struct _tmp42D;struct Cyc_String_pa_struct _tmp42C;struct Cyc_String_pa_struct
_tmp42B;(_tmp42B.tag=0,((_tmp42B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_kind2string(_tmp27B)),((_tmp42C.tag=0,((_tmp42C.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(_tmp27A)),((
_tmp42D.tag=0,((_tmp42D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
_tmp279)),((_tmp42E.tag=0,((_tmp42E.f1=(struct _dyneither_ptr)((struct
_dyneither_ptr)Cyc_Absynpp_typ2string(_tmp278)),((_tmp42F[0]=& _tmp42E,((_tmp42F[
1]=& _tmp42D,((_tmp42F[2]=& _tmp42C,((_tmp42F[3]=& _tmp42B,Cyc_Tcutil_terr(loc,((
_tmp430="Cannot compare region handles for %s and %s\n  kinds %s and %s are not compatible\n",
_tag_dyneither(_tmp430,sizeof(char),82))),_tag_dyneither(_tmp42F,sizeof(void*),4)))))))))))))))))))))))));}
return Cyc_Tcenv_new_outlives_constraints(r,te,0,loc);}else{if(r1_le_r2  && !
r2_le_r1){struct Cyc_Absyn_Tvar*_tmp283;void*_tmp284;struct _tuple11 _tmp282=Cyc_Tcutil_swap_kind(
_tmp279,Cyc_Tcutil_kind_to_bound(_tmp27A));_tmp283=_tmp282.f1;_tmp284=_tmp282.f2;{
struct _tuple11*_tmp431;*oldtv=((_tmp431=_region_malloc(r,sizeof(*_tmp431)),((
_tmp431->f1=_tmp283,((_tmp431->f2=_tmp284,_tmp431))))));};}else{if(!r1_le_r2  && 
r2_le_r1){struct Cyc_Absyn_Tvar*_tmp287;void*_tmp288;struct _tuple11 _tmp286=Cyc_Tcutil_swap_kind(
_tmp278,Cyc_Tcutil_kind_to_bound(_tmp27B));_tmp287=_tmp286.f1;_tmp288=_tmp286.f2;{
struct _tuple11*_tmp432;*oldtv=((_tmp432=_region_malloc(r,sizeof(*_tmp432)),((
_tmp432->f1=_tmp287,((_tmp432->f2=_tmp288,_tmp432))))));};}}}{struct
_RegionHandle*_tmp28A=Cyc_Tcenv_get_fnrgn(te);struct Cyc_List_List*_tmp28B=0;if((
_tmp278 != (void*)& Cyc_Absyn_HeapRgn_val  && _tmp278 != (void*)& Cyc_Absyn_UniqueRgn_val)
 && _tmp278 != (void*)& Cyc_Absyn_RefCntRgn_val){struct Cyc_Absyn_AccessEff_struct
_tmp435;struct Cyc_Absyn_AccessEff_struct*_tmp434;void*eff1=(void*)((_tmp434=
_cycalloc(sizeof(*_tmp434)),((_tmp434[0]=((_tmp435.tag=23,((_tmp435.f1=(void*)
_tmp278,_tmp435)))),_tmp434))));struct _tuple13*_tmp438;struct Cyc_List_List*
_tmp437;_tmp28B=((_tmp437=_region_malloc(_tmp28A,sizeof(*_tmp437)),((_tmp437->hd=((
_tmp438=_region_malloc(_tmp28A,sizeof(*_tmp438)),((_tmp438->f1=eff1,((_tmp438->f2=
_tmp279,_tmp438)))))),((_tmp437->tl=_tmp28B,_tmp437))))));}if((_tmp279 != (void*)&
Cyc_Absyn_HeapRgn_val  && _tmp279 != (void*)& Cyc_Absyn_UniqueRgn_val) && _tmp279 != (
void*)& Cyc_Absyn_RefCntRgn_val){struct Cyc_Absyn_AccessEff_struct _tmp43B;struct
Cyc_Absyn_AccessEff_struct*_tmp43A;void*eff2=(void*)((_tmp43A=_cycalloc(sizeof(*
_tmp43A)),((_tmp43A[0]=((_tmp43B.tag=23,((_tmp43B.f1=(void*)_tmp279,_tmp43B)))),
_tmp43A))));struct _tuple13*_tmp43E;struct Cyc_List_List*_tmp43D;_tmp28B=((_tmp43D=
_region_malloc(_tmp28A,sizeof(*_tmp43D)),((_tmp43D->hd=((_tmp43E=_region_malloc(
_tmp28A,sizeof(*_tmp43E)),((_tmp43E->f1=eff2,((_tmp43E->f2=_tmp278,_tmp43E)))))),((
_tmp43D->tl=_tmp28B,_tmp43D))))));}return Cyc_Tcenv_new_outlives_constraints((
struct _RegionHandle*)_tmp28A,te,_tmp28B,loc);};}void*Cyc_Tcenv_curr_rgn(struct
Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp294=te->le;if(_tmp294 == 0)return(
void*)& Cyc_Absyn_HeapRgn_val;{struct Cyc_Tcenv_Fenv*fe=(struct Cyc_Tcenv_Fenv*)
_tmp294;struct Cyc_Tcenv_Fenv _tmp296;void*_tmp297;struct Cyc_Tcenv_Fenv*_tmp295=fe;
_tmp296=*_tmp295;_tmp297=_tmp296.curr_rgn;return _tmp297;};}void Cyc_Tcenv_check_rgn_accessible(
struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*rgn){const char*_tmp43F;struct Cyc_Tcenv_Fenv*
fe=Cyc_Tcenv_get_fenv(te,((_tmp43F="check_rgn_accessible",_tag_dyneither(_tmp43F,
sizeof(char),21))));struct Cyc_Tcenv_Fenv _tmp299;void*_tmp29A;struct Cyc_RgnOrder_RgnPO*
_tmp29B;struct Cyc_Tcenv_Fenv*_tmp298=fe;_tmp299=*_tmp298;_tmp29A=_tmp299.capability;
_tmp29B=_tmp299.region_order;if(Cyc_Tcutil_region_in_effect(0,rgn,_tmp29A) || 
Cyc_Tcutil_region_in_effect(1,rgn,_tmp29A))return;{struct Cyc_Absyn_AccessEff_struct
_tmp442;struct Cyc_Absyn_AccessEff_struct*_tmp441;if(Cyc_RgnOrder_eff_outlives_eff(
_tmp29B,(void*)((_tmp441=_cycalloc(sizeof(*_tmp441)),((_tmp441[0]=((_tmp442.tag=
23,((_tmp442.f1=(void*)rgn,_tmp442)))),_tmp441)))),_tmp29A))return;}{const char*
_tmp446;void*_tmp445[1];struct Cyc_String_pa_struct _tmp444;(_tmp444.tag=0,((
_tmp444.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
rgn)),((_tmp445[0]=& _tmp444,Cyc_Tcutil_terr(loc,((_tmp446="Expression accesses unavailable region %s",
_tag_dyneither(_tmp446,sizeof(char),42))),_tag_dyneither(_tmp445,sizeof(void*),1)))))));};}
void Cyc_Tcenv_check_rgn_resetable(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*
rgn){Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);{struct Cyc_Tcenv_Fenv _tmp2A4;
struct Cyc_RgnOrder_RgnPO*_tmp2A5;const char*_tmp447;struct Cyc_Tcenv_Fenv*_tmp2A3=
Cyc_Tcenv_get_fenv(te,((_tmp447="check_rgn_resetable",_tag_dyneither(_tmp447,
sizeof(char),20))));_tmp2A4=*_tmp2A3;_tmp2A5=_tmp2A4.region_order;{void*_tmp2A6=
Cyc_Tcutil_compress(rgn);struct Cyc_Absyn_Tvar*_tmp2A8;_LL97: {struct Cyc_Absyn_VarType_struct*
_tmp2A7=(struct Cyc_Absyn_VarType_struct*)_tmp2A6;if(_tmp2A7->tag != 2)goto _LL99;
else{_tmp2A8=_tmp2A7->f1;}}_LL98: if(!Cyc_RgnOrder_is_region_resetable(_tmp2A5,
_tmp2A8)){const char*_tmp44B;void*_tmp44A[1];struct Cyc_String_pa_struct _tmp449;(
_tmp449.tag=0,((_tmp449.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
rgn)),((_tmp44A[0]=& _tmp449,Cyc_Tcutil_terr(loc,((_tmp44B="Region %s is not resetable",
_tag_dyneither(_tmp44B,sizeof(char),27))),_tag_dyneither(_tmp44A,sizeof(void*),1)))))));}
return;_LL99:;_LL9A: {const char*_tmp44E;void*_tmp44D;(_tmp44D=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp44E="check_rgn_resetable",
_tag_dyneither(_tmp44E,sizeof(char),20))),_tag_dyneither(_tmp44D,sizeof(void*),0)));}
_LL96:;};};}int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*te,void*rt_a,void*
rt_b){struct Cyc_Tcenv_Fenv*_tmp2AE=te->le;rt_a=Cyc_Tcutil_compress(rt_a);rt_b=
Cyc_Tcutil_compress(rt_b);if(_tmp2AE == 0){void*_tmp2AF=rt_a;_LL9C: {struct Cyc_Absyn_RefCntRgn_struct*
_tmp2B0=(struct Cyc_Absyn_RefCntRgn_struct*)_tmp2AF;if(_tmp2B0->tag != 22)goto
_LL9E;}_LL9D: return rt_b != (void*)& Cyc_Absyn_UniqueRgn_val;_LL9E: {struct Cyc_Absyn_UniqueRgn_struct*
_tmp2B1=(struct Cyc_Absyn_UniqueRgn_struct*)_tmp2AF;if(_tmp2B1->tag != 21)goto
_LLA0;}_LL9F: return rt_b != (void*)& Cyc_Absyn_RefCntRgn_val;_LLA0: {struct Cyc_Absyn_HeapRgn_struct*
_tmp2B2=(struct Cyc_Absyn_HeapRgn_struct*)_tmp2AF;if(_tmp2B2->tag != 20)goto _LLA2;}
_LLA1: return rt_b == (void*)& Cyc_Absyn_HeapRgn_val;_LLA2:;_LLA3: return 0;_LL9B:;}{
struct Cyc_Tcenv_Fenv*fe=(struct Cyc_Tcenv_Fenv*)_tmp2AE;struct Cyc_Tcenv_Fenv
_tmp2B4;struct Cyc_RgnOrder_RgnPO*_tmp2B5;struct Cyc_Tcenv_Fenv*_tmp2B3=fe;_tmp2B4=*
_tmp2B3;_tmp2B5=_tmp2B4.region_order;{struct Cyc_Absyn_AccessEff_struct _tmp451;
struct Cyc_Absyn_AccessEff_struct*_tmp450;int _tmp2B6=Cyc_RgnOrder_effect_outlives(
_tmp2B5,(void*)((_tmp450=_cycalloc(sizeof(*_tmp450)),((_tmp450[0]=((_tmp451.tag=
23,((_tmp451.f1=(void*)rt_a,_tmp451)))),_tmp450)))),rt_b);return _tmp2B6;};};}
struct _tuple14{void*f1;void*f2;struct Cyc_RgnOrder_RgnPO*f3;unsigned int f4;};void
Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*
eff){struct Cyc_Tcenv_Fenv _tmp2BB;void*_tmp2BC;struct Cyc_RgnOrder_RgnPO*_tmp2BD;
struct Cyc_Tcenv_SharedFenv*_tmp2BE;const char*_tmp452;struct Cyc_Tcenv_Fenv*
_tmp2BA=Cyc_Tcenv_get_fenv(te,((_tmp452="check_effect_accessible",_tag_dyneither(
_tmp452,sizeof(char),24))));_tmp2BB=*_tmp2BA;_tmp2BC=_tmp2BB.capability;_tmp2BD=
_tmp2BB.region_order;_tmp2BE=_tmp2BB.shared;if(Cyc_Tcutil_subset_effect(0,eff,
_tmp2BC))return;if(Cyc_RgnOrder_eff_outlives_eff(_tmp2BD,eff,_tmp2BC))return;{
struct _RegionHandle*frgn=_tmp2BE->frgn;struct _tuple14*_tmp455;struct Cyc_List_List*
_tmp454;_tmp2BE->delayed_effect_checks=((_tmp454=_region_malloc(frgn,sizeof(*
_tmp454)),((_tmp454->hd=((_tmp455=_region_malloc(frgn,sizeof(*_tmp455)),((
_tmp455->f1=_tmp2BC,((_tmp455->f2=eff,((_tmp455->f3=_tmp2BD,((_tmp455->f4=loc,
_tmp455)))))))))),((_tmp454->tl=_tmp2BE->delayed_effect_checks,_tmp454))))));};}
void Cyc_Tcenv_check_delayed_effects(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv
_tmp2C3;struct Cyc_Tcenv_SharedFenv*_tmp2C4;const char*_tmp456;struct Cyc_Tcenv_Fenv*
_tmp2C2=Cyc_Tcenv_get_fenv(te,((_tmp456="check_delayed_effects",_tag_dyneither(
_tmp456,sizeof(char),22))));_tmp2C3=*_tmp2C2;_tmp2C4=_tmp2C3.shared;{struct Cyc_List_List*
_tmp2C5=_tmp2C4->delayed_effect_checks;for(0;_tmp2C5 != 0;_tmp2C5=_tmp2C5->tl){
struct _tuple14 _tmp2C7;void*_tmp2C8;void*_tmp2C9;struct Cyc_RgnOrder_RgnPO*_tmp2CA;
unsigned int _tmp2CB;struct _tuple14*_tmp2C6=(struct _tuple14*)_tmp2C5->hd;_tmp2C7=*
_tmp2C6;_tmp2C8=_tmp2C7.f1;_tmp2C9=_tmp2C7.f2;_tmp2CA=_tmp2C7.f3;_tmp2CB=_tmp2C7.f4;
if(Cyc_Tcutil_subset_effect(1,_tmp2C9,_tmp2C8))continue;if(Cyc_RgnOrder_eff_outlives_eff(
_tmp2CA,_tmp2C9,_tmp2C8))continue;{const char*_tmp45B;void*_tmp45A[2];struct Cyc_String_pa_struct
_tmp459;struct Cyc_String_pa_struct _tmp458;(_tmp458.tag=0,((_tmp458.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp2C9)),((_tmp459.tag=
0,((_tmp459.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
_tmp2C8)),((_tmp45A[0]=& _tmp459,((_tmp45A[1]=& _tmp458,Cyc_Tcutil_terr(_tmp2CB,((
_tmp45B="Capability \n%s\ndoes not cover function's effect\n%s",_tag_dyneither(
_tmp45B,sizeof(char),51))),_tag_dyneither(_tmp45A,sizeof(void*),2)))))))))))));};}};}
struct _tuple15{struct Cyc_RgnOrder_RgnPO*f1;struct Cyc_List_List*f2;unsigned int f3;
};void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,unsigned int loc,
struct Cyc_List_List*po){struct Cyc_Tcenv_Fenv*_tmp2D0=te->le;if(_tmp2D0 == 0){for(
0;po != 0;po=po->tl){struct Cyc_Absyn_AccessEff_struct _tmp45E;struct Cyc_Absyn_AccessEff_struct*
_tmp45D;if(!Cyc_Tcutil_subset_effect(1,(*((struct _tuple13*)po->hd)).f1,Cyc_Absyn_empty_effect)
 || !Cyc_Tcutil_subset_effect(1,(void*)((_tmp45D=_cycalloc(sizeof(*_tmp45D)),((
_tmp45D[0]=((_tmp45E.tag=23,((_tmp45E.f1=(void*)(*((struct _tuple13*)po->hd)).f2,
_tmp45E)))),_tmp45D)))),Cyc_Absyn_empty_effect)){const char*_tmp461;void*_tmp460;(
_tmp460=0,Cyc_Tcutil_terr(loc,((_tmp461="the required region ordering is not satisfied here",
_tag_dyneither(_tmp461,sizeof(char),51))),_tag_dyneither(_tmp460,sizeof(void*),0)));}}
return;}{struct Cyc_Tcenv_Fenv _tmp2D6;struct Cyc_RgnOrder_RgnPO*_tmp2D7;struct Cyc_Tcenv_SharedFenv*
_tmp2D8;struct Cyc_Tcenv_Fenv*_tmp2D5=(struct Cyc_Tcenv_Fenv*)_tmp2D0;_tmp2D6=*
_tmp2D5;_tmp2D7=_tmp2D6.region_order;_tmp2D8=_tmp2D6.shared;if(!Cyc_RgnOrder_satisfies_constraints(
_tmp2D7,po,(void*)& Cyc_Absyn_HeapRgn_val,0)){struct _tuple15*_tmp464;struct Cyc_List_List*
_tmp463;_tmp2D8->delayed_constraint_checks=((_tmp463=_region_malloc(_tmp2D8->frgn,
sizeof(*_tmp463)),((_tmp463->hd=((_tmp464=_region_malloc(_tmp2D8->frgn,sizeof(*
_tmp464)),((_tmp464->f1=_tmp2D7,((_tmp464->f2=po,((_tmp464->f3=loc,_tmp464)))))))),((
_tmp463->tl=_tmp2D8->delayed_constraint_checks,_tmp463))))));}};}void Cyc_Tcenv_check_delayed_constraints(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmp2DD;struct Cyc_Tcenv_SharedFenv*
_tmp2DE;const char*_tmp465;struct Cyc_Tcenv_Fenv*_tmp2DC=Cyc_Tcenv_get_fenv(te,((
_tmp465="check_delayed_constraints",_tag_dyneither(_tmp465,sizeof(char),26))));
_tmp2DD=*_tmp2DC;_tmp2DE=_tmp2DD.shared;{struct Cyc_List_List*_tmp2DF=_tmp2DE->delayed_constraint_checks;
for(0;_tmp2DF != 0;_tmp2DF=_tmp2DF->tl){struct _tuple15 _tmp2E1;struct Cyc_RgnOrder_RgnPO*
_tmp2E2;struct Cyc_List_List*_tmp2E3;unsigned int _tmp2E4;struct _tuple15*_tmp2E0=(
struct _tuple15*)_tmp2DF->hd;_tmp2E1=*_tmp2E0;_tmp2E2=_tmp2E1.f1;_tmp2E3=_tmp2E1.f2;
_tmp2E4=_tmp2E1.f3;if(!Cyc_RgnOrder_satisfies_constraints(_tmp2E2,_tmp2E3,(void*)&
Cyc_Absyn_HeapRgn_val,1)){const char*_tmp468;void*_tmp467;(_tmp467=0,Cyc_Tcutil_terr(
_tmp2E4,((_tmp468="the required region ordering is not satisfied here",
_tag_dyneither(_tmp468,sizeof(char),51))),_tag_dyneither(_tmp467,sizeof(void*),0)));}}};}
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(struct _RegionHandle*r,unsigned int loc,
struct Cyc_Absyn_Fndecl*fd){const struct Cyc_Tcenv_Bindings*locals=0;struct
_dyneither_ptr*_tmp475;const char*_tmp474;void*_tmp473[1];struct Cyc_String_pa_struct
_tmp472;struct Cyc_Absyn_Tvar*_tmp471;struct Cyc_Absyn_Tvar*rgn0=(_tmp471=
_cycalloc(sizeof(*_tmp471)),((_tmp471->name=((_tmp475=_cycalloc(sizeof(*_tmp475)),((
_tmp475[0]=(struct _dyneither_ptr)((_tmp472.tag=0,((_tmp472.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)*(*fd->name).f2),((_tmp473[0]=& _tmp472,Cyc_aprintf(((
_tmp474="`%s",_tag_dyneither(_tmp474,sizeof(char),4))),_tag_dyneither(_tmp473,
sizeof(void*),1)))))))),_tmp475)))),((_tmp471->identity=Cyc_Tcutil_new_tvar_id(),((
_tmp471->kind=(void*)& Cyc_Tcenv_rgn_kb,_tmp471)))))));struct Cyc_List_List*
_tmp476;struct Cyc_List_List*_tmp2E7=(_tmp476=_cycalloc(sizeof(*_tmp476)),((
_tmp476->hd=rgn0,((_tmp476->tl=fd->tvs,_tmp476)))));Cyc_Tcutil_check_unique_tvars(
loc,_tmp2E7);{struct Cyc_RgnOrder_RgnPO*_tmp2E8=Cyc_RgnOrder_initial_fn_po(r,fd->tvs,
fd->rgn_po,(void*)_check_null(fd->effect),rgn0,loc);struct Cyc_Absyn_VarType_struct
_tmp479;struct Cyc_Absyn_VarType_struct*_tmp478;void*param_rgn=(void*)((_tmp478=
_cycalloc(sizeof(*_tmp478)),((_tmp478[0]=((_tmp479.tag=2,((_tmp479.f1=rgn0,
_tmp479)))),_tmp478))));struct Cyc_List_List*_tmp2E9=0;{struct Cyc_List_List*
_tmp2EA=fd->args;for(0;_tmp2EA != 0;_tmp2EA=_tmp2EA->tl){struct Cyc_Absyn_Vardecl
_tmp47F;struct _tuple0*_tmp47E;struct Cyc_Absyn_Vardecl*_tmp47D;struct Cyc_Absyn_Vardecl*
_tmp2EB=(_tmp47D=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)* 1),((_tmp47D[0]=((
_tmp47F.sc=Cyc_Absyn_Public,((_tmp47F.name=((_tmp47E=_cycalloc(sizeof(*_tmp47E)),((
_tmp47E->f1=(union Cyc_Absyn_Nmspace)Cyc_Absyn_Loc_n,((_tmp47E->f2=(*((struct
_tuple8*)_tmp2EA->hd)).f1,_tmp47E)))))),((_tmp47F.tq=(*((struct _tuple8*)_tmp2EA->hd)).f2,((
_tmp47F.type=(*((struct _tuple8*)_tmp2EA->hd)).f3,((_tmp47F.initializer=0,((
_tmp47F.rgn=(void*)param_rgn,((_tmp47F.attributes=0,((_tmp47F.escapes=0,_tmp47F)))))))))))))))),
_tmp47D)));{struct Cyc_List_List _tmp482;struct Cyc_List_List*_tmp481;_tmp2E9=((
_tmp481=_cycalloc(sizeof(struct Cyc_List_List)* 1),((_tmp481[0]=((_tmp482.hd=
_tmp2EB,((_tmp482.tl=_tmp2E9,_tmp482)))),_tmp481))));}{struct Cyc_Absyn_Param_b_struct
_tmp485;struct Cyc_Absyn_Param_b_struct*_tmp484;struct Cyc_Absyn_Param_b_struct*
_tmp2EE=(_tmp484=_cycalloc(sizeof(*_tmp484)),((_tmp484[0]=((_tmp485.tag=3,((
_tmp485.f1=_tmp2EB,_tmp485)))),_tmp484)));struct _dyneither_ptr*_tmp2EF=(*((
struct _tuple8*)_tmp2EA->hd)).f1;struct Cyc_Tcenv_Bindings*_tmp486;locals=(const
struct Cyc_Tcenv_Bindings*)((_tmp486=_region_malloc(r,sizeof(*_tmp486)),((_tmp486->v=
_tmp2EF,((_tmp486->b=(void*)_tmp2EE,((_tmp486->tl=locals,_tmp486))))))));};}}if(
fd->cyc_varargs != 0){struct _dyneither_ptr*_tmp2F7;struct Cyc_Absyn_Tqual _tmp2F8;
void*_tmp2F9;int _tmp2FA;struct Cyc_Absyn_VarargInfo _tmp2F6=*((struct Cyc_Absyn_VarargInfo*)
_check_null(fd->cyc_varargs));_tmp2F7=_tmp2F6.name;_tmp2F8=_tmp2F6.tq;_tmp2F9=
_tmp2F6.type;_tmp2FA=_tmp2F6.inject;if(_tmp2F7 != 0){void*_tmp2FB=Cyc_Absyn_dyneither_typ(
_tmp2F9,param_rgn,_tmp2F8,Cyc_Absyn_false_conref);struct Cyc_Absyn_Vardecl _tmp48C;
struct _tuple0*_tmp48B;struct Cyc_Absyn_Vardecl*_tmp48A;struct Cyc_Absyn_Vardecl*
_tmp2FC=(_tmp48A=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)* 1),((_tmp48A[0]=((
_tmp48C.sc=Cyc_Absyn_Public,((_tmp48C.name=((_tmp48B=_cycalloc(sizeof(*_tmp48B)),((
_tmp48B->f1=(union Cyc_Absyn_Nmspace)Cyc_Absyn_Loc_n,((_tmp48B->f2=(struct
_dyneither_ptr*)_tmp2F7,_tmp48B)))))),((_tmp48C.tq=Cyc_Absyn_empty_tqual(0),((
_tmp48C.type=_tmp2FB,((_tmp48C.initializer=0,((_tmp48C.rgn=(void*)param_rgn,((
_tmp48C.attributes=0,((_tmp48C.escapes=0,_tmp48C)))))))))))))))),_tmp48A)));{
struct Cyc_List_List*_tmp48D;_tmp2E9=((_tmp48D=_cycalloc(sizeof(*_tmp48D)),((
_tmp48D->hd=_tmp2FC,((_tmp48D->tl=_tmp2E9,_tmp48D))))));}{struct Cyc_Absyn_Param_b_struct
_tmp490;struct Cyc_Absyn_Param_b_struct*_tmp48F;struct Cyc_Absyn_Param_b_struct*
_tmp2FE=(_tmp48F=_cycalloc(sizeof(*_tmp48F)),((_tmp48F[0]=((_tmp490.tag=3,((
_tmp490.f1=_tmp2FC,_tmp490)))),_tmp48F)));struct _dyneither_ptr*_tmp2FF=(struct
_dyneither_ptr*)_tmp2F7;struct Cyc_Tcenv_Bindings*_tmp491;locals=(const struct Cyc_Tcenv_Bindings*)((
_tmp491=_region_malloc(r,sizeof(*_tmp491)),((_tmp491->v=_tmp2FF,((_tmp491->b=(
void*)_tmp2FE,((_tmp491->tl=locals,_tmp491))))))));};}else{const char*_tmp494;
void*_tmp493;(_tmp493=0,Cyc_Tcutil_terr(loc,((_tmp494="missing name for varargs",
_tag_dyneither(_tmp494,sizeof(char),25))),_tag_dyneither(_tmp493,sizeof(void*),0)));}}{
struct Cyc_Core_Opt _tmp497;struct Cyc_Core_Opt*_tmp496;fd->param_vardecls=((
_tmp496=_cycalloc(sizeof(struct Cyc_Core_Opt)* 1),((_tmp496[0]=((_tmp497.v=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp2E9),
_tmp497)),_tmp496))));}{struct Cyc_Absyn_JoinEff_struct*_tmp4B5;struct Cyc_List_List*
_tmp4B4;struct Cyc_Absyn_AccessEff_struct _tmp4B3;struct Cyc_Absyn_AccessEff_struct*
_tmp4B2;struct Cyc_List_List*_tmp4B1;struct Cyc_Absyn_JoinEff_struct _tmp4B0;struct
Cyc_Tcenv_CtrlEnv*_tmp4AF;struct Cyc_Tcenv_SharedFenv*_tmp4AE;struct Cyc_Tcenv_Fenv*
_tmp4AD;return(_tmp4AD=_region_malloc(r,sizeof(*_tmp4AD)),((_tmp4AD->shared=(
struct Cyc_Tcenv_SharedFenv*)((_tmp4AE=_region_malloc(r,sizeof(*_tmp4AE)),((
_tmp4AE->frgn=r,((_tmp4AE->return_typ=fd->ret_type,((_tmp4AE->seen_labels=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp4AE->needed_labels=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp4AE->delayed_effect_checks=
0,((_tmp4AE->delayed_constraint_checks=0,_tmp4AE)))))))))))))),((_tmp4AD->type_vars=(
struct Cyc_List_List*)_tmp2E7,((_tmp4AD->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp2E8,((_tmp4AD->locals=(const struct Cyc_Tcenv_Bindings*)locals,((_tmp4AD->encloser=(
struct Cyc_Absyn_Stmt*)fd->body,((_tmp4AD->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
_tmp4AF=_region_malloc(r,sizeof(*_tmp4AF)),((_tmp4AF->ctrl_rgn=r,((_tmp4AF->continue_stmt=(
void*)& Cyc_Tcenv_NotLoop_j_val,((_tmp4AF->break_stmt=(void*)& Cyc_Tcenv_NotLoop_j_val,((
_tmp4AF->fallthru_clause=0,((_tmp4AF->next_stmt=(void*)& Cyc_Tcenv_FnEnd_j_val,((
_tmp4AF->try_depth=0,_tmp4AF)))))))))))))),((_tmp4AD->capability=(void*)((void*)((
_tmp4B5=_cycalloc(sizeof(*_tmp4B5)),((_tmp4B5[0]=((_tmp4B0.tag=24,((_tmp4B0.f1=((
_tmp4B4=_cycalloc(sizeof(*_tmp4B4)),((_tmp4B4->hd=(void*)((_tmp4B2=_cycalloc(
sizeof(*_tmp4B2)),((_tmp4B2[0]=((_tmp4B3.tag=23,((_tmp4B3.f1=(void*)param_rgn,
_tmp4B3)))),_tmp4B2)))),((_tmp4B4->tl=((_tmp4B1=_cycalloc(sizeof(*_tmp4B1)),((
_tmp4B1->hd=(void*)_check_null(fd->effect),((_tmp4B1->tl=0,_tmp4B1)))))),_tmp4B4)))))),
_tmp4B0)))),_tmp4B5))))),((_tmp4AD->curr_rgn=(void*)param_rgn,((_tmp4AD->in_notreadctxt=(
int)0,((_tmp4AD->in_lhs=(int)0,((_tmp4AD->fnrgn=(struct _RegionHandle*)r,(struct
Cyc_Tcenv_Fenv*)_tmp4AD)))))))))))))))))))))));};};}struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(
unsigned int loc,struct Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*fd){struct
Cyc_Tcenv_Fenv _tmp31C;const struct Cyc_Tcenv_Bindings*_tmp31D;struct Cyc_RgnOrder_RgnPO*
_tmp31E;struct Cyc_List_List*_tmp31F;struct Cyc_Tcenv_SharedFenv*_tmp320;struct
_RegionHandle*_tmp321;struct Cyc_Tcenv_Fenv*_tmp31B=old_fenv;_tmp31C=*_tmp31B;
_tmp31D=_tmp31C.locals;_tmp31E=_tmp31C.region_order;_tmp31F=_tmp31C.type_vars;
_tmp320=_tmp31C.shared;_tmp321=_tmp31C.fnrgn;{struct _RegionHandle*_tmp322=
_tmp320->frgn;const struct Cyc_Tcenv_Bindings*_tmp323=(const struct Cyc_Tcenv_Bindings*)
_tmp31D;struct _dyneither_ptr*_tmp4C2;const char*_tmp4C1;void*_tmp4C0[1];struct Cyc_String_pa_struct
_tmp4BF;struct Cyc_Absyn_Tvar*_tmp4BE;struct Cyc_Absyn_Tvar*rgn0=(_tmp4BE=
_cycalloc(sizeof(*_tmp4BE)),((_tmp4BE->name=((_tmp4C2=_cycalloc(sizeof(*_tmp4C2)),((
_tmp4C2[0]=(struct _dyneither_ptr)((_tmp4BF.tag=0,((_tmp4BF.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)*(*fd->name).f2),((_tmp4C0[0]=& _tmp4BF,Cyc_aprintf(((
_tmp4C1="`%s",_tag_dyneither(_tmp4C1,sizeof(char),4))),_tag_dyneither(_tmp4C0,
sizeof(void*),1)))))))),_tmp4C2)))),((_tmp4BE->identity=Cyc_Tcutil_new_tvar_id(),((
_tmp4BE->kind=(void*)& Cyc_Tcenv_rgn_kb,_tmp4BE)))))));{struct Cyc_List_List*
_tmp324=fd->tvs;for(0;_tmp324 != 0;_tmp324=_tmp324->tl){struct Cyc_Absyn_Kind
_tmp326;enum Cyc_Absyn_KindQual _tmp327;enum Cyc_Absyn_AliasQual _tmp328;struct Cyc_Absyn_Kind*
_tmp325=Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)_tmp324->hd,& Cyc_Tcutil_bk);
_tmp326=*_tmp325;_tmp327=_tmp326.kind;_tmp328=_tmp326.aliasqual;if(_tmp327 == Cyc_Absyn_RgnKind){
if(_tmp328 == Cyc_Absyn_Aliasable)_tmp31E=Cyc_RgnOrder_add_youngest(_tmp322,
_tmp31E,(struct Cyc_Absyn_Tvar*)_tmp324->hd,0,0);else{const char*_tmp4C5;void*
_tmp4C4;(_tmp4C4=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp4C5="non-intuitionistic tvar in nested_fenv",_tag_dyneither(_tmp4C5,sizeof(
char),39))),_tag_dyneither(_tmp4C4,sizeof(void*),0)));}}}}_tmp31E=Cyc_RgnOrder_add_youngest(
_tmp322,_tmp31E,rgn0,0,0);{struct Cyc_List_List*_tmp4C6;struct Cyc_List_List*
_tmp32B=(_tmp4C6=_cycalloc(sizeof(*_tmp4C6)),((_tmp4C6->hd=rgn0,((_tmp4C6->tl=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(
fd->tvs,_tmp31F),_tmp4C6)))));Cyc_Tcutil_check_unique_tvars(loc,_tmp32B);{struct
Cyc_Absyn_VarType_struct _tmp4C9;struct Cyc_Absyn_VarType_struct*_tmp4C8;void*
param_rgn=(void*)((_tmp4C8=_cycalloc(sizeof(*_tmp4C8)),((_tmp4C8[0]=((_tmp4C9.tag=
2,((_tmp4C9.f1=rgn0,_tmp4C9)))),_tmp4C8))));struct Cyc_List_List*_tmp32C=0;{
struct Cyc_List_List*_tmp32D=fd->args;for(0;_tmp32D != 0;_tmp32D=_tmp32D->tl){
struct Cyc_Absyn_Vardecl _tmp4CF;struct _tuple0*_tmp4CE;struct Cyc_Absyn_Vardecl*
_tmp4CD;struct Cyc_Absyn_Vardecl*_tmp32E=(_tmp4CD=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)
* 1),((_tmp4CD[0]=((_tmp4CF.sc=Cyc_Absyn_Public,((_tmp4CF.name=((_tmp4CE=
_cycalloc(sizeof(*_tmp4CE)),((_tmp4CE->f1=(union Cyc_Absyn_Nmspace)Cyc_Absyn_Loc_n,((
_tmp4CE->f2=(*((struct _tuple8*)_tmp32D->hd)).f1,_tmp4CE)))))),((_tmp4CF.tq=(*((
struct _tuple8*)_tmp32D->hd)).f2,((_tmp4CF.type=(*((struct _tuple8*)_tmp32D->hd)).f3,((
_tmp4CF.initializer=0,((_tmp4CF.rgn=(void*)param_rgn,((_tmp4CF.attributes=0,((
_tmp4CF.escapes=0,_tmp4CF)))))))))))))))),_tmp4CD)));{struct Cyc_List_List _tmp4D2;
struct Cyc_List_List*_tmp4D1;_tmp32C=((_tmp4D1=_cycalloc(sizeof(struct Cyc_List_List)
* 1),((_tmp4D1[0]=((_tmp4D2.hd=_tmp32E,((_tmp4D2.tl=_tmp32C,_tmp4D2)))),_tmp4D1))));}{
struct Cyc_Absyn_Param_b_struct _tmp4D5;struct Cyc_Absyn_Param_b_struct*_tmp4D4;
struct Cyc_Absyn_Param_b_struct*_tmp331=(_tmp4D4=_cycalloc(sizeof(*_tmp4D4)),((
_tmp4D4[0]=((_tmp4D5.tag=3,((_tmp4D5.f1=_tmp32E,_tmp4D5)))),_tmp4D4)));struct
_dyneither_ptr*_tmp332=(*((struct _tuple8*)_tmp32D->hd)).f1;struct Cyc_Tcenv_Bindings*
_tmp4D6;_tmp323=(const struct Cyc_Tcenv_Bindings*)((_tmp4D6=_region_malloc(_tmp322,
sizeof(*_tmp4D6)),((_tmp4D6->v=_tmp332,((_tmp4D6->b=(void*)_tmp331,((_tmp4D6->tl=
_tmp323,_tmp4D6))))))));};}}if(fd->cyc_varargs != 0){struct _dyneither_ptr*_tmp33A;
struct Cyc_Absyn_Tqual _tmp33B;void*_tmp33C;int _tmp33D;struct Cyc_Absyn_VarargInfo
_tmp339=*((struct Cyc_Absyn_VarargInfo*)_check_null(fd->cyc_varargs));_tmp33A=
_tmp339.name;_tmp33B=_tmp339.tq;_tmp33C=_tmp339.type;_tmp33D=_tmp339.inject;if(
_tmp33A != 0){void*_tmp33E=Cyc_Absyn_dyneither_typ(_tmp33C,param_rgn,_tmp33B,Cyc_Absyn_false_conref);
struct Cyc_Absyn_Vardecl _tmp4DC;struct _tuple0*_tmp4DB;struct Cyc_Absyn_Vardecl*
_tmp4DA;struct Cyc_Absyn_Vardecl*_tmp33F=(_tmp4DA=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)
* 1),((_tmp4DA[0]=((_tmp4DC.sc=Cyc_Absyn_Public,((_tmp4DC.name=((_tmp4DB=
_cycalloc(sizeof(*_tmp4DB)),((_tmp4DB->f1=(union Cyc_Absyn_Nmspace)Cyc_Absyn_Loc_n,((
_tmp4DB->f2=(struct _dyneither_ptr*)_tmp33A,_tmp4DB)))))),((_tmp4DC.tq=Cyc_Absyn_empty_tqual(
0),((_tmp4DC.type=_tmp33E,((_tmp4DC.initializer=0,((_tmp4DC.rgn=(void*)param_rgn,((
_tmp4DC.attributes=0,((_tmp4DC.escapes=0,_tmp4DC)))))))))))))))),_tmp4DA)));{
struct Cyc_List_List*_tmp4DD;_tmp32C=((_tmp4DD=_cycalloc(sizeof(*_tmp4DD)),((
_tmp4DD->hd=_tmp33F,((_tmp4DD->tl=_tmp32C,_tmp4DD))))));}{struct _dyneither_ptr*
_tmp341=(struct _dyneither_ptr*)_tmp33A;struct Cyc_Absyn_Param_b_struct _tmp4E0;
struct Cyc_Absyn_Param_b_struct*_tmp4DF;struct Cyc_Absyn_Param_b_struct*_tmp342=(
_tmp4DF=_cycalloc(sizeof(*_tmp4DF)),((_tmp4DF[0]=((_tmp4E0.tag=3,((_tmp4E0.f1=
_tmp33F,_tmp4E0)))),_tmp4DF)));struct Cyc_Tcenv_Bindings*_tmp4E1;_tmp323=(const
struct Cyc_Tcenv_Bindings*)((_tmp4E1=_region_malloc(_tmp322,sizeof(*_tmp4E1)),((
_tmp4E1->v=_tmp341,((_tmp4E1->b=(void*)_tmp342,((_tmp4E1->tl=_tmp323,_tmp4E1))))))));};}
else{const char*_tmp4E4;void*_tmp4E3;(_tmp4E3=0,Cyc_Tcutil_terr(loc,((_tmp4E4="missing name for varargs",
_tag_dyneither(_tmp4E4,sizeof(char),25))),_tag_dyneither(_tmp4E3,sizeof(void*),0)));}}{
struct Cyc_Core_Opt _tmp4E7;struct Cyc_Core_Opt*_tmp4E6;fd->param_vardecls=((
_tmp4E6=_cycalloc(sizeof(struct Cyc_Core_Opt)* 1),((_tmp4E6[0]=((_tmp4E7.v=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp32C),
_tmp4E7)),_tmp4E6))));}{struct Cyc_Absyn_JoinEff_struct*_tmp505;struct Cyc_List_List*
_tmp504;struct Cyc_Absyn_AccessEff_struct _tmp503;struct Cyc_Absyn_AccessEff_struct*
_tmp502;struct Cyc_List_List*_tmp501;struct Cyc_Absyn_JoinEff_struct _tmp500;struct
Cyc_Tcenv_CtrlEnv*_tmp4FF;struct Cyc_Tcenv_SharedFenv*_tmp4FE;struct Cyc_Tcenv_Fenv*
_tmp4FD;return(struct Cyc_Tcenv_Fenv*)((_tmp4FD=_region_malloc(_tmp322,sizeof(*
_tmp4FD)),((_tmp4FD->shared=(struct Cyc_Tcenv_SharedFenv*)((_tmp4FE=
_region_malloc(_tmp322,sizeof(*_tmp4FE)),((_tmp4FE->frgn=_tmp322,((_tmp4FE->return_typ=
fd->ret_type,((_tmp4FE->seen_labels=((struct Cyc_Dict_Dict(*)(struct _RegionHandle*,
int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(_tmp322,
Cyc_strptrcmp),((_tmp4FE->needed_labels=((struct Cyc_Dict_Dict(*)(struct
_RegionHandle*,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(
_tmp322,Cyc_strptrcmp),((_tmp4FE->delayed_effect_checks=0,((_tmp4FE->delayed_constraint_checks=
0,_tmp4FE)))))))))))))),((_tmp4FD->type_vars=(struct Cyc_List_List*)_tmp32B,((
_tmp4FD->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmp31E,((_tmp4FD->locals=(
const struct Cyc_Tcenv_Bindings*)_tmp323,((_tmp4FD->encloser=(struct Cyc_Absyn_Stmt*)
fd->body,((_tmp4FD->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)((
_tmp4FF=_region_malloc(_tmp322,sizeof(*_tmp4FF)),((_tmp4FF->ctrl_rgn=_tmp322,((
_tmp4FF->continue_stmt=(void*)& Cyc_Tcenv_NotLoop_j_val,((_tmp4FF->break_stmt=(
void*)& Cyc_Tcenv_NotLoop_j_val,((_tmp4FF->fallthru_clause=0,((_tmp4FF->next_stmt=(
void*)& Cyc_Tcenv_FnEnd_j_val,((_tmp4FF->try_depth=0,_tmp4FF))))))))))))))),((
_tmp4FD->capability=(void*)((void*)((_tmp505=_cycalloc(sizeof(*_tmp505)),((
_tmp505[0]=((_tmp500.tag=24,((_tmp500.f1=((_tmp504=_cycalloc(sizeof(*_tmp504)),((
_tmp504->hd=(void*)((_tmp502=_cycalloc(sizeof(*_tmp502)),((_tmp502[0]=((_tmp503.tag=
23,((_tmp503.f1=(void*)param_rgn,_tmp503)))),_tmp502)))),((_tmp504->tl=((_tmp501=
_cycalloc(sizeof(*_tmp501)),((_tmp501->hd=(void*)_check_null(fd->effect),((
_tmp501->tl=0,_tmp501)))))),_tmp504)))))),_tmp500)))),_tmp505))))),((_tmp4FD->curr_rgn=(
void*)param_rgn,((_tmp4FD->in_notreadctxt=(int)0,((_tmp4FD->in_lhs=(int)0,((
_tmp4FD->fnrgn=(struct _RegionHandle*)_tmp321,(struct Cyc_Tcenv_Fenv*)_tmp4FD))))))))))))))))))))))));};};};};}
