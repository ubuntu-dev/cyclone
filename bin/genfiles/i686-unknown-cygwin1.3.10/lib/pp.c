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
_zero_arr_inplace_plus_char(char *x, int orig_i) {
  char **_zap_x = &x;
  *_zap_x = _zero_arr_plus_char(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_short(short *x, int orig_i) {
  short **_zap_x = &x;
  *_zap_x = _zero_arr_plus_short(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_int(int *x, int orig_i) {
  int **_zap_x = &x;
  *_zap_x = _zero_arr_plus_int(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_float(float *x, int orig_i) {
  float **_zap_x = &x;
  *_zap_x = _zero_arr_plus_float(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_double(double *x, int orig_i) {
  double **_zap_x = &x;
  *_zap_x = _zero_arr_plus_double(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_longdouble(long double *x, int orig_i) {
  long double **_zap_x = &x;
  *_zap_x = _zero_arr_plus_longdouble(*_zap_x,1,orig_i);
}
static _INLINE void 
_zero_arr_inplace_plus_voidstar(void **x, int orig_i) {
  void ***_zap_x = &x;
  *_zap_x = _zero_arr_plus_voidstar(*_zap_x,1,orig_i);
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
_zero_arr_inplace_plus_post_char(char *x, int orig_i){
  char ** _zap_x = &x;
  char * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_char(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE short *
_zero_arr_inplace_plus_post_short(short *x, int orig_i){
  short **_zap_x = &x;
  short * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_short(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE int *
_zero_arr_inplace_plus_post_int(int *x, int orig_i){
  int **_zap_x = &x;
  int * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_int(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE float *
_zero_arr_inplace_plus_post_float(float *x, int orig_i){
  float **_zap_x = &x;
  float * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_float(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE double *
_zero_arr_inplace_plus_post_double(double *x, int orig_i){
  double **_zap_x = &x;
  double * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_double(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE long double *
_zero_arr_inplace_plus_post_longdouble(long double *x, int orig_i){
  long double **_zap_x = &x;
  long double * _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_longdouble(_zap_res,1,orig_i);
  return _zap_res;
}
static _INLINE void **
_zero_arr_inplace_plus_post_voidstar(void **x, int orig_i){
  void ***_zap_x = &x;
  void ** _zap_res = *_zap_x;
  *_zap_x = _zero_arr_plus_voidstar(_zap_res,1,orig_i);
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
 struct Cyc_Core_NewRegion{struct _DynRegionHandle*dynregion;};struct Cyc___cycFILE;
struct Cyc_String_pa_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_struct{
int tag;unsigned long f1;};struct Cyc_Double_pa_struct{int tag;double f1;};struct Cyc_LongDouble_pa_struct{
int tag;long double f1;};struct Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{
int tag;unsigned long*f1;};int Cyc_fprintf(struct Cyc___cycFILE*,struct
_dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_struct{int tag;short*
f1;};struct Cyc_UShortPtr_sa_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_struct{
int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_struct{
int tag;struct _dyneither_ptr f1;};struct Cyc_DoublePtr_sa_struct{int tag;double*f1;}
;struct Cyc_FloatPtr_sa_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int
tag;struct _dyneither_ptr f1;};extern char Cyc_FileCloseError[15];struct Cyc_FileCloseError_struct{
char*tag;};extern char Cyc_FileOpenError[14];struct Cyc_FileOpenError_struct{char*
tag;struct _dyneither_ptr f1;};struct Cyc_Core_Opt{void*v;};struct _dyneither_ptr Cyc_Core_new_string(
unsigned int);void*Cyc_Core_identity(void*);int Cyc_Core_intcmp(int,int);extern
char Cyc_Core_Invalid_argument[17];struct Cyc_Core_Invalid_argument_struct{char*
tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[8];struct Cyc_Core_Failure_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[11];struct Cyc_Core_Impossible_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10];struct Cyc_Core_Not_found_struct{
char*tag;};extern char Cyc_Core_Unreachable[12];struct Cyc_Core_Unreachable_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Open_Region[12];struct Cyc_Core_Open_Region_struct{
char*tag;};extern char Cyc_Core_Free_Region[12];struct Cyc_Core_Free_Region_struct{
char*tag;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};int Cyc_List_length(
struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14];struct Cyc_List_List_mismatch_struct{
char*tag;};extern char Cyc_List_Nth[4];struct Cyc_List_Nth_struct{char*tag;};
unsigned long Cyc_strlen(struct _dyneither_ptr s);int Cyc_zstrptrcmp(struct
_dyneither_ptr*,struct _dyneither_ptr*);struct _dyneither_ptr Cyc_str_sepstr(struct
Cyc_List_List*,struct _dyneither_ptr);struct Cyc_Hashtable_Table;struct Cyc_Hashtable_Table*
Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));void Cyc_Hashtable_insert(
struct Cyc_Hashtable_Table*t,void*key,void*val);void*Cyc_Hashtable_lookup(struct
Cyc_Hashtable_Table*t,void*key);int Cyc_Hashtable_hash_stringptr(struct
_dyneither_ptr*p);struct Cyc_Fn_Function{void*(*f)(void*,void*);void*env;};struct
Cyc_Fn_Function*Cyc_Fn_make_fn(void*(*f)(void*,void*),void*x);struct Cyc_Fn_Function*
Cyc_Fn_fp2fn(void*(*f)(void*));void*Cyc_Fn_apply(struct Cyc_Fn_Function*f,void*x);
extern int Cyc_PP_tex_output;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
void Cyc_PP_file_of_doc(struct Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f);struct
_dyneither_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w);struct _tuple0{struct
_dyneither_ptr f1;struct Cyc_List_List*f2;};struct _tuple0*Cyc_PP_string_and_links(
struct Cyc_PP_Doc*d,int w);struct Cyc_PP_Doc*Cyc_PP_nil_doc();struct Cyc_PP_Doc*Cyc_PP_blank_doc();
struct Cyc_PP_Doc*Cyc_PP_line_doc();struct Cyc_PP_Doc*Cyc_PP_oline_doc();struct Cyc_PP_Doc*
Cyc_PP_text(struct _dyneither_ptr s);struct Cyc_PP_Doc*Cyc_PP_textptr(struct
_dyneither_ptr*p);struct Cyc_PP_Doc*Cyc_PP_text_width(struct _dyneither_ptr s,int w);
struct Cyc_PP_Doc*Cyc_PP_hyperlink(struct _dyneither_ptr shrt,struct _dyneither_ptr
full);struct Cyc_PP_Doc*Cyc_PP_nest(int k,struct Cyc_PP_Doc*d);struct Cyc_PP_Doc*Cyc_PP_cat(
struct _dyneither_ptr);struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*doclist);
struct Cyc_PP_Doc*Cyc_PP_cats_arr(struct _dyneither_ptr docs);struct Cyc_PP_Doc*Cyc_PP_doc_union(
struct Cyc_PP_Doc*d1,struct Cyc_PP_Doc*d2);struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*
d);struct Cyc_PP_Doc*Cyc_PP_seq(struct _dyneither_ptr sep,struct Cyc_List_List*l);
struct Cyc_PP_Doc*Cyc_PP_ppseq(struct Cyc_PP_Doc*(*pp)(void*),struct _dyneither_ptr
sep,struct Cyc_List_List*l);struct Cyc_PP_Doc*Cyc_PP_seql(struct _dyneither_ptr sep,
struct Cyc_List_List*l0);struct Cyc_PP_Doc*Cyc_PP_ppseql(struct Cyc_PP_Doc*(*pp)(
void*),struct _dyneither_ptr sep,struct Cyc_List_List*l);struct Cyc_PP_Doc*Cyc_PP_group(
struct _dyneither_ptr start,struct _dyneither_ptr stop,struct _dyneither_ptr sep,
struct Cyc_List_List*l);struct Cyc_PP_Doc*Cyc_PP_groupl(struct _dyneither_ptr start,
struct _dyneither_ptr stop,struct _dyneither_ptr sep,struct Cyc_List_List*l);struct
Cyc_PP_Doc*Cyc_PP_egroup(struct _dyneither_ptr start,struct _dyneither_ptr stop,
struct _dyneither_ptr sep,struct Cyc_List_List*l);struct Cyc_Xarray_Xarray{struct
_RegionHandle*r;struct _dyneither_ptr elmts;int num_elmts;};int Cyc_Xarray_length(
struct Cyc_Xarray_Xarray*);void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*,int);void
Cyc_Xarray_set(struct Cyc_Xarray_Xarray*,int,void*);struct Cyc_Xarray_Xarray*Cyc_Xarray_create(
int,void*);void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);struct Cyc_PP_Empty_struct{
int tag;int f1;};struct Cyc_PP_Single_struct{int tag;void*f1;};struct Cyc_PP_Append_struct{
int tag;void*f1;void*f2;};struct Cyc_PP_Empty_struct Cyc_PP_Empty_stringptr={0,0};
struct Cyc_PP_Empty_struct Cyc_PP_Empty_link={0,0};struct _tuple1{void*f1;void*f2;}
;void*Cyc_PP_append(void*a1,void*a2){struct _tuple1 _tmp3=({struct _tuple1 _tmp2;
_tmp2.f1=a1;_tmp2.f2=a2;_tmp2;});void*_tmp4;void*_tmp6;_LL1: _tmp4=_tmp3.f1;{
struct Cyc_PP_Empty_struct*_tmp5=(struct Cyc_PP_Empty_struct*)_tmp4;if(_tmp5->tag
!= 0)goto _LL3;};_LL2: return a2;_LL3: _tmp6=_tmp3.f2;{struct Cyc_PP_Empty_struct*
_tmp7=(struct Cyc_PP_Empty_struct*)_tmp6;if(_tmp7->tag != 0)goto _LL5;};_LL4: return
a1;_LL5:;_LL6: return(void*)({struct Cyc_PP_Append_struct*_tmp8=_cycalloc(sizeof(*
_tmp8));_tmp8[0]=({struct Cyc_PP_Append_struct _tmp9;_tmp9.tag=2;_tmp9.f1=(void*)
a1;_tmp9.f2=(void*)a2;_tmp9;});_tmp8;});_LL0:;}struct Cyc_List_List*Cyc_PP_list_of_alist_f(
void*y,struct Cyc_List_List*l){void*_tmpA=y;void*_tmpD;void*_tmpF;void*_tmp10;
_LL8: {struct Cyc_PP_Empty_struct*_tmpB=(struct Cyc_PP_Empty_struct*)_tmpA;if(
_tmpB->tag != 0)goto _LLA;}_LL9: return l;_LLA: {struct Cyc_PP_Single_struct*_tmpC=(
struct Cyc_PP_Single_struct*)_tmpA;if(_tmpC->tag != 1)goto _LLC;else{_tmpD=(void*)
_tmpC->f1;}}_LLB: return({struct Cyc_List_List*_tmp11=_cycalloc(sizeof(*_tmp11));
_tmp11->hd=(void*)_tmpD;_tmp11->tl=l;_tmp11;});_LLC: {struct Cyc_PP_Append_struct*
_tmpE=(struct Cyc_PP_Append_struct*)_tmpA;if(_tmpE->tag != 2)goto _LL7;else{_tmpF=(
void*)_tmpE->f1;_tmp10=(void*)_tmpE->f2;}}_LLD: return Cyc_PP_list_of_alist_f(
_tmpF,Cyc_PP_list_of_alist_f(_tmp10,l));_LL7:;}struct Cyc_List_List*Cyc_PP_list_of_alist(
void*x){return Cyc_PP_list_of_alist_f(x,0);}struct Cyc_PP_Ppstate{int ci;int cc;int
cl;int pw;int epw;};struct Cyc_PP_Out{int newcc;int newcl;void*ppout;void*links;};
struct Cyc_PP_Doc{int mwo;int mw;struct Cyc_Fn_Function*f;};static void Cyc_PP_dump_out(
struct Cyc___cycFILE*f,void*al){struct Cyc_Xarray_Xarray*_tmp12=((struct Cyc_Xarray_Xarray*(*)(
int,void*))Cyc_Xarray_create)(16,al);((void(*)(struct Cyc_Xarray_Xarray*,void*))
Cyc_Xarray_add)(_tmp12,al);{int last=0;while(last >= 0){void*_tmp13=((void*(*)(
struct Cyc_Xarray_Xarray*,int))Cyc_Xarray_get)(_tmp12,last);struct _dyneither_ptr*
_tmp16;void*_tmp18;void*_tmp19;_LLF: {struct Cyc_PP_Empty_struct*_tmp14=(struct
Cyc_PP_Empty_struct*)_tmp13;if(_tmp14->tag != 0)goto _LL11;}_LL10: -- last;goto _LLE;
_LL11: {struct Cyc_PP_Single_struct*_tmp15=(struct Cyc_PP_Single_struct*)_tmp13;
if(_tmp15->tag != 1)goto _LL13;else{_tmp16=(struct _dyneither_ptr*)_tmp15->f1;}}
_LL12: -- last;({struct Cyc_String_pa_struct _tmp1C;_tmp1C.tag=0;_tmp1C.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)*_tmp16);({void*_tmp1A[1]={& _tmp1C};Cyc_fprintf(
f,({const char*_tmp1B="%s";_tag_dyneither(_tmp1B,sizeof(char),3);}),
_tag_dyneither(_tmp1A,sizeof(void*),1));});});goto _LLE;_LL13: {struct Cyc_PP_Append_struct*
_tmp17=(struct Cyc_PP_Append_struct*)_tmp13;if(_tmp17->tag != 2)goto _LLE;else{
_tmp18=(void*)_tmp17->f1;_tmp19=(void*)_tmp17->f2;}}_LL14:((void(*)(struct Cyc_Xarray_Xarray*,
int,void*))Cyc_Xarray_set)(_tmp12,last,_tmp19);if(last == ((int(*)(struct Cyc_Xarray_Xarray*))
Cyc_Xarray_length)(_tmp12)- 1)((void(*)(struct Cyc_Xarray_Xarray*,void*))Cyc_Xarray_add)(
_tmp12,_tmp18);else{((void(*)(struct Cyc_Xarray_Xarray*,int,void*))Cyc_Xarray_set)(
_tmp12,last + 1,_tmp18);}++ last;goto _LLE;_LLE:;}};}void Cyc_PP_file_of_doc(struct
Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f){struct Cyc_PP_Out*o=((struct Cyc_PP_Out*(*)(
struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(d->f,({struct Cyc_PP_Ppstate*
_tmp1D=_cycalloc_atomic(sizeof(*_tmp1D));_tmp1D->ci=0;_tmp1D->cc=0;_tmp1D->cl=1;
_tmp1D->pw=w;_tmp1D->epw=w;_tmp1D;}));Cyc_PP_dump_out(f,o->ppout);}struct
_dyneither_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w){struct Cyc_PP_Out*o=((
struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(
d->f,({struct Cyc_PP_Ppstate*_tmp1F=_cycalloc_atomic(sizeof(*_tmp1F));_tmp1F->ci=
0;_tmp1F->cc=0;_tmp1F->cl=1;_tmp1F->pw=w;_tmp1F->epw=w;_tmp1F;}));return(struct
_dyneither_ptr)Cyc_str_sepstr(((struct Cyc_List_List*(*)(void*x))Cyc_PP_list_of_alist)(
o->ppout),({const char*_tmp1E="";_tag_dyneither(_tmp1E,sizeof(char),1);}));}
struct _tuple0*Cyc_PP_string_and_links(struct Cyc_PP_Doc*d,int w){struct Cyc_PP_Out*
o=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(
d->f,({struct Cyc_PP_Ppstate*_tmp22=_cycalloc_atomic(sizeof(*_tmp22));_tmp22->ci=
0;_tmp22->cc=0;_tmp22->cl=1;_tmp22->pw=w;_tmp22->epw=w;_tmp22;}));return({struct
_tuple0*_tmp20=_cycalloc(sizeof(*_tmp20));_tmp20->f1=(struct _dyneither_ptr)Cyc_str_sepstr(((
struct Cyc_List_List*(*)(void*x))Cyc_PP_list_of_alist)(o->ppout),({const char*
_tmp21="";_tag_dyneither(_tmp21,sizeof(char),1);}));_tmp20->f2=((struct Cyc_List_List*(*)(
void*x))Cyc_PP_list_of_alist)(o->links);_tmp20;});}static struct Cyc_Core_Opt*Cyc_PP_bhashtbl=
0;int Cyc_PP_tex_output=0;struct _dyneither_ptr Cyc_PP_nlblanks(int i){if(Cyc_PP_bhashtbl
== 0)Cyc_PP_bhashtbl=({struct Cyc_Core_Opt*_tmp23=_cycalloc(sizeof(*_tmp23));
_tmp23->v=((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(int,int),int(*hash)(
int)))Cyc_Hashtable_create)(61,Cyc_Core_intcmp,(int(*)(int))Cyc_Core_identity);
_tmp23;});if(i < 0)(int)_throw((void*)({struct Cyc_Core_Failure_struct*_tmp24=
_cycalloc(sizeof(*_tmp24));_tmp24[0]=({struct Cyc_Core_Failure_struct _tmp25;
_tmp25.tag=Cyc_Core_Failure;_tmp25.f1=({const char*_tmp26="nlblanks";
_tag_dyneither(_tmp26,sizeof(char),9);});_tmp25;});_tmp24;}));{struct
_handler_cons _tmp27;_push_handler(& _tmp27);{int _tmp29=0;if(setjmp(_tmp27.handler))
_tmp29=1;if(!_tmp29){{struct _dyneither_ptr _tmp2A=*((struct _dyneither_ptr*(*)(
struct Cyc_Hashtable_Table*t,int key))Cyc_Hashtable_lookup)((struct Cyc_Hashtable_Table*)((
struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v,i);_npop_handler(0);return
_tmp2A;};_pop_handler();}else{void*_tmp28=(void*)_exn_thrown;void*_tmp2C=_tmp28;
_LL16: {struct Cyc_Core_Not_found_struct*_tmp2D=(struct Cyc_Core_Not_found_struct*)
_tmp2C;if(_tmp2D->tag != Cyc_Core_Not_found)goto _LL18;}_LL17: if(!Cyc_PP_tex_output){
int _tmp2E=i / 8;int _tmp2F=i % 8;int _tmp30=(2 + _tmp2E)+ _tmp2F;struct _dyneither_ptr
nlb=Cyc_Core_new_string((unsigned int)_tmp30);({struct _dyneither_ptr _tmp31=
_dyneither_ptr_plus(nlb,sizeof(char),0);char _tmp32=*((char*)
_check_dyneither_subscript(_tmp31,sizeof(char),0));char _tmp33='\n';if(
_get_dyneither_size(_tmp31,sizeof(char))== 1  && (_tmp32 == '\000'  && _tmp33 != '\000'))
_throw_arraybounds();*((char*)_tmp31.curr)=_tmp33;});{int j=0;for(0;j < _tmp2E;++ j){({
struct _dyneither_ptr _tmp34=_dyneither_ptr_plus(nlb,sizeof(char),j + 1);char _tmp35=*((
char*)_check_dyneither_subscript(_tmp34,sizeof(char),0));char _tmp36='\t';if(
_get_dyneither_size(_tmp34,sizeof(char))== 1  && (_tmp35 == '\000'  && _tmp36 != '\000'))
_throw_arraybounds();*((char*)_tmp34.curr)=_tmp36;});}}{int j=0;for(0;j < _tmp2F;
++ j){({struct _dyneither_ptr _tmp37=_dyneither_ptr_plus(nlb,sizeof(char),(j + 1)+ 
_tmp2E);char _tmp38=*((char*)_check_dyneither_subscript(_tmp37,sizeof(char),0));
char _tmp39=' ';if(_get_dyneither_size(_tmp37,sizeof(char))== 1  && (_tmp38 == '\000'
 && _tmp39 != '\000'))_throw_arraybounds();*((char*)_tmp37.curr)=_tmp39;});}}((
void(*)(struct Cyc_Hashtable_Table*t,int key,struct _dyneither_ptr*val))Cyc_Hashtable_insert)((
struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v,
i,({struct _dyneither_ptr*_tmp3A=_cycalloc(sizeof(struct _dyneither_ptr)* 1);
_tmp3A[0]=(struct _dyneither_ptr)nlb;_tmp3A;}));return(struct _dyneither_ptr)nlb;}
else{int _tmp3B=3 + i;struct _dyneither_ptr nlb=Cyc_Core_new_string((unsigned int)(
_tmp3B + 1));({struct _dyneither_ptr _tmp3C=_dyneither_ptr_plus(nlb,sizeof(char),0);
char _tmp3D=*((char*)_check_dyneither_subscript(_tmp3C,sizeof(char),0));char
_tmp3E='\\';if(_get_dyneither_size(_tmp3C,sizeof(char))== 1  && (_tmp3D == '\000'
 && _tmp3E != '\000'))_throw_arraybounds();*((char*)_tmp3C.curr)=_tmp3E;});({
struct _dyneither_ptr _tmp3F=_dyneither_ptr_plus(nlb,sizeof(char),1);char _tmp40=*((
char*)_check_dyneither_subscript(_tmp3F,sizeof(char),0));char _tmp41='\\';if(
_get_dyneither_size(_tmp3F,sizeof(char))== 1  && (_tmp40 == '\000'  && _tmp41 != '\000'))
_throw_arraybounds();*((char*)_tmp3F.curr)=_tmp41;});({struct _dyneither_ptr
_tmp42=_dyneither_ptr_plus(nlb,sizeof(char),2);char _tmp43=*((char*)
_check_dyneither_subscript(_tmp42,sizeof(char),0));char _tmp44='\n';if(
_get_dyneither_size(_tmp42,sizeof(char))== 1  && (_tmp43 == '\000'  && _tmp44 != '\000'))
_throw_arraybounds();*((char*)_tmp42.curr)=_tmp44;});{int j=3;for(0;j < _tmp3B;++ j){({
struct _dyneither_ptr _tmp45=_dyneither_ptr_plus(nlb,sizeof(char),j);char _tmp46=*((
char*)_check_dyneither_subscript(_tmp45,sizeof(char),0));char _tmp47='~';if(
_get_dyneither_size(_tmp45,sizeof(char))== 1  && (_tmp46 == '\000'  && _tmp47 != '\000'))
_throw_arraybounds();*((char*)_tmp45.curr)=_tmp47;});}}((void(*)(struct Cyc_Hashtable_Table*
t,int key,struct _dyneither_ptr*val))Cyc_Hashtable_insert)((struct Cyc_Hashtable_Table*)((
struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v,i,({struct _dyneither_ptr*
_tmp48=_cycalloc(sizeof(struct _dyneither_ptr)* 1);_tmp48[0]=(struct
_dyneither_ptr)nlb;_tmp48;}));return(struct _dyneither_ptr)nlb;}_LL18:;_LL19:(
void)_throw(_tmp2C);_LL15:;}};};}static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl=0;
int Cyc_PP_infinity=9999999;struct _tuple2{int f1;struct _dyneither_ptr f2;};static
struct Cyc_PP_Out*Cyc_PP_text_doc_f(struct _tuple2*clo,struct Cyc_PP_Ppstate*st){
struct _tuple2 _tmp4A;int _tmp4B;struct _dyneither_ptr _tmp4C;struct _tuple2*_tmp49=
clo;_tmp4A=*_tmp49;_tmp4B=_tmp4A.f1;_tmp4C=_tmp4A.f2;return({struct Cyc_PP_Out*
_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->newcc=st->cc + _tmp4B;_tmp4D->newcl=st->cl;
_tmp4D->ppout=(void*)({struct Cyc_PP_Single_struct*_tmp4E=_cycalloc(sizeof(*
_tmp4E));_tmp4E[0]=({struct Cyc_PP_Single_struct _tmp4F;_tmp4F.tag=1;_tmp4F.f1=(
void*)({struct _dyneither_ptr*_tmp50=_cycalloc(sizeof(struct _dyneither_ptr)* 1);
_tmp50[0]=_tmp4C;_tmp50;});_tmp4F;});_tmp4E;});_tmp4D->links=(void*)& Cyc_PP_Empty_link;
_tmp4D;});}static struct Cyc_PP_Doc*Cyc_PP_text_doc(struct _dyneither_ptr s){int slen=(
int)Cyc_strlen((struct _dyneither_ptr)s);return({struct Cyc_PP_Doc*_tmp51=
_cycalloc(sizeof(*_tmp51));_tmp51->mwo=slen;_tmp51->mw=Cyc_PP_infinity;_tmp51->f=((
struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),
struct _tuple2*x))Cyc_Fn_make_fn)(Cyc_PP_text_doc_f,({struct _tuple2*_tmp52=
_cycalloc(sizeof(*_tmp52));_tmp52->f1=slen;_tmp52->f2=s;_tmp52;}));_tmp51;});}
struct Cyc_PP_Doc*Cyc_PP_text(struct _dyneither_ptr s){struct Cyc_Hashtable_Table*t;
if(Cyc_PP_str_hashtbl == 0){t=((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(
struct _dyneither_ptr*,struct _dyneither_ptr*),int(*hash)(struct _dyneither_ptr*)))
Cyc_Hashtable_create)(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);Cyc_PP_str_hashtbl=({
struct Cyc_Core_Opt*_tmp53=_cycalloc(sizeof(*_tmp53));_tmp53->v=t;_tmp53;});}
else{t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl))->v;}{
struct _handler_cons _tmp54;_push_handler(& _tmp54);{int _tmp56=0;if(setjmp(_tmp54.handler))
_tmp56=1;if(!_tmp56){{struct Cyc_PP_Doc*_tmp58=((struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*
t,struct _dyneither_ptr*key))Cyc_Hashtable_lookup)(t,({struct _dyneither_ptr*
_tmp57=_cycalloc(sizeof(*_tmp57));_tmp57[0]=s;_tmp57;}));_npop_handler(0);return
_tmp58;};_pop_handler();}else{void*_tmp55=(void*)_exn_thrown;void*_tmp5A=_tmp55;
_LL1B: {struct Cyc_Core_Not_found_struct*_tmp5B=(struct Cyc_Core_Not_found_struct*)
_tmp5A;if(_tmp5B->tag != Cyc_Core_Not_found)goto _LL1D;}_LL1C: {struct Cyc_PP_Doc*d=
Cyc_PP_text_doc(s);((void(*)(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*
key,struct Cyc_PP_Doc*val))Cyc_Hashtable_insert)(t,({struct _dyneither_ptr*_tmp5C=
_cycalloc(sizeof(*_tmp5C));_tmp5C[0]=s;_tmp5C;}),d);return d;}_LL1D:;_LL1E:(void)
_throw(_tmp5A);_LL1A:;}};};}struct Cyc_PP_Doc*Cyc_PP_textptr(struct _dyneither_ptr*
s){return Cyc_PP_text(*s);}static struct Cyc_PP_Doc*Cyc_PP_text_width_doc(struct
_dyneither_ptr s,int slen){return({struct Cyc_PP_Doc*_tmp5D=_cycalloc(sizeof(*
_tmp5D));_tmp5D->mwo=slen;_tmp5D->mw=Cyc_PP_infinity;_tmp5D->f=((struct Cyc_Fn_Function*(*)(
struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x))Cyc_Fn_make_fn)(
Cyc_PP_text_doc_f,({struct _tuple2*_tmp5E=_cycalloc(sizeof(*_tmp5E));_tmp5E->f1=
slen;_tmp5E->f2=s;_tmp5E;}));_tmp5D;});}static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl2=
0;struct Cyc_PP_Doc*Cyc_PP_text_width(struct _dyneither_ptr s,int slen){struct Cyc_Hashtable_Table*
t;if(Cyc_PP_str_hashtbl2 == 0){t=((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(
struct _dyneither_ptr*,struct _dyneither_ptr*),int(*hash)(struct _dyneither_ptr*)))
Cyc_Hashtable_create)(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);Cyc_PP_str_hashtbl2=({
struct Cyc_Core_Opt*_tmp5F=_cycalloc(sizeof(*_tmp5F));_tmp5F->v=t;_tmp5F;});}
else{t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl2))->v;}{
struct _handler_cons _tmp60;_push_handler(& _tmp60);{int _tmp62=0;if(setjmp(_tmp60.handler))
_tmp62=1;if(!_tmp62){{struct Cyc_PP_Doc*_tmp64=((struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*
t,struct _dyneither_ptr*key))Cyc_Hashtable_lookup)(t,({struct _dyneither_ptr*
_tmp63=_cycalloc(sizeof(*_tmp63));_tmp63[0]=s;_tmp63;}));_npop_handler(0);return
_tmp64;};_pop_handler();}else{void*_tmp61=(void*)_exn_thrown;void*_tmp66=_tmp61;
_LL20: {struct Cyc_Core_Not_found_struct*_tmp67=(struct Cyc_Core_Not_found_struct*)
_tmp66;if(_tmp67->tag != Cyc_Core_Not_found)goto _LL22;}_LL21: {struct Cyc_PP_Doc*d=
Cyc_PP_text_width_doc(s,slen);((void(*)(struct Cyc_Hashtable_Table*t,struct
_dyneither_ptr*key,struct Cyc_PP_Doc*val))Cyc_Hashtable_insert)(t,({struct
_dyneither_ptr*_tmp68=_cycalloc(sizeof(*_tmp68));_tmp68[0]=s;_tmp68;}),d);return
d;}_LL22:;_LL23:(void)_throw(_tmp66);_LL1F:;}};};}struct Cyc_Core_Opt*Cyc_PP_nil_doc_opt=
0;struct Cyc_Core_Opt*Cyc_PP_blank_doc_opt=0;struct Cyc_Core_Opt*Cyc_PP_line_doc_opt=
0;struct Cyc_PP_Doc*Cyc_PP_nil_doc(){if(Cyc_PP_nil_doc_opt == 0)Cyc_PP_nil_doc_opt=({
struct Cyc_Core_Opt*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->v=Cyc_PP_text(({
const char*_tmp6A="";_tag_dyneither(_tmp6A,sizeof(char),1);}));_tmp69;});return(
struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_nil_doc_opt))->v;}
struct Cyc_PP_Doc*Cyc_PP_blank_doc(){if(Cyc_PP_blank_doc_opt == 0)Cyc_PP_blank_doc_opt=({
struct Cyc_Core_Opt*_tmp6B=_cycalloc(sizeof(*_tmp6B));_tmp6B->v=Cyc_PP_text(({
const char*_tmp6C="";_tag_dyneither(_tmp6C,sizeof(char),1);}));_tmp6B;});return(
struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_blank_doc_opt))->v;}
struct _tuple3{int f1;struct _dyneither_ptr f2;struct _dyneither_ptr f3;};struct
_tuple4{int f1;int f2;int f3;struct _dyneither_ptr f4;};static struct Cyc_PP_Out*Cyc_PP_hyperlink_f(
struct _tuple3*clo,struct Cyc_PP_Ppstate*st){struct _tuple3 _tmp6E;int _tmp6F;struct
_dyneither_ptr _tmp70;struct _dyneither_ptr _tmp71;struct _tuple3*_tmp6D=clo;_tmp6E=*
_tmp6D;_tmp6F=_tmp6E.f1;_tmp70=_tmp6E.f2;_tmp71=_tmp6E.f3;return({struct Cyc_PP_Out*
_tmp72=_cycalloc(sizeof(*_tmp72));_tmp72->newcc=st->cc + _tmp6F;_tmp72->newcl=st->cl;
_tmp72->ppout=(void*)({struct Cyc_PP_Single_struct*_tmp76=_cycalloc(sizeof(*
_tmp76));_tmp76[0]=({struct Cyc_PP_Single_struct _tmp77;_tmp77.tag=1;_tmp77.f1=(
void*)({struct _dyneither_ptr*_tmp78=_cycalloc(sizeof(struct _dyneither_ptr)* 1);
_tmp78[0]=_tmp70;_tmp78;});_tmp77;});_tmp76;});_tmp72->links=(void*)({struct Cyc_PP_Single_struct*
_tmp73=_cycalloc(sizeof(*_tmp73));_tmp73[0]=({struct Cyc_PP_Single_struct _tmp74;
_tmp74.tag=1;_tmp74.f1=(void*)({struct _tuple4*_tmp75=_cycalloc(sizeof(*_tmp75));
_tmp75->f1=st->cl;_tmp75->f2=st->cc;_tmp75->f3=_tmp6F;_tmp75->f4=_tmp71;_tmp75;});
_tmp74;});_tmp73;});_tmp72;});}struct Cyc_PP_Doc*Cyc_PP_hyperlink(struct
_dyneither_ptr shrt,struct _dyneither_ptr full){int slen=(int)Cyc_strlen((struct
_dyneither_ptr)shrt);return({struct Cyc_PP_Doc*_tmp79=_cycalloc(sizeof(*_tmp79));
_tmp79->mwo=slen;_tmp79->mw=Cyc_PP_infinity;_tmp79->f=((struct Cyc_Fn_Function*(*)(
struct Cyc_PP_Out*(*f)(struct _tuple3*,struct Cyc_PP_Ppstate*),struct _tuple3*x))Cyc_Fn_make_fn)(
Cyc_PP_hyperlink_f,({struct _tuple3*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->f1=
slen;_tmp7A->f2=shrt;_tmp7A->f3=full;_tmp7A;}));_tmp79;});}static struct Cyc_PP_Out*
Cyc_PP_line_f(struct Cyc_PP_Ppstate*st){return({struct Cyc_PP_Out*_tmp7B=_cycalloc(
sizeof(*_tmp7B));_tmp7B->newcc=st->ci;_tmp7B->newcl=st->cl + 1;_tmp7B->ppout=(
void*)({struct Cyc_PP_Single_struct*_tmp7C=_cycalloc(sizeof(*_tmp7C));_tmp7C[0]=({
struct Cyc_PP_Single_struct _tmp7D;_tmp7D.tag=1;_tmp7D.f1=(void*)({struct
_dyneither_ptr*_tmp7E=_cycalloc(sizeof(struct _dyneither_ptr)* 1);_tmp7E[0]=Cyc_PP_nlblanks(
st->ci);_tmp7E;});_tmp7D;});_tmp7C;});_tmp7B->links=(void*)& Cyc_PP_Empty_link;
_tmp7B;});}struct Cyc_PP_Doc*Cyc_PP_line_doc(){if(Cyc_PP_line_doc_opt == 0)Cyc_PP_line_doc_opt=({
struct Cyc_Core_Opt*_tmp7F=_cycalloc(sizeof(*_tmp7F));_tmp7F->v=({struct Cyc_PP_Doc*
_tmp80=_cycalloc(sizeof(*_tmp80));_tmp80->mwo=0;_tmp80->mw=0;_tmp80->f=((struct
Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Ppstate*)))Cyc_Fn_fp2fn)(
Cyc_PP_line_f);_tmp80;});_tmp7F;});return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)
_check_null(Cyc_PP_line_doc_opt))->v;}struct _tuple5{int f1;struct Cyc_PP_Doc*f2;};
static struct Cyc_PP_Out*Cyc_PP_nest_f(struct _tuple5*clo,struct Cyc_PP_Ppstate*st){
struct _tuple5 _tmp82;int _tmp83;struct Cyc_PP_Doc*_tmp84;struct _tuple5*_tmp81=clo;
_tmp82=*_tmp81;_tmp83=_tmp82.f1;_tmp84=_tmp82.f2;{struct Cyc_PP_Ppstate*st2=({
struct Cyc_PP_Ppstate*_tmp85=_cycalloc_atomic(sizeof(*_tmp85));_tmp85->ci=st->ci + 
_tmp83;_tmp85->cc=st->cc;_tmp85->cl=st->cl;_tmp85->pw=st->pw;_tmp85->epw=st->epw;
_tmp85;});return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*
x))Cyc_Fn_apply)(_tmp84->f,st2);};}struct Cyc_PP_Doc*Cyc_PP_nest(int k,struct Cyc_PP_Doc*
d){return({struct Cyc_PP_Doc*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->mwo=d->mwo;
_tmp86->mw=d->mw;_tmp86->f=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(
struct _tuple5*,struct Cyc_PP_Ppstate*),struct _tuple5*x))Cyc_Fn_make_fn)(Cyc_PP_nest_f,({
struct _tuple5*_tmp87=_cycalloc(sizeof(*_tmp87));_tmp87->f1=k;_tmp87->f2=d;_tmp87;}));
_tmp86;});}int Cyc_PP_min(int x,int y){if(x <= y)return x;else{return y;}}int Cyc_PP_max(
int x,int y){if(x >= y)return x;else{return y;}}struct _tuple6{struct Cyc_PP_Doc*f1;
struct Cyc_PP_Doc*f2;};static struct Cyc_PP_Out*Cyc_PP_concat_f(struct _tuple6*clo,
struct Cyc_PP_Ppstate*st){struct _tuple6 _tmp89;struct Cyc_PP_Doc*_tmp8A;struct Cyc_PP_Doc*
_tmp8B;struct _tuple6*_tmp88=clo;_tmp89=*_tmp88;_tmp8A=_tmp89.f1;_tmp8B=_tmp89.f2;{
int epw2=Cyc_PP_max(st->pw - _tmp8B->mw,st->epw - _tmp8A->mwo);struct Cyc_PP_Ppstate*
st2=({struct Cyc_PP_Ppstate*_tmp8E=_cycalloc_atomic(sizeof(*_tmp8E));_tmp8E->ci=
st->ci;_tmp8E->cc=st->cc;_tmp8E->cl=st->cl;_tmp8E->pw=st->pw;_tmp8E->epw=epw2;
_tmp8E;});struct Cyc_PP_Out*o1=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,
struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(_tmp8A->f,st2);struct Cyc_PP_Ppstate*st3=({
struct Cyc_PP_Ppstate*_tmp8D=_cycalloc_atomic(sizeof(*_tmp8D));_tmp8D->ci=st->ci;
_tmp8D->cc=o1->newcc;_tmp8D->cl=o1->newcl;_tmp8D->pw=st->pw;_tmp8D->epw=epw2;
_tmp8D;});struct Cyc_PP_Out*o2=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,
struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(_tmp8B->f,st3);struct Cyc_PP_Out*o3=({
struct Cyc_PP_Out*_tmp8C=_cycalloc(sizeof(*_tmp8C));_tmp8C->newcc=o2->newcc;
_tmp8C->newcl=o2->newcl;_tmp8C->ppout=((void*(*)(void*a1,void*a2))Cyc_PP_append)(
o1->ppout,o2->ppout);_tmp8C->links=((void*(*)(void*a1,void*a2))Cyc_PP_append)(o1->links,
o2->links);_tmp8C;});return o3;};}static struct Cyc_PP_Doc*Cyc_PP_concat(struct Cyc_PP_Doc*
d1,struct Cyc_PP_Doc*d2){return({struct Cyc_PP_Doc*_tmp8F=_cycalloc(sizeof(*_tmp8F));
_tmp8F->mwo=Cyc_PP_min(d1->mw,d1->mwo + d2->mwo);_tmp8F->mw=Cyc_PP_min(d1->mw,d1->mwo
+ d2->mw);_tmp8F->f=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct
_tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x))Cyc_Fn_make_fn)(Cyc_PP_concat_f,({
struct _tuple6*_tmp90=_cycalloc(sizeof(*_tmp90));_tmp90->f1=d1;_tmp90->f2=d2;
_tmp90;}));_tmp8F;});}struct Cyc_PP_Doc*Cyc_PP_cat(struct _dyneither_ptr l){struct
Cyc_PP_Doc*_tmp91=Cyc_PP_nil_doc();{int i=(int)(_get_dyneither_size(l,sizeof(
struct Cyc_PP_Doc*))- 1);for(0;i >= 0;-- i){_tmp91=Cyc_PP_concat(*((struct Cyc_PP_Doc**)
_check_dyneither_subscript(l,sizeof(struct Cyc_PP_Doc*),i)),_tmp91);}}return
_tmp91;}static struct Cyc_PP_Out*Cyc_PP_long_cats_f(struct Cyc_List_List*ds0,struct
Cyc_PP_Ppstate*st){struct Cyc_List_List*os=0;{struct Cyc_List_List*_tmp92=ds0;for(
0;_tmp92 != 0;_tmp92=_tmp92->tl){struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)_tmp92->hd;
struct Cyc_PP_Out*o=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*
x))Cyc_Fn_apply)(d->f,st);st=({struct Cyc_PP_Ppstate*_tmp93=_cycalloc_atomic(
sizeof(*_tmp93));_tmp93->ci=st->ci;_tmp93->cc=o->newcc;_tmp93->cl=o->newcl;
_tmp93->pw=st->pw;_tmp93->epw=st->epw - d->mwo;_tmp93;});os=({struct Cyc_List_List*
_tmp94=_cycalloc(sizeof(*_tmp94));_tmp94->hd=o;_tmp94->tl=os;_tmp94;});}}{int
newcc=((struct Cyc_PP_Out*)((struct Cyc_List_List*)_check_null(os))->hd)->newcc;
int newcl=((struct Cyc_PP_Out*)os->hd)->newcl;void*s=(void*)& Cyc_PP_Empty_stringptr;
void*links=(void*)& Cyc_PP_Empty_link;for(0;os != 0;os=os->tl){s=((void*(*)(void*
a1,void*a2))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->ppout,s);links=((void*(*)(
void*a1,void*a2))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->links,links);}
return({struct Cyc_PP_Out*_tmp95=_cycalloc(sizeof(*_tmp95));_tmp95->newcc=newcc;
_tmp95->newcl=newcl;_tmp95->ppout=s;_tmp95->links=links;_tmp95;});};}static
struct Cyc_PP_Doc*Cyc_PP_long_cats(struct Cyc_List_List*doclist){struct Cyc_List_List*
orig=doclist;struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)((struct Cyc_List_List*)
_check_null(doclist))->hd;doclist=doclist->tl;{int mw=d->mw;int mwo=d->mw;{struct
Cyc_List_List*_tmp96=doclist;for(0;_tmp96 != 0;_tmp96=_tmp96->tl){int mw2=Cyc_PP_min(
mw,mwo + ((struct Cyc_PP_Doc*)_tmp96->hd)->mwo);int mwo2=Cyc_PP_min(mw,mwo + ((
struct Cyc_PP_Doc*)_tmp96->hd)->mw);mw=mw2;mwo=mwo2;}}return({struct Cyc_PP_Doc*
_tmp97=_cycalloc(sizeof(*_tmp97));_tmp97->mwo=mw;_tmp97->mw=mwo;_tmp97->f=((
struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_List_List*,struct Cyc_PP_Ppstate*),
struct Cyc_List_List*x))Cyc_Fn_make_fn)(Cyc_PP_long_cats_f,orig);_tmp97;});};}
struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*doclist){if(doclist == 0)return
Cyc_PP_nil_doc();else{if(doclist->tl == 0)return(struct Cyc_PP_Doc*)doclist->hd;
else{if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(doclist)> 30)return Cyc_PP_long_cats(
doclist);else{return Cyc_PP_concat((struct Cyc_PP_Doc*)doclist->hd,Cyc_PP_cats(
doclist->tl));}}}}static struct Cyc_PP_Out*Cyc_PP_cats_arr_f(struct _dyneither_ptr*
docs_ptr,struct Cyc_PP_Ppstate*st){struct Cyc_List_List*os=0;struct _dyneither_ptr
docs=*docs_ptr;int sz=(int)_get_dyneither_size(docs,sizeof(struct Cyc_PP_Doc*));{
int i=0;for(0;i < sz;++ i){struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)
_check_dyneither_subscript(docs,sizeof(struct Cyc_PP_Doc*),i));struct Cyc_PP_Out*o=((
struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(
d->f,st);st=({struct Cyc_PP_Ppstate*_tmp98=_cycalloc_atomic(sizeof(*_tmp98));
_tmp98->ci=st->ci;_tmp98->cc=o->newcc;_tmp98->cl=o->newcl;_tmp98->pw=st->pw;
_tmp98->epw=st->epw - d->mwo;_tmp98;});os=({struct Cyc_List_List*_tmp99=_cycalloc(
sizeof(*_tmp99));_tmp99->hd=o;_tmp99->tl=os;_tmp99;});}}{int newcc=((struct Cyc_PP_Out*)((
struct Cyc_List_List*)_check_null(os))->hd)->newcc;int newcl=((struct Cyc_PP_Out*)
os->hd)->newcl;void*s=(void*)& Cyc_PP_Empty_stringptr;void*links=(void*)& Cyc_PP_Empty_link;
for(0;os != 0;os=os->tl){s=((void*(*)(void*a1,void*a2))Cyc_PP_append)(((struct Cyc_PP_Out*)
os->hd)->ppout,s);links=((void*(*)(void*a1,void*a2))Cyc_PP_append)(((struct Cyc_PP_Out*)
os->hd)->links,links);}return({struct Cyc_PP_Out*_tmp9A=_cycalloc(sizeof(*_tmp9A));
_tmp9A->newcc=newcc;_tmp9A->newcl=newcl;_tmp9A->ppout=s;_tmp9A->links=links;
_tmp9A;});};}struct Cyc_PP_Doc*Cyc_PP_cats_arr(struct _dyneither_ptr docs){int sz=(
int)_get_dyneither_size(docs,sizeof(struct Cyc_PP_Doc*));if(sz == 0)(int)_throw((
void*)({struct Cyc_Core_Failure_struct*_tmp9B=_cycalloc(sizeof(*_tmp9B));_tmp9B[0]=({
struct Cyc_Core_Failure_struct _tmp9C;_tmp9C.tag=Cyc_Core_Failure;_tmp9C.f1=({
const char*_tmp9D="cats_arr -- size zero array";_tag_dyneither(_tmp9D,sizeof(char),
28);});_tmp9C;});_tmp9B;}));{struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)
_check_dyneither_subscript(docs,sizeof(struct Cyc_PP_Doc*),0));int mw=d->mw;int mwo=
d->mw;{int i=1;for(0;i < sz;++ i){int mw2=Cyc_PP_min(mw,mwo + (*((struct Cyc_PP_Doc**)
_check_dyneither_subscript(docs,sizeof(struct Cyc_PP_Doc*),i)))->mwo);int mwo2=Cyc_PP_min(
mw,mwo + (*((struct Cyc_PP_Doc**)_check_dyneither_subscript(docs,sizeof(struct Cyc_PP_Doc*),
i)))->mw);mw=mw2;mwo=mwo2;}}return({struct Cyc_PP_Doc*_tmp9E=_cycalloc(sizeof(*
_tmp9E));_tmp9E->mwo=mw;_tmp9E->mw=mwo;_tmp9E->f=((struct Cyc_Fn_Function*(*)(
struct Cyc_PP_Out*(*f)(struct _dyneither_ptr*,struct Cyc_PP_Ppstate*),struct
_dyneither_ptr*x))Cyc_Fn_make_fn)(Cyc_PP_cats_arr_f,({struct _dyneither_ptr*
_tmp9F=_cycalloc(sizeof(struct _dyneither_ptr)* 1);_tmp9F[0]=docs;_tmp9F;}));
_tmp9E;});};}static struct Cyc_PP_Out*Cyc_PP_doc_union_f(struct _tuple6*clo,struct
Cyc_PP_Ppstate*st){struct _tuple6 _tmpA1;struct Cyc_PP_Doc*_tmpA2;struct Cyc_PP_Doc*
_tmpA3;struct _tuple6*_tmpA0=clo;_tmpA1=*_tmpA0;_tmpA2=_tmpA1.f1;_tmpA3=_tmpA1.f2;{
int dfits=st->cc + _tmpA2->mwo <= st->epw  || st->cc + _tmpA2->mw <= st->pw;if(dfits)
return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(
_tmpA2->f,st);else{return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct
Cyc_PP_Ppstate*x))Cyc_Fn_apply)(_tmpA3->f,st);}};}struct Cyc_PP_Doc*Cyc_PP_doc_union(
struct Cyc_PP_Doc*d,struct Cyc_PP_Doc*d2){return({struct Cyc_PP_Doc*_tmpA4=
_cycalloc(sizeof(*_tmpA4));_tmpA4->mwo=Cyc_PP_min(d->mwo,d2->mwo);_tmpA4->mw=Cyc_PP_min(
d->mw,d2->mw);_tmpA4->f=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct
_tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x))Cyc_Fn_make_fn)(Cyc_PP_doc_union_f,({
struct _tuple6*_tmpA5=_cycalloc(sizeof(*_tmpA5));_tmpA5->f1=d;_tmpA5->f2=d2;
_tmpA5;}));_tmpA4;});}struct Cyc_PP_Doc*Cyc_PP_oline_doc(){return Cyc_PP_doc_union(
Cyc_PP_nil_doc(),Cyc_PP_line_doc());}static struct Cyc_PP_Out*Cyc_PP_tab_f(struct
Cyc_PP_Doc*d,struct Cyc_PP_Ppstate*st){struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*
_tmpA6=_cycalloc_atomic(sizeof(*_tmpA6));_tmpA6->ci=st->cc;_tmpA6->cc=st->cc;
_tmpA6->cl=st->cl;_tmpA6->pw=st->pw;_tmpA6->epw=st->epw;_tmpA6;});return((struct
Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply)(d->f,
st2);}struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*d){struct Cyc_PP_Doc*d2=({
struct Cyc_PP_Doc*_tmpA7=_cycalloc(sizeof(*_tmpA7));_tmpA7->mwo=d->mwo;_tmpA7->mw=
d->mw;_tmpA7->f=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Doc*,
struct Cyc_PP_Ppstate*),struct Cyc_PP_Doc*x))Cyc_Fn_make_fn)(Cyc_PP_tab_f,d);
_tmpA7;});return d2;}static struct Cyc_PP_Doc*Cyc_PP_ppseq_f(struct Cyc_PP_Doc*(*pp)(
void*),struct _dyneither_ptr sep,struct Cyc_List_List*l){if(l == 0)return Cyc_PP_nil_doc();
else{if(l->tl == 0)return pp((void*)l->hd);else{return({struct Cyc_PP_Doc*_tmpA8[4];
_tmpA8[3]=Cyc_PP_ppseq_f(pp,sep,l->tl);_tmpA8[2]=Cyc_PP_oline_doc();_tmpA8[1]=
Cyc_PP_text(sep);_tmpA8[0]=pp((void*)l->hd);Cyc_PP_cat(_tag_dyneither(_tmpA8,
sizeof(struct Cyc_PP_Doc*),4));});}}}struct Cyc_PP_Doc*Cyc_PP_ppseq(struct Cyc_PP_Doc*(*
pp)(void*),struct _dyneither_ptr sep,struct Cyc_List_List*l){return Cyc_PP_tab(Cyc_PP_ppseq_f(
pp,sep,l));}struct Cyc_PP_Doc*Cyc_PP_seq_f(struct _dyneither_ptr sep,struct Cyc_List_List*
l){if(l == 0)return Cyc_PP_nil_doc();else{if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;
else{struct Cyc_PP_Doc*sep2=Cyc_PP_text(sep);struct Cyc_PP_Doc*oline=Cyc_PP_oline_doc();
struct Cyc_List_List*_tmpA9=l;while(_tmpA9->tl != 0){struct Cyc_List_List*_tmpAA=
_tmpA9->tl;_tmpA9->tl=({struct Cyc_List_List*_tmpAB=_cycalloc(sizeof(*_tmpAB));
_tmpAB->hd=sep2;_tmpAB->tl=({struct Cyc_List_List*_tmpAC=_cycalloc(sizeof(*_tmpAC));
_tmpAC->hd=oline;_tmpAC->tl=_tmpAA;_tmpAC;});_tmpAB;});_tmpA9=_tmpAA;}return Cyc_PP_cats(
l);}}}struct Cyc_PP_Doc*Cyc_PP_seq(struct _dyneither_ptr sep,struct Cyc_List_List*l){
return Cyc_PP_tab(Cyc_PP_seq_f(sep,l));}struct Cyc_PP_Doc*Cyc_PP_ppseql_f(struct
Cyc_PP_Doc*(*pp)(void*),struct _dyneither_ptr sep,struct Cyc_List_List*l){if(l == 0)
return Cyc_PP_nil_doc();else{if(l->tl == 0)return pp((void*)l->hd);else{return({
struct Cyc_PP_Doc*_tmpAD[4];_tmpAD[3]=Cyc_PP_ppseql_f(pp,sep,l->tl);_tmpAD[2]=Cyc_PP_line_doc();
_tmpAD[1]=Cyc_PP_text(sep);_tmpAD[0]=pp((void*)l->hd);Cyc_PP_cat(_tag_dyneither(
_tmpAD,sizeof(struct Cyc_PP_Doc*),4));});}}}struct Cyc_PP_Doc*Cyc_PP_ppseql(struct
Cyc_PP_Doc*(*pp)(void*),struct _dyneither_ptr sep,struct Cyc_List_List*l){return Cyc_PP_tab(
Cyc_PP_ppseql_f(pp,sep,l));}static struct Cyc_PP_Doc*Cyc_PP_seql_f(struct
_dyneither_ptr sep,struct Cyc_List_List*l){if(l == 0)return Cyc_PP_nil_doc();else{
if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;else{return({struct Cyc_PP_Doc*_tmpAE[
4];_tmpAE[3]=Cyc_PP_seql_f(sep,l->tl);_tmpAE[2]=Cyc_PP_line_doc();_tmpAE[1]=Cyc_PP_text(
sep);_tmpAE[0]=(struct Cyc_PP_Doc*)l->hd;Cyc_PP_cat(_tag_dyneither(_tmpAE,sizeof(
struct Cyc_PP_Doc*),4));});}}}struct Cyc_PP_Doc*Cyc_PP_seql(struct _dyneither_ptr
sep,struct Cyc_List_List*l0){return Cyc_PP_tab(Cyc_PP_seql_f(sep,l0));}struct Cyc_PP_Doc*
Cyc_PP_group(struct _dyneither_ptr start,struct _dyneither_ptr stop,struct
_dyneither_ptr sep,struct Cyc_List_List*ss){return({struct Cyc_PP_Doc*_tmpAF[3];
_tmpAF[2]=Cyc_PP_text(stop);_tmpAF[1]=Cyc_PP_seq(sep,ss);_tmpAF[0]=Cyc_PP_text(
start);Cyc_PP_cat(_tag_dyneither(_tmpAF,sizeof(struct Cyc_PP_Doc*),3));});}struct
Cyc_PP_Doc*Cyc_PP_egroup(struct _dyneither_ptr start,struct _dyneither_ptr stop,
struct _dyneither_ptr sep,struct Cyc_List_List*ss){if(ss == 0)return Cyc_PP_nil_doc();
else{return({struct Cyc_PP_Doc*_tmpB0[3];_tmpB0[2]=Cyc_PP_text(stop);_tmpB0[1]=
Cyc_PP_seq(sep,ss);_tmpB0[0]=Cyc_PP_text(start);Cyc_PP_cat(_tag_dyneither(_tmpB0,
sizeof(struct Cyc_PP_Doc*),3));});}}struct Cyc_PP_Doc*Cyc_PP_groupl(struct
_dyneither_ptr start,struct _dyneither_ptr stop,struct _dyneither_ptr sep,struct Cyc_List_List*
ss){return({struct Cyc_PP_Doc*_tmpB1[3];_tmpB1[2]=Cyc_PP_text(stop);_tmpB1[1]=Cyc_PP_seql(
sep,ss);_tmpB1[0]=Cyc_PP_text(start);Cyc_PP_cat(_tag_dyneither(_tmpB1,sizeof(
struct Cyc_PP_Doc*),3));});}
