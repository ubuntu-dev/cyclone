#ifndef _SETJMP_H_
#define _SETJMP_H_
#ifndef _jmp_buf_def_
#define _jmp_buf_def_
typedef int jmp_buf[192];
#endif
extern int setjmp(jmp_buf);
#endif
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
extern struct _xtunion_struct ADD_PREFIX(Null_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Null_Exception);
extern struct _xtunion_struct ADD_PREFIX(Array_bounds_struct);
extern struct _xtunion_struct * ADD_PREFIX(Array_bounds);
extern struct _xtunion_struct ADD_PREFIX(Match_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Match_Exception);
extern struct _xtunion_struct ADD_PREFIX(Bad_alloc_struct);
extern struct _xtunion_struct * ADD_PREFIX(Bad_alloc);

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
 struct Cyc_Core_NewRegion{struct _DynRegionHandle*dynregion;};struct Cyc_Core_Opt{
void*v;};extern char Cyc_Core_Invalid_argument[21];struct Cyc_Core_Invalid_argument_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[12];struct Cyc_Core_Failure_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[15];struct Cyc_Core_Impossible_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[14];extern char Cyc_Core_Unreachable[
16];struct Cyc_Core_Unreachable_struct{char*tag;struct _dyneither_ptr f1;};extern
char Cyc_Core_Open_Region[16];extern char Cyc_Core_Free_Region[16];struct Cyc___cycFILE;
extern struct Cyc___cycFILE*Cyc_stdout;struct Cyc_Cstdio___abstractFILE;struct Cyc_String_pa_struct{
int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_struct{int tag;unsigned long f1;}
;struct Cyc_Double_pa_struct{int tag;double f1;};struct Cyc_LongDouble_pa_struct{int
tag;long double f1;};struct Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{
int tag;unsigned long*f1;};struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,
struct _dyneither_ptr);int Cyc_fprintf(struct Cyc___cycFILE*,struct _dyneither_ptr,
struct _dyneither_ptr);int Cyc_fputc(int,struct Cyc___cycFILE*);struct Cyc_ShortPtr_sa_struct{
int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{int tag;unsigned short*f1;};
struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{int tag;
unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
extern char Cyc_FileCloseError[19];extern char Cyc_FileOpenError[18];struct Cyc_FileOpenError_struct{
char*tag;struct _dyneither_ptr f1;};int Cyc_file_string_write(struct Cyc___cycFILE*,
struct _dyneither_ptr src,int src_offset,int max_count);struct Cyc_List_List{void*hd;
struct Cyc_List_List*tl;};int Cyc_List_length(struct Cyc_List_List*x);struct Cyc_List_List*
Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[
18];struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);struct Cyc_List_List*
Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[
8];int Cyc_List_exists(int(*pred)(void*),struct Cyc_List_List*x);unsigned long Cyc_strlen(
struct _dyneither_ptr s);struct Cyc_Lineno_Pos{struct _dyneither_ptr logical_file;
struct _dyneither_ptr line;int line_no;int col;};extern char Cyc_Position_Exit[9];
struct Cyc_Position_Segment;struct Cyc_Position_Error{struct _dyneither_ptr source;
struct Cyc_Position_Segment*seg;void*kind;struct _dyneither_ptr desc;};extern char
Cyc_Position_Nocontext[14];struct _dyneither_ptr Cyc_Position_get_line_directive(
struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Loc_n_struct{int tag;};struct Cyc_Absyn_Rel_n_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{int tag;struct Cyc_List_List*
f1;};union Cyc_Absyn_Nmspace_union{struct Cyc_Absyn_Loc_n_struct Loc_n;struct Cyc_Absyn_Rel_n_struct
Rel_n;struct Cyc_Absyn_Abs_n_struct Abs_n;};struct _tuple0{union Cyc_Absyn_Nmspace_union
f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Conref;struct Cyc_Absyn_Tqual{int
print_const;int q_volatile;int q_restrict;int real_const;struct Cyc_Position_Segment*
loc;};struct Cyc_Absyn_Eq_constr_struct{int tag;void*f1;};struct Cyc_Absyn_Forward_constr_struct{
int tag;struct Cyc_Absyn_Conref*f1;};struct Cyc_Absyn_No_constr_struct{int tag;};
union Cyc_Absyn_Constraint_union{struct Cyc_Absyn_Eq_constr_struct Eq_constr;struct
Cyc_Absyn_Forward_constr_struct Forward_constr;struct Cyc_Absyn_No_constr_struct
No_constr;};struct Cyc_Absyn_Conref{union Cyc_Absyn_Constraint_union v;};struct Cyc_Absyn_Eq_kb_struct{
int tag;void*f1;};struct Cyc_Absyn_Unknown_kb_struct{int tag;struct Cyc_Core_Opt*f1;
};struct Cyc_Absyn_Less_kb_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;};struct
Cyc_Absyn_Tvar{struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_Upper_b_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_PtrLoc{struct Cyc_Position_Segment*
ptr_loc;struct Cyc_Position_Segment*rgn_loc;struct Cyc_Position_Segment*zt_loc;};
struct Cyc_Absyn_PtrAtts{void*rgn;struct Cyc_Absyn_Conref*nullable;struct Cyc_Absyn_Conref*
bounds;struct Cyc_Absyn_Conref*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct
Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts
ptr_atts;};struct Cyc_Absyn_VarargInfo{struct Cyc_Core_Opt*name;struct Cyc_Absyn_Tqual
tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;struct
Cyc_Core_Opt*effect;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;};struct
Cyc_Absyn_UnknownTunionInfo{struct _tuple0*name;int is_xtunion;int is_flat;};struct
Cyc_Absyn_UnknownTunion_struct{int tag;struct Cyc_Absyn_UnknownTunionInfo f1;};
struct Cyc_Absyn_KnownTunion_struct{int tag;struct Cyc_Absyn_Tuniondecl**f1;};union
Cyc_Absyn_TunionInfoU_union{struct Cyc_Absyn_UnknownTunion_struct UnknownTunion;
struct Cyc_Absyn_KnownTunion_struct KnownTunion;};struct Cyc_Absyn_TunionInfo{union
Cyc_Absyn_TunionInfoU_union tunion_info;struct Cyc_List_List*targs;struct Cyc_Core_Opt*
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
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*
f1;};struct Cyc_Absyn_DynRgnType_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
void**f4;};struct Cyc_Absyn_ValueofType_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_TagType_struct{int tag;void*f1;};struct Cyc_Absyn_AccessEff_struct{
int tag;void*f1;};struct Cyc_Absyn_JoinEff_struct{int tag;struct Cyc_List_List*f1;};
struct Cyc_Absyn_RgnsEff_struct{int tag;void*f1;};struct Cyc_Absyn_NoTypes_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*f2;};struct Cyc_Absyn_WithTypes_struct{
int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;struct Cyc_Core_Opt*
f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Regparm_att_struct{int tag;int f1;};
struct Cyc_Absyn_Aligned_att_struct{int tag;int f1;};struct Cyc_Absyn_Section_att_struct{
int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Format_att_struct{int tag;void*f1;
int f2;int f3;};struct Cyc_Absyn_Initializes_att_struct{int tag;int f1;};struct Cyc_Absyn_Mode_att_struct{
int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Carray_mod_struct{int tag;struct
Cyc_Absyn_Conref*f1;struct Cyc_Position_Segment*f2;};struct Cyc_Absyn_ConstArray_mod_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Conref*f2;struct Cyc_Position_Segment*
f3;};struct Cyc_Absyn_Pointer_mod_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct
Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_struct{int tag;void*f1;};struct
Cyc_Absyn_TypeParams_mod_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*
f2;int f3;};struct Cyc_Absyn_Attributes_mod_struct{int tag;struct Cyc_Position_Segment*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Char_c_struct{int tag;void*f1;char f2;
};struct Cyc_Absyn_Short_c_struct{int tag;void*f1;short f2;};struct Cyc_Absyn_Int_c_struct{
int tag;void*f1;int f2;};struct Cyc_Absyn_LongLong_c_struct{int tag;void*f1;
long long f2;};struct Cyc_Absyn_Float_c_struct{int tag;struct _dyneither_ptr f1;};
struct Cyc_Absyn_String_c_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Null_c_struct{
int tag;};union Cyc_Absyn_Cnst_union{struct Cyc_Absyn_Char_c_struct Char_c;struct Cyc_Absyn_Short_c_struct
Short_c;struct Cyc_Absyn_Int_c_struct Int_c;struct Cyc_Absyn_LongLong_c_struct
LongLong_c;struct Cyc_Absyn_Float_c_struct Float_c;struct Cyc_Absyn_String_c_struct
String_c;struct Cyc_Absyn_Null_c_struct Null_c;};struct Cyc_Absyn_VarargCallInfo{
int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
struct Cyc_Absyn_StructField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_struct{
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
_dyneither_ptr*f2;};struct Cyc_Absyn_AggrArrow_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Subscript_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_struct{int tag;struct Cyc_List_List*
f1;};struct _tuple1{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual f2;void*f3;};
struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;
int f4;};struct Cyc_Absyn_Struct_e_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*
f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Tunion_e_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*f2;struct Cyc_Absyn_Tunionfield*
f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*
f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{int tag;
struct _tuple0*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Valueof_e_struct{int tag;void*f1;
};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;void*r;struct Cyc_Position_Segment*
loc;void*annot;};struct Cyc_Absyn_Exp_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};
struct _tuple2{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_struct{
int tag;struct _tuple2 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{int tag;struct
Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct _dyneither_ptr*f1;
struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _tuple2 f2;struct _tuple2 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Fallthru_s_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_struct{
int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_struct{
int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple2 f2;};struct Cyc_Absyn_TryCatch_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ResetRegion_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stmt{void*r;struct Cyc_Position_Segment*
loc;struct Cyc_List_List*non_local_preds;int try_depth;void*annot;};struct Cyc_Absyn_Var_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_Reference_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_struct{
int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_struct{
int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_struct{int tag;
struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_struct{int tag;struct Cyc_Absyn_AggrInfo
f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Tunion_p_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;struct Cyc_List_List*
f3;int f4;};struct Cyc_Absyn_Int_p_struct{int tag;void*f1;int f2;};struct Cyc_Absyn_Char_p_struct{
int tag;char f1;};struct Cyc_Absyn_Float_p_struct{int tag;struct _dyneither_ptr f1;};
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
struct Cyc_List_List*attributes;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*
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
f1;};struct Cyc_Absyn_Region_d_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*
f2;int f3;struct Cyc_Absyn_Exp*f4;};struct Cyc_Absyn_Alias_d_struct{int tag;struct
Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Tvar*f2;struct Cyc_Absyn_Vardecl*f3;};struct Cyc_Absyn_Aggr_d_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Tunion_d_struct{int tag;
struct Cyc_Absyn_Tuniondecl*f1;};struct Cyc_Absyn_Enum_d_struct{int tag;struct Cyc_Absyn_Enumdecl*
f1;};struct Cyc_Absyn_Typedef_d_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};
struct Cyc_Absyn_Namespace_d_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Using_d_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ExternC_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*
r;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(
struct Cyc_Position_Segment*);void*Cyc_Absyn_conref_def(void*,struct Cyc_Absyn_Conref*
x);void*Cyc_Absyn_compress_kb(void*);void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*
k,struct Cyc_Core_Opt*tenv);extern void*Cyc_Absyn_bounds_one;struct Cyc_Absyn_Exp*
Cyc_Absyn_times_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftyp_exp(void*t,struct Cyc_Position_Segment*);
struct _dyneither_ptr Cyc_Absyn_attribute2string(void*);struct _tuple3{void*f1;
struct _tuple0*f2;};struct _tuple3 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfoU_union);
struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{
int expand_typedefs: 1;int qvar_to_Cids: 1;int add_cyc_prefix: 1;int to_VC: 1;int
decls_first: 1;int rewrite_temp_tvars: 1;int print_all_tvars: 1;int print_all_kinds: 1;
int print_all_effects: 1;int print_using_stmts: 1;int print_externC_stmts: 1;int
print_full_evars: 1;int print_zeroterm: 1;int generate_line_directives: 1;int
use_curr_namespace: 1;struct Cyc_List_List*curr_namespace;};void Cyc_Absynpp_set_params(
struct Cyc_Absynpp_Params*fs);int Cyc_Absynpp_is_anon_aggrtype(void*t);extern
struct _dyneither_ptr*Cyc_Absynpp_cyc_stringptr;int Cyc_Absynpp_exp_prec(struct Cyc_Absyn_Exp*);
struct _dyneither_ptr Cyc_Absynpp_char_escape(char);struct _dyneither_ptr Cyc_Absynpp_string_escape(
struct _dyneither_ptr);struct _dyneither_ptr Cyc_Absynpp_prim2str(void*p);int Cyc_Absynpp_is_declaration(
struct Cyc_Absyn_Stmt*s);struct _tuple4{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual
f2;void*f3;};struct _tuple1*Cyc_Absynpp_arg_mk_opt(struct _tuple4*arg);struct
_tuple5{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;};struct _tuple5
Cyc_Absynpp_to_tms(struct _RegionHandle*,struct Cyc_Absyn_Tqual tq,void*t);struct
_tuple6{unsigned int f1;int f2;};struct _tuple6 Cyc_Evexp_eval_const_uint_exp(struct
Cyc_Absyn_Exp*e);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;extern char Cyc_Set_Absent[
11];struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct
_RegionHandle*r;struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[12];extern char
Cyc_Dict_Absent[11];struct _tuple7{void*f1;void*f2;};struct _tuple7*Cyc_Dict_rchoose(
struct _RegionHandle*r,struct Cyc_Dict_Dict d);struct _tuple7*Cyc_Dict_rchoose(
struct _RegionHandle*,struct Cyc_Dict_Dict d);struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*
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
le;int allow_valueof;};void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct
_dyneither_ptr ap);void*Cyc_Tcutil_compress(void*t);extern void*Cyc_Cyclone_c_compiler;
static int Cyc_Absyndump_expand_typedefs;static int Cyc_Absyndump_qvar_to_Cids;
static int Cyc_Absyndump_add_cyc_prefix;static int Cyc_Absyndump_generate_line_directives;
static int Cyc_Absyndump_to_VC;void Cyc_Absyndump_set_params(struct Cyc_Absynpp_Params*
fs);void Cyc_Absyndump_set_params(struct Cyc_Absynpp_Params*fs){Cyc_Absyndump_expand_typedefs=
fs->expand_typedefs;Cyc_Absyndump_qvar_to_Cids=fs->qvar_to_Cids;Cyc_Absyndump_add_cyc_prefix=
fs->add_cyc_prefix;Cyc_Absyndump_to_VC=fs->to_VC;Cyc_Absyndump_generate_line_directives=
fs->generate_line_directives;Cyc_Absynpp_set_params(fs);}void Cyc_Absyndump_dumptyp(
void*);void Cyc_Absyndump_dumpntyp(void*t);void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*);
void Cyc_Absyndump_dumpexp_prec(int,struct Cyc_Absyn_Exp*);void Cyc_Absyndump_dumppat(
struct Cyc_Absyn_Pat*);void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*);void Cyc_Absyndump_dumpvardecl(
struct Cyc_Absyn_Vardecl*,struct Cyc_Position_Segment*);void Cyc_Absyndump_dumpdecl(
struct Cyc_Absyn_Decl*);void Cyc_Absyndump_dumptms(struct Cyc_List_List*tms,void(*f)(
void*),void*a);void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual,void*,void(*f)(
void*),void*);void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*fields);void
Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*fields);void Cyc_Absyndump_dumploc(
struct Cyc_Position_Segment*);struct Cyc___cycFILE**Cyc_Absyndump_dump_file=& Cyc_stdout;
void Cyc_Absyndump_ignore(void*x);void Cyc_Absyndump_ignore(void*x){return;}static
unsigned int Cyc_Absyndump_pos=0;static char Cyc_Absyndump_prev_char='x';int Cyc_Absyndump_need_space_before();
int Cyc_Absyndump_need_space_before(){switch(Cyc_Absyndump_prev_char){case '{':
_LL0: goto _LL1;case '}': _LL1: goto _LL2;case '(': _LL2: goto _LL3;case ')': _LL3: goto _LL4;
case '[': _LL4: goto _LL5;case ']': _LL5: goto _LL6;case ';': _LL6: goto _LL7;case ',': _LL7:
goto _LL8;case '=': _LL8: goto _LL9;case '?': _LL9: goto _LLA;case '!': _LLA: goto _LLB;case ' ':
_LLB: goto _LLC;case '\n': _LLC: goto _LLD;case '*': _LLD: return 0;default: _LLE: return 1;}}
void Cyc_Absyndump_dump(struct _dyneither_ptr s);void Cyc_Absyndump_dump(struct
_dyneither_ptr s){int sz=(int)Cyc_strlen((struct _dyneither_ptr)s);Cyc_Absyndump_pos
+=sz;if(Cyc_Absyndump_pos > 80){Cyc_Absyndump_pos=(unsigned int)sz;Cyc_fputc((int)'\n',*
Cyc_Absyndump_dump_file);}else{if(Cyc_Absyndump_need_space_before())Cyc_fputc((
int)' ',*Cyc_Absyndump_dump_file);}if(sz >= 1){Cyc_Absyndump_prev_char=*((const
char*)_check_dyneither_subscript(s,sizeof(char),sz - 1));Cyc_file_string_write(*
Cyc_Absyndump_dump_file,s,0,sz);}}void Cyc_Absyndump_dump_nospace(struct
_dyneither_ptr s);void Cyc_Absyndump_dump_nospace(struct _dyneither_ptr s){int sz=(
int)Cyc_strlen((struct _dyneither_ptr)s);Cyc_Absyndump_pos +=sz;if(sz >= 1){Cyc_file_string_write(*
Cyc_Absyndump_dump_file,s,0,sz);Cyc_Absyndump_prev_char=*((const char*)
_check_dyneither_subscript(s,sizeof(char),sz - 1));}}void Cyc_Absyndump_dump_char(
int c);void Cyc_Absyndump_dump_char(int c){++ Cyc_Absyndump_pos;Cyc_fputc(c,*Cyc_Absyndump_dump_file);
Cyc_Absyndump_prev_char=(char)c;}void Cyc_Absyndump_dumploc(struct Cyc_Position_Segment*
loc);void Cyc_Absyndump_dumploc(struct Cyc_Position_Segment*loc){if(loc == 0)
return;if(!Cyc_Absyndump_generate_line_directives)return;{struct _dyneither_ptr
_tmp0=Cyc_Position_get_line_directive(loc);Cyc_Absyndump_dump(_tmp0);}}void Cyc_Absyndump_dump_str(
struct _dyneither_ptr*s);void Cyc_Absyndump_dump_str(struct _dyneither_ptr*s){Cyc_Absyndump_dump(*
s);}void Cyc_Absyndump_dump_semi();void Cyc_Absyndump_dump_semi(){Cyc_Absyndump_dump_char((
int)';');}void Cyc_Absyndump_dump_sep(void(*f)(void*),struct Cyc_List_List*l,
struct _dyneither_ptr sep);void Cyc_Absyndump_dump_sep(void(*f)(void*),struct Cyc_List_List*
l,struct _dyneither_ptr sep){if(l == 0)return;for(0;l->tl != 0;l=l->tl){f((void*)l->hd);
Cyc_Absyndump_dump_nospace(sep);}f((void*)l->hd);}void Cyc_Absyndump_dump_sep_c(
void(*f)(void*,void*),void*env,struct Cyc_List_List*l,struct _dyneither_ptr sep);
void Cyc_Absyndump_dump_sep_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*
l,struct _dyneither_ptr sep){if(l == 0)return;for(0;l->tl != 0;l=l->tl){f(env,(void*)
l->hd);Cyc_Absyndump_dump_nospace(sep);}f(env,(void*)l->hd);}void Cyc_Absyndump_group(
void(*f)(void*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep);void Cyc_Absyndump_group(void(*f)(void*),
struct Cyc_List_List*l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct
_dyneither_ptr sep){Cyc_Absyndump_dump_nospace(start);Cyc_Absyndump_dump_sep(f,l,
sep);Cyc_Absyndump_dump_nospace(end);}void Cyc_Absyndump_group_c(void(*f)(void*,
void*),void*env,struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep);void Cyc_Absyndump_group_c(void(*f)(
void*,void*),void*env,struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep){Cyc_Absyndump_dump_nospace(start);Cyc_Absyndump_dump_sep_c(
f,env,l,sep);Cyc_Absyndump_dump_nospace(end);}void Cyc_Absyndump_egroup(void(*f)(
void*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct _dyneither_ptr end,
struct _dyneither_ptr sep);void Cyc_Absyndump_egroup(void(*f)(void*),struct Cyc_List_List*
l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep){if(
l != 0)Cyc_Absyndump_group(f,l,start,end,sep);}void Cyc_Absyndump_dumpqvar(struct
_tuple0*v);void Cyc_Absyndump_dumpqvar(struct _tuple0*v){struct Cyc_List_List*_tmp1=
0;struct _dyneither_ptr**prefix=0;{union Cyc_Absyn_Nmspace_union _tmp2=(*v).f1;
struct Cyc_List_List*_tmp3;struct Cyc_List_List*_tmp4;_LL11: if((_tmp2.Loc_n).tag != 
0)goto _LL13;_LL12: _tmp3=0;goto _LL14;_LL13: if((_tmp2.Rel_n).tag != 1)goto _LL15;
_tmp3=(_tmp2.Rel_n).f1;_LL14: _tmp1=_tmp3;goto _LL10;_LL15: if((_tmp2.Abs_n).tag != 
2)goto _LL10;_tmp4=(_tmp2.Abs_n).f1;_LL16: if(Cyc_Absyndump_qvar_to_Cids  && Cyc_Absyndump_add_cyc_prefix)
prefix=(struct _dyneither_ptr**)& Cyc_Absynpp_cyc_stringptr;_tmp1=_tmp4;goto _LL10;
_LL10:;}if(prefix != 0){Cyc_Absyndump_dump_str(*prefix);if(Cyc_Absyndump_qvar_to_Cids)
Cyc_Absyndump_dump_char((int)'_');else{const char*_tmp2BE;Cyc_Absyndump_dump_nospace(((
_tmp2BE="::",_tag_dyneither(_tmp2BE,sizeof(char),3))));}}if(_tmp1 != 0){Cyc_Absyndump_dump_nospace(*((
struct _dyneither_ptr*)_tmp1->hd));for(_tmp1=_tmp1->tl;_tmp1 != 0;_tmp1=_tmp1->tl){
if(Cyc_Absyndump_qvar_to_Cids)Cyc_Absyndump_dump_char((int)'_');else{const char*
_tmp2BF;Cyc_Absyndump_dump_nospace(((_tmp2BF="::",_tag_dyneither(_tmp2BF,sizeof(
char),3))));}Cyc_Absyndump_dump_nospace(*((struct _dyneither_ptr*)_tmp1->hd));}
if(Cyc_Absyndump_qvar_to_Cids){const char*_tmp2C0;Cyc_Absyndump_dump_nospace(((
_tmp2C0="_",_tag_dyneither(_tmp2C0,sizeof(char),2))));}else{const char*_tmp2C1;
Cyc_Absyndump_dump_nospace(((_tmp2C1="::",_tag_dyneither(_tmp2C1,sizeof(char),3))));}
Cyc_Absyndump_dump_nospace(*(*v).f2);}else{if(prefix != 0)Cyc_Absyndump_dump_nospace(*(*
v).f2);else{Cyc_Absyndump_dump_str((*v).f2);}}}void Cyc_Absyndump_dumptq(struct
Cyc_Absyn_Tqual tq);void Cyc_Absyndump_dumptq(struct Cyc_Absyn_Tqual tq){if(tq.q_restrict){
const char*_tmp2C2;Cyc_Absyndump_dump(((_tmp2C2="restrict",_tag_dyneither(_tmp2C2,
sizeof(char),9))));}if(tq.q_volatile){const char*_tmp2C3;Cyc_Absyndump_dump(((
_tmp2C3="volatile",_tag_dyneither(_tmp2C3,sizeof(char),9))));}if(tq.print_const){
const char*_tmp2C4;Cyc_Absyndump_dump(((_tmp2C4="const",_tag_dyneither(_tmp2C4,
sizeof(char),6))));}}void Cyc_Absyndump_dumpscope(void*sc);void Cyc_Absyndump_dumpscope(
void*sc){void*_tmpC=sc;_LL18: if((int)_tmpC != 0)goto _LL1A;_LL19:{const char*
_tmp2C5;Cyc_Absyndump_dump(((_tmp2C5="static",_tag_dyneither(_tmp2C5,sizeof(char),
7))));}return;_LL1A: if((int)_tmpC != 2)goto _LL1C;_LL1B: return;_LL1C: if((int)_tmpC
!= 3)goto _LL1E;_LL1D:{const char*_tmp2C6;Cyc_Absyndump_dump(((_tmp2C6="extern",
_tag_dyneither(_tmp2C6,sizeof(char),7))));}return;_LL1E: if((int)_tmpC != 4)goto
_LL20;_LL1F:{const char*_tmp2C7;Cyc_Absyndump_dump(((_tmp2C7="extern \"C\"",
_tag_dyneither(_tmp2C7,sizeof(char),11))));}return;_LL20: if((int)_tmpC != 1)goto
_LL22;_LL21:{const char*_tmp2C8;Cyc_Absyndump_dump(((_tmp2C8="abstract",
_tag_dyneither(_tmp2C8,sizeof(char),9))));}return;_LL22: if((int)_tmpC != 5)goto
_LL17;_LL23:{const char*_tmp2C9;Cyc_Absyndump_dump(((_tmp2C9="register",
_tag_dyneither(_tmp2C9,sizeof(char),9))));}return;_LL17:;}void Cyc_Absyndump_dumpkind(
void*k);void Cyc_Absyndump_dumpkind(void*k){void*_tmp12=k;_LL25: if((int)_tmp12 != 
0)goto _LL27;_LL26:{const char*_tmp2CA;Cyc_Absyndump_dump(((_tmp2CA="A",
_tag_dyneither(_tmp2CA,sizeof(char),2))));}return;_LL27: if((int)_tmp12 != 1)goto
_LL29;_LL28:{const char*_tmp2CB;Cyc_Absyndump_dump(((_tmp2CB="M",_tag_dyneither(
_tmp2CB,sizeof(char),2))));}return;_LL29: if((int)_tmp12 != 2)goto _LL2B;_LL2A:{
const char*_tmp2CC;Cyc_Absyndump_dump(((_tmp2CC="B",_tag_dyneither(_tmp2CC,
sizeof(char),2))));}return;_LL2B: if((int)_tmp12 != 3)goto _LL2D;_LL2C:{const char*
_tmp2CD;Cyc_Absyndump_dump(((_tmp2CD="R",_tag_dyneither(_tmp2CD,sizeof(char),2))));}
return;_LL2D: if((int)_tmp12 != 4)goto _LL2F;_LL2E:{const char*_tmp2CE;Cyc_Absyndump_dump(((
_tmp2CE="UR",_tag_dyneither(_tmp2CE,sizeof(char),3))));}return;_LL2F: if((int)
_tmp12 != 5)goto _LL31;_LL30:{const char*_tmp2CF;Cyc_Absyndump_dump(((_tmp2CF="TR",
_tag_dyneither(_tmp2CF,sizeof(char),3))));}return;_LL31: if((int)_tmp12 != 6)goto
_LL33;_LL32:{const char*_tmp2D0;Cyc_Absyndump_dump(((_tmp2D0="E",_tag_dyneither(
_tmp2D0,sizeof(char),2))));}return;_LL33: if((int)_tmp12 != 7)goto _LL24;_LL34:{
const char*_tmp2D1;Cyc_Absyndump_dump(((_tmp2D1="I",_tag_dyneither(_tmp2D1,
sizeof(char),2))));}return;_LL24:;}void Cyc_Absyndump_dumpaggr_kind(void*k);void
Cyc_Absyndump_dumpaggr_kind(void*k){void*_tmp1B=k;_LL36: if((int)_tmp1B != 0)goto
_LL38;_LL37:{const char*_tmp2D2;Cyc_Absyndump_dump(((_tmp2D2="struct",
_tag_dyneither(_tmp2D2,sizeof(char),7))));}return;_LL38: if((int)_tmp1B != 1)goto
_LL35;_LL39:{const char*_tmp2D3;Cyc_Absyndump_dump(((_tmp2D3="union",
_tag_dyneither(_tmp2D3,sizeof(char),6))));}return;_LL35:;}void Cyc_Absyndump_dumptps(
struct Cyc_List_List*ts);void Cyc_Absyndump_dumptps(struct Cyc_List_List*ts){const
char*_tmp2D6;const char*_tmp2D5;const char*_tmp2D4;Cyc_Absyndump_egroup(Cyc_Absyndump_dumptyp,
ts,((_tmp2D4="<",_tag_dyneither(_tmp2D4,sizeof(char),2))),((_tmp2D5=">",
_tag_dyneither(_tmp2D5,sizeof(char),2))),((_tmp2D6=",",_tag_dyneither(_tmp2D6,
sizeof(char),2))));}void Cyc_Absyndump_dumptvar(struct Cyc_Absyn_Tvar*tv);void Cyc_Absyndump_dumptvar(
struct Cyc_Absyn_Tvar*tv){Cyc_Absyndump_dump_str(tv->name);}void Cyc_Absyndump_dumpkindedtvar(
struct Cyc_Absyn_Tvar*tv);void Cyc_Absyndump_dumpkindedtvar(struct Cyc_Absyn_Tvar*
tv){Cyc_Absyndump_dump_str(tv->name);{void*_tmp21=Cyc_Absyn_compress_kb((void*)
tv->kind);void*_tmp22;void*_tmp23;_LL3B: if(*((int*)_tmp21)!= 0)goto _LL3D;_tmp22=(
void*)((struct Cyc_Absyn_Eq_kb_struct*)_tmp21)->f1;if((int)_tmp22 != 2)goto _LL3D;
_LL3C: goto _LL3E;_LL3D: if(*((int*)_tmp21)!= 2)goto _LL3F;_LL3E: goto _LL40;_LL3F: if(*((
int*)_tmp21)!= 1)goto _LL41;_LL40:{const char*_tmp2D7;Cyc_Absyndump_dump(((_tmp2D7="::?",
_tag_dyneither(_tmp2D7,sizeof(char),4))));}goto _LL3A;_LL41: if(*((int*)_tmp21)!= 
0)goto _LL3A;_tmp23=(void*)((struct Cyc_Absyn_Eq_kb_struct*)_tmp21)->f1;_LL42:{
const char*_tmp2D8;Cyc_Absyndump_dump(((_tmp2D8="::",_tag_dyneither(_tmp2D8,
sizeof(char),3))));}Cyc_Absyndump_dumpkind(_tmp23);goto _LL3A;_LL3A:;}}void Cyc_Absyndump_dumptvars(
struct Cyc_List_List*tvs);void Cyc_Absyndump_dumptvars(struct Cyc_List_List*tvs){
const char*_tmp2DB;const char*_tmp2DA;const char*_tmp2D9;((void(*)(void(*f)(struct
Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_egroup)(Cyc_Absyndump_dumptvar,
tvs,((_tmp2D9="<",_tag_dyneither(_tmp2D9,sizeof(char),2))),((_tmp2DA=">",
_tag_dyneither(_tmp2DA,sizeof(char),2))),((_tmp2DB=",",_tag_dyneither(_tmp2DB,
sizeof(char),2))));}void Cyc_Absyndump_dumpkindedtvars(struct Cyc_List_List*tvs);
void Cyc_Absyndump_dumpkindedtvars(struct Cyc_List_List*tvs){const char*_tmp2DE;
const char*_tmp2DD;const char*_tmp2DC;((void(*)(void(*f)(struct Cyc_Absyn_Tvar*),
struct Cyc_List_List*l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct
_dyneither_ptr sep))Cyc_Absyndump_egroup)(Cyc_Absyndump_dumpkindedtvar,tvs,((
_tmp2DC="<",_tag_dyneither(_tmp2DC,sizeof(char),2))),((_tmp2DD=">",
_tag_dyneither(_tmp2DD,sizeof(char),2))),((_tmp2DE=",",_tag_dyneither(_tmp2DE,
sizeof(char),2))));}struct _tuple8{struct Cyc_Absyn_Tqual f1;void*f2;};void Cyc_Absyndump_dumparg(
struct _tuple8*pr);void Cyc_Absyndump_dumparg(struct _tuple8*pr){((void(*)(struct
Cyc_Absyn_Tqual,void*,void(*f)(int),int))Cyc_Absyndump_dumptqtd)((*pr).f1,(*pr).f2,(
void(*)(int x))Cyc_Absyndump_ignore,0);}void Cyc_Absyndump_dumpargs(struct Cyc_List_List*
ts);void Cyc_Absyndump_dumpargs(struct Cyc_List_List*ts){const char*_tmp2E1;const
char*_tmp2E0;const char*_tmp2DF;((void(*)(void(*f)(struct _tuple8*),struct Cyc_List_List*
l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumparg,ts,((_tmp2DF="(",_tag_dyneither(_tmp2DF,sizeof(char),2))),((
_tmp2E0=")",_tag_dyneither(_tmp2E0,sizeof(char),2))),((_tmp2E1=",",
_tag_dyneither(_tmp2E1,sizeof(char),2))));}void Cyc_Absyndump_dump_callconv(
struct Cyc_List_List*atts);void Cyc_Absyndump_dump_callconv(struct Cyc_List_List*
atts){for(0;atts != 0;atts=atts->tl){void*_tmp2F=(void*)atts->hd;_LL44: if((int)
_tmp2F != 0)goto _LL46;_LL45:{const char*_tmp2E2;Cyc_Absyndump_dump(((_tmp2E2="_stdcall",
_tag_dyneither(_tmp2E2,sizeof(char),9))));}return;_LL46: if((int)_tmp2F != 1)goto
_LL48;_LL47:{const char*_tmp2E3;Cyc_Absyndump_dump(((_tmp2E3="_cdecl",
_tag_dyneither(_tmp2E3,sizeof(char),7))));}return;_LL48: if((int)_tmp2F != 2)goto
_LL4A;_LL49:{const char*_tmp2E4;Cyc_Absyndump_dump(((_tmp2E4="_fastcall",
_tag_dyneither(_tmp2E4,sizeof(char),10))));}return;_LL4A:;_LL4B: goto _LL43;_LL43:;}}
void Cyc_Absyndump_dump_noncallconv(struct Cyc_List_List*atts);void Cyc_Absyndump_dump_noncallconv(
struct Cyc_List_List*atts){int hasatt=0;{struct Cyc_List_List*atts2=atts;for(0;
atts2 != 0;atts2=atts2->tl){void*_tmp33=(void*)((struct Cyc_List_List*)_check_null(
atts))->hd;_LL4D: if((int)_tmp33 != 0)goto _LL4F;_LL4E: goto _LL50;_LL4F: if((int)
_tmp33 != 1)goto _LL51;_LL50: goto _LL52;_LL51: if((int)_tmp33 != 2)goto _LL53;_LL52:
goto _LL4C;_LL53:;_LL54: hasatt=1;goto _LL4C;_LL4C:;}}if(!hasatt)return;{const char*
_tmp2E5;Cyc_Absyndump_dump(((_tmp2E5="__declspec(",_tag_dyneither(_tmp2E5,
sizeof(char),12))));}for(0;atts != 0;atts=atts->tl){void*_tmp35=(void*)atts->hd;
_LL56: if((int)_tmp35 != 0)goto _LL58;_LL57: goto _LL59;_LL58: if((int)_tmp35 != 1)goto
_LL5A;_LL59: goto _LL5B;_LL5A: if((int)_tmp35 != 2)goto _LL5C;_LL5B: goto _LL55;_LL5C:;
_LL5D: Cyc_Absyndump_dump(Cyc_Absyn_attribute2string((void*)atts->hd));goto _LL55;
_LL55:;}Cyc_Absyndump_dump_char((int)')');}void Cyc_Absyndump_dumpatts(struct Cyc_List_List*
atts);void Cyc_Absyndump_dumpatts(struct Cyc_List_List*atts){if(atts == 0)return;{
void*_tmp36=Cyc_Cyclone_c_compiler;_LL5F: if((int)_tmp36 != 0)goto _LL61;_LL60:{
const char*_tmp2E6;Cyc_Absyndump_dump(((_tmp2E6=" __attribute__((",_tag_dyneither(
_tmp2E6,sizeof(char),17))));}for(0;atts != 0;atts=atts->tl){Cyc_Absyndump_dump(
Cyc_Absyn_attribute2string((void*)atts->hd));if(atts->tl != 0){const char*_tmp2E7;
Cyc_Absyndump_dump(((_tmp2E7=",",_tag_dyneither(_tmp2E7,sizeof(char),2))));}}{
const char*_tmp2E8;Cyc_Absyndump_dump(((_tmp2E8=")) ",_tag_dyneither(_tmp2E8,
sizeof(char),4))));}return;_LL61: if((int)_tmp36 != 1)goto _LL5E;_LL62: Cyc_Absyndump_dump_noncallconv(
atts);return;_LL5E:;}}int Cyc_Absyndump_next_is_pointer(struct Cyc_List_List*tms);
int Cyc_Absyndump_next_is_pointer(struct Cyc_List_List*tms){if(tms == 0)return 0;{
void*_tmp3A=(void*)tms->hd;_LL64: if(*((int*)_tmp3A)!= 2)goto _LL66;_LL65: return 1;
_LL66:;_LL67: return 0;_LL63:;}}static void Cyc_Absyndump_dumprgn(void*t);static void
Cyc_Absyndump_dumprgn(void*t){void*_tmp3B=Cyc_Tcutil_compress(t);_LL69: if((int)
_tmp3B != 2)goto _LL6B;_LL6A:{const char*_tmp2E9;Cyc_Absyndump_dump(((_tmp2E9="`H",
_tag_dyneither(_tmp2E9,sizeof(char),3))));}goto _LL68;_LL6B:;_LL6C: Cyc_Absyndump_dumpntyp(
t);goto _LL68;_LL68:;}struct _tuple9{struct Cyc_List_List*f1;struct Cyc_List_List*f2;
};static struct _tuple9 Cyc_Absyndump_effects_split(void*t);static struct _tuple9 Cyc_Absyndump_effects_split(
void*t){struct Cyc_List_List*rgions=0;struct Cyc_List_List*effects=0;{void*_tmp3D=
Cyc_Tcutil_compress(t);void*_tmp3E;struct Cyc_List_List*_tmp3F;_LL6E: if(_tmp3D <= (
void*)4)goto _LL72;if(*((int*)_tmp3D)!= 19)goto _LL70;_tmp3E=(void*)((struct Cyc_Absyn_AccessEff_struct*)
_tmp3D)->f1;_LL6F:{struct Cyc_List_List*_tmp2EA;rgions=((_tmp2EA=_cycalloc(
sizeof(*_tmp2EA)),((_tmp2EA->hd=(void*)_tmp3E,((_tmp2EA->tl=rgions,_tmp2EA))))));}
goto _LL6D;_LL70: if(*((int*)_tmp3D)!= 20)goto _LL72;_tmp3F=((struct Cyc_Absyn_JoinEff_struct*)
_tmp3D)->f1;_LL71: for(0;_tmp3F != 0;_tmp3F=_tmp3F->tl){struct Cyc_List_List*_tmp42;
struct Cyc_List_List*_tmp43;struct _tuple9 _tmp41=Cyc_Absyndump_effects_split((void*)
_tmp3F->hd);_tmp42=_tmp41.f1;_tmp43=_tmp41.f2;rgions=Cyc_List_imp_append(_tmp42,
rgions);effects=Cyc_List_imp_append(_tmp43,effects);}goto _LL6D;_LL72:;_LL73:{
struct Cyc_List_List*_tmp2EB;effects=((_tmp2EB=_cycalloc(sizeof(*_tmp2EB)),((
_tmp2EB->hd=(void*)t,((_tmp2EB->tl=effects,_tmp2EB))))));}goto _LL6D;_LL6D:;}{
struct _tuple9 _tmp2EC;return(_tmp2EC.f1=rgions,((_tmp2EC.f2=effects,_tmp2EC)));}}
static void Cyc_Absyndump_dumpeff(void*t);static void Cyc_Absyndump_dumpeff(void*t){
struct Cyc_List_List*_tmp47;struct Cyc_List_List*_tmp48;struct _tuple9 _tmp46=Cyc_Absyndump_effects_split(
t);_tmp47=_tmp46.f1;_tmp48=_tmp46.f2;_tmp47=Cyc_List_imp_rev(_tmp47);_tmp48=Cyc_List_imp_rev(
_tmp48);for(0;_tmp48 != 0;_tmp48=_tmp48->tl){Cyc_Absyndump_dumpntyp((void*)_tmp48->hd);
Cyc_Absyndump_dump_char((int)'+');}Cyc_Absyndump_dump_char((int)'{');for(0;
_tmp47 != 0;_tmp47=_tmp47->tl){Cyc_Absyndump_dumprgn((void*)_tmp47->hd);if(_tmp47->tl
!= 0)Cyc_Absyndump_dump_char((int)',');}Cyc_Absyndump_dump_char((int)'}');}void
Cyc_Absyndump_dumpntyp(void*t);void Cyc_Absyndump_dumpntyp(void*t){void*_tmp49=t;
struct Cyc_Absyn_Tvar*_tmp4A;struct Cyc_Core_Opt*_tmp4B;struct Cyc_Core_Opt*_tmp4C;
int _tmp4D;struct Cyc_Core_Opt*_tmp4E;struct Cyc_Core_Opt*_tmp4F;struct Cyc_Core_Opt
_tmp50;void*_tmp51;int _tmp52;struct Cyc_Absyn_TunionInfo _tmp53;union Cyc_Absyn_TunionInfoU_union
_tmp54;struct Cyc_List_List*_tmp55;struct Cyc_Core_Opt*_tmp56;struct Cyc_Absyn_TunionFieldInfo
_tmp57;union Cyc_Absyn_TunionFieldInfoU_union _tmp58;struct Cyc_List_List*_tmp59;
void*_tmp5A;void*_tmp5B;void*_tmp5C;void*_tmp5D;void*_tmp5E;void*_tmp5F;void*
_tmp60;void*_tmp61;void*_tmp62;void*_tmp63;void*_tmp64;void*_tmp65;void*_tmp66;
void*_tmp67;void*_tmp68;void*_tmp69;void*_tmp6A;void*_tmp6B;void*_tmp6C;void*
_tmp6D;void*_tmp6E;void*_tmp6F;void*_tmp70;void*_tmp71;void*_tmp72;void*_tmp73;
void*_tmp74;void*_tmp75;void*_tmp76;void*_tmp77;int _tmp78;struct Cyc_List_List*
_tmp79;struct Cyc_Absyn_AggrInfo _tmp7A;union Cyc_Absyn_AggrInfoU_union _tmp7B;
struct Cyc_List_List*_tmp7C;void*_tmp7D;struct Cyc_List_List*_tmp7E;struct _tuple0*
_tmp7F;struct Cyc_List_List*_tmp80;struct _tuple0*_tmp81;struct Cyc_List_List*
_tmp82;struct Cyc_Absyn_Exp*_tmp83;void*_tmp84;void*_tmp85;void*_tmp86;void*
_tmp87;_LL75: if(_tmp49 <= (void*)4)goto _LL7B;if(*((int*)_tmp49)!= 7)goto _LL77;
_LL76: goto _LL78;_LL77: if(*((int*)_tmp49)!= 8)goto _LL79;_LL78: goto _LL7A;_LL79: if(*((
int*)_tmp49)!= 4)goto _LL7B;_LL7A: return;_LL7B: if((int)_tmp49 != 0)goto _LL7D;_LL7C:{
const char*_tmp2ED;Cyc_Absyndump_dump(((_tmp2ED="void",_tag_dyneither(_tmp2ED,
sizeof(char),5))));}return;_LL7D: if(_tmp49 <= (void*)4)goto _LLA5;if(*((int*)
_tmp49)!= 1)goto _LL7F;_tmp4A=((struct Cyc_Absyn_VarType_struct*)_tmp49)->f1;_LL7E:
Cyc_Absyndump_dump_str(_tmp4A->name);return;_LL7F: if(*((int*)_tmp49)!= 0)goto
_LL81;_tmp4B=((struct Cyc_Absyn_Evar_struct*)_tmp49)->f1;_tmp4C=((struct Cyc_Absyn_Evar_struct*)
_tmp49)->f2;if(_tmp4C != 0)goto _LL81;_tmp4D=((struct Cyc_Absyn_Evar_struct*)_tmp49)->f3;
_LL80:{const char*_tmp2EE;Cyc_Absyndump_dump(((_tmp2EE="%",_tag_dyneither(_tmp2EE,
sizeof(char),2))));}if(_tmp4B == 0){const char*_tmp2EF;Cyc_Absyndump_dump(((
_tmp2EF="?",_tag_dyneither(_tmp2EF,sizeof(char),2))));}else{Cyc_Absyndump_dumpkind((
void*)_tmp4B->v);}{const char*_tmp2F3;void*_tmp2F2[1];struct Cyc_Int_pa_struct
_tmp2F1;Cyc_Absyndump_dump((struct _dyneither_ptr)((_tmp2F1.tag=1,((_tmp2F1.f1=(
unsigned long)_tmp4D,((_tmp2F2[0]=& _tmp2F1,Cyc_aprintf(((_tmp2F3="(%d)",
_tag_dyneither(_tmp2F3,sizeof(char),5))),_tag_dyneither(_tmp2F2,sizeof(void*),1)))))))));}
return;_LL81: if(*((int*)_tmp49)!= 0)goto _LL83;_tmp4E=((struct Cyc_Absyn_Evar_struct*)
_tmp49)->f1;_tmp4F=((struct Cyc_Absyn_Evar_struct*)_tmp49)->f2;if(_tmp4F == 0)goto
_LL83;_tmp50=*_tmp4F;_tmp51=(void*)_tmp50.v;_tmp52=((struct Cyc_Absyn_Evar_struct*)
_tmp49)->f3;_LL82: Cyc_Absyndump_dumpntyp(_tmp51);return;_LL83: if(*((int*)_tmp49)
!= 2)goto _LL85;_tmp53=((struct Cyc_Absyn_TunionType_struct*)_tmp49)->f1;_tmp54=
_tmp53.tunion_info;_tmp55=_tmp53.targs;_tmp56=_tmp53.rgn;_LL84:{union Cyc_Absyn_TunionInfoU_union
_tmp8E=_tmp54;struct Cyc_Absyn_UnknownTunionInfo _tmp8F;struct _tuple0*_tmp90;int
_tmp91;struct Cyc_Absyn_Tuniondecl**_tmp92;struct Cyc_Absyn_Tuniondecl*_tmp93;
struct Cyc_Absyn_Tuniondecl _tmp94;struct _tuple0*_tmp95;int _tmp96;_LLC8: if((_tmp8E.UnknownTunion).tag
!= 0)goto _LLCA;_tmp8F=(_tmp8E.UnknownTunion).f1;_tmp90=_tmp8F.name;_tmp91=_tmp8F.is_xtunion;
_LLC9: _tmp95=_tmp90;_tmp96=_tmp91;goto _LLCB;_LLCA: if((_tmp8E.KnownTunion).tag != 
1)goto _LLC7;_tmp92=(_tmp8E.KnownTunion).f1;_tmp93=*_tmp92;_tmp94=*_tmp93;_tmp95=
_tmp94.name;_tmp96=_tmp94.is_xtunion;_LLCB: if(_tmp96){const char*_tmp2F4;Cyc_Absyndump_dump(((
_tmp2F4="xtunion ",_tag_dyneither(_tmp2F4,sizeof(char),9))));}else{const char*
_tmp2F5;Cyc_Absyndump_dump(((_tmp2F5="tunion ",_tag_dyneither(_tmp2F5,sizeof(
char),8))));}{void*r=(unsigned int)_tmp56?(void*)_tmp56->v:(void*)2;{void*_tmp99=
Cyc_Tcutil_compress(r);_LLCD: if((int)_tmp99 != 2)goto _LLCF;_LLCE: goto _LLCC;_LLCF:;
_LLD0: Cyc_Absyndump_dumptyp(r);{const char*_tmp2F6;Cyc_Absyndump_dump(((_tmp2F6=" ",
_tag_dyneither(_tmp2F6,sizeof(char),2))));}goto _LLCC;_LLCC:;}Cyc_Absyndump_dumpqvar(
_tmp95);Cyc_Absyndump_dumptps(_tmp55);goto _LLC7;}_LLC7:;}goto _LL74;_LL85: if(*((
int*)_tmp49)!= 3)goto _LL87;_tmp57=((struct Cyc_Absyn_TunionFieldType_struct*)
_tmp49)->f1;_tmp58=_tmp57.field_info;_tmp59=_tmp57.targs;_LL86:{union Cyc_Absyn_TunionFieldInfoU_union
_tmp9B=_tmp58;struct Cyc_Absyn_UnknownTunionFieldInfo _tmp9C;struct _tuple0*_tmp9D;
struct _tuple0*_tmp9E;int _tmp9F;struct Cyc_Absyn_Tuniondecl*_tmpA0;struct Cyc_Absyn_Tuniondecl
_tmpA1;struct _tuple0*_tmpA2;int _tmpA3;struct Cyc_Absyn_Tunionfield*_tmpA4;struct
Cyc_Absyn_Tunionfield _tmpA5;struct _tuple0*_tmpA6;_LLD2: if((_tmp9B.UnknownTunionfield).tag
!= 0)goto _LLD4;_tmp9C=(_tmp9B.UnknownTunionfield).f1;_tmp9D=_tmp9C.tunion_name;
_tmp9E=_tmp9C.field_name;_tmp9F=_tmp9C.is_xtunion;_LLD3: _tmpA2=_tmp9D;_tmpA3=
_tmp9F;_tmpA6=_tmp9E;goto _LLD5;_LLD4: if((_tmp9B.KnownTunionfield).tag != 1)goto
_LLD1;_tmpA0=(_tmp9B.KnownTunionfield).f1;_tmpA1=*_tmpA0;_tmpA2=_tmpA1.name;
_tmpA3=_tmpA1.is_xtunion;_tmpA4=(_tmp9B.KnownTunionfield).f2;_tmpA5=*_tmpA4;
_tmpA6=_tmpA5.name;_LLD5: if(_tmpA3){const char*_tmp2F7;Cyc_Absyndump_dump(((
_tmp2F7="xtunion ",_tag_dyneither(_tmp2F7,sizeof(char),9))));}else{const char*
_tmp2F8;Cyc_Absyndump_dump(((_tmp2F8="tunion ",_tag_dyneither(_tmp2F8,sizeof(
char),8))));}Cyc_Absyndump_dumpqvar(_tmpA2);{const char*_tmp2F9;Cyc_Absyndump_dump(((
_tmp2F9=".",_tag_dyneither(_tmp2F9,sizeof(char),2))));}Cyc_Absyndump_dumpqvar(
_tmpA6);Cyc_Absyndump_dumptps(_tmp59);goto _LLD1;_LLD1:;}goto _LL74;_LL87: if(*((
int*)_tmp49)!= 5)goto _LL89;_tmp5A=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp5A != 2)goto _LL89;_tmp5B=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp5B != 2)goto _LL89;_LL88: goto _LL8A;_LL89: if(*((int*)_tmp49)
!= 5)goto _LL8B;_tmp5C=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;if((
int)_tmp5C != 0)goto _LL8B;_tmp5D=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f2;
if((int)_tmp5D != 2)goto _LL8B;_LL8A:{const char*_tmp2FA;Cyc_Absyndump_dump(((
_tmp2FA="int",_tag_dyneither(_tmp2FA,sizeof(char),4))));}return;_LL8B: if(*((int*)
_tmp49)!= 5)goto _LL8D;_tmp5E=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;
if((int)_tmp5E != 2)goto _LL8D;_tmp5F=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp5F != 3)goto _LL8D;_LL8C: goto _LL8E;_LL8D: if(*((int*)_tmp49)
!= 5)goto _LL8F;_tmp60=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;if((
int)_tmp60 != 0)goto _LL8F;_tmp61=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f2;
if((int)_tmp61 != 3)goto _LL8F;_LL8E:{const char*_tmp2FB;Cyc_Absyndump_dump(((
_tmp2FB="long",_tag_dyneither(_tmp2FB,sizeof(char),5))));}return;_LL8F: if(*((int*)
_tmp49)!= 5)goto _LL91;_tmp62=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;
if((int)_tmp62 != 2)goto _LL91;_tmp63=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp63 != 0)goto _LL91;_LL90:{const char*_tmp2FC;Cyc_Absyndump_dump(((
_tmp2FC="char",_tag_dyneither(_tmp2FC,sizeof(char),5))));}return;_LL91: if(*((int*)
_tmp49)!= 5)goto _LL93;_tmp64=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;
if((int)_tmp64 != 0)goto _LL93;_tmp65=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp65 != 0)goto _LL93;_LL92:{const char*_tmp2FD;Cyc_Absyndump_dump(((
_tmp2FD="signed char",_tag_dyneither(_tmp2FD,sizeof(char),12))));}return;_LL93:
if(*((int*)_tmp49)!= 5)goto _LL95;_tmp66=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp66 != 1)goto _LL95;_tmp67=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp67 != 0)goto _LL95;_LL94:{const char*_tmp2FE;Cyc_Absyndump_dump(((
_tmp2FE="unsigned char",_tag_dyneither(_tmp2FE,sizeof(char),14))));}return;_LL95:
if(*((int*)_tmp49)!= 5)goto _LL97;_tmp68=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp68 != 2)goto _LL97;_tmp69=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp69 != 1)goto _LL97;_LL96: goto _LL98;_LL97: if(*((int*)_tmp49)
!= 5)goto _LL99;_tmp6A=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;if((
int)_tmp6A != 0)goto _LL99;_tmp6B=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f2;
if((int)_tmp6B != 1)goto _LL99;_LL98:{const char*_tmp2FF;Cyc_Absyndump_dump(((
_tmp2FF="short",_tag_dyneither(_tmp2FF,sizeof(char),6))));}return;_LL99: if(*((
int*)_tmp49)!= 5)goto _LL9B;_tmp6C=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp6C != 1)goto _LL9B;_tmp6D=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp6D != 1)goto _LL9B;_LL9A:{const char*_tmp300;Cyc_Absyndump_dump(((
_tmp300="unsigned short",_tag_dyneither(_tmp300,sizeof(char),15))));}return;
_LL9B: if(*((int*)_tmp49)!= 5)goto _LL9D;_tmp6E=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp6E != 1)goto _LL9D;_tmp6F=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp6F != 2)goto _LL9D;_LL9C:{const char*_tmp301;Cyc_Absyndump_dump(((
_tmp301="unsigned int",_tag_dyneither(_tmp301,sizeof(char),13))));}return;_LL9D:
if(*((int*)_tmp49)!= 5)goto _LL9F;_tmp70=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp70 != 1)goto _LL9F;_tmp71=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp71 != 3)goto _LL9F;_LL9E:{const char*_tmp302;Cyc_Absyndump_dump(((
_tmp302="unsigned long",_tag_dyneither(_tmp302,sizeof(char),14))));}return;_LL9F:
if(*((int*)_tmp49)!= 5)goto _LLA1;_tmp72=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f1;if((int)_tmp72 != 2)goto _LLA1;_tmp73=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp73 != 4)goto _LLA1;_LLA0: goto _LLA2;_LLA1: if(*((int*)_tmp49)
!= 5)goto _LLA3;_tmp74=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;if((
int)_tmp74 != 0)goto _LLA3;_tmp75=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f2;
if((int)_tmp75 != 4)goto _LLA3;_LLA2: {void*_tmpB3=Cyc_Cyclone_c_compiler;_LLD7:
if((int)_tmpB3 != 0)goto _LLD9;_LLD8:{const char*_tmp303;Cyc_Absyndump_dump(((
_tmp303="long long",_tag_dyneither(_tmp303,sizeof(char),10))));}return;_LLD9: if((
int)_tmpB3 != 1)goto _LLD6;_LLDA:{const char*_tmp304;Cyc_Absyndump_dump(((_tmp304="__int64",
_tag_dyneither(_tmp304,sizeof(char),8))));}return;_LLD6:;}_LLA3: if(*((int*)
_tmp49)!= 5)goto _LLA5;_tmp76=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp49)->f1;
if((int)_tmp76 != 1)goto _LLA5;_tmp77=(void*)((struct Cyc_Absyn_IntType_struct*)
_tmp49)->f2;if((int)_tmp77 != 4)goto _LLA5;_LLA4: {void*_tmpB6=Cyc_Cyclone_c_compiler;
_LLDC: if((int)_tmpB6 != 1)goto _LLDE;_LLDD:{const char*_tmp305;Cyc_Absyndump_dump(((
_tmp305="unsigned __int64",_tag_dyneither(_tmp305,sizeof(char),17))));}return;
_LLDE: if((int)_tmpB6 != 0)goto _LLDB;_LLDF:{const char*_tmp306;Cyc_Absyndump_dump(((
_tmp306="unsigned long long",_tag_dyneither(_tmp306,sizeof(char),19))));}return;
_LLDB:;}_LLA5: if((int)_tmp49 != 1)goto _LLA7;_LLA6:{const char*_tmp307;Cyc_Absyndump_dump(((
_tmp307="float",_tag_dyneither(_tmp307,sizeof(char),6))));}return;_LLA7: if(
_tmp49 <= (void*)4)goto _LLBD;if(*((int*)_tmp49)!= 6)goto _LLA9;_tmp78=((struct Cyc_Absyn_DoubleType_struct*)
_tmp49)->f1;_LLA8: if(_tmp78){const char*_tmp308;Cyc_Absyndump_dump(((_tmp308="long double",
_tag_dyneither(_tmp308,sizeof(char),12))));}else{const char*_tmp309;Cyc_Absyndump_dump(((
_tmp309="double",_tag_dyneither(_tmp309,sizeof(char),7))));}return;_LLA9: if(*((
int*)_tmp49)!= 9)goto _LLAB;_tmp79=((struct Cyc_Absyn_TupleType_struct*)_tmp49)->f1;
_LLAA: Cyc_Absyndump_dump_char((int)'$');Cyc_Absyndump_dumpargs(_tmp79);return;
_LLAB: if(*((int*)_tmp49)!= 10)goto _LLAD;_tmp7A=((struct Cyc_Absyn_AggrType_struct*)
_tmp49)->f1;_tmp7B=_tmp7A.aggr_info;_tmp7C=_tmp7A.targs;_LLAC: {void*_tmpBD;
struct _tuple0*_tmpBE;struct _tuple3 _tmpBC=Cyc_Absyn_aggr_kinded_name(_tmp7B);
_tmpBD=_tmpBC.f1;_tmpBE=_tmpBC.f2;Cyc_Absyndump_dumpaggr_kind(_tmpBD);Cyc_Absyndump_dumpqvar(
_tmpBE);Cyc_Absyndump_dumptps(_tmp7C);return;}_LLAD: if(*((int*)_tmp49)!= 11)goto
_LLAF;_tmp7D=(void*)((struct Cyc_Absyn_AnonAggrType_struct*)_tmp49)->f1;_tmp7E=((
struct Cyc_Absyn_AnonAggrType_struct*)_tmp49)->f2;_LLAE: Cyc_Absyndump_dumpaggr_kind(
_tmp7D);Cyc_Absyndump_dump_char((int)'{');Cyc_Absyndump_dumpaggrfields(_tmp7E);
Cyc_Absyndump_dump_char((int)'}');return;_LLAF: if(*((int*)_tmp49)!= 12)goto _LLB1;
_tmp7F=((struct Cyc_Absyn_EnumType_struct*)_tmp49)->f1;_LLB0:{const char*_tmp30A;
Cyc_Absyndump_dump(((_tmp30A="enum ",_tag_dyneither(_tmp30A,sizeof(char),6))));}
Cyc_Absyndump_dumpqvar(_tmp7F);return;_LLB1: if(*((int*)_tmp49)!= 13)goto _LLB3;
_tmp80=((struct Cyc_Absyn_AnonEnumType_struct*)_tmp49)->f1;_LLB2:{const char*
_tmp30B;Cyc_Absyndump_dump(((_tmp30B="enum {",_tag_dyneither(_tmp30B,sizeof(char),
7))));}Cyc_Absyndump_dumpenumfields(_tmp80);{const char*_tmp30C;Cyc_Absyndump_dump(((
_tmp30C="}",_tag_dyneither(_tmp30C,sizeof(char),2))));}return;_LLB3: if(*((int*)
_tmp49)!= 16)goto _LLB5;_tmp81=((struct Cyc_Absyn_TypedefType_struct*)_tmp49)->f1;
_tmp82=((struct Cyc_Absyn_TypedefType_struct*)_tmp49)->f2;_LLB4:(Cyc_Absyndump_dumpqvar(
_tmp81),Cyc_Absyndump_dumptps(_tmp82));return;_LLB5: if(*((int*)_tmp49)!= 17)goto
_LLB7;_tmp83=((struct Cyc_Absyn_ValueofType_struct*)_tmp49)->f1;_LLB6:{const char*
_tmp30D;Cyc_Absyndump_dump(((_tmp30D="valueof_t(",_tag_dyneither(_tmp30D,sizeof(
char),11))));}Cyc_Absyndump_dumpexp(_tmp83);{const char*_tmp30E;Cyc_Absyndump_dump(((
_tmp30E=")",_tag_dyneither(_tmp30E,sizeof(char),2))));}return;_LLB7: if(*((int*)
_tmp49)!= 14)goto _LLB9;_tmp84=(void*)((struct Cyc_Absyn_RgnHandleType_struct*)
_tmp49)->f1;_LLB8:{const char*_tmp30F;Cyc_Absyndump_dump(((_tmp30F="region_t<",
_tag_dyneither(_tmp30F,sizeof(char),10))));}Cyc_Absyndump_dumprgn(_tmp84);{const
char*_tmp310;Cyc_Absyndump_dump(((_tmp310=">",_tag_dyneither(_tmp310,sizeof(char),
2))));}return;_LLB9: if(*((int*)_tmp49)!= 15)goto _LLBB;_tmp85=(void*)((struct Cyc_Absyn_DynRgnType_struct*)
_tmp49)->f1;_tmp86=(void*)((struct Cyc_Absyn_DynRgnType_struct*)_tmp49)->f2;_LLBA:{
const char*_tmp311;Cyc_Absyndump_dump(((_tmp311="dynregion_t<",_tag_dyneither(
_tmp311,sizeof(char),13))));}Cyc_Absyndump_dumprgn(_tmp85);{const char*_tmp312;
Cyc_Absyndump_dump(((_tmp312=",",_tag_dyneither(_tmp312,sizeof(char),2))));}Cyc_Absyndump_dumprgn(
_tmp86);{const char*_tmp313;Cyc_Absyndump_dump(((_tmp313=">",_tag_dyneither(
_tmp313,sizeof(char),2))));}return;_LLBB: if(*((int*)_tmp49)!= 18)goto _LLBD;
_tmp87=(void*)((struct Cyc_Absyn_TagType_struct*)_tmp49)->f1;_LLBC:{const char*
_tmp314;Cyc_Absyndump_dump(((_tmp314="tag_t<",_tag_dyneither(_tmp314,sizeof(char),
7))));}Cyc_Absyndump_dumpntyp(_tmp87);{const char*_tmp315;Cyc_Absyndump_dump(((
_tmp315=">",_tag_dyneither(_tmp315,sizeof(char),2))));}return;_LLBD: if((int)
_tmp49 != 3)goto _LLBF;_LLBE:{const char*_tmp316;Cyc_Absyndump_dump(((_tmp316="`U",
_tag_dyneither(_tmp316,sizeof(char),3))));}goto _LL74;_LLBF: if((int)_tmp49 != 2)
goto _LLC1;_LLC0: goto _LLC2;_LLC1: if(_tmp49 <= (void*)4)goto _LLC3;if(*((int*)_tmp49)
!= 19)goto _LLC3;_LLC2: goto _LLC4;_LLC3: if(_tmp49 <= (void*)4)goto _LLC5;if(*((int*)
_tmp49)!= 21)goto _LLC5;_LLC4: goto _LLC6;_LLC5: if(_tmp49 <= (void*)4)goto _LL74;if(*((
int*)_tmp49)!= 20)goto _LL74;_LLC6: return;_LL74:;}void Cyc_Absyndump_dumpvaropt(
struct Cyc_Core_Opt*vo);void Cyc_Absyndump_dumpvaropt(struct Cyc_Core_Opt*vo){if(vo
!= 0)Cyc_Absyndump_dump_str((struct _dyneither_ptr*)vo->v);}void Cyc_Absyndump_dumpfunarg(
struct _tuple1*t);void Cyc_Absyndump_dumpfunarg(struct _tuple1*t){((void(*)(struct
Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Core_Opt*),struct Cyc_Core_Opt*))Cyc_Absyndump_dumptqtd)((*
t).f2,(*t).f3,Cyc_Absyndump_dumpvaropt,(*t).f1);}void Cyc_Absyndump_dump_rgncmp(
struct _tuple7*cmp);void Cyc_Absyndump_dump_rgncmp(struct _tuple7*cmp){struct
_tuple7 _tmpCD;void*_tmpCE;void*_tmpCF;struct _tuple7*_tmpCC=cmp;_tmpCD=*_tmpCC;
_tmpCE=_tmpCD.f1;_tmpCF=_tmpCD.f2;Cyc_Absyndump_dumpeff(_tmpCE);Cyc_Absyndump_dump_char((
int)'>');Cyc_Absyndump_dumprgn(_tmpCF);}void Cyc_Absyndump_dump_rgnpo(struct Cyc_List_List*
rgn_po);void Cyc_Absyndump_dump_rgnpo(struct Cyc_List_List*rgn_po){const char*
_tmp317;((void(*)(void(*f)(struct _tuple7*),struct Cyc_List_List*l,struct
_dyneither_ptr sep))Cyc_Absyndump_dump_sep)(Cyc_Absyndump_dump_rgncmp,rgn_po,((
_tmp317=",",_tag_dyneither(_tmp317,sizeof(char),2))));}void Cyc_Absyndump_dumpfunargs(
struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,
struct Cyc_Core_Opt*effopt,struct Cyc_List_List*rgn_po);void Cyc_Absyndump_dumpfunargs(
struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,
struct Cyc_Core_Opt*effopt,struct Cyc_List_List*rgn_po){Cyc_Absyndump_dump_char((
int)'(');for(0;args != 0;args=args->tl){Cyc_Absyndump_dumpfunarg((struct _tuple1*)
args->hd);if((args->tl != 0  || c_varargs) || cyc_varargs != 0)Cyc_Absyndump_dump_char((
int)',');}if(c_varargs){const char*_tmp318;Cyc_Absyndump_dump(((_tmp318="...",
_tag_dyneither(_tmp318,sizeof(char),4))));}else{if(cyc_varargs != 0){struct
_tuple1*_tmp319;struct _tuple1*_tmpD2=(_tmp319=_cycalloc(sizeof(*_tmp319)),((
_tmp319->f1=cyc_varargs->name,((_tmp319->f2=cyc_varargs->tq,((_tmp319->f3=(void*)
cyc_varargs->type,_tmp319)))))));{const char*_tmp31A;Cyc_Absyndump_dump(((_tmp31A="...",
_tag_dyneither(_tmp31A,sizeof(char),4))));}if(cyc_varargs->inject){const char*
_tmp31B;Cyc_Absyndump_dump(((_tmp31B=" inject ",_tag_dyneither(_tmp31B,sizeof(
char),9))));}Cyc_Absyndump_dumpfunarg(_tmpD2);}}if(effopt != 0){Cyc_Absyndump_dump_semi();
Cyc_Absyndump_dumpeff((void*)effopt->v);}if(rgn_po != 0){Cyc_Absyndump_dump_char((
int)':');Cyc_Absyndump_dump_rgnpo(rgn_po);}Cyc_Absyndump_dump_char((int)')');}
void Cyc_Absyndump_dumptyp(void*t);void Cyc_Absyndump_dumptyp(void*t){((void(*)(
struct Cyc_Absyn_Tqual,void*,void(*f)(int),int))Cyc_Absyndump_dumptqtd)(Cyc_Absyn_empty_tqual(
0),t,(void(*)(int x))Cyc_Absyndump_ignore,0);}void Cyc_Absyndump_dumpdesignator(
void*d);void Cyc_Absyndump_dumpdesignator(void*d){void*_tmpD6=d;struct Cyc_Absyn_Exp*
_tmpD7;struct _dyneither_ptr*_tmpD8;_LLE1: if(*((int*)_tmpD6)!= 0)goto _LLE3;_tmpD7=((
struct Cyc_Absyn_ArrayElement_struct*)_tmpD6)->f1;_LLE2:{const char*_tmp31C;Cyc_Absyndump_dump(((
_tmp31C=".[",_tag_dyneither(_tmp31C,sizeof(char),3))));}Cyc_Absyndump_dumpexp(
_tmpD7);Cyc_Absyndump_dump_char((int)']');goto _LLE0;_LLE3: if(*((int*)_tmpD6)!= 1)
goto _LLE0;_tmpD8=((struct Cyc_Absyn_FieldName_struct*)_tmpD6)->f1;_LLE4: Cyc_Absyndump_dump_char((
int)'.');Cyc_Absyndump_dump_nospace(*_tmpD8);goto _LLE0;_LLE0:;}struct _tuple10{
struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};void Cyc_Absyndump_dumpde(struct
_tuple10*de);void Cyc_Absyndump_dumpde(struct _tuple10*de){{const char*_tmp31F;
const char*_tmp31E;const char*_tmp31D;Cyc_Absyndump_egroup(Cyc_Absyndump_dumpdesignator,(*
de).f1,((_tmp31D="",_tag_dyneither(_tmp31D,sizeof(char),1))),((_tmp31E="=",
_tag_dyneither(_tmp31E,sizeof(char),2))),((_tmp31F="=",_tag_dyneither(_tmp31F,
sizeof(char),2))));}Cyc_Absyndump_dumpexp((*de).f2);}void Cyc_Absyndump_dumpexps_prec(
int inprec,struct Cyc_List_List*es);void Cyc_Absyndump_dumpexps_prec(int inprec,
struct Cyc_List_List*es){const char*_tmp322;const char*_tmp321;const char*_tmp320;((
void(*)(void(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*l,struct
_dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group_c)(
Cyc_Absyndump_dumpexp_prec,inprec,es,((_tmp320="",_tag_dyneither(_tmp320,sizeof(
char),1))),((_tmp321="",_tag_dyneither(_tmp321,sizeof(char),1))),((_tmp322=",",
_tag_dyneither(_tmp322,sizeof(char),2))));}void Cyc_Absyndump_dumpexp_prec(int
inprec,struct Cyc_Absyn_Exp*e);void Cyc_Absyndump_dumpexp_prec(int inprec,struct Cyc_Absyn_Exp*
e){int myprec=Cyc_Absynpp_exp_prec(e);if(inprec >= myprec){const char*_tmp323;Cyc_Absyndump_dump_nospace(((
_tmp323="(",_tag_dyneither(_tmp323,sizeof(char),2))));}{void*_tmpE1=(void*)e->r;
union Cyc_Absyn_Cnst_union _tmpE2;void*_tmpE3;char _tmpE4;union Cyc_Absyn_Cnst_union
_tmpE5;void*_tmpE6;short _tmpE7;union Cyc_Absyn_Cnst_union _tmpE8;void*_tmpE9;int
_tmpEA;union Cyc_Absyn_Cnst_union _tmpEB;void*_tmpEC;int _tmpED;union Cyc_Absyn_Cnst_union
_tmpEE;void*_tmpEF;int _tmpF0;union Cyc_Absyn_Cnst_union _tmpF1;void*_tmpF2;
long long _tmpF3;union Cyc_Absyn_Cnst_union _tmpF4;struct _dyneither_ptr _tmpF5;union
Cyc_Absyn_Cnst_union _tmpF6;union Cyc_Absyn_Cnst_union _tmpF7;struct _dyneither_ptr
_tmpF8;struct _tuple0*_tmpF9;struct _tuple0*_tmpFA;void*_tmpFB;struct Cyc_List_List*
_tmpFC;struct Cyc_Absyn_Exp*_tmpFD;struct Cyc_Core_Opt*_tmpFE;struct Cyc_Absyn_Exp*
_tmpFF;struct Cyc_Absyn_Exp*_tmp100;void*_tmp101;struct Cyc_Absyn_Exp*_tmp102;void*
_tmp103;struct Cyc_Absyn_Exp*_tmp104;void*_tmp105;struct Cyc_Absyn_Exp*_tmp106;
void*_tmp107;struct Cyc_Absyn_Exp*_tmp108;struct Cyc_Absyn_Exp*_tmp109;struct Cyc_Absyn_Exp*
_tmp10A;struct Cyc_Absyn_Exp*_tmp10B;struct Cyc_Absyn_Exp*_tmp10C;struct Cyc_Absyn_Exp*
_tmp10D;struct Cyc_Absyn_Exp*_tmp10E;struct Cyc_Absyn_Exp*_tmp10F;struct Cyc_Absyn_Exp*
_tmp110;struct Cyc_Absyn_Exp*_tmp111;struct Cyc_List_List*_tmp112;struct Cyc_Absyn_Exp*
_tmp113;struct Cyc_List_List*_tmp114;struct Cyc_Absyn_Exp*_tmp115;struct Cyc_Absyn_Exp*
_tmp116;struct Cyc_Absyn_Exp*_tmp117;void*_tmp118;struct Cyc_Absyn_Exp*_tmp119;
struct Cyc_Absyn_Exp*_tmp11A;struct Cyc_Absyn_Exp*_tmp11B;struct Cyc_Absyn_Exp*
_tmp11C;void*_tmp11D;struct Cyc_Absyn_Exp*_tmp11E;void*_tmp11F;void*_tmp120;void*
_tmp121;struct _dyneither_ptr*_tmp122;void*_tmp123;void*_tmp124;unsigned int
_tmp125;struct Cyc_List_List*_tmp126;void*_tmp127;struct Cyc_Absyn_Exp*_tmp128;
struct Cyc_Absyn_Exp*_tmp129;struct _dyneither_ptr*_tmp12A;struct Cyc_Absyn_Exp*
_tmp12B;struct _dyneither_ptr*_tmp12C;struct Cyc_Absyn_Exp*_tmp12D;struct Cyc_Absyn_Exp*
_tmp12E;struct Cyc_List_List*_tmp12F;struct _tuple1*_tmp130;struct Cyc_List_List*
_tmp131;struct Cyc_List_List*_tmp132;struct Cyc_Absyn_Vardecl*_tmp133;struct Cyc_Absyn_Exp*
_tmp134;struct Cyc_Absyn_Exp*_tmp135;struct _tuple0*_tmp136;struct Cyc_List_List*
_tmp137;struct Cyc_List_List*_tmp138;struct Cyc_List_List*_tmp139;struct Cyc_List_List*
_tmp13A;struct Cyc_Absyn_Tunionfield*_tmp13B;struct _tuple0*_tmp13C;struct _tuple0*
_tmp13D;struct Cyc_Absyn_MallocInfo _tmp13E;int _tmp13F;struct Cyc_Absyn_Exp*_tmp140;
void**_tmp141;struct Cyc_Absyn_Exp*_tmp142;struct Cyc_Absyn_Exp*_tmp143;struct Cyc_Absyn_Exp*
_tmp144;struct Cyc_Core_Opt*_tmp145;struct Cyc_List_List*_tmp146;struct Cyc_Absyn_Stmt*
_tmp147;_LLE6: if(*((int*)_tmpE1)!= 0)goto _LLE8;_tmpE2=((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1).Char_c).tag != 0)
goto _LLE8;_tmpE3=(_tmpE2.Char_c).f1;_tmpE4=(_tmpE2.Char_c).f2;_LLE7: Cyc_Absyndump_dump_char((
int)'\'');Cyc_Absyndump_dump_nospace(Cyc_Absynpp_char_escape(_tmpE4));Cyc_Absyndump_dump_char((
int)'\'');goto _LLE5;_LLE8: if(*((int*)_tmpE1)!= 0)goto _LLEA;_tmpE5=((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1).Short_c).tag != 1)
goto _LLEA;_tmpE6=(_tmpE5.Short_c).f1;_tmpE7=(_tmpE5.Short_c).f2;_LLE9:{const char*
_tmp327;void*_tmp326[1];struct Cyc_Int_pa_struct _tmp325;Cyc_Absyndump_dump((
struct _dyneither_ptr)((_tmp325.tag=1,((_tmp325.f1=(unsigned long)((int)_tmpE7),((
_tmp326[0]=& _tmp325,Cyc_aprintf(((_tmp327="%d",_tag_dyneither(_tmp327,sizeof(
char),3))),_tag_dyneither(_tmp326,sizeof(void*),1)))))))));}goto _LLE5;_LLEA: if(*((
int*)_tmpE1)!= 0)goto _LLEC;_tmpE8=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;
if(((((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1).Int_c).tag != 2)goto _LLEC;
_tmpE9=(_tmpE8.Int_c).f1;if((int)_tmpE9 != 2)goto _LLEC;_tmpEA=(_tmpE8.Int_c).f2;
_LLEB: _tmpED=_tmpEA;goto _LLED;_LLEC: if(*((int*)_tmpE1)!= 0)goto _LLEE;_tmpEB=((
struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1).Int_c).tag != 2)goto _LLEE;_tmpEC=(_tmpEB.Int_c).f1;if((int)_tmpEC != 
0)goto _LLEE;_tmpED=(_tmpEB.Int_c).f2;_LLED:{const char*_tmp32B;void*_tmp32A[1];
struct Cyc_Int_pa_struct _tmp329;Cyc_Absyndump_dump((struct _dyneither_ptr)((
_tmp329.tag=1,((_tmp329.f1=(unsigned long)_tmpED,((_tmp32A[0]=& _tmp329,Cyc_aprintf(((
_tmp32B="%d",_tag_dyneither(_tmp32B,sizeof(char),3))),_tag_dyneither(_tmp32A,
sizeof(void*),1)))))))));}goto _LLE5;_LLEE: if(*((int*)_tmpE1)!= 0)goto _LLF0;
_tmpEE=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1).Int_c).tag != 2)goto _LLF0;_tmpEF=(_tmpEE.Int_c).f1;if((int)_tmpEF != 
1)goto _LLF0;_tmpF0=(_tmpEE.Int_c).f2;_LLEF:{const char*_tmp32F;void*_tmp32E[1];
struct Cyc_Int_pa_struct _tmp32D;Cyc_Absyndump_dump((struct _dyneither_ptr)((
_tmp32D.tag=1,((_tmp32D.f1=(unsigned int)_tmpF0,((_tmp32E[0]=& _tmp32D,Cyc_aprintf(((
_tmp32F="%u",_tag_dyneither(_tmp32F,sizeof(char),3))),_tag_dyneither(_tmp32E,
sizeof(void*),1)))))))));}goto _LLE5;_LLF0: if(*((int*)_tmpE1)!= 0)goto _LLF2;
_tmpF1=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1).LongLong_c).tag != 3)goto _LLF2;_tmpF2=(_tmpF1.LongLong_c).f1;_tmpF3=(
_tmpF1.LongLong_c).f2;_LLF1:{const char*_tmp330;Cyc_Absyndump_dump(((_tmp330="<<FIX LONG LONG CONSTANT>>",
_tag_dyneither(_tmp330,sizeof(char),27))));}goto _LLE5;_LLF2: if(*((int*)_tmpE1)!= 
0)goto _LLF4;_tmpF4=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;if(((((struct
Cyc_Absyn_Const_e_struct*)_tmpE1)->f1).Float_c).tag != 4)goto _LLF4;_tmpF5=(_tmpF4.Float_c).f1;
_LLF3: Cyc_Absyndump_dump(_tmpF5);goto _LLE5;_LLF4: if(*((int*)_tmpE1)!= 0)goto
_LLF6;_tmpF6=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;if(((((struct Cyc_Absyn_Const_e_struct*)
_tmpE1)->f1).Null_c).tag != 6)goto _LLF6;_LLF5:{const char*_tmp331;Cyc_Absyndump_dump(((
_tmp331="NULL",_tag_dyneither(_tmp331,sizeof(char),5))));}goto _LLE5;_LLF6: if(*((
int*)_tmpE1)!= 0)goto _LLF8;_tmpF7=((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1;
if(((((struct Cyc_Absyn_Const_e_struct*)_tmpE1)->f1).String_c).tag != 5)goto _LLF8;
_tmpF8=(_tmpF7.String_c).f1;_LLF7: Cyc_Absyndump_dump_char((int)'"');Cyc_Absyndump_dump_nospace(
Cyc_Absynpp_string_escape(_tmpF8));Cyc_Absyndump_dump_char((int)'"');goto _LLE5;
_LLF8: if(*((int*)_tmpE1)!= 2)goto _LLFA;_tmpF9=((struct Cyc_Absyn_UnknownId_e_struct*)
_tmpE1)->f1;_LLF9: _tmpFA=_tmpF9;goto _LLFB;_LLFA: if(*((int*)_tmpE1)!= 1)goto _LLFC;
_tmpFA=((struct Cyc_Absyn_Var_e_struct*)_tmpE1)->f1;_LLFB: Cyc_Absyndump_dumpqvar(
_tmpFA);goto _LLE5;_LLFC: if(*((int*)_tmpE1)!= 3)goto _LLFE;_tmpFB=(void*)((struct
Cyc_Absyn_Primop_e_struct*)_tmpE1)->f1;_tmpFC=((struct Cyc_Absyn_Primop_e_struct*)
_tmpE1)->f2;_LLFD: {struct _dyneither_ptr _tmp153=Cyc_Absynpp_prim2str(_tmpFB);
switch(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmpFC)){case 1: _LL14E:
if(_tmpFB == (void*)((void*)19)){{const char*_tmp332;Cyc_Absyndump_dump(((_tmp332="numelts(",
_tag_dyneither(_tmp332,sizeof(char),9))));}Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)((
struct Cyc_List_List*)_check_null(_tmpFC))->hd);{const char*_tmp333;Cyc_Absyndump_dump(((
_tmp333=")",_tag_dyneither(_tmp333,sizeof(char),2))));}}else{Cyc_Absyndump_dump(
_tmp153);Cyc_Absyndump_dumpexp_prec(myprec,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)
_check_null(_tmpFC))->hd);}break;case 2: _LL14F: Cyc_Absyndump_dumpexp_prec(myprec,(
struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmpFC))->hd);Cyc_Absyndump_dump(
_tmp153);Cyc_Absyndump_dump_char((int)' ');Cyc_Absyndump_dumpexp_prec(myprec,(
struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmpFC->tl))->hd);break;
default: _LL150: {struct Cyc_Core_Failure_struct _tmp339;const char*_tmp338;struct
Cyc_Core_Failure_struct*_tmp337;(int)_throw((void*)((_tmp337=_cycalloc(sizeof(*
_tmp337)),((_tmp337[0]=((_tmp339.tag=Cyc_Core_Failure,((_tmp339.f1=((_tmp338="Absyndump -- Bad number of arguments to primop",
_tag_dyneither(_tmp338,sizeof(char),47))),_tmp339)))),_tmp337)))));}}goto _LLE5;}
_LLFE: if(*((int*)_tmpE1)!= 4)goto _LL100;_tmpFD=((struct Cyc_Absyn_AssignOp_e_struct*)
_tmpE1)->f1;_tmpFE=((struct Cyc_Absyn_AssignOp_e_struct*)_tmpE1)->f2;_tmpFF=((
struct Cyc_Absyn_AssignOp_e_struct*)_tmpE1)->f3;_LLFF: Cyc_Absyndump_dumpexp_prec(
myprec,_tmpFD);if(_tmpFE != 0)Cyc_Absyndump_dump(Cyc_Absynpp_prim2str((void*)
_tmpFE->v));{const char*_tmp33A;Cyc_Absyndump_dump_nospace(((_tmp33A="=",
_tag_dyneither(_tmp33A,sizeof(char),2))));}Cyc_Absyndump_dumpexp_prec(myprec,
_tmpFF);goto _LLE5;_LL100: if(*((int*)_tmpE1)!= 5)goto _LL102;_tmp100=((struct Cyc_Absyn_Increment_e_struct*)
_tmpE1)->f1;_tmp101=(void*)((struct Cyc_Absyn_Increment_e_struct*)_tmpE1)->f2;if((
int)_tmp101 != 0)goto _LL102;_LL101:{const char*_tmp33B;Cyc_Absyndump_dump(((
_tmp33B="++",_tag_dyneither(_tmp33B,sizeof(char),3))));}Cyc_Absyndump_dumpexp_prec(
myprec,_tmp100);goto _LLE5;_LL102: if(*((int*)_tmpE1)!= 5)goto _LL104;_tmp102=((
struct Cyc_Absyn_Increment_e_struct*)_tmpE1)->f1;_tmp103=(void*)((struct Cyc_Absyn_Increment_e_struct*)
_tmpE1)->f2;if((int)_tmp103 != 2)goto _LL104;_LL103:{const char*_tmp33C;Cyc_Absyndump_dump(((
_tmp33C="--",_tag_dyneither(_tmp33C,sizeof(char),3))));}Cyc_Absyndump_dumpexp_prec(
myprec,_tmp102);goto _LLE5;_LL104: if(*((int*)_tmpE1)!= 5)goto _LL106;_tmp104=((
struct Cyc_Absyn_Increment_e_struct*)_tmpE1)->f1;_tmp105=(void*)((struct Cyc_Absyn_Increment_e_struct*)
_tmpE1)->f2;if((int)_tmp105 != 1)goto _LL106;_LL105: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp104);{const char*_tmp33D;Cyc_Absyndump_dump(((_tmp33D="++",
_tag_dyneither(_tmp33D,sizeof(char),3))));}goto _LLE5;_LL106: if(*((int*)_tmpE1)!= 
5)goto _LL108;_tmp106=((struct Cyc_Absyn_Increment_e_struct*)_tmpE1)->f1;_tmp107=(
void*)((struct Cyc_Absyn_Increment_e_struct*)_tmpE1)->f2;if((int)_tmp107 != 3)goto
_LL108;_LL107: Cyc_Absyndump_dumpexp_prec(myprec,_tmp106);{const char*_tmp33E;Cyc_Absyndump_dump(((
_tmp33E="--",_tag_dyneither(_tmp33E,sizeof(char),3))));}goto _LLE5;_LL108: if(*((
int*)_tmpE1)!= 6)goto _LL10A;_tmp108=((struct Cyc_Absyn_Conditional_e_struct*)
_tmpE1)->f1;_tmp109=((struct Cyc_Absyn_Conditional_e_struct*)_tmpE1)->f2;_tmp10A=((
struct Cyc_Absyn_Conditional_e_struct*)_tmpE1)->f3;_LL109: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp108);Cyc_Absyndump_dump_char((int)'?');Cyc_Absyndump_dumpexp_prec(0,
_tmp109);Cyc_Absyndump_dump_char((int)':');Cyc_Absyndump_dumpexp_prec(myprec,
_tmp10A);goto _LLE5;_LL10A: if(*((int*)_tmpE1)!= 7)goto _LL10C;_tmp10B=((struct Cyc_Absyn_And_e_struct*)
_tmpE1)->f1;_tmp10C=((struct Cyc_Absyn_And_e_struct*)_tmpE1)->f2;_LL10B: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp10B);{const char*_tmp33F;Cyc_Absyndump_dump(((_tmp33F=" && ",
_tag_dyneither(_tmp33F,sizeof(char),5))));}Cyc_Absyndump_dumpexp_prec(myprec,
_tmp10C);goto _LLE5;_LL10C: if(*((int*)_tmpE1)!= 8)goto _LL10E;_tmp10D=((struct Cyc_Absyn_Or_e_struct*)
_tmpE1)->f1;_tmp10E=((struct Cyc_Absyn_Or_e_struct*)_tmpE1)->f2;_LL10D: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp10D);{const char*_tmp340;Cyc_Absyndump_dump(((_tmp340=" || ",
_tag_dyneither(_tmp340,sizeof(char),5))));}Cyc_Absyndump_dumpexp_prec(myprec,
_tmp10E);goto _LLE5;_LL10E: if(*((int*)_tmpE1)!= 9)goto _LL110;_tmp10F=((struct Cyc_Absyn_SeqExp_e_struct*)
_tmpE1)->f1;_tmp110=((struct Cyc_Absyn_SeqExp_e_struct*)_tmpE1)->f2;_LL10F: Cyc_Absyndump_dump_char((
int)'(');Cyc_Absyndump_dumpexp_prec(myprec,_tmp10F);Cyc_Absyndump_dump_char((int)',');
Cyc_Absyndump_dumpexp_prec(myprec,_tmp110);Cyc_Absyndump_dump_char((int)')');
goto _LLE5;_LL110: if(*((int*)_tmpE1)!= 10)goto _LL112;_tmp111=((struct Cyc_Absyn_UnknownCall_e_struct*)
_tmpE1)->f1;_tmp112=((struct Cyc_Absyn_UnknownCall_e_struct*)_tmpE1)->f2;_LL111:
_tmp113=_tmp111;_tmp114=_tmp112;goto _LL113;_LL112: if(*((int*)_tmpE1)!= 11)goto
_LL114;_tmp113=((struct Cyc_Absyn_FnCall_e_struct*)_tmpE1)->f1;_tmp114=((struct
Cyc_Absyn_FnCall_e_struct*)_tmpE1)->f2;_LL113: Cyc_Absyndump_dumpexp_prec(myprec,
_tmp113);{const char*_tmp341;Cyc_Absyndump_dump_nospace(((_tmp341="(",
_tag_dyneither(_tmp341,sizeof(char),2))));}Cyc_Absyndump_dumpexps_prec(20,
_tmp114);{const char*_tmp342;Cyc_Absyndump_dump_nospace(((_tmp342=")",
_tag_dyneither(_tmp342,sizeof(char),2))));}goto _LLE5;_LL114: if(*((int*)_tmpE1)!= 
12)goto _LL116;_tmp115=((struct Cyc_Absyn_Throw_e_struct*)_tmpE1)->f1;_LL115:{
const char*_tmp343;Cyc_Absyndump_dump(((_tmp343="throw",_tag_dyneither(_tmp343,
sizeof(char),6))));}Cyc_Absyndump_dumpexp_prec(myprec,_tmp115);goto _LLE5;_LL116:
if(*((int*)_tmpE1)!= 13)goto _LL118;_tmp116=((struct Cyc_Absyn_NoInstantiate_e_struct*)
_tmpE1)->f1;_LL117: _tmp117=_tmp116;goto _LL119;_LL118: if(*((int*)_tmpE1)!= 14)
goto _LL11A;_tmp117=((struct Cyc_Absyn_Instantiate_e_struct*)_tmpE1)->f1;_LL119:
Cyc_Absyndump_dumpexp_prec(inprec,_tmp117);goto _LLE5;_LL11A: if(*((int*)_tmpE1)!= 
15)goto _LL11C;_tmp118=(void*)((struct Cyc_Absyn_Cast_e_struct*)_tmpE1)->f1;
_tmp119=((struct Cyc_Absyn_Cast_e_struct*)_tmpE1)->f2;_LL11B: Cyc_Absyndump_dump_char((
int)'(');Cyc_Absyndump_dumptyp(_tmp118);Cyc_Absyndump_dump_char((int)')');Cyc_Absyndump_dumpexp_prec(
myprec,_tmp119);goto _LLE5;_LL11C: if(*((int*)_tmpE1)!= 16)goto _LL11E;_tmp11A=((
struct Cyc_Absyn_Address_e_struct*)_tmpE1)->f1;_LL11D: Cyc_Absyndump_dump_char((
int)'&');Cyc_Absyndump_dumpexp_prec(myprec,_tmp11A);goto _LLE5;_LL11E: if(*((int*)
_tmpE1)!= 17)goto _LL120;_tmp11B=((struct Cyc_Absyn_New_e_struct*)_tmpE1)->f1;
_tmp11C=((struct Cyc_Absyn_New_e_struct*)_tmpE1)->f2;_LL11F:{const char*_tmp344;
Cyc_Absyndump_dump(((_tmp344="new ",_tag_dyneither(_tmp344,sizeof(char),5))));}
Cyc_Absyndump_dumpexp_prec(myprec,_tmp11C);goto _LLE5;_LL120: if(*((int*)_tmpE1)!= 
18)goto _LL122;_tmp11D=(void*)((struct Cyc_Absyn_Sizeoftyp_e_struct*)_tmpE1)->f1;
_LL121:{const char*_tmp345;Cyc_Absyndump_dump(((_tmp345="sizeof(",_tag_dyneither(
_tmp345,sizeof(char),8))));}Cyc_Absyndump_dumptyp(_tmp11D);Cyc_Absyndump_dump_char((
int)')');goto _LLE5;_LL122: if(*((int*)_tmpE1)!= 19)goto _LL124;_tmp11E=((struct Cyc_Absyn_Sizeofexp_e_struct*)
_tmpE1)->f1;_LL123:{const char*_tmp346;Cyc_Absyndump_dump(((_tmp346="sizeof(",
_tag_dyneither(_tmp346,sizeof(char),8))));}Cyc_Absyndump_dumpexp(_tmp11E);Cyc_Absyndump_dump_char((
int)')');goto _LLE5;_LL124: if(*((int*)_tmpE1)!= 39)goto _LL126;_tmp11F=(void*)((
struct Cyc_Absyn_Valueof_e_struct*)_tmpE1)->f1;_LL125:{const char*_tmp347;Cyc_Absyndump_dump(((
_tmp347="valueof(",_tag_dyneither(_tmp347,sizeof(char),9))));}Cyc_Absyndump_dumptyp(
_tmp11F);Cyc_Absyndump_dump_char((int)')');goto _LLE5;_LL126: if(*((int*)_tmpE1)!= 
20)goto _LL128;_tmp120=(void*)((struct Cyc_Absyn_Offsetof_e_struct*)_tmpE1)->f1;
_tmp121=(void*)((struct Cyc_Absyn_Offsetof_e_struct*)_tmpE1)->f2;if(*((int*)
_tmp121)!= 0)goto _LL128;_tmp122=((struct Cyc_Absyn_StructField_struct*)_tmp121)->f1;
_LL127:{const char*_tmp348;Cyc_Absyndump_dump(((_tmp348="offsetof(",
_tag_dyneither(_tmp348,sizeof(char),10))));}Cyc_Absyndump_dumptyp(_tmp120);Cyc_Absyndump_dump_char((
int)',');Cyc_Absyndump_dump_nospace(*_tmp122);Cyc_Absyndump_dump_char((int)')');
goto _LLE5;_LL128: if(*((int*)_tmpE1)!= 20)goto _LL12A;_tmp123=(void*)((struct Cyc_Absyn_Offsetof_e_struct*)
_tmpE1)->f1;_tmp124=(void*)((struct Cyc_Absyn_Offsetof_e_struct*)_tmpE1)->f2;if(*((
int*)_tmp124)!= 1)goto _LL12A;_tmp125=((struct Cyc_Absyn_TupleIndex_struct*)
_tmp124)->f1;_LL129:{const char*_tmp349;Cyc_Absyndump_dump(((_tmp349="offsetof(",
_tag_dyneither(_tmp349,sizeof(char),10))));}Cyc_Absyndump_dumptyp(_tmp123);Cyc_Absyndump_dump_char((
int)',');{const char*_tmp34D;void*_tmp34C[1];struct Cyc_Int_pa_struct _tmp34B;Cyc_Absyndump_dump((
struct _dyneither_ptr)((_tmp34B.tag=1,((_tmp34B.f1=(unsigned long)((int)_tmp125),((
_tmp34C[0]=& _tmp34B,Cyc_aprintf(((_tmp34D="%d",_tag_dyneither(_tmp34D,sizeof(
char),3))),_tag_dyneither(_tmp34C,sizeof(void*),1)))))))));}Cyc_Absyndump_dump_char((
int)')');goto _LLE5;_LL12A: if(*((int*)_tmpE1)!= 21)goto _LL12C;_tmp126=((struct Cyc_Absyn_Gentyp_e_struct*)
_tmpE1)->f1;_tmp127=(void*)((struct Cyc_Absyn_Gentyp_e_struct*)_tmpE1)->f2;_LL12B:{
const char*_tmp34E;Cyc_Absyndump_dump(((_tmp34E="__gen(",_tag_dyneither(_tmp34E,
sizeof(char),7))));}Cyc_Absyndump_dumptvars(_tmp126);Cyc_Absyndump_dumptyp(
_tmp127);Cyc_Absyndump_dump_char((int)')');goto _LLE5;_LL12C: if(*((int*)_tmpE1)!= 
22)goto _LL12E;_tmp128=((struct Cyc_Absyn_Deref_e_struct*)_tmpE1)->f1;_LL12D: Cyc_Absyndump_dump_char((
int)'*');Cyc_Absyndump_dumpexp_prec(myprec,_tmp128);goto _LLE5;_LL12E: if(*((int*)
_tmpE1)!= 23)goto _LL130;_tmp129=((struct Cyc_Absyn_AggrMember_e_struct*)_tmpE1)->f1;
_tmp12A=((struct Cyc_Absyn_AggrMember_e_struct*)_tmpE1)->f2;_LL12F: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp129);Cyc_Absyndump_dump_char((int)'.');Cyc_Absyndump_dump_nospace(*
_tmp12A);goto _LLE5;_LL130: if(*((int*)_tmpE1)!= 24)goto _LL132;_tmp12B=((struct Cyc_Absyn_AggrArrow_e_struct*)
_tmpE1)->f1;_tmp12C=((struct Cyc_Absyn_AggrArrow_e_struct*)_tmpE1)->f2;_LL131: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp12B);{const char*_tmp34F;Cyc_Absyndump_dump_nospace(((_tmp34F="->",
_tag_dyneither(_tmp34F,sizeof(char),3))));}Cyc_Absyndump_dump_nospace(*_tmp12C);
goto _LLE5;_LL132: if(*((int*)_tmpE1)!= 25)goto _LL134;_tmp12D=((struct Cyc_Absyn_Subscript_e_struct*)
_tmpE1)->f1;_tmp12E=((struct Cyc_Absyn_Subscript_e_struct*)_tmpE1)->f2;_LL133: Cyc_Absyndump_dumpexp_prec(
myprec,_tmp12D);Cyc_Absyndump_dump_char((int)'[');Cyc_Absyndump_dumpexp(_tmp12E);
Cyc_Absyndump_dump_char((int)']');goto _LLE5;_LL134: if(*((int*)_tmpE1)!= 26)goto
_LL136;_tmp12F=((struct Cyc_Absyn_Tuple_e_struct*)_tmpE1)->f1;_LL135:{const char*
_tmp350;Cyc_Absyndump_dump(((_tmp350="$(",_tag_dyneither(_tmp350,sizeof(char),3))));}
Cyc_Absyndump_dumpexps_prec(20,_tmp12F);Cyc_Absyndump_dump_char((int)')');goto
_LLE5;_LL136: if(*((int*)_tmpE1)!= 27)goto _LL138;_tmp130=((struct Cyc_Absyn_CompoundLit_e_struct*)
_tmpE1)->f1;_tmp131=((struct Cyc_Absyn_CompoundLit_e_struct*)_tmpE1)->f2;_LL137:
Cyc_Absyndump_dump_char((int)'(');Cyc_Absyndump_dumptyp((*_tmp130).f3);Cyc_Absyndump_dump_char((
int)')');{const char*_tmp353;const char*_tmp352;const char*_tmp351;((void(*)(void(*
f)(struct _tuple10*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dumpde,
_tmp131,((_tmp351="{",_tag_dyneither(_tmp351,sizeof(char),2))),((_tmp352="}",
_tag_dyneither(_tmp352,sizeof(char),2))),((_tmp353=",",_tag_dyneither(_tmp353,
sizeof(char),2))));}goto _LLE5;_LL138: if(*((int*)_tmpE1)!= 28)goto _LL13A;_tmp132=((
struct Cyc_Absyn_Array_e_struct*)_tmpE1)->f1;_LL139:{const char*_tmp356;const char*
_tmp355;const char*_tmp354;((void(*)(void(*f)(struct _tuple10*),struct Cyc_List_List*
l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumpde,_tmp132,((_tmp354="{",_tag_dyneither(_tmp354,sizeof(char),2))),((
_tmp355="}",_tag_dyneither(_tmp355,sizeof(char),2))),((_tmp356=",",
_tag_dyneither(_tmp356,sizeof(char),2))));}goto _LLE5;_LL13A: if(*((int*)_tmpE1)!= 
29)goto _LL13C;_tmp133=((struct Cyc_Absyn_Comprehension_e_struct*)_tmpE1)->f1;
_tmp134=((struct Cyc_Absyn_Comprehension_e_struct*)_tmpE1)->f2;_tmp135=((struct
Cyc_Absyn_Comprehension_e_struct*)_tmpE1)->f3;_LL13B:{const char*_tmp357;Cyc_Absyndump_dump(((
_tmp357="new {for",_tag_dyneither(_tmp357,sizeof(char),9))));}Cyc_Absyndump_dump_str((*
_tmp133->name).f2);Cyc_Absyndump_dump_char((int)'<');Cyc_Absyndump_dumpexp(
_tmp134);Cyc_Absyndump_dump_char((int)':');Cyc_Absyndump_dumpexp(_tmp135);Cyc_Absyndump_dump_char((
int)'}');goto _LLE5;_LL13C: if(*((int*)_tmpE1)!= 30)goto _LL13E;_tmp136=((struct Cyc_Absyn_Struct_e_struct*)
_tmpE1)->f1;_tmp137=((struct Cyc_Absyn_Struct_e_struct*)_tmpE1)->f2;_tmp138=((
struct Cyc_Absyn_Struct_e_struct*)_tmpE1)->f3;_LL13D: Cyc_Absyndump_dumpqvar(
_tmp136);Cyc_Absyndump_dump_char((int)'{');if(_tmp137 != 0)Cyc_Absyndump_dumptps(
_tmp137);{const char*_tmp35A;const char*_tmp359;const char*_tmp358;((void(*)(void(*
f)(struct _tuple10*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dumpde,
_tmp138,((_tmp358="",_tag_dyneither(_tmp358,sizeof(char),1))),((_tmp359="}",
_tag_dyneither(_tmp359,sizeof(char),2))),((_tmp35A=",",_tag_dyneither(_tmp35A,
sizeof(char),2))));}goto _LLE5;_LL13E: if(*((int*)_tmpE1)!= 31)goto _LL140;_tmp139=((
struct Cyc_Absyn_AnonStruct_e_struct*)_tmpE1)->f2;_LL13F:{const char*_tmp35D;const
char*_tmp35C;const char*_tmp35B;((void(*)(void(*f)(struct _tuple10*),struct Cyc_List_List*
l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumpde,_tmp139,((_tmp35B="{",_tag_dyneither(_tmp35B,sizeof(char),2))),((
_tmp35C="}",_tag_dyneither(_tmp35C,sizeof(char),2))),((_tmp35D=",",
_tag_dyneither(_tmp35D,sizeof(char),2))));}goto _LLE5;_LL140: if(*((int*)_tmpE1)!= 
32)goto _LL142;_tmp13A=((struct Cyc_Absyn_Tunion_e_struct*)_tmpE1)->f1;_tmp13B=((
struct Cyc_Absyn_Tunion_e_struct*)_tmpE1)->f3;_LL141: Cyc_Absyndump_dumpqvar(
_tmp13B->name);if(_tmp13A != 0){const char*_tmp360;const char*_tmp35F;const char*
_tmp35E;((void(*)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*l,struct
_dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumpexp,_tmp13A,((_tmp35E="(",_tag_dyneither(_tmp35E,sizeof(char),
2))),((_tmp35F=")",_tag_dyneither(_tmp35F,sizeof(char),2))),((_tmp360=",",
_tag_dyneither(_tmp360,sizeof(char),2))));}goto _LLE5;_LL142: if(*((int*)_tmpE1)!= 
33)goto _LL144;_tmp13C=((struct Cyc_Absyn_Enum_e_struct*)_tmpE1)->f1;_LL143: Cyc_Absyndump_dumpqvar(
_tmp13C);goto _LLE5;_LL144: if(*((int*)_tmpE1)!= 34)goto _LL146;_tmp13D=((struct Cyc_Absyn_AnonEnum_e_struct*)
_tmpE1)->f1;_LL145: Cyc_Absyndump_dumpqvar(_tmp13D);goto _LLE5;_LL146: if(*((int*)
_tmpE1)!= 35)goto _LL148;_tmp13E=((struct Cyc_Absyn_Malloc_e_struct*)_tmpE1)->f1;
_tmp13F=_tmp13E.is_calloc;_tmp140=_tmp13E.rgn;_tmp141=_tmp13E.elt_type;_tmp142=
_tmp13E.num_elts;_LL147: if(_tmp13F){if(_tmp140 != 0){{const char*_tmp361;Cyc_Absyndump_dump(((
_tmp361="rcalloc(",_tag_dyneither(_tmp361,sizeof(char),9))));}Cyc_Absyndump_dumpexp((
struct Cyc_Absyn_Exp*)_tmp140);{const char*_tmp362;Cyc_Absyndump_dump(((_tmp362=",",
_tag_dyneither(_tmp362,sizeof(char),2))));}}else{const char*_tmp363;Cyc_Absyndump_dump(((
_tmp363="calloc",_tag_dyneither(_tmp363,sizeof(char),7))));}Cyc_Absyndump_dumpexp(
_tmp142);{const char*_tmp364;Cyc_Absyndump_dump(((_tmp364=",",_tag_dyneither(
_tmp364,sizeof(char),2))));}Cyc_Absyndump_dumpexp(Cyc_Absyn_sizeoftyp_exp(*((
void**)_check_null(_tmp141)),0));{const char*_tmp365;Cyc_Absyndump_dump(((_tmp365=")",
_tag_dyneither(_tmp365,sizeof(char),2))));}}else{if(_tmp140 != 0){{const char*
_tmp366;Cyc_Absyndump_dump(((_tmp366="rmalloc(",_tag_dyneither(_tmp366,sizeof(
char),9))));}Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)_tmp140);{const char*
_tmp367;Cyc_Absyndump_dump(((_tmp367=",",_tag_dyneither(_tmp367,sizeof(char),2))));}}
else{const char*_tmp368;Cyc_Absyndump_dump(((_tmp368="malloc(",_tag_dyneither(
_tmp368,sizeof(char),8))));}if(_tmp141 != 0)Cyc_Absyndump_dumpexp(Cyc_Absyn_times_exp(
Cyc_Absyn_sizeoftyp_exp(*_tmp141,0),_tmp142,0));else{Cyc_Absyndump_dumpexp(
_tmp142);}{const char*_tmp369;Cyc_Absyndump_dump(((_tmp369=")",_tag_dyneither(
_tmp369,sizeof(char),2))));}}goto _LLE5;_LL148: if(*((int*)_tmpE1)!= 36)goto _LL14A;
_tmp143=((struct Cyc_Absyn_Swap_e_struct*)_tmpE1)->f1;_tmp144=((struct Cyc_Absyn_Swap_e_struct*)
_tmpE1)->f2;_LL149:{const char*_tmp36A;Cyc_Absyndump_dump(((_tmp36A="cycswap(",
_tag_dyneither(_tmp36A,sizeof(char),9))));}Cyc_Absyndump_dumpexp_prec(myprec,
_tmp143);Cyc_Absyndump_dump_char((int)',');Cyc_Absyndump_dumpexp_prec(myprec,
_tmp144);Cyc_Absyndump_dump_char((int)')');goto _LLE5;_LL14A: if(*((int*)_tmpE1)!= 
37)goto _LL14C;_tmp145=((struct Cyc_Absyn_UnresolvedMem_e_struct*)_tmpE1)->f1;
_tmp146=((struct Cyc_Absyn_UnresolvedMem_e_struct*)_tmpE1)->f2;_LL14B:{const char*
_tmp36D;const char*_tmp36C;const char*_tmp36B;((void(*)(void(*f)(struct _tuple10*),
struct Cyc_List_List*l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct
_dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dumpde,_tmp146,((_tmp36B="{",
_tag_dyneither(_tmp36B,sizeof(char),2))),((_tmp36C="}",_tag_dyneither(_tmp36C,
sizeof(char),2))),((_tmp36D=",",_tag_dyneither(_tmp36D,sizeof(char),2))));}goto
_LLE5;_LL14C: if(*((int*)_tmpE1)!= 38)goto _LLE5;_tmp147=((struct Cyc_Absyn_StmtExp_e_struct*)
_tmpE1)->f1;_LL14D:{const char*_tmp36E;Cyc_Absyndump_dump_nospace(((_tmp36E="({",
_tag_dyneither(_tmp36E,sizeof(char),3))));}Cyc_Absyndump_dumpstmt(_tmp147);{
const char*_tmp36F;Cyc_Absyndump_dump_nospace(((_tmp36F="})",_tag_dyneither(
_tmp36F,sizeof(char),3))));}goto _LLE5;_LLE5:;}if(inprec >= myprec)Cyc_Absyndump_dump_char((
int)')');}void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*e);void Cyc_Absyndump_dumpexp(
struct Cyc_Absyn_Exp*e){Cyc_Absyndump_dumpexp_prec(0,e);}void Cyc_Absyndump_dumpswitchclauses(
struct Cyc_List_List*scs);void Cyc_Absyndump_dumpswitchclauses(struct Cyc_List_List*
scs){for(0;scs != 0;scs=scs->tl){struct Cyc_Absyn_Switch_clause*_tmp18E=(struct Cyc_Absyn_Switch_clause*)
scs->hd;if(_tmp18E->where_clause == 0  && (void*)(_tmp18E->pattern)->r == (void*)((
void*)0)){const char*_tmp370;Cyc_Absyndump_dump(((_tmp370="default:",
_tag_dyneither(_tmp370,sizeof(char),9))));}else{{const char*_tmp371;Cyc_Absyndump_dump(((
_tmp371="case",_tag_dyneither(_tmp371,sizeof(char),5))));}Cyc_Absyndump_dumppat(
_tmp18E->pattern);if(_tmp18E->where_clause != 0){{const char*_tmp372;Cyc_Absyndump_dump(((
_tmp372="&&",_tag_dyneither(_tmp372,sizeof(char),3))));}Cyc_Absyndump_dumpexp((
struct Cyc_Absyn_Exp*)_check_null(_tmp18E->where_clause));}{const char*_tmp373;Cyc_Absyndump_dump_nospace(((
_tmp373=":",_tag_dyneither(_tmp373,sizeof(char),2))));}}Cyc_Absyndump_dumpstmt(
_tmp18E->body);}}void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*s);void Cyc_Absyndump_dumpstmt(
struct Cyc_Absyn_Stmt*s){void*_tmp193=(void*)s->r;struct Cyc_Absyn_Exp*_tmp194;
struct Cyc_Absyn_Stmt*_tmp195;struct Cyc_Absyn_Stmt*_tmp196;struct Cyc_Absyn_Exp*
_tmp197;struct Cyc_Absyn_Exp*_tmp198;struct Cyc_Absyn_Exp*_tmp199;struct Cyc_Absyn_Stmt*
_tmp19A;struct Cyc_Absyn_Stmt*_tmp19B;struct _tuple2 _tmp19C;struct Cyc_Absyn_Exp*
_tmp19D;struct Cyc_Absyn_Stmt*_tmp19E;struct _dyneither_ptr*_tmp19F;struct Cyc_Absyn_Exp*
_tmp1A0;struct _tuple2 _tmp1A1;struct Cyc_Absyn_Exp*_tmp1A2;struct _tuple2 _tmp1A3;
struct Cyc_Absyn_Exp*_tmp1A4;struct Cyc_Absyn_Stmt*_tmp1A5;struct Cyc_Absyn_Exp*
_tmp1A6;struct Cyc_List_List*_tmp1A7;struct Cyc_Absyn_Decl*_tmp1A8;struct Cyc_Absyn_Stmt*
_tmp1A9;struct _dyneither_ptr*_tmp1AA;struct Cyc_Absyn_Stmt*_tmp1AB;struct Cyc_Absyn_Stmt*
_tmp1AC;struct _tuple2 _tmp1AD;struct Cyc_Absyn_Exp*_tmp1AE;struct Cyc_List_List*
_tmp1AF;struct Cyc_List_List*_tmp1B0;struct Cyc_Absyn_Stmt*_tmp1B1;struct Cyc_List_List*
_tmp1B2;struct Cyc_Absyn_Exp*_tmp1B3;_LL153: if((int)_tmp193 != 0)goto _LL155;_LL154:
Cyc_Absyndump_dump_semi();goto _LL152;_LL155: if(_tmp193 <= (void*)1)goto _LL157;if(*((
int*)_tmp193)!= 0)goto _LL157;_tmp194=((struct Cyc_Absyn_Exp_s_struct*)_tmp193)->f1;
_LL156: Cyc_Absyndump_dumploc(s->loc);Cyc_Absyndump_dumpexp(_tmp194);Cyc_Absyndump_dump_semi();
goto _LL152;_LL157: if(_tmp193 <= (void*)1)goto _LL159;if(*((int*)_tmp193)!= 1)goto
_LL159;_tmp195=((struct Cyc_Absyn_Seq_s_struct*)_tmp193)->f1;_tmp196=((struct Cyc_Absyn_Seq_s_struct*)
_tmp193)->f2;_LL158: if(Cyc_Absynpp_is_declaration(_tmp195)){Cyc_Absyndump_dump_char((
int)'{');Cyc_Absyndump_dumpstmt(_tmp195);Cyc_Absyndump_dump_char((int)'}');}
else{Cyc_Absyndump_dumpstmt(_tmp195);}if(Cyc_Absynpp_is_declaration(_tmp196)){
Cyc_Absyndump_dump_char((int)'{');Cyc_Absyndump_dumpstmt(_tmp196);Cyc_Absyndump_dump_char((
int)'}');}else{Cyc_Absyndump_dumpstmt(_tmp196);}goto _LL152;_LL159: if(_tmp193 <= (
void*)1)goto _LL15B;if(*((int*)_tmp193)!= 2)goto _LL15B;_tmp197=((struct Cyc_Absyn_Return_s_struct*)
_tmp193)->f1;if(_tmp197 != 0)goto _LL15B;_LL15A: Cyc_Absyndump_dumploc(s->loc);{
const char*_tmp374;Cyc_Absyndump_dump(((_tmp374="return;",_tag_dyneither(_tmp374,
sizeof(char),8))));}goto _LL152;_LL15B: if(_tmp193 <= (void*)1)goto _LL15D;if(*((int*)
_tmp193)!= 2)goto _LL15D;_tmp198=((struct Cyc_Absyn_Return_s_struct*)_tmp193)->f1;
_LL15C: Cyc_Absyndump_dumploc(s->loc);{const char*_tmp375;Cyc_Absyndump_dump(((
_tmp375="return",_tag_dyneither(_tmp375,sizeof(char),7))));}Cyc_Absyndump_dumpexp((
struct Cyc_Absyn_Exp*)_check_null(_tmp198));Cyc_Absyndump_dump_semi();goto _LL152;
_LL15D: if(_tmp193 <= (void*)1)goto _LL15F;if(*((int*)_tmp193)!= 3)goto _LL15F;
_tmp199=((struct Cyc_Absyn_IfThenElse_s_struct*)_tmp193)->f1;_tmp19A=((struct Cyc_Absyn_IfThenElse_s_struct*)
_tmp193)->f2;_tmp19B=((struct Cyc_Absyn_IfThenElse_s_struct*)_tmp193)->f3;_LL15E:
Cyc_Absyndump_dumploc(s->loc);{const char*_tmp376;Cyc_Absyndump_dump(((_tmp376="if(",
_tag_dyneither(_tmp376,sizeof(char),4))));}Cyc_Absyndump_dumpexp(_tmp199);{void*
_tmp1B7=(void*)_tmp19A->r;_LL17A: if(_tmp1B7 <= (void*)1)goto _LL182;if(*((int*)
_tmp1B7)!= 1)goto _LL17C;_LL17B: goto _LL17D;_LL17C: if(*((int*)_tmp1B7)!= 11)goto
_LL17E;_LL17D: goto _LL17F;_LL17E: if(*((int*)_tmp1B7)!= 3)goto _LL180;_LL17F: goto
_LL181;_LL180: if(*((int*)_tmp1B7)!= 12)goto _LL182;_LL181:{const char*_tmp377;Cyc_Absyndump_dump_nospace(((
_tmp377="){",_tag_dyneither(_tmp377,sizeof(char),3))));}Cyc_Absyndump_dumpstmt(
_tmp19A);Cyc_Absyndump_dump_char((int)'}');goto _LL179;_LL182:;_LL183: Cyc_Absyndump_dump_char((
int)')');Cyc_Absyndump_dumpstmt(_tmp19A);_LL179:;}{void*_tmp1B9=(void*)_tmp19B->r;
_LL185: if((int)_tmp1B9 != 0)goto _LL187;_LL186: goto _LL184;_LL187:;_LL188:{const
char*_tmp378;Cyc_Absyndump_dump(((_tmp378="else{",_tag_dyneither(_tmp378,sizeof(
char),6))));}Cyc_Absyndump_dumpstmt(_tmp19B);Cyc_Absyndump_dump_char((int)'}');
goto _LL184;_LL184:;}goto _LL152;_LL15F: if(_tmp193 <= (void*)1)goto _LL161;if(*((int*)
_tmp193)!= 4)goto _LL161;_tmp19C=((struct Cyc_Absyn_While_s_struct*)_tmp193)->f1;
_tmp19D=_tmp19C.f1;_tmp19E=((struct Cyc_Absyn_While_s_struct*)_tmp193)->f2;_LL160:
Cyc_Absyndump_dumploc(s->loc);{const char*_tmp379;Cyc_Absyndump_dump(((_tmp379="while(",
_tag_dyneither(_tmp379,sizeof(char),7))));}Cyc_Absyndump_dumpexp(_tmp19D);{const
char*_tmp37A;Cyc_Absyndump_dump_nospace(((_tmp37A="){",_tag_dyneither(_tmp37A,
sizeof(char),3))));}Cyc_Absyndump_dumpstmt(_tmp19E);Cyc_Absyndump_dump_char((int)'}');
goto _LL152;_LL161: if(_tmp193 <= (void*)1)goto _LL163;if(*((int*)_tmp193)!= 5)goto
_LL163;_LL162: Cyc_Absyndump_dumploc(s->loc);{const char*_tmp37B;Cyc_Absyndump_dump(((
_tmp37B="break;",_tag_dyneither(_tmp37B,sizeof(char),7))));}goto _LL152;_LL163:
if(_tmp193 <= (void*)1)goto _LL165;if(*((int*)_tmp193)!= 6)goto _LL165;_LL164: Cyc_Absyndump_dumploc(
s->loc);{const char*_tmp37C;Cyc_Absyndump_dump(((_tmp37C="continue;",
_tag_dyneither(_tmp37C,sizeof(char),10))));}goto _LL152;_LL165: if(_tmp193 <= (void*)
1)goto _LL167;if(*((int*)_tmp193)!= 7)goto _LL167;_tmp19F=((struct Cyc_Absyn_Goto_s_struct*)
_tmp193)->f1;_LL166: Cyc_Absyndump_dumploc(s->loc);{const char*_tmp37D;Cyc_Absyndump_dump(((
_tmp37D="goto",_tag_dyneither(_tmp37D,sizeof(char),5))));}Cyc_Absyndump_dump_str(
_tmp19F);Cyc_Absyndump_dump_semi();goto _LL152;_LL167: if(_tmp193 <= (void*)1)goto
_LL169;if(*((int*)_tmp193)!= 8)goto _LL169;_tmp1A0=((struct Cyc_Absyn_For_s_struct*)
_tmp193)->f1;_tmp1A1=((struct Cyc_Absyn_For_s_struct*)_tmp193)->f2;_tmp1A2=
_tmp1A1.f1;_tmp1A3=((struct Cyc_Absyn_For_s_struct*)_tmp193)->f3;_tmp1A4=_tmp1A3.f1;
_tmp1A5=((struct Cyc_Absyn_For_s_struct*)_tmp193)->f4;_LL168: Cyc_Absyndump_dumploc(
s->loc);{const char*_tmp37E;Cyc_Absyndump_dump(((_tmp37E="for(",_tag_dyneither(
_tmp37E,sizeof(char),5))));}Cyc_Absyndump_dumpexp(_tmp1A0);Cyc_Absyndump_dump_semi();
Cyc_Absyndump_dumpexp(_tmp1A2);Cyc_Absyndump_dump_semi();Cyc_Absyndump_dumpexp(
_tmp1A4);{const char*_tmp37F;Cyc_Absyndump_dump_nospace(((_tmp37F="){",
_tag_dyneither(_tmp37F,sizeof(char),3))));}Cyc_Absyndump_dumpstmt(_tmp1A5);Cyc_Absyndump_dump_char((
int)'}');goto _LL152;_LL169: if(_tmp193 <= (void*)1)goto _LL16B;if(*((int*)_tmp193)
!= 9)goto _LL16B;_tmp1A6=((struct Cyc_Absyn_Switch_s_struct*)_tmp193)->f1;_tmp1A7=((
struct Cyc_Absyn_Switch_s_struct*)_tmp193)->f2;_LL16A: Cyc_Absyndump_dumploc(s->loc);{
const char*_tmp380;Cyc_Absyndump_dump(((_tmp380="switch(",_tag_dyneither(_tmp380,
sizeof(char),8))));}Cyc_Absyndump_dumpexp(_tmp1A6);{const char*_tmp381;Cyc_Absyndump_dump_nospace(((
_tmp381="){",_tag_dyneither(_tmp381,sizeof(char),3))));}Cyc_Absyndump_dumpswitchclauses(
_tmp1A7);Cyc_Absyndump_dump_char((int)'}');goto _LL152;_LL16B: if(_tmp193 <= (void*)
1)goto _LL16D;if(*((int*)_tmp193)!= 11)goto _LL16D;_tmp1A8=((struct Cyc_Absyn_Decl_s_struct*)
_tmp193)->f1;_tmp1A9=((struct Cyc_Absyn_Decl_s_struct*)_tmp193)->f2;_LL16C: Cyc_Absyndump_dumpdecl(
_tmp1A8);Cyc_Absyndump_dumpstmt(_tmp1A9);goto _LL152;_LL16D: if(_tmp193 <= (void*)1)
goto _LL16F;if(*((int*)_tmp193)!= 12)goto _LL16F;_tmp1AA=((struct Cyc_Absyn_Label_s_struct*)
_tmp193)->f1;_tmp1AB=((struct Cyc_Absyn_Label_s_struct*)_tmp193)->f2;_LL16E: if(
Cyc_Absynpp_is_declaration(_tmp1AB)){Cyc_Absyndump_dump_str(_tmp1AA);{const char*
_tmp382;Cyc_Absyndump_dump_nospace(((_tmp382=": {",_tag_dyneither(_tmp382,
sizeof(char),4))));}Cyc_Absyndump_dumpstmt(_tmp1AB);Cyc_Absyndump_dump_char((int)'}');}
else{Cyc_Absyndump_dump_str(_tmp1AA);Cyc_Absyndump_dump_char((int)':');Cyc_Absyndump_dumpstmt(
_tmp1AB);}goto _LL152;_LL16F: if(_tmp193 <= (void*)1)goto _LL171;if(*((int*)_tmp193)
!= 13)goto _LL171;_tmp1AC=((struct Cyc_Absyn_Do_s_struct*)_tmp193)->f1;_tmp1AD=((
struct Cyc_Absyn_Do_s_struct*)_tmp193)->f2;_tmp1AE=_tmp1AD.f1;_LL170: Cyc_Absyndump_dumploc(
s->loc);{const char*_tmp383;Cyc_Absyndump_dump(((_tmp383="do{",_tag_dyneither(
_tmp383,sizeof(char),4))));}Cyc_Absyndump_dumpstmt(_tmp1AC);{const char*_tmp384;
Cyc_Absyndump_dump_nospace(((_tmp384="}while(",_tag_dyneither(_tmp384,sizeof(
char),8))));}Cyc_Absyndump_dumpexp(_tmp1AE);{const char*_tmp385;Cyc_Absyndump_dump_nospace(((
_tmp385=");",_tag_dyneither(_tmp385,sizeof(char),3))));}goto _LL152;_LL171: if(
_tmp193 <= (void*)1)goto _LL173;if(*((int*)_tmp193)!= 10)goto _LL173;_tmp1AF=((
struct Cyc_Absyn_Fallthru_s_struct*)_tmp193)->f1;if(_tmp1AF != 0)goto _LL173;_LL172:{
const char*_tmp386;Cyc_Absyndump_dump(((_tmp386="fallthru;",_tag_dyneither(
_tmp386,sizeof(char),10))));}goto _LL152;_LL173: if(_tmp193 <= (void*)1)goto _LL175;
if(*((int*)_tmp193)!= 10)goto _LL175;_tmp1B0=((struct Cyc_Absyn_Fallthru_s_struct*)
_tmp193)->f1;_LL174:{const char*_tmp387;Cyc_Absyndump_dump(((_tmp387="fallthru(",
_tag_dyneither(_tmp387,sizeof(char),10))));}Cyc_Absyndump_dumpexps_prec(20,
_tmp1B0);{const char*_tmp388;Cyc_Absyndump_dump_nospace(((_tmp388=");",
_tag_dyneither(_tmp388,sizeof(char),3))));}goto _LL152;_LL175: if(_tmp193 <= (void*)
1)goto _LL177;if(*((int*)_tmp193)!= 14)goto _LL177;_tmp1B1=((struct Cyc_Absyn_TryCatch_s_struct*)
_tmp193)->f1;_tmp1B2=((struct Cyc_Absyn_TryCatch_s_struct*)_tmp193)->f2;_LL176:
Cyc_Absyndump_dumploc(s->loc);{const char*_tmp389;Cyc_Absyndump_dump(((_tmp389="try",
_tag_dyneither(_tmp389,sizeof(char),4))));}Cyc_Absyndump_dumpstmt(_tmp1B1);{
const char*_tmp38A;Cyc_Absyndump_dump(((_tmp38A="catch{",_tag_dyneither(_tmp38A,
sizeof(char),7))));}Cyc_Absyndump_dumpswitchclauses(_tmp1B2);Cyc_Absyndump_dump_char((
int)'}');goto _LL152;_LL177: if(_tmp193 <= (void*)1)goto _LL152;if(*((int*)_tmp193)
!= 15)goto _LL152;_tmp1B3=((struct Cyc_Absyn_ResetRegion_s_struct*)_tmp193)->f1;
_LL178: Cyc_Absyndump_dumploc(s->loc);{const char*_tmp38B;Cyc_Absyndump_dump(((
_tmp38B="reset_region(",_tag_dyneither(_tmp38B,sizeof(char),14))));}Cyc_Absyndump_dumpexp(
_tmp1B3);{const char*_tmp38C;Cyc_Absyndump_dump(((_tmp38C=");",_tag_dyneither(
_tmp38C,sizeof(char),3))));}goto _LL152;_LL152:;}struct _tuple11{struct Cyc_List_List*
f1;struct Cyc_Absyn_Pat*f2;};void Cyc_Absyndump_dumpdp(struct _tuple11*dp);void Cyc_Absyndump_dumpdp(
struct _tuple11*dp){{const char*_tmp38F;const char*_tmp38E;const char*_tmp38D;Cyc_Absyndump_egroup(
Cyc_Absyndump_dumpdesignator,(*dp).f1,((_tmp38D="",_tag_dyneither(_tmp38D,
sizeof(char),1))),((_tmp38E="=",_tag_dyneither(_tmp38E,sizeof(char),2))),((
_tmp38F="=",_tag_dyneither(_tmp38F,sizeof(char),2))));}Cyc_Absyndump_dumppat((*
dp).f2);}void Cyc_Absyndump_dumppat(struct Cyc_Absyn_Pat*p);void Cyc_Absyndump_dumppat(
struct Cyc_Absyn_Pat*p){void*_tmp1D2=(void*)p->r;void*_tmp1D3;int _tmp1D4;void*
_tmp1D5;int _tmp1D6;void*_tmp1D7;int _tmp1D8;char _tmp1D9;struct _dyneither_ptr
_tmp1DA;struct Cyc_Absyn_Vardecl*_tmp1DB;struct Cyc_Absyn_Pat*_tmp1DC;struct Cyc_Absyn_Pat
_tmp1DD;void*_tmp1DE;struct Cyc_Absyn_Vardecl*_tmp1DF;struct Cyc_Absyn_Pat*_tmp1E0;
struct Cyc_List_List*_tmp1E1;int _tmp1E2;struct Cyc_Absyn_Pat*_tmp1E3;struct Cyc_Absyn_Vardecl*
_tmp1E4;struct Cyc_Absyn_Pat*_tmp1E5;struct Cyc_Absyn_Pat _tmp1E6;void*_tmp1E7;
struct Cyc_Absyn_Vardecl*_tmp1E8;struct Cyc_Absyn_Pat*_tmp1E9;struct Cyc_Absyn_Tvar*
_tmp1EA;struct Cyc_Absyn_Vardecl*_tmp1EB;struct _tuple0*_tmp1EC;struct _tuple0*
_tmp1ED;struct Cyc_List_List*_tmp1EE;int _tmp1EF;struct Cyc_Absyn_AggrInfo _tmp1F0;
union Cyc_Absyn_AggrInfoU_union _tmp1F1;struct Cyc_List_List*_tmp1F2;struct Cyc_List_List*
_tmp1F3;int _tmp1F4;struct Cyc_Absyn_Tunionfield*_tmp1F5;struct Cyc_List_List*
_tmp1F6;int _tmp1F7;struct Cyc_Absyn_Enumfield*_tmp1F8;struct Cyc_Absyn_Enumfield*
_tmp1F9;struct Cyc_Absyn_Exp*_tmp1FA;_LL18A: if((int)_tmp1D2 != 0)goto _LL18C;_LL18B:
Cyc_Absyndump_dump_char((int)'_');goto _LL189;_LL18C: if((int)_tmp1D2 != 1)goto
_LL18E;_LL18D:{const char*_tmp390;Cyc_Absyndump_dump(((_tmp390="NULL",
_tag_dyneither(_tmp390,sizeof(char),5))));}goto _LL189;_LL18E: if(_tmp1D2 <= (void*)
2)goto _LL190;if(*((int*)_tmp1D2)!= 7)goto _LL190;_tmp1D3=(void*)((struct Cyc_Absyn_Int_p_struct*)
_tmp1D2)->f1;if((int)_tmp1D3 != 2)goto _LL190;_tmp1D4=((struct Cyc_Absyn_Int_p_struct*)
_tmp1D2)->f2;_LL18F: _tmp1D6=_tmp1D4;goto _LL191;_LL190: if(_tmp1D2 <= (void*)2)goto
_LL192;if(*((int*)_tmp1D2)!= 7)goto _LL192;_tmp1D5=(void*)((struct Cyc_Absyn_Int_p_struct*)
_tmp1D2)->f1;if((int)_tmp1D5 != 0)goto _LL192;_tmp1D6=((struct Cyc_Absyn_Int_p_struct*)
_tmp1D2)->f2;_LL191:{const char*_tmp394;void*_tmp393[1];struct Cyc_Int_pa_struct
_tmp392;Cyc_Absyndump_dump((struct _dyneither_ptr)((_tmp392.tag=1,((_tmp392.f1=(
unsigned long)_tmp1D6,((_tmp393[0]=& _tmp392,Cyc_aprintf(((_tmp394="%d",
_tag_dyneither(_tmp394,sizeof(char),3))),_tag_dyneither(_tmp393,sizeof(void*),1)))))))));}
goto _LL189;_LL192: if(_tmp1D2 <= (void*)2)goto _LL194;if(*((int*)_tmp1D2)!= 7)goto
_LL194;_tmp1D7=(void*)((struct Cyc_Absyn_Int_p_struct*)_tmp1D2)->f1;if((int)
_tmp1D7 != 1)goto _LL194;_tmp1D8=((struct Cyc_Absyn_Int_p_struct*)_tmp1D2)->f2;
_LL193:{const char*_tmp398;void*_tmp397[1];struct Cyc_Int_pa_struct _tmp396;Cyc_Absyndump_dump((
struct _dyneither_ptr)((_tmp396.tag=1,((_tmp396.f1=(unsigned int)_tmp1D8,((
_tmp397[0]=& _tmp396,Cyc_aprintf(((_tmp398="%u",_tag_dyneither(_tmp398,sizeof(
char),3))),_tag_dyneither(_tmp397,sizeof(void*),1)))))))));}goto _LL189;_LL194:
if(_tmp1D2 <= (void*)2)goto _LL196;if(*((int*)_tmp1D2)!= 8)goto _LL196;_tmp1D9=((
struct Cyc_Absyn_Char_p_struct*)_tmp1D2)->f1;_LL195:{const char*_tmp399;Cyc_Absyndump_dump(((
_tmp399="'",_tag_dyneither(_tmp399,sizeof(char),2))));}Cyc_Absyndump_dump_nospace(
Cyc_Absynpp_char_escape(_tmp1D9));{const char*_tmp39A;Cyc_Absyndump_dump_nospace(((
_tmp39A="'",_tag_dyneither(_tmp39A,sizeof(char),2))));}goto _LL189;_LL196: if(
_tmp1D2 <= (void*)2)goto _LL198;if(*((int*)_tmp1D2)!= 9)goto _LL198;_tmp1DA=((
struct Cyc_Absyn_Float_p_struct*)_tmp1D2)->f1;_LL197: Cyc_Absyndump_dump(_tmp1DA);
goto _LL189;_LL198: if(_tmp1D2 <= (void*)2)goto _LL19A;if(*((int*)_tmp1D2)!= 0)goto
_LL19A;_tmp1DB=((struct Cyc_Absyn_Var_p_struct*)_tmp1D2)->f1;_tmp1DC=((struct Cyc_Absyn_Var_p_struct*)
_tmp1D2)->f2;_tmp1DD=*_tmp1DC;_tmp1DE=(void*)_tmp1DD.r;if((int)_tmp1DE != 0)goto
_LL19A;_LL199: Cyc_Absyndump_dumpqvar(_tmp1DB->name);goto _LL189;_LL19A: if(_tmp1D2
<= (void*)2)goto _LL19C;if(*((int*)_tmp1D2)!= 0)goto _LL19C;_tmp1DF=((struct Cyc_Absyn_Var_p_struct*)
_tmp1D2)->f1;_tmp1E0=((struct Cyc_Absyn_Var_p_struct*)_tmp1D2)->f2;_LL19B: Cyc_Absyndump_dumpqvar(
_tmp1DF->name);{const char*_tmp39B;Cyc_Absyndump_dump(((_tmp39B=" as ",
_tag_dyneither(_tmp39B,sizeof(char),5))));}Cyc_Absyndump_dumppat(_tmp1E0);goto
_LL189;_LL19C: if(_tmp1D2 <= (void*)2)goto _LL19E;if(*((int*)_tmp1D2)!= 3)goto
_LL19E;_tmp1E1=((struct Cyc_Absyn_Tuple_p_struct*)_tmp1D2)->f1;_tmp1E2=((struct
Cyc_Absyn_Tuple_p_struct*)_tmp1D2)->f2;_LL19D: {const char*_tmp39D;const char*
_tmp39C;struct _dyneither_ptr term=_tmp1E2?(_tmp39D=", ...)",_tag_dyneither(
_tmp39D,sizeof(char),7)):((_tmp39C=")",_tag_dyneither(_tmp39C,sizeof(char),2)));{
const char*_tmp39F;const char*_tmp39E;((void(*)(void(*f)(struct Cyc_Absyn_Pat*),
struct Cyc_List_List*l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct
_dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dumppat,_tmp1E1,((_tmp39E="$(",
_tag_dyneither(_tmp39E,sizeof(char),3))),term,((_tmp39F=",",_tag_dyneither(
_tmp39F,sizeof(char),2))));}goto _LL189;}_LL19E: if(_tmp1D2 <= (void*)2)goto _LL1A0;
if(*((int*)_tmp1D2)!= 4)goto _LL1A0;_tmp1E3=((struct Cyc_Absyn_Pointer_p_struct*)
_tmp1D2)->f1;_LL19F:{const char*_tmp3A0;Cyc_Absyndump_dump(((_tmp3A0="&",
_tag_dyneither(_tmp3A0,sizeof(char),2))));}Cyc_Absyndump_dumppat(_tmp1E3);goto
_LL189;_LL1A0: if(_tmp1D2 <= (void*)2)goto _LL1A2;if(*((int*)_tmp1D2)!= 1)goto
_LL1A2;_tmp1E4=((struct Cyc_Absyn_Reference_p_struct*)_tmp1D2)->f1;_tmp1E5=((
struct Cyc_Absyn_Reference_p_struct*)_tmp1D2)->f2;_tmp1E6=*_tmp1E5;_tmp1E7=(void*)
_tmp1E6.r;if((int)_tmp1E7 != 0)goto _LL1A2;_LL1A1:{const char*_tmp3A1;Cyc_Absyndump_dump(((
_tmp3A1="*",_tag_dyneither(_tmp3A1,sizeof(char),2))));}Cyc_Absyndump_dumpqvar(
_tmp1E4->name);goto _LL189;_LL1A2: if(_tmp1D2 <= (void*)2)goto _LL1A4;if(*((int*)
_tmp1D2)!= 1)goto _LL1A4;_tmp1E8=((struct Cyc_Absyn_Reference_p_struct*)_tmp1D2)->f1;
_tmp1E9=((struct Cyc_Absyn_Reference_p_struct*)_tmp1D2)->f2;_LL1A3:{const char*
_tmp3A2;Cyc_Absyndump_dump(((_tmp3A2="*",_tag_dyneither(_tmp3A2,sizeof(char),2))));}
Cyc_Absyndump_dumpqvar(_tmp1E8->name);{const char*_tmp3A3;Cyc_Absyndump_dump(((
_tmp3A3=" as ",_tag_dyneither(_tmp3A3,sizeof(char),5))));}Cyc_Absyndump_dumppat(
_tmp1E9);goto _LL189;_LL1A4: if(_tmp1D2 <= (void*)2)goto _LL1A6;if(*((int*)_tmp1D2)
!= 2)goto _LL1A6;_tmp1EA=((struct Cyc_Absyn_TagInt_p_struct*)_tmp1D2)->f1;_tmp1EB=((
struct Cyc_Absyn_TagInt_p_struct*)_tmp1D2)->f2;_LL1A5: Cyc_Absyndump_dumpqvar(
_tmp1EB->name);Cyc_Absyndump_dump_char((int)'<');Cyc_Absyndump_dumptvar(_tmp1EA);
Cyc_Absyndump_dump_char((int)'>');goto _LL189;_LL1A6: if(_tmp1D2 <= (void*)2)goto
_LL1A8;if(*((int*)_tmp1D2)!= 12)goto _LL1A8;_tmp1EC=((struct Cyc_Absyn_UnknownId_p_struct*)
_tmp1D2)->f1;_LL1A7: Cyc_Absyndump_dumpqvar(_tmp1EC);goto _LL189;_LL1A8: if(_tmp1D2
<= (void*)2)goto _LL1AA;if(*((int*)_tmp1D2)!= 13)goto _LL1AA;_tmp1ED=((struct Cyc_Absyn_UnknownCall_p_struct*)
_tmp1D2)->f1;_tmp1EE=((struct Cyc_Absyn_UnknownCall_p_struct*)_tmp1D2)->f2;
_tmp1EF=((struct Cyc_Absyn_UnknownCall_p_struct*)_tmp1D2)->f3;_LL1A9: {const char*
_tmp3A5;const char*_tmp3A4;struct _dyneither_ptr term=_tmp1EF?(_tmp3A5=", ...)",
_tag_dyneither(_tmp3A5,sizeof(char),7)):((_tmp3A4=")",_tag_dyneither(_tmp3A4,
sizeof(char),2)));Cyc_Absyndump_dumpqvar(_tmp1ED);{const char*_tmp3A7;const char*
_tmp3A6;((void(*)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct
_dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumppat,_tmp1EE,((_tmp3A6="(",_tag_dyneither(_tmp3A6,sizeof(char),
2))),term,((_tmp3A7=",",_tag_dyneither(_tmp3A7,sizeof(char),2))));}goto _LL189;}
_LL1AA: if(_tmp1D2 <= (void*)2)goto _LL1AC;if(*((int*)_tmp1D2)!= 5)goto _LL1AC;
_tmp1F0=((struct Cyc_Absyn_Aggr_p_struct*)_tmp1D2)->f1;_tmp1F1=_tmp1F0.aggr_info;
_tmp1F2=((struct Cyc_Absyn_Aggr_p_struct*)_tmp1D2)->f2;_tmp1F3=((struct Cyc_Absyn_Aggr_p_struct*)
_tmp1D2)->f3;_tmp1F4=((struct Cyc_Absyn_Aggr_p_struct*)_tmp1D2)->f4;_LL1AB: {
struct _tuple0*_tmp212;struct _tuple3 _tmp211=Cyc_Absyn_aggr_kinded_name(_tmp1F1);
_tmp212=_tmp211.f2;{const char*_tmp3A9;const char*_tmp3A8;struct _dyneither_ptr term=
_tmp1F4?(_tmp3A9=", ...)",_tag_dyneither(_tmp3A9,sizeof(char),7)):((_tmp3A8=")",
_tag_dyneither(_tmp3A8,sizeof(char),2)));Cyc_Absyndump_dumpqvar(_tmp212);Cyc_Absyndump_dump_char((
int)'{');{const char*_tmp3AC;const char*_tmp3AB;const char*_tmp3AA;((void(*)(void(*
f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_egroup)(Cyc_Absyndump_dumptvar,
_tmp1F2,((_tmp3AA="[",_tag_dyneither(_tmp3AA,sizeof(char),2))),((_tmp3AB="]",
_tag_dyneither(_tmp3AB,sizeof(char),2))),((_tmp3AC=",",_tag_dyneither(_tmp3AC,
sizeof(char),2))));}{const char*_tmp3AE;const char*_tmp3AD;((void(*)(void(*f)(
struct _tuple11*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dumpdp,
_tmp1F3,((_tmp3AD="",_tag_dyneither(_tmp3AD,sizeof(char),1))),term,((_tmp3AE=",",
_tag_dyneither(_tmp3AE,sizeof(char),2))));}goto _LL189;}}_LL1AC: if(_tmp1D2 <= (
void*)2)goto _LL1AE;if(*((int*)_tmp1D2)!= 6)goto _LL1AE;_tmp1F5=((struct Cyc_Absyn_Tunion_p_struct*)
_tmp1D2)->f2;_tmp1F6=((struct Cyc_Absyn_Tunion_p_struct*)_tmp1D2)->f3;_tmp1F7=((
struct Cyc_Absyn_Tunion_p_struct*)_tmp1D2)->f4;_LL1AD: {const char*_tmp3B0;const
char*_tmp3AF;struct _dyneither_ptr term=_tmp1F7?(_tmp3B0=", ...)",_tag_dyneither(
_tmp3B0,sizeof(char),7)):((_tmp3AF=")",_tag_dyneither(_tmp3AF,sizeof(char),2)));
Cyc_Absyndump_dumpqvar(_tmp1F5->name);if(_tmp1F6 != 0){const char*_tmp3B2;const
char*_tmp3B1;((void(*)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,
struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(
Cyc_Absyndump_dumppat,_tmp1F6,((_tmp3B1="(",_tag_dyneither(_tmp3B1,sizeof(char),
2))),term,((_tmp3B2=",",_tag_dyneither(_tmp3B2,sizeof(char),2))));}goto _LL189;}
_LL1AE: if(_tmp1D2 <= (void*)2)goto _LL1B0;if(*((int*)_tmp1D2)!= 10)goto _LL1B0;
_tmp1F8=((struct Cyc_Absyn_Enum_p_struct*)_tmp1D2)->f2;_LL1AF: _tmp1F9=_tmp1F8;
goto _LL1B1;_LL1B0: if(_tmp1D2 <= (void*)2)goto _LL1B2;if(*((int*)_tmp1D2)!= 11)goto
_LL1B2;_tmp1F9=((struct Cyc_Absyn_AnonEnum_p_struct*)_tmp1D2)->f2;_LL1B1: Cyc_Absyndump_dumpqvar(
_tmp1F9->name);goto _LL189;_LL1B2: if(_tmp1D2 <= (void*)2)goto _LL189;if(*((int*)
_tmp1D2)!= 14)goto _LL189;_tmp1FA=((struct Cyc_Absyn_Exp_p_struct*)_tmp1D2)->f1;
_LL1B3: Cyc_Absyndump_dumpexp(_tmp1FA);goto _LL189;_LL189:;}void Cyc_Absyndump_dumptunionfield(
struct Cyc_Absyn_Tunionfield*ef);void Cyc_Absyndump_dumptunionfield(struct Cyc_Absyn_Tunionfield*
ef){Cyc_Absyndump_dumpqvar(ef->name);if(ef->typs != 0)Cyc_Absyndump_dumpargs(ef->typs);}
void Cyc_Absyndump_dumptunionfields(struct Cyc_List_List*fields);void Cyc_Absyndump_dumptunionfields(
struct Cyc_List_List*fields){const char*_tmp3B3;((void(*)(void(*f)(struct Cyc_Absyn_Tunionfield*),
struct Cyc_List_List*l,struct _dyneither_ptr sep))Cyc_Absyndump_dump_sep)(Cyc_Absyndump_dumptunionfield,
fields,((_tmp3B3=",",_tag_dyneither(_tmp3B3,sizeof(char),2))));}void Cyc_Absyndump_dumpenumfield(
struct Cyc_Absyn_Enumfield*ef);void Cyc_Absyndump_dumpenumfield(struct Cyc_Absyn_Enumfield*
ef){Cyc_Absyndump_dumpqvar(ef->name);if(ef->tag != 0){{const char*_tmp3B4;Cyc_Absyndump_dump(((
_tmp3B4=" = ",_tag_dyneither(_tmp3B4,sizeof(char),4))));}Cyc_Absyndump_dumpexp((
struct Cyc_Absyn_Exp*)_check_null(ef->tag));}}void Cyc_Absyndump_dumpenumfields(
struct Cyc_List_List*fields);void Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*
fields){const char*_tmp3B5;((void(*)(void(*f)(struct Cyc_Absyn_Enumfield*),struct
Cyc_List_List*l,struct _dyneither_ptr sep))Cyc_Absyndump_dump_sep)(Cyc_Absyndump_dumpenumfield,
fields,((_tmp3B5=",",_tag_dyneither(_tmp3B5,sizeof(char),2))));}void Cyc_Absyndump_dumpaggrfields(
struct Cyc_List_List*fields);void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*
fields){for(0;fields != 0;fields=fields->tl){struct Cyc_Absyn_Aggrfield _tmp222;
struct _dyneither_ptr*_tmp223;struct Cyc_Absyn_Tqual _tmp224;void*_tmp225;struct Cyc_Absyn_Exp*
_tmp226;struct Cyc_List_List*_tmp227;struct Cyc_Absyn_Aggrfield*_tmp221=(struct Cyc_Absyn_Aggrfield*)
fields->hd;_tmp222=*_tmp221;_tmp223=_tmp222.name;_tmp224=_tmp222.tq;_tmp225=(
void*)_tmp222.type;_tmp226=_tmp222.width;_tmp227=_tmp222.attributes;{void*
_tmp228=Cyc_Cyclone_c_compiler;_LL1B5: if((int)_tmp228 != 0)goto _LL1B7;_LL1B6:((
void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _dyneither_ptr*),struct
_dyneither_ptr*))Cyc_Absyndump_dumptqtd)(_tmp224,_tmp225,Cyc_Absyndump_dump_str,
_tmp223);Cyc_Absyndump_dumpatts(_tmp227);goto _LL1B4;_LL1B7: if((int)_tmp228 != 1)
goto _LL1B4;_LL1B8: Cyc_Absyndump_dumpatts(_tmp227);((void(*)(struct Cyc_Absyn_Tqual,
void*,void(*f)(struct _dyneither_ptr*),struct _dyneither_ptr*))Cyc_Absyndump_dumptqtd)(
_tmp224,_tmp225,Cyc_Absyndump_dump_str,_tmp223);goto _LL1B4;_LL1B4:;}if(_tmp226 != 
0){Cyc_Absyndump_dump_char((int)':');Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)
_tmp226);}Cyc_Absyndump_dump_semi();}}void Cyc_Absyndump_dumptypedefname(struct
Cyc_Absyn_Typedefdecl*td);void Cyc_Absyndump_dumptypedefname(struct Cyc_Absyn_Typedefdecl*
td){Cyc_Absyndump_dumpqvar(td->name);Cyc_Absyndump_dumptvars(td->tvs);}static
void Cyc_Absyndump_dump_atts_qvar(struct Cyc_Absyn_Fndecl*fd);static void Cyc_Absyndump_dump_atts_qvar(
struct Cyc_Absyn_Fndecl*fd){Cyc_Absyndump_dumpatts(fd->attributes);Cyc_Absyndump_dumpqvar(
fd->name);}static void Cyc_Absyndump_dump_callconv_qvar(struct _tuple3*pr);static
void Cyc_Absyndump_dump_callconv_qvar(struct _tuple3*pr){{void*_tmp229=(*pr).f1;
_LL1BA: if((int)_tmp229 != 8)goto _LL1BC;_LL1BB: goto _LL1B9;_LL1BC: if((int)_tmp229 != 
0)goto _LL1BE;_LL1BD:{const char*_tmp3B6;Cyc_Absyndump_dump(((_tmp3B6="_stdcall",
_tag_dyneither(_tmp3B6,sizeof(char),9))));}goto _LL1B9;_LL1BE: if((int)_tmp229 != 1)
goto _LL1C0;_LL1BF:{const char*_tmp3B7;Cyc_Absyndump_dump(((_tmp3B7="_cdecl",
_tag_dyneither(_tmp3B7,sizeof(char),7))));}goto _LL1B9;_LL1C0: if((int)_tmp229 != 2)
goto _LL1C2;_LL1C1:{const char*_tmp3B8;Cyc_Absyndump_dump(((_tmp3B8="_fastcall",
_tag_dyneither(_tmp3B8,sizeof(char),10))));}goto _LL1B9;_LL1C2:;_LL1C3: goto _LL1B9;
_LL1B9:;}Cyc_Absyndump_dumpqvar((*pr).f2);}static void Cyc_Absyndump_dump_callconv_fdqvar(
struct Cyc_Absyn_Fndecl*fd);static void Cyc_Absyndump_dump_callconv_fdqvar(struct
Cyc_Absyn_Fndecl*fd){Cyc_Absyndump_dump_callconv(fd->attributes);Cyc_Absyndump_dumpqvar(
fd->name);}static void Cyc_Absyndump_dumpids(struct Cyc_List_List*vds);static void
Cyc_Absyndump_dumpids(struct Cyc_List_List*vds){for(0;vds != 0;vds=vds->tl){Cyc_Absyndump_dumpqvar(((
struct Cyc_Absyn_Vardecl*)vds->hd)->name);if(vds->tl != 0)Cyc_Absyndump_dump_char((
int)',');}}void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*vd,struct Cyc_Position_Segment*
loc);void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*vd,struct Cyc_Position_Segment*
loc){struct Cyc_Absyn_Vardecl _tmp22E;void*_tmp22F;struct _tuple0*_tmp230;struct Cyc_Absyn_Tqual
_tmp231;void*_tmp232;struct Cyc_Absyn_Exp*_tmp233;struct Cyc_List_List*_tmp234;
struct Cyc_Absyn_Vardecl*_tmp22D=vd;_tmp22E=*_tmp22D;_tmp22F=(void*)_tmp22E.sc;
_tmp230=_tmp22E.name;_tmp231=_tmp22E.tq;_tmp232=(void*)_tmp22E.type;_tmp233=
_tmp22E.initializer;_tmp234=_tmp22E.attributes;Cyc_Absyndump_dumploc(loc);{void*
_tmp235=Cyc_Cyclone_c_compiler;_LL1C5: if((int)_tmp235 != 0)goto _LL1C7;_LL1C6: if(
_tmp22F == (void*)3  && Cyc_Absyndump_qvar_to_Cids){void*_tmp236=Cyc_Tcutil_compress(
_tmp232);_LL1CA: if(_tmp236 <= (void*)4)goto _LL1CC;if(*((int*)_tmp236)!= 8)goto
_LL1CC;_LL1CB: goto _LL1C9;_LL1CC:;_LL1CD: Cyc_Absyndump_dumpscope(_tmp22F);_LL1C9:;}
else{Cyc_Absyndump_dumpscope(_tmp22F);}((void(*)(struct Cyc_Absyn_Tqual,void*,
void(*f)(struct _tuple0*),struct _tuple0*))Cyc_Absyndump_dumptqtd)(_tmp231,_tmp232,
Cyc_Absyndump_dumpqvar,_tmp230);Cyc_Absyndump_dumpatts(_tmp234);goto _LL1C4;
_LL1C7: if((int)_tmp235 != 1)goto _LL1C4;_LL1C8: Cyc_Absyndump_dumpatts(_tmp234);Cyc_Absyndump_dumpscope(
_tmp22F);{struct _RegionHandle _tmp237=_new_region("temp");struct _RegionHandle*
temp=& _tmp237;_push_region(temp);{struct Cyc_Absyn_Tqual _tmp239;void*_tmp23A;
struct Cyc_List_List*_tmp23B;struct _tuple5 _tmp238=Cyc_Absynpp_to_tms(temp,_tmp231,
_tmp232);_tmp239=_tmp238.f1;_tmp23A=_tmp238.f2;_tmp23B=_tmp238.f3;{void*
call_conv=(void*)8;{struct Cyc_List_List*tms2=_tmp23B;for(0;tms2 != 0;tms2=tms2->tl){
void*_tmp23C=(void*)tms2->hd;struct Cyc_List_List*_tmp23D;_LL1CF: if(*((int*)
_tmp23C)!= 5)goto _LL1D1;_tmp23D=((struct Cyc_Absyn_Attributes_mod_struct*)_tmp23C)->f2;
_LL1D0: for(0;_tmp23D != 0;_tmp23D=_tmp23D->tl){void*_tmp23E=(void*)_tmp23D->hd;
_LL1D4: if((int)_tmp23E != 0)goto _LL1D6;_LL1D5: call_conv=(void*)0;goto _LL1D3;
_LL1D6: if((int)_tmp23E != 1)goto _LL1D8;_LL1D7: call_conv=(void*)1;goto _LL1D3;
_LL1D8: if((int)_tmp23E != 2)goto _LL1DA;_LL1D9: call_conv=(void*)2;goto _LL1D3;
_LL1DA:;_LL1DB: goto _LL1D3;_LL1D3:;}goto _LL1CE;_LL1D1:;_LL1D2: goto _LL1CE;_LL1CE:;}}
Cyc_Absyndump_dumptq(_tmp239);Cyc_Absyndump_dumpntyp(_tmp23A);{struct _tuple3
_tmp3B9;struct _tuple3 _tmp23F=(_tmp3B9.f1=call_conv,((_tmp3B9.f2=_tmp230,_tmp3B9)));((
void(*)(struct Cyc_List_List*tms,void(*f)(struct _tuple3*),struct _tuple3*a))Cyc_Absyndump_dumptms)(
Cyc_List_imp_rev(_tmp23B),Cyc_Absyndump_dump_callconv_qvar,& _tmp23F);}}}
_npop_handler(0);goto _LL1C4;;_pop_region(temp);}_LL1C4:;}if(_tmp233 != 0){Cyc_Absyndump_dump_char((
int)'=');Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)_tmp233);}Cyc_Absyndump_dump_semi();}
struct _tuple12{struct Cyc_Position_Segment*f1;struct _tuple0*f2;int f3;};void Cyc_Absyndump_dumpdecl(
struct Cyc_Absyn_Decl*d);void Cyc_Absyndump_dumpdecl(struct Cyc_Absyn_Decl*d){void*
_tmp241=(void*)d->r;struct Cyc_Absyn_Vardecl*_tmp242;struct Cyc_Absyn_Fndecl*
_tmp243;struct Cyc_Absyn_Aggrdecl*_tmp244;struct Cyc_Absyn_Tuniondecl*_tmp245;
struct Cyc_Absyn_Tuniondecl _tmp246;void*_tmp247;struct _tuple0*_tmp248;struct Cyc_List_List*
_tmp249;struct Cyc_Core_Opt*_tmp24A;int _tmp24B;int _tmp24C;struct Cyc_Absyn_Enumdecl*
_tmp24D;struct Cyc_Absyn_Enumdecl _tmp24E;void*_tmp24F;struct _tuple0*_tmp250;
struct Cyc_Core_Opt*_tmp251;struct Cyc_Absyn_Pat*_tmp252;struct Cyc_Absyn_Exp*
_tmp253;struct Cyc_List_List*_tmp254;struct Cyc_Absyn_Tvar*_tmp255;struct Cyc_Absyn_Vardecl*
_tmp256;int _tmp257;struct Cyc_Absyn_Exp*_tmp258;struct Cyc_Absyn_Exp*_tmp259;
struct Cyc_Absyn_Tvar*_tmp25A;struct Cyc_Absyn_Vardecl*_tmp25B;struct Cyc_Absyn_Typedefdecl*
_tmp25C;struct _dyneither_ptr*_tmp25D;struct Cyc_List_List*_tmp25E;struct _tuple0*
_tmp25F;struct Cyc_List_List*_tmp260;struct Cyc_List_List*_tmp261;struct Cyc_List_List*
_tmp262;struct Cyc_List_List*_tmp263;_LL1DD: if(_tmp241 <= (void*)2)goto _LL1F9;if(*((
int*)_tmp241)!= 0)goto _LL1DF;_tmp242=((struct Cyc_Absyn_Var_d_struct*)_tmp241)->f1;
_LL1DE: Cyc_Absyndump_dumpvardecl(_tmp242,d->loc);goto _LL1DC;_LL1DF: if(*((int*)
_tmp241)!= 1)goto _LL1E1;_tmp243=((struct Cyc_Absyn_Fn_d_struct*)_tmp241)->f1;
_LL1E0: Cyc_Absyndump_dumploc(d->loc);{void*_tmp264=Cyc_Cyclone_c_compiler;_LL1FE:
if((int)_tmp264 != 1)goto _LL200;_LL1FF: Cyc_Absyndump_dumpatts(_tmp243->attributes);
goto _LL1FD;_LL200: if((int)_tmp264 != 0)goto _LL1FD;_LL201: goto _LL1FD;_LL1FD:;}if(
_tmp243->is_inline){void*_tmp265=Cyc_Cyclone_c_compiler;_LL203: if((int)_tmp265 != 
1)goto _LL205;_LL204:{const char*_tmp3BA;Cyc_Absyndump_dump(((_tmp3BA="__inline",
_tag_dyneither(_tmp3BA,sizeof(char),9))));}goto _LL202;_LL205:;_LL206:{const char*
_tmp3BB;Cyc_Absyndump_dump(((_tmp3BB="inline",_tag_dyneither(_tmp3BB,sizeof(char),
7))));}goto _LL202;_LL202:;}Cyc_Absyndump_dumpscope((void*)_tmp243->sc);{struct
Cyc_Absyn_FnType_struct _tmp3C1;struct Cyc_Absyn_FnInfo _tmp3C0;struct Cyc_Absyn_FnType_struct*
_tmp3BF;void*t=(void*)((_tmp3BF=_cycalloc(sizeof(*_tmp3BF)),((_tmp3BF[0]=((
_tmp3C1.tag=8,((_tmp3C1.f1=((_tmp3C0.tvars=_tmp243->tvs,((_tmp3C0.effect=_tmp243->effect,((
_tmp3C0.ret_typ=(void*)((void*)_tmp243->ret_type),((_tmp3C0.args=((struct Cyc_List_List*(*)(
struct _tuple1*(*f)(struct _tuple4*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Absynpp_arg_mk_opt,
_tmp243->args),((_tmp3C0.c_varargs=_tmp243->c_varargs,((_tmp3C0.cyc_varargs=
_tmp243->cyc_varargs,((_tmp3C0.rgn_po=_tmp243->rgn_po,((_tmp3C0.attributes=0,
_tmp3C0)))))))))))))))),_tmp3C1)))),_tmp3BF))));{void*_tmp268=Cyc_Cyclone_c_compiler;
_LL208: if((int)_tmp268 != 0)goto _LL20A;_LL209:((void(*)(struct Cyc_Absyn_Tqual,
void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))Cyc_Absyndump_dumptqtd)(
Cyc_Absyn_empty_tqual(0),t,Cyc_Absyndump_dump_atts_qvar,_tmp243);goto _LL207;
_LL20A: if((int)_tmp268 != 1)goto _LL207;_LL20B:((void(*)(struct Cyc_Absyn_Tqual,
void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))Cyc_Absyndump_dumptqtd)(
Cyc_Absyn_empty_tqual(0),t,Cyc_Absyndump_dump_callconv_fdqvar,_tmp243);goto
_LL207;_LL207:;}Cyc_Absyndump_dump_char((int)'{');Cyc_Absyndump_dumpstmt(_tmp243->body);
Cyc_Absyndump_dump_char((int)'}');goto _LL1DC;}_LL1E1: if(*((int*)_tmp241)!= 6)
goto _LL1E3;_tmp244=((struct Cyc_Absyn_Aggr_d_struct*)_tmp241)->f1;_LL1E2: Cyc_Absyndump_dumpscope((
void*)_tmp244->sc);Cyc_Absyndump_dumpaggr_kind((void*)_tmp244->kind);Cyc_Absyndump_dumpqvar(
_tmp244->name);Cyc_Absyndump_dumptvars(_tmp244->tvs);if(_tmp244->impl == 0)Cyc_Absyndump_dump_semi();
else{Cyc_Absyndump_dump_char((int)'{');if(((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp244->impl))->exist_vars != 0){const char*_tmp3C4;const char*_tmp3C3;
const char*_tmp3C2;((void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*
l,struct _dyneither_ptr start,struct _dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_egroup)(
Cyc_Absyndump_dumpkindedtvar,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp244->impl))->exist_vars,((
_tmp3C2="<",_tag_dyneither(_tmp3C2,sizeof(char),2))),((_tmp3C3=">",
_tag_dyneither(_tmp3C3,sizeof(char),2))),((_tmp3C4=",",_tag_dyneither(_tmp3C4,
sizeof(char),2))));}if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp244->impl))->rgn_po
!= 0){Cyc_Absyndump_dump_char((int)':');Cyc_Absyndump_dump_rgnpo(((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp244->impl))->rgn_po);}Cyc_Absyndump_dumpaggrfields(((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp244->impl))->fields);{const char*_tmp3C5;Cyc_Absyndump_dump(((
_tmp3C5="}",_tag_dyneither(_tmp3C5,sizeof(char),2))));}Cyc_Absyndump_dumpatts(
_tmp244->attributes);{const char*_tmp3C6;Cyc_Absyndump_dump(((_tmp3C6=";",
_tag_dyneither(_tmp3C6,sizeof(char),2))));}}goto _LL1DC;_LL1E3: if(*((int*)_tmp241)
!= 7)goto _LL1E5;_tmp245=((struct Cyc_Absyn_Tunion_d_struct*)_tmp241)->f1;_tmp246=*
_tmp245;_tmp247=(void*)_tmp246.sc;_tmp248=_tmp246.name;_tmp249=_tmp246.tvs;
_tmp24A=_tmp246.fields;_tmp24B=_tmp246.is_xtunion;_tmp24C=_tmp246.is_flat;_LL1E4:
Cyc_Absyndump_dumpscope(_tmp247);{const char*_tmp3C8;const char*_tmp3C7;Cyc_Absyndump_dump(
_tmp24B?(_tmp3C8="xtunion",_tag_dyneither(_tmp3C8,sizeof(char),8)):((_tmp3C7="tunion",
_tag_dyneither(_tmp3C7,sizeof(char),7))));}if(_tmp24C){const char*_tmp3C9;Cyc_Absyndump_dump_nospace(((
_tmp3C9=" __attribute__((flat))",_tag_dyneither(_tmp3C9,sizeof(char),23))));}Cyc_Absyndump_dumpqvar(
_tmp248);Cyc_Absyndump_dumptvars(_tmp249);if(_tmp24A == 0)Cyc_Absyndump_dump_semi();
else{Cyc_Absyndump_dump_char((int)'{');Cyc_Absyndump_dumptunionfields((struct Cyc_List_List*)
_tmp24A->v);{const char*_tmp3CA;Cyc_Absyndump_dump_nospace(((_tmp3CA="};",
_tag_dyneither(_tmp3CA,sizeof(char),3))));}}goto _LL1DC;_LL1E5: if(*((int*)_tmp241)
!= 8)goto _LL1E7;_tmp24D=((struct Cyc_Absyn_Enum_d_struct*)_tmp241)->f1;_tmp24E=*
_tmp24D;_tmp24F=(void*)_tmp24E.sc;_tmp250=_tmp24E.name;_tmp251=_tmp24E.fields;
_LL1E6: Cyc_Absyndump_dumpscope(_tmp24F);{const char*_tmp3CB;Cyc_Absyndump_dump(((
_tmp3CB="enum ",_tag_dyneither(_tmp3CB,sizeof(char),6))));}Cyc_Absyndump_dumpqvar(
_tmp250);if(_tmp251 == 0)Cyc_Absyndump_dump_semi();else{Cyc_Absyndump_dump_char((
int)'{');Cyc_Absyndump_dumpenumfields((struct Cyc_List_List*)_tmp251->v);{const
char*_tmp3CC;Cyc_Absyndump_dump_nospace(((_tmp3CC="};",_tag_dyneither(_tmp3CC,
sizeof(char),3))));}}return;_LL1E7: if(*((int*)_tmp241)!= 2)goto _LL1E9;_tmp252=((
struct Cyc_Absyn_Let_d_struct*)_tmp241)->f1;_tmp253=((struct Cyc_Absyn_Let_d_struct*)
_tmp241)->f3;_LL1E8:{const char*_tmp3CD;Cyc_Absyndump_dump(((_tmp3CD="let",
_tag_dyneither(_tmp3CD,sizeof(char),4))));}Cyc_Absyndump_dumppat(_tmp252);Cyc_Absyndump_dump_char((
int)'=');Cyc_Absyndump_dumpexp(_tmp253);Cyc_Absyndump_dump_semi();goto _LL1DC;
_LL1E9: if(*((int*)_tmp241)!= 3)goto _LL1EB;_tmp254=((struct Cyc_Absyn_Letv_d_struct*)
_tmp241)->f1;_LL1EA:{const char*_tmp3CE;Cyc_Absyndump_dump(((_tmp3CE="let ",
_tag_dyneither(_tmp3CE,sizeof(char),5))));}Cyc_Absyndump_dumpids(_tmp254);Cyc_Absyndump_dump_semi();
goto _LL1DC;_LL1EB: if(*((int*)_tmp241)!= 4)goto _LL1ED;_tmp255=((struct Cyc_Absyn_Region_d_struct*)
_tmp241)->f1;_tmp256=((struct Cyc_Absyn_Region_d_struct*)_tmp241)->f2;_tmp257=((
struct Cyc_Absyn_Region_d_struct*)_tmp241)->f3;_tmp258=((struct Cyc_Absyn_Region_d_struct*)
_tmp241)->f4;_LL1EC:{const char*_tmp3CF;Cyc_Absyndump_dump(((_tmp3CF="region",
_tag_dyneither(_tmp3CF,sizeof(char),7))));}if(_tmp257){const char*_tmp3D0;Cyc_Absyndump_dump(((
_tmp3D0="[resetable]",_tag_dyneither(_tmp3D0,sizeof(char),12))));}{const char*
_tmp3D1;Cyc_Absyndump_dump(((_tmp3D1="<",_tag_dyneither(_tmp3D1,sizeof(char),2))));}
Cyc_Absyndump_dumptvar(_tmp255);{const char*_tmp3D2;Cyc_Absyndump_dump(((_tmp3D2="> ",
_tag_dyneither(_tmp3D2,sizeof(char),3))));}Cyc_Absyndump_dumpqvar(_tmp256->name);
if(_tmp258 != 0){{const char*_tmp3D3;Cyc_Absyndump_dump(((_tmp3D3=" = open(",
_tag_dyneither(_tmp3D3,sizeof(char),9))));}Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)
_tmp258);{const char*_tmp3D4;Cyc_Absyndump_dump(((_tmp3D4=")",_tag_dyneither(
_tmp3D4,sizeof(char),2))));}}Cyc_Absyndump_dump_semi();goto _LL1DC;_LL1ED: if(*((
int*)_tmp241)!= 5)goto _LL1EF;_tmp259=((struct Cyc_Absyn_Alias_d_struct*)_tmp241)->f1;
_tmp25A=((struct Cyc_Absyn_Alias_d_struct*)_tmp241)->f2;_tmp25B=((struct Cyc_Absyn_Alias_d_struct*)
_tmp241)->f3;_LL1EE:{const char*_tmp3D5;Cyc_Absyndump_dump(((_tmp3D5="alias ",
_tag_dyneither(_tmp3D5,sizeof(char),7))));}{const char*_tmp3D6;Cyc_Absyndump_dump(((
_tmp3D6="<",_tag_dyneither(_tmp3D6,sizeof(char),2))));}Cyc_Absyndump_dumptvar(
_tmp25A);{const char*_tmp3D7;Cyc_Absyndump_dump(((_tmp3D7=">",_tag_dyneither(
_tmp3D7,sizeof(char),2))));}Cyc_Absyndump_dumpqvar(_tmp25B->name);{const char*
_tmp3D8;Cyc_Absyndump_dump(((_tmp3D8=" = ",_tag_dyneither(_tmp3D8,sizeof(char),4))));}
Cyc_Absyndump_dumpexp(_tmp259);Cyc_Absyndump_dump_semi();goto _LL1DC;_LL1EF: if(*((
int*)_tmp241)!= 9)goto _LL1F1;_tmp25C=((struct Cyc_Absyn_Typedef_d_struct*)_tmp241)->f1;
_LL1F0: if(!Cyc_Absyndump_expand_typedefs  || _tmp25C->defn != 0  && Cyc_Absynpp_is_anon_aggrtype((
void*)((struct Cyc_Core_Opt*)_check_null(_tmp25C->defn))->v)){{const char*_tmp3D9;
Cyc_Absyndump_dump(((_tmp3D9="typedef",_tag_dyneither(_tmp3D9,sizeof(char),8))));}{
void*t;if(_tmp25C->defn == 0)t=Cyc_Absyn_new_evar(_tmp25C->kind,0);else{t=(void*)((
struct Cyc_Core_Opt*)_check_null(_tmp25C->defn))->v;}((void(*)(struct Cyc_Absyn_Tqual,
void*,void(*f)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*))Cyc_Absyndump_dumptqtd)(
_tmp25C->tq,t,Cyc_Absyndump_dumptypedefname,_tmp25C);Cyc_Absyndump_dumpatts(
_tmp25C->atts);Cyc_Absyndump_dump_semi();}}goto _LL1DC;_LL1F1: if(*((int*)_tmp241)
!= 10)goto _LL1F3;_tmp25D=((struct Cyc_Absyn_Namespace_d_struct*)_tmp241)->f1;
_tmp25E=((struct Cyc_Absyn_Namespace_d_struct*)_tmp241)->f2;_LL1F2:{const char*
_tmp3DA;Cyc_Absyndump_dump(((_tmp3DA="namespace",_tag_dyneither(_tmp3DA,sizeof(
char),10))));}Cyc_Absyndump_dump_str(_tmp25D);Cyc_Absyndump_dump_char((int)'{');
for(0;_tmp25E != 0;_tmp25E=_tmp25E->tl){Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)
_tmp25E->hd);}Cyc_Absyndump_dump_char((int)'}');goto _LL1DC;_LL1F3: if(*((int*)
_tmp241)!= 11)goto _LL1F5;_tmp25F=((struct Cyc_Absyn_Using_d_struct*)_tmp241)->f1;
_tmp260=((struct Cyc_Absyn_Using_d_struct*)_tmp241)->f2;_LL1F4:{const char*_tmp3DB;
Cyc_Absyndump_dump(((_tmp3DB="using",_tag_dyneither(_tmp3DB,sizeof(char),6))));}
Cyc_Absyndump_dumpqvar(_tmp25F);Cyc_Absyndump_dump_char((int)'{');for(0;_tmp260
!= 0;_tmp260=_tmp260->tl){Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)_tmp260->hd);}
Cyc_Absyndump_dump_char((int)'}');goto _LL1DC;_LL1F5: if(*((int*)_tmp241)!= 12)
goto _LL1F7;_tmp261=((struct Cyc_Absyn_ExternC_d_struct*)_tmp241)->f1;_LL1F6:{
const char*_tmp3DC;Cyc_Absyndump_dump(((_tmp3DC="extern \"C\" {",_tag_dyneither(
_tmp3DC,sizeof(char),13))));}for(0;_tmp261 != 0;_tmp261=_tmp261->tl){Cyc_Absyndump_dumpdecl((
struct Cyc_Absyn_Decl*)_tmp261->hd);}Cyc_Absyndump_dump_char((int)'}');goto _LL1DC;
_LL1F7: if(*((int*)_tmp241)!= 13)goto _LL1F9;_tmp262=((struct Cyc_Absyn_ExternCinclude_d_struct*)
_tmp241)->f1;_tmp263=((struct Cyc_Absyn_ExternCinclude_d_struct*)_tmp241)->f2;
_LL1F8:{const char*_tmp3DD;Cyc_Absyndump_dump(((_tmp3DD="extern \"C include\" {",
_tag_dyneither(_tmp3DD,sizeof(char),21))));}for(0;_tmp262 != 0;_tmp262=_tmp262->tl){
Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)_tmp262->hd);}Cyc_Absyndump_dump_char((
int)'}');if(_tmp263 != 0){{const char*_tmp3DE;Cyc_Absyndump_dump(((_tmp3DE=" export {",
_tag_dyneither(_tmp3DE,sizeof(char),10))));}for(0;_tmp263 != 0;_tmp263=_tmp263->tl){
struct _tuple0*_tmp28A;struct _tuple12 _tmp289=*((struct _tuple12*)_tmp263->hd);
_tmp28A=_tmp289.f2;Cyc_Absyndump_dumpqvar(_tmp28A);if(_tmp263->tl != 0)Cyc_Absyndump_dump_char((
int)',');}{const char*_tmp3DF;Cyc_Absyndump_dump(((_tmp3DF="}",_tag_dyneither(
_tmp3DF,sizeof(char),2))));}}goto _LL1DC;_LL1F9: if((int)_tmp241 != 0)goto _LL1FB;
_LL1FA:{const char*_tmp3E0;Cyc_Absyndump_dump(((_tmp3E0=" __cyclone_port_on__; ",
_tag_dyneither(_tmp3E0,sizeof(char),23))));}goto _LL1DC;_LL1FB: if((int)_tmp241 != 
1)goto _LL1DC;_LL1FC:{const char*_tmp3E1;Cyc_Absyndump_dump(((_tmp3E1=" __cyclone_port_off__; ",
_tag_dyneither(_tmp3E1,sizeof(char),24))));}goto _LL1DC;_LL1DC:;}static void Cyc_Absyndump_dump_upperbound(
struct Cyc_Absyn_Exp*e);static void Cyc_Absyndump_dump_upperbound(struct Cyc_Absyn_Exp*
e){struct _tuple6 pr=Cyc_Evexp_eval_const_uint_exp(e);if(pr.f1 != 1  || !pr.f2){Cyc_Absyndump_dump_char((
int)'{');Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_char((int)'}');}}void Cyc_Absyndump_dumptms(
struct Cyc_List_List*tms,void(*f)(void*),void*a);void Cyc_Absyndump_dumptms(struct
Cyc_List_List*tms,void(*f)(void*),void*a){if(tms == 0){f(a);return;}{void*_tmp28E=(
void*)tms->hd;struct Cyc_Absyn_PtrAtts _tmp28F;void*_tmp290;struct Cyc_Absyn_Conref*
_tmp291;struct Cyc_Absyn_Conref*_tmp292;struct Cyc_Absyn_Conref*_tmp293;struct Cyc_Absyn_Tqual
_tmp294;_LL20D: if(*((int*)_tmp28E)!= 2)goto _LL20F;_tmp28F=((struct Cyc_Absyn_Pointer_mod_struct*)
_tmp28E)->f1;_tmp290=(void*)_tmp28F.rgn;_tmp291=_tmp28F.nullable;_tmp292=_tmp28F.bounds;
_tmp293=_tmp28F.zero_term;_tmp294=((struct Cyc_Absyn_Pointer_mod_struct*)_tmp28E)->f2;
_LL20E:{void*_tmp295=Cyc_Absyn_conref_def(Cyc_Absyn_bounds_one,_tmp292);struct
Cyc_Absyn_Exp*_tmp296;_LL212: if((int)_tmp295 != 0)goto _LL214;_LL213: Cyc_Absyndump_dump_char((
int)'?');goto _LL211;_LL214: if(_tmp295 <= (void*)1)goto _LL211;if(*((int*)_tmp295)
!= 0)goto _LL211;_tmp296=((struct Cyc_Absyn_Upper_b_struct*)_tmp295)->f1;_LL215:
Cyc_Absyndump_dump_char((int)(((int(*)(int,struct Cyc_Absyn_Conref*x))Cyc_Absyn_conref_def)(
1,_tmp291)?'*':'@'));Cyc_Absyndump_dump_upperbound(_tmp296);goto _LL211;_LL211:;}
if(((int(*)(int,struct Cyc_Absyn_Conref*x))Cyc_Absyn_conref_def)(0,_tmp293)){
const char*_tmp3E2;Cyc_Absyndump_dump(((_tmp3E2="ZEROTERM",_tag_dyneither(_tmp3E2,
sizeof(char),9))));}{void*_tmp298=Cyc_Tcutil_compress(_tmp290);struct Cyc_Absyn_Tvar*
_tmp299;struct Cyc_Core_Opt*_tmp29A;_LL217: if((int)_tmp298 != 2)goto _LL219;_LL218:
goto _LL216;_LL219: if(_tmp298 <= (void*)4)goto _LL21D;if(*((int*)_tmp298)!= 1)goto
_LL21B;_tmp299=((struct Cyc_Absyn_VarType_struct*)_tmp298)->f1;_LL21A: Cyc_Absyndump_dump_str(
_tmp299->name);goto _LL216;_LL21B: if(*((int*)_tmp298)!= 0)goto _LL21D;_tmp29A=((
struct Cyc_Absyn_Evar_struct*)_tmp298)->f2;if(_tmp29A != 0)goto _LL21D;_LL21C: Cyc_Absyndump_dumpntyp(
Cyc_Tcutil_compress(_tmp290));goto _LL216;_LL21D:;_LL21E: {const char*_tmp3E5;void*
_tmp3E4;(_tmp3E4=0,Cyc_Tcutil_impos(((_tmp3E5="dumptms: bad rgn type in Pointer_mod",
_tag_dyneither(_tmp3E5,sizeof(char),37))),_tag_dyneither(_tmp3E4,sizeof(void*),0)));}
_LL216:;}Cyc_Absyndump_dumptq(_tmp294);Cyc_Absyndump_dumptms(tms->tl,f,a);
return;_LL20F:;_LL210: {int next_is_pointer=0;if(tms->tl != 0){void*_tmp29D=(void*)((
struct Cyc_List_List*)_check_null(tms->tl))->hd;_LL220: if(*((int*)_tmp29D)!= 2)
goto _LL222;_LL221: next_is_pointer=1;goto _LL21F;_LL222:;_LL223: goto _LL21F;_LL21F:;}
if(next_is_pointer)Cyc_Absyndump_dump_char((int)'(');Cyc_Absyndump_dumptms(tms->tl,
f,a);if(next_is_pointer)Cyc_Absyndump_dump_char((int)')');{void*_tmp29E=(void*)
tms->hd;struct Cyc_Absyn_Conref*_tmp29F;struct Cyc_Absyn_Exp*_tmp2A0;struct Cyc_Absyn_Conref*
_tmp2A1;void*_tmp2A2;struct Cyc_List_List*_tmp2A3;int _tmp2A4;struct Cyc_Absyn_VarargInfo*
_tmp2A5;struct Cyc_Core_Opt*_tmp2A6;struct Cyc_List_List*_tmp2A7;void*_tmp2A8;
struct Cyc_List_List*_tmp2A9;struct Cyc_Position_Segment*_tmp2AA;struct Cyc_List_List*
_tmp2AB;struct Cyc_Position_Segment*_tmp2AC;int _tmp2AD;struct Cyc_List_List*
_tmp2AE;_LL225: if(*((int*)_tmp29E)!= 0)goto _LL227;_tmp29F=((struct Cyc_Absyn_Carray_mod_struct*)
_tmp29E)->f1;_LL226:{const char*_tmp3E6;Cyc_Absyndump_dump(((_tmp3E6="[]",
_tag_dyneither(_tmp3E6,sizeof(char),3))));}if(((int(*)(int,struct Cyc_Absyn_Conref*
x))Cyc_Absyn_conref_def)(0,_tmp29F)){const char*_tmp3E7;Cyc_Absyndump_dump(((
_tmp3E7="ZEROTERM",_tag_dyneither(_tmp3E7,sizeof(char),9))));}goto _LL224;_LL227:
if(*((int*)_tmp29E)!= 1)goto _LL229;_tmp2A0=((struct Cyc_Absyn_ConstArray_mod_struct*)
_tmp29E)->f1;_tmp2A1=((struct Cyc_Absyn_ConstArray_mod_struct*)_tmp29E)->f2;
_LL228: Cyc_Absyndump_dump_char((int)'[');Cyc_Absyndump_dumpexp(_tmp2A0);Cyc_Absyndump_dump_char((
int)']');if(((int(*)(int,struct Cyc_Absyn_Conref*x))Cyc_Absyn_conref_def)(0,
_tmp2A1)){const char*_tmp3E8;Cyc_Absyndump_dump(((_tmp3E8="ZEROTERM",
_tag_dyneither(_tmp3E8,sizeof(char),9))));}goto _LL224;_LL229: if(*((int*)_tmp29E)
!= 3)goto _LL22B;_tmp2A2=(void*)((struct Cyc_Absyn_Function_mod_struct*)_tmp29E)->f1;
if(*((int*)_tmp2A2)!= 1)goto _LL22B;_tmp2A3=((struct Cyc_Absyn_WithTypes_struct*)
_tmp2A2)->f1;_tmp2A4=((struct Cyc_Absyn_WithTypes_struct*)_tmp2A2)->f2;_tmp2A5=((
struct Cyc_Absyn_WithTypes_struct*)_tmp2A2)->f3;_tmp2A6=((struct Cyc_Absyn_WithTypes_struct*)
_tmp2A2)->f4;_tmp2A7=((struct Cyc_Absyn_WithTypes_struct*)_tmp2A2)->f5;_LL22A: Cyc_Absyndump_dumpfunargs(
_tmp2A3,_tmp2A4,_tmp2A5,_tmp2A6,_tmp2A7);goto _LL224;_LL22B: if(*((int*)_tmp29E)!= 
3)goto _LL22D;_tmp2A8=(void*)((struct Cyc_Absyn_Function_mod_struct*)_tmp29E)->f1;
if(*((int*)_tmp2A8)!= 0)goto _LL22D;_tmp2A9=((struct Cyc_Absyn_NoTypes_struct*)
_tmp2A8)->f1;_tmp2AA=((struct Cyc_Absyn_NoTypes_struct*)_tmp2A8)->f2;_LL22C:{
const char*_tmp3EB;const char*_tmp3EA;const char*_tmp3E9;((void(*)(void(*f)(struct
_dyneither_ptr*),struct Cyc_List_List*l,struct _dyneither_ptr start,struct
_dyneither_ptr end,struct _dyneither_ptr sep))Cyc_Absyndump_group)(Cyc_Absyndump_dump_str,
_tmp2A9,((_tmp3E9="(",_tag_dyneither(_tmp3E9,sizeof(char),2))),((_tmp3EA=")",
_tag_dyneither(_tmp3EA,sizeof(char),2))),((_tmp3EB=",",_tag_dyneither(_tmp3EB,
sizeof(char),2))));}goto _LL224;_LL22D: if(*((int*)_tmp29E)!= 4)goto _LL22F;_tmp2AB=((
struct Cyc_Absyn_TypeParams_mod_struct*)_tmp29E)->f1;_tmp2AC=((struct Cyc_Absyn_TypeParams_mod_struct*)
_tmp29E)->f2;_tmp2AD=((struct Cyc_Absyn_TypeParams_mod_struct*)_tmp29E)->f3;
_LL22E: if(_tmp2AD)Cyc_Absyndump_dumpkindedtvars(_tmp2AB);else{Cyc_Absyndump_dumptvars(
_tmp2AB);}goto _LL224;_LL22F: if(*((int*)_tmp29E)!= 5)goto _LL231;_tmp2AE=((struct
Cyc_Absyn_Attributes_mod_struct*)_tmp29E)->f2;_LL230: Cyc_Absyndump_dumpatts(
_tmp2AE);goto _LL224;_LL231: if(*((int*)_tmp29E)!= 2)goto _LL224;_LL232: {const char*
_tmp3EE;void*_tmp3ED;(_tmp3ED=0,Cyc_Tcutil_impos(((_tmp3EE="dumptms",
_tag_dyneither(_tmp3EE,sizeof(char),8))),_tag_dyneither(_tmp3ED,sizeof(void*),0)));}
_LL224:;}return;}_LL20C:;}}void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual tq,
void*t,void(*f)(void*),void*a);void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual
tq,void*t,void(*f)(void*),void*a){struct _RegionHandle _tmp2B7=_new_region("temp");
struct _RegionHandle*temp=& _tmp2B7;_push_region(temp);{struct Cyc_Absyn_Tqual
_tmp2B9;void*_tmp2BA;struct Cyc_List_List*_tmp2BB;struct _tuple5 _tmp2B8=Cyc_Absynpp_to_tms(
temp,tq,t);_tmp2B9=_tmp2B8.f1;_tmp2BA=_tmp2B8.f2;_tmp2BB=_tmp2B8.f3;Cyc_Absyndump_dumptq(
_tmp2B9);Cyc_Absyndump_dumpntyp(_tmp2BA);Cyc_Absyndump_dumptms(Cyc_List_imp_rev(
_tmp2BB),f,a);};_pop_region(temp);}void Cyc_Absyndump_dumpdecllist2file(struct Cyc_List_List*
tdl,struct Cyc___cycFILE*f);void Cyc_Absyndump_dumpdecllist2file(struct Cyc_List_List*
tdl,struct Cyc___cycFILE*f){Cyc_Absyndump_pos=0;*Cyc_Absyndump_dump_file=f;for(0;
tdl != 0;tdl=tdl->tl){Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)tdl->hd);}{
const char*_tmp3F1;void*_tmp3F0;(_tmp3F0=0,Cyc_fprintf(f,((_tmp3F1="\n",
_tag_dyneither(_tmp3F1,sizeof(char),2))),_tag_dyneither(_tmp3F0,sizeof(void*),0)));}}