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
struct _RegionHandle*Cyc_Core_heap_region;extern char Cyc_Core_Open_Region[16];
extern char Cyc_Core_Free_Region[16];struct Cyc___cycFILE;extern struct Cyc___cycFILE*
Cyc_stderr;struct Cyc_Cstdio___abstractFILE;struct Cyc_String_pa_struct{int tag;
struct _dyneither_ptr f1;};struct Cyc_Int_pa_struct{int tag;unsigned long f1;};struct
Cyc_Double_pa_struct{int tag;double f1;};struct Cyc_LongDouble_pa_struct{int tag;
long double f1;};struct Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{
int tag;unsigned long*f1;};struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,
struct _dyneither_ptr);int Cyc_fflush(struct Cyc___cycFILE*);int Cyc_fprintf(struct
Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_struct{
int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{int tag;unsigned short*f1;};
struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{int tag;
unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
extern char Cyc_FileCloseError[19];extern char Cyc_FileOpenError[18];struct Cyc_FileOpenError_struct{
char*tag;struct _dyneither_ptr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*
tl;};extern char Cyc_List_List_mismatch[18];struct Cyc_List_List*Cyc_List_rev(
struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*
x);struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*
y);extern char Cyc_List_Nth[8];struct Cyc_Iter_Iter{void*env;int(*next)(void*env,
void*dest);};int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Dict_T;struct
Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;struct Cyc_Dict_T*t;};
extern char Cyc_Dict_Present[12];extern char Cyc_Dict_Absent[11];struct Cyc_Dict_Dict
Cyc_Dict_rempty(struct _RegionHandle*,int(*cmp)(void*,void*));int Cyc_Dict_is_empty(
struct Cyc_Dict_Dict d);int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);struct Cyc_Dict_Dict
Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);void*Cyc_Dict_lookup(struct
Cyc_Dict_Dict d,void*k);void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);
struct _tuple0{void*f1;void*f2;};struct _tuple0*Cyc_Dict_rchoose(struct
_RegionHandle*r,struct Cyc_Dict_Dict d);struct _tuple0*Cyc_Dict_rchoose(struct
_RegionHandle*,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct
_RegionHandle*,struct Cyc_Dict_Dict,void*);struct Cyc_Set_Set;struct Cyc_Set_Set*
Cyc_Set_empty(int(*cmp)(void*,void*));struct Cyc_Set_Set*Cyc_Set_rempty(struct
_RegionHandle*r,int(*cmp)(void*,void*));int Cyc_Set_member(struct Cyc_Set_Set*s,
void*elt);extern char Cyc_Set_Absent[11];struct Cyc_SlowDict_Dict;extern char Cyc_SlowDict_Present[
12];extern char Cyc_SlowDict_Absent[11];struct Cyc_Lineno_Pos{struct _dyneither_ptr
logical_file;struct _dyneither_ptr line;int line_no;int col;};extern char Cyc_Position_Exit[
9];struct Cyc_Position_Segment;struct Cyc_Position_Error{struct _dyneither_ptr
source;struct Cyc_Position_Segment*seg;void*kind;struct _dyneither_ptr desc;};
extern char Cyc_Position_Nocontext[14];int Cyc_strptrcmp(struct _dyneither_ptr*s1,
struct _dyneither_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
struct Cyc_Absyn_Loc_n_struct{int tag;};struct Cyc_Absyn_Rel_n_struct{int tag;struct
Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{int tag;struct Cyc_List_List*f1;};
union Cyc_Absyn_Nmspace_union{struct Cyc_Absyn_Loc_n_struct Loc_n;struct Cyc_Absyn_Rel_n_struct
Rel_n;struct Cyc_Absyn_Abs_n_struct Abs_n;};struct _tuple1{union Cyc_Absyn_Nmspace_union
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
Cyc_Absyn_UnknownTunionInfo{struct _tuple1*name;int is_xtunion;int is_flat;};struct
Cyc_Absyn_UnknownTunion_struct{int tag;struct Cyc_Absyn_UnknownTunionInfo f1;};
struct Cyc_Absyn_KnownTunion_struct{int tag;struct Cyc_Absyn_Tuniondecl**f1;};union
Cyc_Absyn_TunionInfoU_union{struct Cyc_Absyn_UnknownTunion_struct UnknownTunion;
struct Cyc_Absyn_KnownTunion_struct KnownTunion;};struct Cyc_Absyn_TunionInfo{union
Cyc_Absyn_TunionInfoU_union tunion_info;struct Cyc_List_List*targs;struct Cyc_Core_Opt*
rgn;};struct Cyc_Absyn_UnknownTunionFieldInfo{struct _tuple1*tunion_name;struct
_tuple1*field_name;int is_xtunion;};struct Cyc_Absyn_UnknownTunionfield_struct{int
tag;struct Cyc_Absyn_UnknownTunionFieldInfo f1;};struct Cyc_Absyn_KnownTunionfield_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};union Cyc_Absyn_TunionFieldInfoU_union{
struct Cyc_Absyn_UnknownTunionfield_struct UnknownTunionfield;struct Cyc_Absyn_KnownTunionfield_struct
KnownTunionfield;};struct Cyc_Absyn_TunionFieldInfo{union Cyc_Absyn_TunionFieldInfoU_union
field_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownAggr_struct{int tag;
void*f1;struct _tuple1*f2;};struct Cyc_Absyn_KnownAggr_struct{int tag;struct Cyc_Absyn_Aggrdecl**
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
int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*
f1;};struct Cyc_Absyn_DynRgnType_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
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
_tuple1*f1;void*f2;};struct Cyc_Absyn_UnknownId_e_struct{int tag;struct _tuple1*f1;
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
f1;};struct _tuple2{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual f2;void*f3;};
struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;
int f4;};struct Cyc_Absyn_Struct_e_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*
f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Tunion_e_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*f2;struct Cyc_Absyn_Tunionfield*
f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*
f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{int tag;
struct _tuple1*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Valueof_e_struct{int tag;void*f1;
};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;void*r;struct Cyc_Position_Segment*
loc;void*annot;};struct Cyc_Absyn_Exp_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};
struct _tuple3{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_struct{
int tag;struct _tuple3 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{int tag;struct
Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct _dyneither_ptr*f1;
struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _tuple3 f2;struct _tuple3 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Fallthru_s_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_struct{
int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_struct{
int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple3 f2;};struct Cyc_Absyn_TryCatch_s_struct{
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
f2;};struct Cyc_Absyn_UnknownId_p_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_struct{
int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;struct Cyc_Core_Opt*
topt;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*
pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*
body;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Global_b_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Param_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Local_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{void*sc;struct
_tuple1*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;
struct Cyc_Core_Opt*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{
void*sc;int is_inline;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
effect;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;struct Cyc_Core_Opt*
cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;
struct Cyc_List_List*attributes;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*
name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*
attributes;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct
Cyc_List_List*rgn_po;struct Cyc_List_List*fields;};struct Cyc_Absyn_Aggrdecl{void*
kind;void*sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*
impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Tunionfield{struct _tuple1*
name;struct Cyc_List_List*typs;struct Cyc_Position_Segment*loc;void*sc;};struct Cyc_Absyn_Tuniondecl{
void*sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int
is_xtunion;int is_flat;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*
tag;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Enumdecl{void*sc;struct
_tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct
_tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
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
f2;};struct Cyc_Absyn_Using_d_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ExternC_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*
r;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];int Cyc_Absyn_varlist_cmp(
struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(
struct Cyc_Position_Segment*);extern struct Cyc_Absyn_Conref*Cyc_Absyn_false_conref;
extern void*Cyc_Absyn_empty_effect;extern struct _tuple1*Cyc_Absyn_exn_name;extern
struct Cyc_Absyn_Tuniondecl*Cyc_Absyn_exn_tud;void*Cyc_Absyn_dyneither_typ(void*t,
void*rgn,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Conref*zero_term);struct Cyc_Absynpp_Params{
int expand_typedefs: 1;int qvar_to_Cids: 1;int add_cyc_prefix: 1;int to_VC: 1;int
decls_first: 1;int rewrite_temp_tvars: 1;int print_all_tvars: 1;int print_all_kinds: 1;
int print_all_effects: 1;int print_using_stmts: 1;int print_externC_stmts: 1;int
print_full_evars: 1;int print_zeroterm: 1;int generate_line_directives: 1;int
use_curr_namespace: 1;struct Cyc_List_List*curr_namespace;};struct _dyneither_ptr
Cyc_Absynpp_typ2string(void*);struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*
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
le;int allow_valueof;};void*Cyc_Tcenv_env_err(struct _dyneither_ptr msg);struct
_RegionHandle*Cyc_Tcenv_get_fnrgn(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_tc_init(struct _RegionHandle*);struct Cyc_Tcenv_Genv*Cyc_Tcenv_empty_genv(
struct _RegionHandle*);struct Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(struct
_RegionHandle*,struct Cyc_Position_Segment*,struct Cyc_Absyn_Fndecl*);struct Cyc_Tcenv_Fenv*
Cyc_Tcenv_nested_fenv(struct Cyc_Position_Segment*,struct Cyc_Tcenv_Fenv*old_fenv,
struct Cyc_Absyn_Fndecl*new_fn);struct Cyc_List_List*Cyc_Tcenv_resolve_namespace(
struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,struct _dyneither_ptr*,struct
Cyc_List_List*);void*Cyc_Tcenv_lookup_ordinary(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,
struct Cyc_Position_Segment*,struct _tuple1*);struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(
struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,struct _tuple1*);struct Cyc_Absyn_Tuniondecl**
Cyc_Tcenv_lookup_tuniondecl(struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,
struct _tuple1*);struct Cyc_Absyn_Tuniondecl***Cyc_Tcenv_lookup_xtuniondecl(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,struct _tuple1*);
struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*,struct
Cyc_Position_Segment*,struct _tuple1*);struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(
struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,struct _tuple1*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_allow_valueof(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);void*Cyc_Tcenv_return_typ(
struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_local_var(
struct _RegionHandle*,struct Cyc_Position_Segment*,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Vardecl*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_pat_var(struct _RegionHandle*,struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Vardecl*);struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(
struct Cyc_Tcenv_Tenv*);struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct
Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(struct
_RegionHandle*,struct Cyc_Position_Segment*,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_loop(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*
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
Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_process_continue(struct Cyc_Tcenv_Tenv*,struct
Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt**);void Cyc_Tcenv_process_break(struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt**);void Cyc_Tcenv_process_goto(struct
Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,struct _dyneither_ptr*,struct Cyc_Absyn_Stmt**);
struct _tuple4{struct Cyc_Absyn_Switch_clause*f1;struct Cyc_List_List*f2;struct Cyc_Tcenv_CList*
f3;};struct _tuple4*Cyc_Tcenv_process_fallthru(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,
struct Cyc_Absyn_Switch_clause***);struct Cyc_Absyn_Stmt*Cyc_Tcenv_get_encloser(
struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_encloser(struct
_RegionHandle*,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_label(struct Cyc_Tcenv_Tenv*,struct _dyneither_ptr*,struct Cyc_Absyn_Stmt*);
int Cyc_Tcenv_all_labels_resolved(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(
struct _RegionHandle*,struct Cyc_Position_Segment*,struct Cyc_Tcenv_Tenv*);struct
Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(struct _RegionHandle*,struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Tvar*name);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,struct Cyc_Position_Segment*
loc);void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(
struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,void*r,int resetable,int opened);void
Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,
void*rgn);void Cyc_Tcenv_check_rgn_resetable(struct Cyc_Tcenv_Tenv*,struct Cyc_Position_Segment*,
void*rgn);void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,struct
Cyc_Position_Segment*loc,void*eff);int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*,
void*r1,void*r2);void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,struct Cyc_List_List*po);void Cyc_Tcenv_check_delayed_effects(
struct Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*
te);void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);void
Cyc_Tcutil_terr(struct Cyc_Position_Segment*,struct _dyneither_ptr fmt,struct
_dyneither_ptr ap);extern struct Cyc_Core_Opt*Cyc_Tcutil_empty_var_set;void*Cyc_Tcutil_tvar_kind(
struct Cyc_Absyn_Tvar*t);void*Cyc_Tcutil_compress(void*t);int Cyc_Tcutil_subset_effect(
int may_constrain_evars,void*e1,void*e2);int Cyc_Tcutil_region_in_effect(int
constrain,void*r,void*e);void Cyc_Tcutil_check_unique_tvars(struct Cyc_Position_Segment*,
struct Cyc_List_List*);struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k);int Cyc_Tcutil_new_tvar_id();
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*);void Cyc_Tcutil_add_tvar_identities(
struct Cyc_List_List*);char Cyc_Tcenv_Env_error[14]="\000\000\000\000Env_error\000";
void*Cyc_Tcenv_env_err(struct _dyneither_ptr msg);void*Cyc_Tcenv_env_err(struct
_dyneither_ptr msg){{const char*_tmp2D6;void*_tmp2D5[1];struct Cyc_String_pa_struct
_tmp2D4;(_tmp2D4.tag=0,((_tmp2D4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
msg),((_tmp2D5[0]=& _tmp2D4,Cyc_fprintf(Cyc_stderr,((_tmp2D6="Internal error in tcenv: %s\n",
_tag_dyneither(_tmp2D6,sizeof(char),29))),_tag_dyneither(_tmp2D5,sizeof(void*),1)))))));}
Cyc_fflush((struct Cyc___cycFILE*)Cyc_stderr);(int)_throw((void*)Cyc_Tcenv_Env_error);}
struct Cyc_Tcenv_Tenv;struct Cyc_Tcenv_Genv;struct Cyc_Tcenv_CtrlEnv{struct
_RegionHandle*ctrl_rgn;void*continue_stmt;void*break_stmt;struct _tuple4*
fallthru_clause;void*next_stmt;int try_depth;};struct Cyc_Tcenv_SharedFenv{struct
_RegionHandle*frgn;void*return_typ;struct Cyc_Dict_Dict seen_labels;struct Cyc_Dict_Dict
needed_labels;struct Cyc_List_List*delayed_effect_checks;struct Cyc_List_List*
delayed_constraint_checks;};struct Cyc_Tcenv_Bindings{struct _dyneither_ptr*v;void*
b;struct Cyc_Tcenv_Bindings*tl;};struct Cyc_Tcenv_Fenv{struct Cyc_Tcenv_SharedFenv*
shared;struct Cyc_List_List*type_vars;struct Cyc_RgnOrder_RgnPO*region_order;
struct Cyc_Tcenv_Bindings*locals;struct Cyc_Absyn_Stmt*encloser;struct Cyc_Tcenv_CtrlEnv*
ctrl_env;void*capability;void*curr_rgn;int in_notreadctxt;struct _RegionHandle*
fnrgn;};char Cyc_Tcenv_NoBinding[14]="\000\000\000\000NoBinding\000";void*Cyc_Tcenv_lookup_binding(
struct Cyc_Tcenv_Bindings*bs,struct _dyneither_ptr*v);void*Cyc_Tcenv_lookup_binding(
struct Cyc_Tcenv_Bindings*bs,struct _dyneither_ptr*v){for(0;(unsigned int)bs;bs=bs->tl){
if(Cyc_strptrcmp(v,bs->v)== 0)return(void*)bs->b;}(int)_throw((void*)Cyc_Tcenv_NoBinding);}
struct Cyc_Tcenv_Genv*Cyc_Tcenv_empty_genv(struct _RegionHandle*r);struct Cyc_Tcenv_Genv*
Cyc_Tcenv_empty_genv(struct _RegionHandle*r){struct Cyc_Tcenv_Genv*_tmp2D7;return(
_tmp2D7=_region_malloc(r,sizeof(*_tmp2D7)),((_tmp2D7->grgn=r,((_tmp2D7->namespaces=((
struct Cyc_Set_Set*(*)(struct _RegionHandle*r,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Set_rempty)(r,Cyc_strptrcmp),((_tmp2D7->aggrdecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp2D7->tuniondecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp2D7->enumdecls=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp2D7->typedefs=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp2D7->ordinaries=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,
struct _dyneither_ptr*)))Cyc_Dict_rempty)(r,Cyc_strptrcmp),((_tmp2D7->availables=
0,_tmp2D7)))))))))))))))));}struct _tuple5{void*f1;int f2;};struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_tc_init(struct _RegionHandle*r);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init(
struct _RegionHandle*r){{struct Cyc_Core_Opt*_tmp2D8;Cyc_Tcutil_empty_var_set=((
_tmp2D8=_cycalloc(sizeof(*_tmp2D8)),((_tmp2D8->v=((struct Cyc_Set_Set*(*)(int(*
cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),
_tmp2D8))));}{struct Cyc_Tcenv_Genv*_tmp5=Cyc_Tcenv_empty_genv(r);{struct Cyc_Absyn_Tuniondecl**
_tmp2D9;_tmp5->tuniondecls=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k,struct Cyc_Absyn_Tuniondecl**v))Cyc_Dict_insert)(_tmp5->tuniondecls,(*
Cyc_Absyn_exn_name).f2,((_tmp2D9=_cycalloc(sizeof(*_tmp2D9)),((_tmp2D9[0]=Cyc_Absyn_exn_tud,
_tmp2D9)))));}{struct Cyc_List_List*_tmp7=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)
_check_null(Cyc_Absyn_exn_tud->fields))->v;for(0;_tmp7 != 0;_tmp7=_tmp7->tl){
struct Cyc_Tcenv_TunionRes_struct*_tmp2DF;struct Cyc_Tcenv_TunionRes_struct _tmp2DE;
struct _tuple5*_tmp2DD;_tmp5->ordinaries=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict
d,struct _dyneither_ptr*k,struct _tuple5*v))Cyc_Dict_insert)(_tmp5->ordinaries,(*((
struct Cyc_Absyn_Tunionfield*)_tmp7->hd)->name).f2,((_tmp2DD=_region_malloc(r,
sizeof(*_tmp2DD)),((_tmp2DD->f1=(void*)((_tmp2DF=_cycalloc(sizeof(*_tmp2DF)),((
_tmp2DF[0]=((_tmp2DE.tag=2,((_tmp2DE.f1=Cyc_Absyn_exn_tud,((_tmp2DE.f2=(struct
Cyc_Absyn_Tunionfield*)_tmp7->hd,_tmp2DE)))))),_tmp2DF)))),((_tmp2DD->f2=1,
_tmp2DD)))))));}}{struct Cyc_Dict_Dict ae=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k,struct Cyc_Tcenv_Genv*v))Cyc_Dict_insert)(((struct Cyc_Dict_Dict(*)(
struct _RegionHandle*,int(*cmp)(struct Cyc_List_List*,struct Cyc_List_List*)))Cyc_Dict_rempty)(
r,Cyc_Absyn_varlist_cmp),0,_tmp5);struct Cyc_Tcenv_Tenv*_tmp2E0;return(_tmp2E0=
_region_malloc(r,sizeof(*_tmp2E0)),((_tmp2E0->ns=0,((_tmp2E0->ae=ae,((_tmp2E0->le=
0,((_tmp2E0->allow_valueof=0,_tmp2E0)))))))));}}}static struct Cyc_Tcenv_Genv*Cyc_Tcenv_lookup_namespace(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _dyneither_ptr*n,
struct Cyc_List_List*ns);static struct Cyc_Tcenv_Genv*Cyc_Tcenv_lookup_namespace(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _dyneither_ptr*n,
struct Cyc_List_List*ns){return((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,
struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,Cyc_Tcenv_resolve_namespace(te,
loc,n,ns));}static struct Cyc_List_List*Cyc_Tcenv_outer_namespace(struct Cyc_List_List*
ns);static struct Cyc_List_List*Cyc_Tcenv_outer_namespace(struct Cyc_List_List*ns){
if(ns == 0){const char*_tmp2E1;return((struct Cyc_List_List*(*)(struct
_dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp2E1="outer_namespace",_tag_dyneither(
_tmp2E1,sizeof(char),16))));}return((struct Cyc_List_List*(*)(struct Cyc_List_List*
x))Cyc_List_rev)(((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_rev)(ns)))->tl);}static int Cyc_Tcenv_same_namespace(
struct Cyc_List_List*n1,struct Cyc_List_List*n2);static int Cyc_Tcenv_same_namespace(
struct Cyc_List_List*n1,struct Cyc_List_List*n2){if(n1 == n2)return 1;for(0;n1 != 0;
n1=n1->tl){if(n2 == 0)return 0;if(Cyc_strptrcmp((struct _dyneither_ptr*)n1->hd,(
struct _dyneither_ptr*)n2->hd)!= 0)return 0;n2=n2->tl;}if(n2 != 0)return 0;return 1;}
static void Cyc_Tcenv_check_repeat(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,void*(*lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct
_dyneither_ptr*v,struct Cyc_List_List*cns,struct Cyc_List_List*nss);static void Cyc_Tcenv_check_repeat(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,void*(*lookup)(struct Cyc_Tcenv_Genv*,
struct _dyneither_ptr*),struct _dyneither_ptr*v,struct Cyc_List_List*cns,struct Cyc_List_List*
nss){for(0;nss != 0;nss=nss->tl){if(!Cyc_Tcenv_same_namespace(cns,(struct Cyc_List_List*)
nss->hd)){struct Cyc_Tcenv_Genv*ge2=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,(struct Cyc_List_List*)nss->hd);
struct _handler_cons _tmpD;_push_handler(& _tmpD);{int _tmpF=0;if(setjmp(_tmpD.handler))
_tmpF=1;if(!_tmpF){lookup(ge2,v);{const char*_tmp2E5;void*_tmp2E4[1];struct Cyc_String_pa_struct
_tmp2E3;(_tmp2E3.tag=0,((_tmp2E3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*
v),((_tmp2E4[0]=& _tmp2E3,Cyc_Tcutil_terr(loc,((_tmp2E5="%s is ambiguous",
_tag_dyneither(_tmp2E5,sizeof(char),16))),_tag_dyneither(_tmp2E4,sizeof(void*),1)))))));};
_pop_handler();}else{void*_tmpE=(void*)_exn_thrown;void*_tmp14=_tmpE;_LL1: if(
_tmp14 != Cyc_Dict_Absent)goto _LL3;_LL2: goto _LL0;_LL3:;_LL4:(void)_throw(_tmp14);
_LL0:;}}}}return;}static void*Cyc_Tcenv_scoped_lookup(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,void*(*lookup)(struct Cyc_Tcenv_Genv*,struct
_dyneither_ptr*),struct _dyneither_ptr*v);static void*Cyc_Tcenv_scoped_lookup(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,void*(*lookup)(struct Cyc_Tcenv_Genv*,
struct _dyneither_ptr*),struct _dyneither_ptr*v){struct Cyc_List_List*cns=te->ns;
while(1){struct Cyc_Tcenv_Genv*ge=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,
struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,cns);{struct _handler_cons _tmp15;
_push_handler(& _tmp15);{int _tmp17=0;if(setjmp(_tmp15.handler))_tmp17=1;if(!
_tmp17){{void*result=lookup(ge,v);Cyc_Tcenv_check_repeat(te,loc,lookup,v,cns,ge->availables);{
void*_tmp18=result;_npop_handler(0);return _tmp18;}};_pop_handler();}else{void*
_tmp16=(void*)_exn_thrown;void*_tmp1A=_tmp16;_LL6: if(_tmp1A != Cyc_Dict_Absent)
goto _LL8;_LL7: goto _LL5;_LL8:;_LL9:(void)_throw(_tmp1A);_LL5:;}}}{struct Cyc_List_List*
_tmp1B=ge->availables;for(0;_tmp1B != 0;_tmp1B=_tmp1B->tl){struct Cyc_Tcenv_Genv*
ge2=((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(
te->ae,(struct Cyc_List_List*)_tmp1B->hd);struct _handler_cons _tmp1C;_push_handler(&
_tmp1C);{int _tmp1E=0;if(setjmp(_tmp1C.handler))_tmp1E=1;if(!_tmp1E){{void*result=
lookup(ge2,v);Cyc_Tcenv_check_repeat(te,loc,lookup,v,(struct Cyc_List_List*)
_tmp1B->hd,_tmp1B->tl);{void*_tmp1F=result;_npop_handler(0);return _tmp1F;}};
_pop_handler();}else{void*_tmp1D=(void*)_exn_thrown;void*_tmp21=_tmp1D;_LLB: if(
_tmp21 != Cyc_Dict_Absent)goto _LLD;_LLC: goto _LLA;_LLD:;_LLE:(void)_throw(_tmp21);
_LLA:;}}}}if(cns == 0)(int)_throw((void*)Cyc_Dict_Absent);cns=Cyc_Tcenv_outer_namespace(
cns);}}static void*Cyc_Tcenv_lookup_ordinary_global_f(struct Cyc_Tcenv_Genv*ge,
struct _dyneither_ptr*v);static void*Cyc_Tcenv_lookup_ordinary_global_f(struct Cyc_Tcenv_Genv*
ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp22=ge->ordinaries;struct
_tuple5*ans=((struct _tuple5*(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp22,v);(*ans).f2=1;return(*ans).f1;}static void*Cyc_Tcenv_lookup_ordinary_global(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _tuple1*q);static
void*Cyc_Tcenv_lookup_ordinary_global(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct _tuple1*q){struct _tuple1 _tmp24;union Cyc_Absyn_Nmspace_union _tmp25;
struct _dyneither_ptr*_tmp26;struct _tuple1*_tmp23=q;_tmp24=*_tmp23;_tmp25=_tmp24.f1;
_tmp26=_tmp24.f2;{union Cyc_Absyn_Nmspace_union _tmp27=_tmp25;struct Cyc_List_List*
_tmp28;struct Cyc_List_List*_tmp29;struct Cyc_List_List _tmp2A;struct _dyneither_ptr*
_tmp2B;struct Cyc_List_List*_tmp2C;struct Cyc_List_List*_tmp2D;_LL10: if((_tmp27.Loc_n).tag
!= 0)goto _LL12;_LL11: goto _LL13;_LL12: if((_tmp27.Rel_n).tag != 1)goto _LL14;_tmp28=(
_tmp27.Rel_n).f1;if(_tmp28 != 0)goto _LL14;_LL13: return Cyc_Tcenv_scoped_lookup(te,
loc,Cyc_Tcenv_lookup_ordinary_global_f,_tmp26);_LL14: if((_tmp27.Rel_n).tag != 1)
goto _LL16;_tmp29=(_tmp27.Rel_n).f1;if(_tmp29 == 0)goto _LL16;_tmp2A=*_tmp29;_tmp2B=(
struct _dyneither_ptr*)_tmp2A.hd;_tmp2C=_tmp2A.tl;_LL15: {struct Cyc_Tcenv_Genv*
_tmp2E=Cyc_Tcenv_lookup_namespace(te,loc,_tmp2B,_tmp2C);return Cyc_Tcenv_lookup_ordinary_global_f(
_tmp2E,_tmp26);}_LL16: if((_tmp27.Abs_n).tag != 2)goto _LLF;_tmp2D=(_tmp27.Abs_n).f1;
_LL17: return Cyc_Tcenv_lookup_ordinary_global_f(((struct Cyc_Tcenv_Genv*(*)(struct
Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp2D),_tmp26);
_LLF:;}}struct Cyc_List_List*Cyc_Tcenv_resolve_namespace(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,struct _dyneither_ptr*n,struct Cyc_List_List*ns);
struct Cyc_List_List*Cyc_Tcenv_resolve_namespace(struct Cyc_Tcenv_Tenv*te,struct
Cyc_Position_Segment*loc,struct _dyneither_ptr*n,struct Cyc_List_List*ns){struct
Cyc_List_List*_tmp2F=te->ns;struct _RegionHandle*_tmp30=Cyc_Tcenv_get_fnrgn(te);
struct Cyc_List_List*_tmp31=0;while(1){struct Cyc_Tcenv_Genv*_tmp32=((struct Cyc_Tcenv_Genv*(*)(
struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp2F);
struct Cyc_List_List*_tmp33=_tmp32->availables;struct Cyc_Set_Set*_tmp34=_tmp32->namespaces;{
struct Cyc_List_List*_tmp35=_tmp33;for(0;_tmp35 != 0;_tmp35=_tmp35->tl){struct Cyc_Set_Set*
_tmp36=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))
Cyc_Dict_lookup)(te->ae,(struct Cyc_List_List*)_tmp35->hd))->namespaces;if(((int(*)(
struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp36,n)){struct
Cyc_List_List*_tmp2E8;struct Cyc_List_List*_tmp2E7;_tmp31=((_tmp2E7=
_region_malloc(_tmp30,sizeof(*_tmp2E7)),((_tmp2E7->hd=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(_tmp2F,((_tmp2E8=
_cycalloc(sizeof(*_tmp2E8)),((_tmp2E8->hd=n,((_tmp2E8->tl=ns,_tmp2E8))))))),((
_tmp2E7->tl=_tmp31,_tmp2E7))))));}}}if(((int(*)(struct Cyc_Set_Set*s,struct
_dyneither_ptr*elt))Cyc_Set_member)(_tmp34,n)){struct Cyc_List_List*_tmp2EB;
struct Cyc_List_List*_tmp2EA;_tmp31=((_tmp2EA=_region_malloc(_tmp30,sizeof(*
_tmp2EA)),((_tmp2EA->hd=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct
Cyc_List_List*y))Cyc_List_append)(_tmp2F,((_tmp2EB=_cycalloc(sizeof(*_tmp2EB)),((
_tmp2EB->hd=n,((_tmp2EB->tl=ns,_tmp2EB))))))),((_tmp2EA->tl=_tmp31,_tmp2EA))))));}
if(_tmp31 != 0){if(_tmp31->tl != 0){const char*_tmp2EF;void*_tmp2EE[1];struct Cyc_String_pa_struct
_tmp2ED;(_tmp2ED.tag=0,((_tmp2ED.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*
n),((_tmp2EE[0]=& _tmp2ED,Cyc_Tcutil_terr(loc,((_tmp2EF="%s is ambiguous",
_tag_dyneither(_tmp2EF,sizeof(char),16))),_tag_dyneither(_tmp2EE,sizeof(void*),1)))))));}
return(struct Cyc_List_List*)_tmp31->hd;}if(_tmp2F == 0)(int)_throw((void*)Cyc_Dict_Absent);
_tmp2F=Cyc_Tcenv_outer_namespace(_tmp2F);}}static struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v);static struct Cyc_Absyn_Aggrdecl**
Cyc_Tcenv_lookup_aggrdecl_f(struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){
struct Cyc_Dict_Dict _tmp3E=ge->aggrdecls;return((struct Cyc_Absyn_Aggrdecl**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp3E,v);}struct
Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct _tuple1*q);struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct
Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _tuple1*q){struct _tuple1
_tmp40;union Cyc_Absyn_Nmspace_union _tmp41;struct _dyneither_ptr*_tmp42;struct
_tuple1*_tmp3F=q;_tmp40=*_tmp3F;_tmp41=_tmp40.f1;_tmp42=_tmp40.f2;{union Cyc_Absyn_Nmspace_union
_tmp43=_tmp41;struct Cyc_List_List*_tmp44;struct Cyc_List_List*_tmp45;struct Cyc_List_List*
_tmp46;struct Cyc_List_List _tmp47;struct _dyneither_ptr*_tmp48;struct Cyc_List_List*
_tmp49;_LL19: if((_tmp43.Loc_n).tag != 0)goto _LL1B;_LL1A: goto _LL1C;_LL1B: if((
_tmp43.Rel_n).tag != 1)goto _LL1D;_tmp44=(_tmp43.Rel_n).f1;if(_tmp44 != 0)goto _LL1D;
_LL1C: return((struct Cyc_Absyn_Aggrdecl**(*)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct Cyc_Absyn_Aggrdecl**(*lookup)(struct Cyc_Tcenv_Genv*,struct
_dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_aggrdecl_f,
_tmp42);_LL1D: if((_tmp43.Abs_n).tag != 2)goto _LL1F;_tmp45=(_tmp43.Abs_n).f1;_LL1E: {
struct Cyc_Dict_Dict _tmp4A=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,
struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp45))->aggrdecls;return((
struct Cyc_Absyn_Aggrdecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp4A,_tmp42);}_LL1F: if((_tmp43.Rel_n).tag != 1)goto _LL18;_tmp46=(_tmp43.Rel_n).f1;
if(_tmp46 == 0)goto _LL18;_tmp47=*_tmp46;_tmp48=(struct _dyneither_ptr*)_tmp47.hd;
_tmp49=_tmp47.tl;_LL20: {struct Cyc_Dict_Dict _tmp4B=(Cyc_Tcenv_lookup_namespace(
te,loc,_tmp48,_tmp49))->aggrdecls;return((struct Cyc_Absyn_Aggrdecl**(*)(struct
Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp4B,_tmp42);}_LL18:;}}
static struct Cyc_Absyn_Tuniondecl**Cyc_Tcenv_lookup_tuniondecl_f(struct Cyc_Tcenv_Genv*
ge,struct _dyneither_ptr*v);static struct Cyc_Absyn_Tuniondecl**Cyc_Tcenv_lookup_tuniondecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp4C=ge->tuniondecls;
return((struct Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp4C,v);}struct Cyc_Absyn_Tuniondecl**Cyc_Tcenv_lookup_tuniondecl(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _tuple1*q);struct
Cyc_Absyn_Tuniondecl**Cyc_Tcenv_lookup_tuniondecl(struct Cyc_Tcenv_Tenv*te,struct
Cyc_Position_Segment*loc,struct _tuple1*q){struct _tuple1 _tmp4E;union Cyc_Absyn_Nmspace_union
_tmp4F;struct _dyneither_ptr*_tmp50;struct _tuple1*_tmp4D=q;_tmp4E=*_tmp4D;_tmp4F=
_tmp4E.f1;_tmp50=_tmp4E.f2;{union Cyc_Absyn_Nmspace_union _tmp51=_tmp4F;struct Cyc_List_List*
_tmp52;struct Cyc_List_List*_tmp53;struct Cyc_List_List _tmp54;struct _dyneither_ptr*
_tmp55;struct Cyc_List_List*_tmp56;struct Cyc_List_List*_tmp57;_LL22: if((_tmp51.Loc_n).tag
!= 0)goto _LL24;_LL23: goto _LL25;_LL24: if((_tmp51.Rel_n).tag != 1)goto _LL26;_tmp52=(
_tmp51.Rel_n).f1;if(_tmp52 != 0)goto _LL26;_LL25: return((struct Cyc_Absyn_Tuniondecl**(*)(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Tuniondecl**(*
lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(
te,loc,Cyc_Tcenv_lookup_tuniondecl_f,_tmp50);_LL26: if((_tmp51.Rel_n).tag != 1)
goto _LL28;_tmp53=(_tmp51.Rel_n).f1;if(_tmp53 == 0)goto _LL28;_tmp54=*_tmp53;_tmp55=(
struct _dyneither_ptr*)_tmp54.hd;_tmp56=_tmp54.tl;_LL27: {struct Cyc_Dict_Dict
_tmp58=(Cyc_Tcenv_lookup_namespace(te,loc,_tmp55,_tmp56))->tuniondecls;return((
struct Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp58,_tmp50);}_LL28: if((_tmp51.Abs_n).tag != 2)goto _LL21;_tmp57=(_tmp51.Abs_n).f1;
_LL29: {struct Cyc_Dict_Dict _tmp59=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp57))->tuniondecls;return((
struct Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp59,_tmp50);}_LL21:;}}static struct Cyc_Absyn_Tuniondecl**Cyc_Tcenv_lookup_xtuniondecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v);static struct Cyc_Absyn_Tuniondecl**
Cyc_Tcenv_lookup_xtuniondecl_f(struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){
return((struct Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(ge->tuniondecls,v);}struct Cyc_Absyn_Tuniondecl***
Cyc_Tcenv_lookup_xtuniondecl(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,struct _tuple1*q);struct Cyc_Absyn_Tuniondecl***Cyc_Tcenv_lookup_xtuniondecl(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,
struct _tuple1*q){struct _tuple1 _tmp5B;union Cyc_Absyn_Nmspace_union _tmp5C;struct
_dyneither_ptr*_tmp5D;struct _tuple1*_tmp5A=q;_tmp5B=*_tmp5A;_tmp5C=_tmp5B.f1;
_tmp5D=_tmp5B.f2;{union Cyc_Absyn_Nmspace_union _tmp5E=_tmp5C;struct Cyc_List_List*
_tmp5F;struct Cyc_List_List*_tmp60;struct Cyc_List_List _tmp61;struct _dyneither_ptr*
_tmp62;struct Cyc_List_List*_tmp63;struct Cyc_List_List*_tmp64;_LL2B: if((_tmp5E.Rel_n).tag
!= 1)goto _LL2D;_tmp5F=(_tmp5E.Rel_n).f1;if(_tmp5F != 0)goto _LL2D;_LL2C: {struct
_handler_cons _tmp65;_push_handler(& _tmp65);{int _tmp67=0;if(setjmp(_tmp65.handler))
_tmp67=1;if(!_tmp67){{struct Cyc_Absyn_Tuniondecl***_tmp2F0;struct Cyc_Absyn_Tuniondecl***
_tmp69=(_tmp2F0=_region_malloc(r,sizeof(*_tmp2F0)),((_tmp2F0[0]=((struct Cyc_Absyn_Tuniondecl**(*)(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Tuniondecl**(*
lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(
te,loc,Cyc_Tcenv_lookup_xtuniondecl_f,_tmp5D),_tmp2F0)));_npop_handler(0);return
_tmp69;};_pop_handler();}else{void*_tmp66=(void*)_exn_thrown;void*_tmp6B=_tmp66;
_LL34: if(_tmp6B != Cyc_Dict_Absent)goto _LL36;_LL35: return 0;_LL36:;_LL37:(void)
_throw(_tmp6B);_LL33:;}}}_LL2D: if((_tmp5E.Loc_n).tag != 0)goto _LL2F;_LL2E:{const
char*_tmp2F3;void*_tmp2F2;(_tmp2F2=0,Cyc_Tcutil_terr(loc,((_tmp2F3="lookup_xtuniondecl: impossible",
_tag_dyneither(_tmp2F3,sizeof(char),31))),_tag_dyneither(_tmp2F2,sizeof(void*),0)));}
return 0;_LL2F: if((_tmp5E.Rel_n).tag != 1)goto _LL31;_tmp60=(_tmp5E.Rel_n).f1;if(
_tmp60 == 0)goto _LL31;_tmp61=*_tmp60;_tmp62=(struct _dyneither_ptr*)_tmp61.hd;
_tmp63=_tmp61.tl;_LL30: {struct Cyc_Tcenv_Genv*ge;{struct _handler_cons _tmp6E;
_push_handler(& _tmp6E);{int _tmp70=0;if(setjmp(_tmp6E.handler))_tmp70=1;if(!
_tmp70){ge=Cyc_Tcenv_lookup_namespace(te,loc,_tmp62,_tmp63);;_pop_handler();}
else{void*_tmp6F=(void*)_exn_thrown;void*_tmp72=_tmp6F;_LL39: if(_tmp72 != Cyc_Dict_Absent)
goto _LL3B;_LL3A:{const char*_tmp2F6;void*_tmp2F5;(_tmp2F5=0,Cyc_Tcutil_terr(loc,((
_tmp2F6="bad qualified name for xtunion",_tag_dyneither(_tmp2F6,sizeof(char),31))),
_tag_dyneither(_tmp2F5,sizeof(void*),0)));}(int)_throw((void*)Cyc_Dict_Absent);
_LL3B:;_LL3C:(void)_throw(_tmp72);_LL38:;}}}{struct Cyc_Dict_Dict _tmp75=ge->tuniondecls;
struct Cyc_Absyn_Tuniondecl***_tmp2F7;return(_tmp2F7=_region_malloc(r,sizeof(*
_tmp2F7)),((_tmp2F7[0]=((struct Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,
struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp75,_tmp5D),_tmp2F7)));}}_LL31: if((
_tmp5E.Abs_n).tag != 2)goto _LL2A;_tmp64=(_tmp5E.Abs_n).f1;_LL32: {struct Cyc_Dict_Dict
_tmp77=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))
Cyc_Dict_lookup)(te->ae,_tmp64))->tuniondecls;struct Cyc_Absyn_Tuniondecl***
_tmp2F8;return(_tmp2F8=_region_malloc(r,sizeof(*_tmp2F8)),((_tmp2F8[0]=((struct
Cyc_Absyn_Tuniondecl**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp77,_tmp5D),_tmp2F8)));}_LL2A:;}}static struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v);static struct Cyc_Absyn_Enumdecl**
Cyc_Tcenv_lookup_enumdecl_f(struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){
struct Cyc_Dict_Dict _tmp79=ge->enumdecls;return((struct Cyc_Absyn_Enumdecl**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp79,v);}struct
Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct _tuple1*q);struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct
Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _tuple1*q){struct _tuple1
_tmp7B;union Cyc_Absyn_Nmspace_union _tmp7C;struct _dyneither_ptr*_tmp7D;struct
_tuple1*_tmp7A=q;_tmp7B=*_tmp7A;_tmp7C=_tmp7B.f1;_tmp7D=_tmp7B.f2;{union Cyc_Absyn_Nmspace_union
_tmp7E=_tmp7C;struct Cyc_List_List*_tmp7F;struct Cyc_List_List*_tmp80;struct Cyc_List_List
_tmp81;struct _dyneither_ptr*_tmp82;struct Cyc_List_List*_tmp83;struct Cyc_List_List*
_tmp84;_LL3E: if((_tmp7E.Loc_n).tag != 0)goto _LL40;_LL3F: goto _LL41;_LL40: if((
_tmp7E.Rel_n).tag != 1)goto _LL42;_tmp7F=(_tmp7E.Rel_n).f1;if(_tmp7F != 0)goto _LL42;
_LL41: return((struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct Cyc_Absyn_Enumdecl**(*lookup)(struct Cyc_Tcenv_Genv*,struct
_dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(te,loc,Cyc_Tcenv_lookup_enumdecl_f,
_tmp7D);_LL42: if((_tmp7E.Rel_n).tag != 1)goto _LL44;_tmp80=(_tmp7E.Rel_n).f1;if(
_tmp80 == 0)goto _LL44;_tmp81=*_tmp80;_tmp82=(struct _dyneither_ptr*)_tmp81.hd;
_tmp83=_tmp81.tl;_LL43: {struct Cyc_Dict_Dict _tmp85=(Cyc_Tcenv_lookup_namespace(
te,loc,_tmp82,_tmp83))->enumdecls;return((struct Cyc_Absyn_Enumdecl**(*)(struct
Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp85,_tmp7D);}_LL44:
if((_tmp7E.Abs_n).tag != 2)goto _LL3D;_tmp84=(_tmp7E.Abs_n).f1;_LL45: {struct Cyc_Dict_Dict
_tmp86=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))
Cyc_Dict_lookup)(te->ae,_tmp84))->enumdecls;return((struct Cyc_Absyn_Enumdecl**(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(_tmp86,_tmp7D);}
_LL3D:;}}static struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl_f(struct
Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v);static struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl_f(
struct Cyc_Tcenv_Genv*ge,struct _dyneither_ptr*v){struct Cyc_Dict_Dict _tmp87=ge->typedefs;
return((struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k))Cyc_Dict_lookup)(_tmp87,v);}struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct _tuple1*q);struct
Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,struct _tuple1*q){struct _tuple1 _tmp89;union Cyc_Absyn_Nmspace_union
_tmp8A;struct _dyneither_ptr*_tmp8B;struct _tuple1*_tmp88=q;_tmp89=*_tmp88;_tmp8A=
_tmp89.f1;_tmp8B=_tmp89.f2;{union Cyc_Absyn_Nmspace_union _tmp8C=_tmp8A;struct Cyc_List_List*
_tmp8D;struct Cyc_List_List*_tmp8E;struct Cyc_List_List _tmp8F;struct _dyneither_ptr*
_tmp90;struct Cyc_List_List*_tmp91;struct Cyc_List_List*_tmp92;_LL47: if((_tmp8C.Loc_n).tag
!= 0)goto _LL49;_LL48: goto _LL4A;_LL49: if((_tmp8C.Rel_n).tag != 1)goto _LL4B;_tmp8D=(
_tmp8C.Rel_n).f1;if(_tmp8D != 0)goto _LL4B;_LL4A: return((struct Cyc_Absyn_Typedefdecl*(*)(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Typedefdecl*(*
lookup)(struct Cyc_Tcenv_Genv*,struct _dyneither_ptr*),struct _dyneither_ptr*v))Cyc_Tcenv_scoped_lookup)(
te,loc,Cyc_Tcenv_lookup_typedefdecl_f,_tmp8B);_LL4B: if((_tmp8C.Rel_n).tag != 1)
goto _LL4D;_tmp8E=(_tmp8C.Rel_n).f1;if(_tmp8E == 0)goto _LL4D;_tmp8F=*_tmp8E;_tmp90=(
struct _dyneither_ptr*)_tmp8F.hd;_tmp91=_tmp8F.tl;_LL4C: {struct Cyc_Dict_Dict
_tmp93=(Cyc_Tcenv_lookup_namespace(te,loc,_tmp90,_tmp91))->typedefs;return((
struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp93,_tmp8B);}_LL4D: if((_tmp8C.Abs_n).tag != 2)goto _LL46;_tmp92=(_tmp8C.Abs_n).f1;
_LL4E: {struct Cyc_Dict_Dict _tmp94=(((struct Cyc_Tcenv_Genv*(*)(struct Cyc_Dict_Dict
d,struct Cyc_List_List*k))Cyc_Dict_lookup)(te->ae,_tmp92))->typedefs;return((
struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup)(
_tmp94,_tmp8B);}_LL46:;}}static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_get_fenv(struct Cyc_Tcenv_Tenv*
te,struct _dyneither_ptr err_msg);static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_get_fenv(
struct Cyc_Tcenv_Tenv*te,struct _dyneither_ptr err_msg){struct Cyc_Tcenv_Fenv*_tmp95=
te->le;if((struct Cyc_Tcenv_Tenv*)te == 0)((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(
err_msg);return(struct Cyc_Tcenv_Fenv*)_check_null(_tmp95);}struct _RegionHandle*
Cyc_Tcenv_coerce_heap_region();struct _RegionHandle*Cyc_Tcenv_coerce_heap_region(){
return(struct _RegionHandle*)Cyc_Core_heap_region;}struct _RegionHandle*Cyc_Tcenv_get_fnrgn(
struct Cyc_Tcenv_Tenv*te);struct _RegionHandle*Cyc_Tcenv_get_fnrgn(struct Cyc_Tcenv_Tenv*
te){struct Cyc_Tcenv_Fenv*_tmp96=te->le;if(_tmp96 != 0){struct Cyc_Tcenv_Fenv _tmp98;
struct _RegionHandle*_tmp99;struct Cyc_Tcenv_Fenv*_tmp97=(struct Cyc_Tcenv_Fenv*)
_tmp96;_tmp98=*_tmp97;_tmp99=_tmp98.fnrgn;return _tmp99;}return Cyc_Tcenv_coerce_heap_region();}
static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_fenv(struct _RegionHandle*l,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Tcenv_Fenv*fe);static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_fenv(struct
_RegionHandle*l,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Fenv*fe){if(te->le == 0){
const char*_tmp2F9;((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp2F9="put_fenv",
_tag_dyneither(_tmp2F9,sizeof(char),9))));}{struct Cyc_Tcenv_Tenv*_tmp2FA;return(
_tmp2FA=_region_malloc(l,sizeof(*_tmp2FA)),((_tmp2FA->ns=te->ns,((_tmp2FA->ae=te->ae,((
_tmp2FA->le=(struct Cyc_Tcenv_Fenv*)fe,((_tmp2FA->allow_valueof=te->allow_valueof,
_tmp2FA)))))))));}}static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_emptyfenv(struct
_RegionHandle*l,struct Cyc_Tcenv_Tenv*te);static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_emptyfenv(
struct _RegionHandle*l,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Tenv*_tmp2FB;
return(_tmp2FB=_region_malloc(l,sizeof(*_tmp2FB)),((_tmp2FB->ns=te->ns,((_tmp2FB->ae=
te->ae,((_tmp2FB->le=0,((_tmp2FB->allow_valueof=te->allow_valueof,_tmp2FB)))))))));}
static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_old_ctrl(struct _RegionHandle*l,
struct Cyc_Tcenv_Fenv*f);static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_old_ctrl(
struct _RegionHandle*l,struct Cyc_Tcenv_Fenv*f){struct Cyc_Tcenv_SharedFenv*_tmp9E;
struct Cyc_List_List*_tmp9F;struct Cyc_RgnOrder_RgnPO*_tmpA0;struct Cyc_Tcenv_Bindings*
_tmpA1;struct Cyc_Absyn_Stmt*_tmpA2;struct Cyc_Tcenv_CtrlEnv*_tmpA3;void*_tmpA4;
void*_tmpA5;int _tmpA6;struct _RegionHandle*_tmpA7;struct Cyc_Tcenv_Fenv _tmp9D=*f;
_tmp9E=_tmp9D.shared;_tmp9F=_tmp9D.type_vars;_tmpA0=_tmp9D.region_order;_tmpA1=
_tmp9D.locals;_tmpA2=_tmp9D.encloser;_tmpA3=_tmp9D.ctrl_env;_tmpA4=(void*)_tmp9D.capability;
_tmpA5=(void*)_tmp9D.curr_rgn;_tmpA6=_tmp9D.in_notreadctxt;_tmpA7=_tmp9D.fnrgn;{
struct Cyc_Tcenv_Fenv*_tmp2FC;return(_tmp2FC=_region_malloc(l,sizeof(*_tmp2FC)),((
_tmp2FC->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp9E,((_tmp2FC->type_vars=(struct
Cyc_List_List*)_tmp9F,((_tmp2FC->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmpA0,((
_tmp2FC->locals=(struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)_tmpA1),((
_tmp2FC->encloser=(struct Cyc_Absyn_Stmt*)_tmpA2,((_tmp2FC->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
struct Cyc_Tcenv_CtrlEnv*)_tmpA3),((_tmp2FC->capability=(void*)((void*)_tmpA4),((
_tmp2FC->curr_rgn=(void*)((void*)_tmpA5),((_tmp2FC->in_notreadctxt=(int)_tmpA6,((
_tmp2FC->fnrgn=(struct _RegionHandle*)l,_tmp2FC)))))))))))))))))))));}}static
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_new_ctrl(struct _RegionHandle*l,struct
Cyc_Tcenv_Fenv*f);static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_copy_fenv_new_ctrl(struct
_RegionHandle*l,struct Cyc_Tcenv_Fenv*f){struct Cyc_Tcenv_SharedFenv*_tmpAA;struct
Cyc_List_List*_tmpAB;struct Cyc_RgnOrder_RgnPO*_tmpAC;struct Cyc_Tcenv_Bindings*
_tmpAD;struct Cyc_Absyn_Stmt*_tmpAE;struct Cyc_Tcenv_CtrlEnv*_tmpAF;void*_tmpB0;
void*_tmpB1;int _tmpB2;struct Cyc_Tcenv_Fenv _tmpA9=*f;_tmpAA=_tmpA9.shared;_tmpAB=
_tmpA9.type_vars;_tmpAC=_tmpA9.region_order;_tmpAD=_tmpA9.locals;_tmpAE=_tmpA9.encloser;
_tmpAF=_tmpA9.ctrl_env;_tmpB0=(void*)_tmpA9.capability;_tmpB1=(void*)_tmpA9.curr_rgn;
_tmpB2=_tmpA9.in_notreadctxt;{struct _RegionHandle*_tmpB4;void*_tmpB5;void*_tmpB6;
struct _tuple4*_tmpB7;void*_tmpB8;int _tmpB9;struct Cyc_Tcenv_CtrlEnv _tmpB3=*_tmpAF;
_tmpB4=_tmpB3.ctrl_rgn;_tmpB5=(void*)_tmpB3.continue_stmt;_tmpB6=(void*)_tmpB3.break_stmt;
_tmpB7=_tmpB3.fallthru_clause;_tmpB8=(void*)_tmpB3.next_stmt;_tmpB9=_tmpB3.try_depth;{
struct Cyc_Tcenv_CtrlEnv*_tmp2FD;struct Cyc_Tcenv_CtrlEnv*_tmpBA=(_tmp2FD=
_region_malloc(l,sizeof(*_tmp2FD)),((_tmp2FD->ctrl_rgn=_tmpB4,((_tmp2FD->continue_stmt=(
void*)_tmpB5,((_tmp2FD->break_stmt=(void*)_tmpB6,((_tmp2FD->fallthru_clause=
_tmpB7,((_tmp2FD->next_stmt=(void*)_tmpB8,((_tmp2FD->try_depth=_tmpB9,_tmp2FD)))))))))))));
struct Cyc_Tcenv_Fenv*_tmp2FE;return(_tmp2FE=_region_malloc(l,sizeof(*_tmp2FE)),((
_tmp2FE->shared=(struct Cyc_Tcenv_SharedFenv*)_tmpAA,((_tmp2FE->type_vars=(struct
Cyc_List_List*)_tmpAB,((_tmp2FE->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmpAC,((
_tmp2FE->locals=(struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)_tmpAD),((
_tmp2FE->encloser=(struct Cyc_Absyn_Stmt*)_tmpAE,((_tmp2FE->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)
_tmpBA,((_tmp2FE->capability=(void*)((void*)_tmpB0),((_tmp2FE->curr_rgn=(void*)((
void*)_tmpB1),((_tmp2FE->in_notreadctxt=(int)_tmpB2,((_tmp2FE->fnrgn=(struct
_RegionHandle*)l,_tmp2FE)))))))))))))))))))));}}}void*Cyc_Tcenv_return_typ(
struct Cyc_Tcenv_Tenv*te);void*Cyc_Tcenv_return_typ(struct Cyc_Tcenv_Tenv*te){
struct Cyc_Tcenv_Fenv _tmpBF;struct Cyc_Tcenv_SharedFenv*_tmpC0;const char*_tmp2FF;
struct Cyc_Tcenv_Fenv*_tmpBE=Cyc_Tcenv_get_fenv(te,((_tmp2FF="return_typ",
_tag_dyneither(_tmp2FF,sizeof(char),11))));_tmpBF=*_tmpBE;_tmpC0=_tmpBF.shared;{
void*_tmpC2;struct Cyc_Tcenv_SharedFenv _tmpC1=*_tmpC0;_tmpC2=(void*)_tmpC1.return_typ;
return _tmpC2;}}struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*
te);struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*te){
struct Cyc_Tcenv_Fenv*_tmpC3=te->le;if(te->le == 0)return 0;{struct Cyc_List_List*
_tmpC5;struct Cyc_Tcenv_Fenv _tmpC4=*((struct Cyc_Tcenv_Fenv*)_check_null(_tmpC3));
_tmpC5=_tmpC4.type_vars;return _tmpC5;}}struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(
struct Cyc_Tcenv_Tenv*te);struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Core_Opt*_tmp300;return(_tmp300=_cycalloc(
sizeof(*_tmp300)),((_tmp300->v=Cyc_Tcenv_lookup_type_vars(te),_tmp300)));}struct
Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(struct _RegionHandle*r,struct Cyc_Position_Segment*
loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*tvs);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(
struct _RegionHandle*r,struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te,
struct Cyc_List_List*tvs){struct Cyc_Tcenv_Fenv _tmpC9;struct Cyc_Tcenv_SharedFenv*
_tmpCA;struct Cyc_List_List*_tmpCB;struct Cyc_RgnOrder_RgnPO*_tmpCC;struct Cyc_Tcenv_Bindings*
_tmpCD;struct Cyc_Absyn_Stmt*_tmpCE;struct Cyc_Tcenv_CtrlEnv*_tmpCF;void*_tmpD0;
void*_tmpD1;int _tmpD2;const char*_tmp301;struct Cyc_Tcenv_Fenv*_tmpC8=Cyc_Tcenv_get_fenv(
te,((_tmp301="add_type_vars",_tag_dyneither(_tmp301,sizeof(char),14))));_tmpC9=*
_tmpC8;_tmpCA=_tmpC9.shared;_tmpCB=_tmpC9.type_vars;_tmpCC=_tmpC9.region_order;
_tmpCD=_tmpC9.locals;_tmpCE=_tmpC9.encloser;_tmpCF=_tmpC9.ctrl_env;_tmpD0=(void*)
_tmpC9.capability;_tmpD1=(void*)_tmpC9.curr_rgn;_tmpD2=_tmpC9.in_notreadctxt;Cyc_Tcutil_add_tvar_identities(
tvs);{struct Cyc_List_List*_tmpD3=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_append)(tvs,_tmpCB);Cyc_Tcutil_check_unique_tvars(
loc,_tmpD3);{struct Cyc_Tcenv_Fenv*_tmp302;struct Cyc_Tcenv_Fenv*_tmpD4=(_tmp302=
_region_malloc(r,sizeof(*_tmp302)),((_tmp302->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmpCA,((_tmp302->type_vars=(struct Cyc_List_List*)_tmpD3,((_tmp302->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmpCC,((_tmp302->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmpCD),((_tmp302->encloser=(struct Cyc_Absyn_Stmt*)
_tmpCE,((_tmp302->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmpCF),((_tmp302->capability=(void*)((void*)_tmpD0),((_tmp302->curr_rgn=(void*)((
void*)_tmpD1),((_tmp302->in_notreadctxt=(int)_tmpD2,((_tmp302->fnrgn=(struct
_RegionHandle*)r,_tmp302)))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,te,
_tmpD4);}}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(struct _RegionHandle*r,struct
Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(struct _RegionHandle*
r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpD6=te->le;if(_tmpD6 == 0)
return Cyc_Tcenv_put_emptyfenv(r,te);else{struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_copy_fenv_old_ctrl(
r,(struct Cyc_Tcenv_Fenv*)_tmpD6);return Cyc_Tcenv_put_fenv(r,te,fe);}}struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_local_var(struct _RegionHandle*r,struct Cyc_Position_Segment*loc,
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Vardecl*vd);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_local_var(
struct _RegionHandle*r,struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te,
struct Cyc_Absyn_Vardecl*vd){struct _dyneither_ptr*_tmpD7=(*vd->name).f2;{union Cyc_Absyn_Nmspace_union
_tmpD8=(*vd->name).f1;_LL50: if((_tmpD8.Loc_n).tag != 0)goto _LL52;_LL51: goto _LL4F;
_LL52:;_LL53: {struct Cyc_Core_Impossible_struct _tmp308;const char*_tmp307;struct
Cyc_Core_Impossible_struct*_tmp306;(int)_throw((void*)((_tmp306=_cycalloc(
sizeof(*_tmp306)),((_tmp306[0]=((_tmp308.tag=Cyc_Core_Impossible,((_tmp308.f1=((
_tmp307="add_local_var: called with Rel_n",_tag_dyneither(_tmp307,sizeof(char),
33))),_tmp308)))),_tmp306)))));}_LL4F:;}{struct Cyc_Tcenv_Fenv _tmpDE;struct Cyc_Tcenv_SharedFenv*
_tmpDF;struct Cyc_List_List*_tmpE0;struct Cyc_RgnOrder_RgnPO*_tmpE1;struct Cyc_Tcenv_Bindings*
_tmpE2;struct Cyc_Absyn_Stmt*_tmpE3;struct Cyc_Tcenv_CtrlEnv*_tmpE4;void*_tmpE5;
void*_tmpE6;int _tmpE7;const char*_tmp309;struct Cyc_Tcenv_Fenv*_tmpDD=Cyc_Tcenv_get_fenv(
te,((_tmp309="add_local_var",_tag_dyneither(_tmp309,sizeof(char),14))));_tmpDE=*
_tmpDD;_tmpDF=_tmpDE.shared;_tmpE0=_tmpDE.type_vars;_tmpE1=_tmpDE.region_order;
_tmpE2=_tmpDE.locals;_tmpE3=_tmpDE.encloser;_tmpE4=_tmpDE.ctrl_env;_tmpE5=(void*)
_tmpDE.capability;_tmpE6=(void*)_tmpDE.curr_rgn;_tmpE7=_tmpDE.in_notreadctxt;{
struct Cyc_Absyn_Local_b_struct*_tmp30F;struct Cyc_Absyn_Local_b_struct _tmp30E;
struct Cyc_Tcenv_Bindings*_tmp30D;struct Cyc_Tcenv_Bindings*_tmpE8=(_tmp30D=
_region_malloc(r,sizeof(*_tmp30D)),((_tmp30D->v=_tmpD7,((_tmp30D->b=(void*)((
void*)((_tmp30F=_cycalloc(sizeof(*_tmp30F)),((_tmp30F[0]=((_tmp30E.tag=3,((
_tmp30E.f1=vd,_tmp30E)))),_tmp30F))))),((_tmp30D->tl=(struct Cyc_Tcenv_Bindings*)
_tmpE2,_tmp30D)))))));struct Cyc_Tcenv_Fenv*_tmp310;struct Cyc_Tcenv_Fenv*_tmpE9=(
_tmp310=_region_malloc(r,sizeof(*_tmp310)),((_tmp310->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmpDF,((_tmp310->type_vars=(struct Cyc_List_List*)_tmpE0,((_tmp310->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmpE1,((_tmp310->locals=(struct Cyc_Tcenv_Bindings*)
_tmpE8,((_tmp310->encloser=(struct Cyc_Absyn_Stmt*)_tmpE3,((_tmp310->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmpE4),((_tmp310->capability=(
void*)((void*)_tmpE5),((_tmp310->curr_rgn=(void*)((void*)_tmpE6),((_tmp310->in_notreadctxt=(
int)_tmpE7,((_tmp310->fnrgn=(struct _RegionHandle*)r,_tmp310)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmpE9);}}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpEE=te->le;
if(_tmpEE == 0)return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*
_tmpF0;struct Cyc_List_List*_tmpF1;struct Cyc_RgnOrder_RgnPO*_tmpF2;struct Cyc_Tcenv_Bindings*
_tmpF3;struct Cyc_Absyn_Stmt*_tmpF4;struct Cyc_Tcenv_CtrlEnv*_tmpF5;void*_tmpF6;
void*_tmpF7;int _tmpF8;struct Cyc_Tcenv_Fenv _tmpEF=*_tmpEE;_tmpF0=_tmpEF.shared;
_tmpF1=_tmpEF.type_vars;_tmpF2=_tmpEF.region_order;_tmpF3=_tmpEF.locals;_tmpF4=
_tmpEF.encloser;_tmpF5=_tmpEF.ctrl_env;_tmpF6=(void*)_tmpEF.capability;_tmpF7=(
void*)_tmpEF.curr_rgn;_tmpF8=_tmpEF.in_notreadctxt;{struct Cyc_Tcenv_Fenv*_tmp311;
struct Cyc_Tcenv_Fenv*_tmpF9=(_tmp311=_region_malloc(r,sizeof(*_tmp311)),((
_tmp311->shared=(struct Cyc_Tcenv_SharedFenv*)_tmpF0,((_tmp311->type_vars=(struct
Cyc_List_List*)_tmpF1,((_tmp311->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmpF2,((
_tmp311->locals=(struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)_tmpF3),((
_tmp311->encloser=(struct Cyc_Absyn_Stmt*)_tmpF4,((_tmp311->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
struct Cyc_Tcenv_CtrlEnv*)_tmpF5),((_tmp311->capability=(void*)((void*)_tmpF6),((
_tmp311->curr_rgn=(void*)((void*)_tmpF7),((_tmp311->in_notreadctxt=(int)1,((
_tmp311->fnrgn=(struct _RegionHandle*)r,_tmp311)))))))))))))))))))));return Cyc_Tcenv_put_fenv(
r,te,_tmpF9);}}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(struct
_RegionHandle*r,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmpFB=te->le;
if(_tmpFB == 0)return Cyc_Tcenv_put_emptyfenv(r,te);{struct Cyc_Tcenv_SharedFenv*
_tmpFD;struct Cyc_List_List*_tmpFE;struct Cyc_RgnOrder_RgnPO*_tmpFF;struct Cyc_Tcenv_Bindings*
_tmp100;struct Cyc_Absyn_Stmt*_tmp101;struct Cyc_Tcenv_CtrlEnv*_tmp102;void*
_tmp103;void*_tmp104;int _tmp105;struct Cyc_Tcenv_Fenv _tmpFC=*_tmpFB;_tmpFD=_tmpFC.shared;
_tmpFE=_tmpFC.type_vars;_tmpFF=_tmpFC.region_order;_tmp100=_tmpFC.locals;_tmp101=
_tmpFC.encloser;_tmp102=_tmpFC.ctrl_env;_tmp103=(void*)_tmpFC.capability;_tmp104=(
void*)_tmpFC.curr_rgn;_tmp105=_tmpFC.in_notreadctxt;{struct Cyc_Tcenv_Fenv*
_tmp312;struct Cyc_Tcenv_Fenv*_tmp106=(_tmp312=_region_malloc(r,sizeof(*_tmp312)),((
_tmp312->shared=(struct Cyc_Tcenv_SharedFenv*)_tmpFD,((_tmp312->type_vars=(struct
Cyc_List_List*)_tmpFE,((_tmp312->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmpFF,((
_tmp312->locals=(struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)_tmp100),((
_tmp312->encloser=(struct Cyc_Absyn_Stmt*)_tmp101,((_tmp312->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
struct Cyc_Tcenv_CtrlEnv*)_tmp102),((_tmp312->capability=(void*)((void*)_tmp103),((
_tmp312->curr_rgn=(void*)((void*)_tmp104),((_tmp312->in_notreadctxt=(int)0,((
_tmp312->fnrgn=(struct _RegionHandle*)r,_tmp312)))))))))))))))))))));return Cyc_Tcenv_put_fenv(
r,te,_tmp106);}}}int Cyc_Tcenv_in_notreadctxt(struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_in_notreadctxt(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*_tmp108=te->le;if(_tmp108 == 0)
return 0;{struct Cyc_Tcenv_Fenv _tmp10A;int _tmp10B;struct Cyc_Tcenv_Fenv*_tmp109=(
struct Cyc_Tcenv_Fenv*)_tmp108;_tmp10A=*_tmp109;_tmp10B=_tmp10A.in_notreadctxt;
return _tmp10B;}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_pat_var(struct _RegionHandle*r,
struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Vardecl*
vd);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_pat_var(struct _RegionHandle*r,struct Cyc_Position_Segment*
loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Vardecl*vd){struct _dyneither_ptr*
_tmp10C=(*vd->name).f2;struct Cyc_Tcenv_Fenv _tmp10F;struct Cyc_Tcenv_SharedFenv*
_tmp110;struct Cyc_List_List*_tmp111;struct Cyc_RgnOrder_RgnPO*_tmp112;struct Cyc_Tcenv_Bindings*
_tmp113;struct Cyc_Absyn_Stmt*_tmp114;struct Cyc_Tcenv_CtrlEnv*_tmp115;void*
_tmp116;void*_tmp117;int _tmp118;const char*_tmp313;struct Cyc_Tcenv_Fenv*_tmp10E=
Cyc_Tcenv_get_fenv(te,((_tmp313="add_pat_var",_tag_dyneither(_tmp313,sizeof(char),
12))));_tmp10F=*_tmp10E;_tmp110=_tmp10F.shared;_tmp111=_tmp10F.type_vars;_tmp112=
_tmp10F.region_order;_tmp113=_tmp10F.locals;_tmp114=_tmp10F.encloser;_tmp115=
_tmp10F.ctrl_env;_tmp116=(void*)_tmp10F.capability;_tmp117=(void*)_tmp10F.curr_rgn;
_tmp118=_tmp10F.in_notreadctxt;{struct Cyc_Absyn_Pat_b_struct*_tmp319;struct Cyc_Absyn_Pat_b_struct
_tmp318;struct Cyc_Tcenv_Bindings*_tmp317;struct Cyc_Tcenv_Bindings*_tmp119=(
_tmp317=_region_malloc(r,sizeof(*_tmp317)),((_tmp317->v=_tmp10C,((_tmp317->b=(
void*)((void*)((_tmp319=_cycalloc(sizeof(*_tmp319)),((_tmp319[0]=((_tmp318.tag=4,((
_tmp318.f1=vd,_tmp318)))),_tmp319))))),((_tmp317->tl=(struct Cyc_Tcenv_Bindings*)
_tmp113,_tmp317)))))));struct Cyc_Tcenv_Fenv*_tmp31A;struct Cyc_Tcenv_Fenv*_tmp11A=(
_tmp31A=_region_malloc(r,sizeof(*_tmp31A)),((_tmp31A->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp110,((_tmp31A->type_vars=(struct Cyc_List_List*)_tmp111,((_tmp31A->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp112,((_tmp31A->locals=(struct Cyc_Tcenv_Bindings*)
_tmp119,((_tmp31A->encloser=(struct Cyc_Absyn_Stmt*)_tmp114,((_tmp31A->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp115),((_tmp31A->capability=(
void*)((void*)_tmp116),((_tmp31A->curr_rgn=(void*)((void*)_tmp117),((_tmp31A->in_notreadctxt=(
int)_tmp118,((_tmp31A->fnrgn=(struct _RegionHandle*)r,_tmp31A)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp11A);}}void*Cyc_Tcenv_lookup_ordinary(struct
_RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,struct
_tuple1*q);void*Cyc_Tcenv_lookup_ordinary(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Position_Segment*loc,struct _tuple1*q){struct Cyc_Tcenv_Fenv*_tmp11F=
te->le;struct _tuple1 _tmp121;union Cyc_Absyn_Nmspace_union _tmp122;struct
_dyneither_ptr*_tmp123;struct _tuple1*_tmp120=q;_tmp121=*_tmp120;_tmp122=_tmp121.f1;
_tmp123=_tmp121.f2;{union Cyc_Absyn_Nmspace_union _tmp124=_tmp122;struct Cyc_List_List*
_tmp125;_LL55: if((_tmp124.Loc_n).tag != 0)goto _LL57;_LL56: if(_tmp11F == 0)(int)
_throw((void*)Cyc_Dict_Absent);goto _LL58;_LL57: if((_tmp124.Rel_n).tag != 1)goto
_LL59;_tmp125=(_tmp124.Rel_n).f1;if(_tmp125 != 0)goto _LL59;if(!(_tmp11F != 0))goto
_LL59;_LL58: {struct Cyc_Tcenv_Fenv _tmp127;struct Cyc_Tcenv_Bindings*_tmp128;
struct Cyc_Tcenv_Fenv*_tmp126=(struct Cyc_Tcenv_Fenv*)_tmp11F;_tmp127=*_tmp126;
_tmp128=_tmp127.locals;{struct _handler_cons _tmp129;_push_handler(& _tmp129);{int
_tmp12B=0;if(setjmp(_tmp129.handler))_tmp12B=1;if(!_tmp12B){{struct Cyc_Tcenv_VarRes_struct
_tmp31D;struct Cyc_Tcenv_VarRes_struct*_tmp31C;void*_tmp12E=(void*)((_tmp31C=
_region_malloc(r,sizeof(*_tmp31C)),((_tmp31C[0]=((_tmp31D.tag=0,((_tmp31D.f1=(
void*)Cyc_Tcenv_lookup_binding(_tmp128,_tmp123),_tmp31D)))),_tmp31C))));
_npop_handler(0);return _tmp12E;};_pop_handler();}else{void*_tmp12A=(void*)
_exn_thrown;void*_tmp130=_tmp12A;_LL5C:;_LL5D: return(void*)Cyc_Tcenv_lookup_ordinary_global(
te,loc,q);_LL5E:;_LL5F:(void)_throw(_tmp130);_LL5B:;}}}}_LL59:;_LL5A: {struct
_handler_cons _tmp131;_push_handler(& _tmp131);{int _tmp133=0;if(setjmp(_tmp131.handler))
_tmp133=1;if(!_tmp133){{void*_tmp134=(void*)Cyc_Tcenv_lookup_ordinary_global(te,
loc,q);_npop_handler(0);return _tmp134;};_pop_handler();}else{void*_tmp132=(void*)
_exn_thrown;void*_tmp136=_tmp132;_LL61: if(_tmp136 != Cyc_Dict_Absent)goto _LL63;
_LL62: {struct Cyc_Tcenv_VarRes_struct _tmp320;struct Cyc_Tcenv_VarRes_struct*
_tmp31F;return(void*)((_tmp31F=_region_malloc(r,sizeof(*_tmp31F)),((_tmp31F[0]=((
_tmp320.tag=0,((_tmp320.f1=(void*)((void*)0),_tmp320)))),_tmp31F))));}_LL63:;
_LL64:(void)_throw(_tmp136);_LL60:;}}}_LL54:;}}void Cyc_Tcenv_process_continue(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt**sopt);void
Cyc_Tcenv_process_continue(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct
Cyc_Absyn_Stmt**sopt){struct Cyc_Tcenv_Fenv _tmp13B;struct Cyc_Tcenv_CtrlEnv*
_tmp13C;const char*_tmp321;struct Cyc_Tcenv_Fenv*_tmp13A=Cyc_Tcenv_get_fenv(te,((
_tmp321="process_continue",_tag_dyneither(_tmp321,sizeof(char),17))));_tmp13B=*
_tmp13A;_tmp13C=_tmp13B.ctrl_env;{void*_tmp13D=(void*)_tmp13C->continue_stmt;
struct Cyc_Absyn_Stmt*_tmp13E;_LL66: if(_tmp13D <= (void*)3)goto _LL68;if(*((int*)
_tmp13D)!= 0)goto _LL68;_tmp13E=((struct Cyc_Tcenv_Stmt_j_struct*)_tmp13D)->f1;
_LL67:{struct Cyc_List_List*_tmp322;_tmp13E->non_local_preds=((_tmp322=_cycalloc(
sizeof(*_tmp322)),((_tmp322->hd=s,((_tmp322->tl=_tmp13E->non_local_preds,_tmp322))))));}*
sopt=(struct Cyc_Absyn_Stmt*)_tmp13E;return;_LL68: if((int)_tmp13D != 0)goto _LL6A;
_LL69:{const char*_tmp325;void*_tmp324;(_tmp324=0,Cyc_Tcutil_terr(s->loc,((
_tmp325="continue not in a loop",_tag_dyneither(_tmp325,sizeof(char),23))),
_tag_dyneither(_tmp324,sizeof(void*),0)));}return;_LL6A: if((int)_tmp13D != 1)goto
_LL6C;_LL6B: goto _LL6D;_LL6C: if((int)_tmp13D != 2)goto _LL65;_LL6D: {const char*
_tmp326;((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp326="bad continue destination",
_tag_dyneither(_tmp326,sizeof(char),25))));}_LL65:;}}void Cyc_Tcenv_process_break(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt**sopt);void
Cyc_Tcenv_process_break(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt**
sopt){struct Cyc_Tcenv_Fenv _tmp145;struct Cyc_Tcenv_CtrlEnv*_tmp146;struct Cyc_Tcenv_SharedFenv*
_tmp147;const char*_tmp327;struct Cyc_Tcenv_Fenv*_tmp144=Cyc_Tcenv_get_fenv(te,((
_tmp327="process_break",_tag_dyneither(_tmp327,sizeof(char),14))));_tmp145=*
_tmp144;_tmp146=_tmp145.ctrl_env;_tmp147=_tmp145.shared;{void*_tmp148=(void*)
_tmp146->break_stmt;struct Cyc_Absyn_Stmt*_tmp149;_LL6F: if(_tmp148 <= (void*)3)
goto _LL71;if(*((int*)_tmp148)!= 0)goto _LL71;_tmp149=((struct Cyc_Tcenv_Stmt_j_struct*)
_tmp148)->f1;_LL70:{struct Cyc_List_List*_tmp328;_tmp149->non_local_preds=((
_tmp328=_cycalloc(sizeof(*_tmp328)),((_tmp328->hd=s,((_tmp328->tl=_tmp149->non_local_preds,
_tmp328))))));}*sopt=(struct Cyc_Absyn_Stmt*)_tmp149;return;_LL71: if((int)_tmp148
!= 0)goto _LL73;_LL72:{const char*_tmp32B;void*_tmp32A;(_tmp32A=0,Cyc_Tcutil_terr(
s->loc,((_tmp32B="break not in a loop or switch",_tag_dyneither(_tmp32B,sizeof(
char),30))),_tag_dyneither(_tmp32A,sizeof(void*),0)));}return;_LL73: if((int)
_tmp148 != 2)goto _LL75;_LL74: if((void*)_tmp147->return_typ != (void*)((void*)0)){
const char*_tmp32E;void*_tmp32D;(_tmp32D=0,Cyc_Tcutil_terr(s->loc,((_tmp32E="break causes function not to return a value",
_tag_dyneither(_tmp32E,sizeof(char),44))),_tag_dyneither(_tmp32D,sizeof(void*),0)));}
return;_LL75: if((int)_tmp148 != 1)goto _LL6E;_LL76:{const char*_tmp331;void*_tmp330;(
_tmp330=0,Cyc_Tcutil_terr(s->loc,((_tmp331="break causes outer switch clause to implicitly fallthru",
_tag_dyneither(_tmp331,sizeof(char),56))),_tag_dyneither(_tmp330,sizeof(void*),0)));}
return;_LL6E:;}}void Cyc_Tcenv_process_goto(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*
s,struct _dyneither_ptr*l,struct Cyc_Absyn_Stmt**sopt);void Cyc_Tcenv_process_goto(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct _dyneither_ptr*l,struct Cyc_Absyn_Stmt**
sopt){struct Cyc_Tcenv_Fenv _tmp153;struct Cyc_Tcenv_SharedFenv*_tmp154;const char*
_tmp332;struct Cyc_Tcenv_Fenv*_tmp152=Cyc_Tcenv_get_fenv(te,((_tmp332="process_goto",
_tag_dyneither(_tmp332,sizeof(char),13))));_tmp153=*_tmp152;_tmp154=_tmp153.shared;{
struct Cyc_Absyn_Stmt**sopt2=((struct Cyc_Absyn_Stmt**(*)(struct Cyc_Dict_Dict d,
struct _dyneither_ptr*k))Cyc_Dict_lookup_opt)(_tmp154->seen_labels,l);if(sopt2 == 
0){struct Cyc_Dict_Dict _tmp155=_tmp154->needed_labels;struct Cyc_List_List**slopt=((
struct Cyc_List_List**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup_opt)(
_tmp155,l);struct _RegionHandle*frgn=_tmp154->frgn;if(slopt == 0){struct Cyc_List_List**
_tmp333;slopt=((_tmp333=_region_malloc(frgn,sizeof(*_tmp333)),((_tmp333[0]=0,
_tmp333))));}{struct Cyc_List_List*_tmp334;struct Cyc_List_List*new_needed=(
_tmp334=_cycalloc(sizeof(*_tmp334)),((_tmp334->hd=s,((_tmp334->tl=*((struct Cyc_List_List**)
_check_null(slopt)),_tmp334)))));_tmp154->needed_labels=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k,struct Cyc_List_List*v))Cyc_Dict_insert)(
_tmp155,l,new_needed);}}else{struct Cyc_Absyn_Stmt*s=*sopt2;{struct Cyc_List_List*
_tmp335;s->non_local_preds=((_tmp335=_cycalloc(sizeof(*_tmp335)),((_tmp335->hd=s,((
_tmp335->tl=s->non_local_preds,_tmp335))))));}*sopt=(struct Cyc_Absyn_Stmt*)s;}}}
struct _tuple4*Cyc_Tcenv_process_fallthru(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*
s,struct Cyc_Absyn_Switch_clause***clauseopt);struct _tuple4*Cyc_Tcenv_process_fallthru(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Switch_clause***
clauseopt){struct Cyc_Tcenv_Fenv _tmp15B;struct Cyc_Tcenv_CtrlEnv*_tmp15C;const char*
_tmp336;struct Cyc_Tcenv_Fenv*_tmp15A=Cyc_Tcenv_get_fenv(te,((_tmp336="process_fallthru",
_tag_dyneither(_tmp336,sizeof(char),17))));_tmp15B=*_tmp15A;_tmp15C=_tmp15B.ctrl_env;{
struct _tuple4*_tmp15D=(struct _tuple4*)_tmp15C->fallthru_clause;if(_tmp15D != 0){{
struct Cyc_List_List*_tmp337;(((*_tmp15D).f1)->body)->non_local_preds=((_tmp337=
_cycalloc(sizeof(*_tmp337)),((_tmp337->hd=s,((_tmp337->tl=(((*_tmp15D).f1)->body)->non_local_preds,
_tmp337))))));}{struct Cyc_Absyn_Switch_clause**_tmp338;*clauseopt=(struct Cyc_Absyn_Switch_clause**)((
_tmp338=_cycalloc(sizeof(*_tmp338)),((_tmp338[0]=(*_tmp15D).f1,_tmp338))));}}
return _tmp15D;}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(struct _RegionHandle*
r,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*new_tvs,struct Cyc_List_List*vds,
struct Cyc_Absyn_Switch_clause*clause);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*new_tvs,struct
Cyc_List_List*vds,struct Cyc_Absyn_Switch_clause*clause){struct Cyc_Tcenv_Fenv
_tmp162;struct Cyc_Tcenv_SharedFenv*_tmp163;struct Cyc_List_List*_tmp164;struct Cyc_RgnOrder_RgnPO*
_tmp165;struct Cyc_Tcenv_Bindings*_tmp166;struct Cyc_Absyn_Stmt*_tmp167;struct Cyc_Tcenv_CtrlEnv*
_tmp168;void*_tmp169;void*_tmp16A;int _tmp16B;const char*_tmp339;struct Cyc_Tcenv_Fenv*
_tmp161=Cyc_Tcenv_get_fenv(te,((_tmp339="set_fallthru",_tag_dyneither(_tmp339,
sizeof(char),13))));_tmp162=*_tmp161;_tmp163=_tmp162.shared;_tmp164=_tmp162.type_vars;
_tmp165=_tmp162.region_order;_tmp166=_tmp162.locals;_tmp167=_tmp162.encloser;
_tmp168=_tmp162.ctrl_env;_tmp169=(void*)_tmp162.capability;_tmp16A=(void*)
_tmp162.curr_rgn;_tmp16B=_tmp162.in_notreadctxt;{struct Cyc_Tcenv_CtrlEnv _tmp16D;
struct _RegionHandle*_tmp16E;void*_tmp16F;void*_tmp170;struct _tuple4*_tmp171;void*
_tmp172;int _tmp173;struct Cyc_Tcenv_CtrlEnv*_tmp16C=_tmp168;_tmp16D=*_tmp16C;
_tmp16E=_tmp16D.ctrl_rgn;_tmp16F=(void*)_tmp16D.continue_stmt;_tmp170=(void*)
_tmp16D.break_stmt;_tmp171=_tmp16D.fallthru_clause;_tmp172=(void*)_tmp16D.next_stmt;
_tmp173=_tmp16D.try_depth;{struct Cyc_List_List*ft_typ=0;for(0;vds != 0;vds=vds->tl){
struct Cyc_List_List*_tmp33A;ft_typ=((_tmp33A=_region_malloc(_tmp16E,sizeof(*
_tmp33A)),((_tmp33A->hd=(void*)((void*)((struct Cyc_Absyn_Vardecl*)vds->hd)->type),((
_tmp33A->tl=ft_typ,_tmp33A))))));}{struct Cyc_Tcenv_CList*_tmp175=(struct Cyc_Tcenv_CList*)
Cyc_List_imp_rev(ft_typ);struct _tuple4*_tmp33D;struct Cyc_Tcenv_CtrlEnv*_tmp33C;
struct Cyc_Tcenv_CtrlEnv*_tmp176=(_tmp33C=_region_malloc(r,sizeof(*_tmp33C)),((
_tmp33C->ctrl_rgn=_tmp16E,((_tmp33C->continue_stmt=(void*)_tmp16F,((_tmp33C->break_stmt=(
void*)_tmp170,((_tmp33C->fallthru_clause=((_tmp33D=_region_malloc(_tmp16E,
sizeof(*_tmp33D)),((_tmp33D->f1=clause,((_tmp33D->f2=new_tvs,((_tmp33D->f3=
_tmp175,_tmp33D)))))))),((_tmp33C->next_stmt=(void*)_tmp172,((_tmp33C->try_depth=
_tmp173,_tmp33C)))))))))))));struct Cyc_Tcenv_Fenv*_tmp33E;struct Cyc_Tcenv_Fenv*
_tmp177=(_tmp33E=_region_malloc(r,sizeof(*_tmp33E)),((_tmp33E->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp163,((_tmp33E->type_vars=(struct Cyc_List_List*)_tmp164,((_tmp33E->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp165,((_tmp33E->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp166),((_tmp33E->encloser=(struct Cyc_Absyn_Stmt*)
_tmp167,((_tmp33E->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)_tmp176,((_tmp33E->capability=(
void*)((void*)_tmp169),((_tmp33E->curr_rgn=(void*)((void*)_tmp16A),((_tmp33E->in_notreadctxt=(
int)_tmp16B,((_tmp33E->fnrgn=(struct _RegionHandle*)r,_tmp33E)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp177);}}}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){const char*_tmp33F;struct Cyc_Tcenv_Fenv*
fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(te,((_tmp33F="clear_fallthru",
_tag_dyneither(_tmp33F,sizeof(char),15)))));struct Cyc_Tcenv_Fenv _tmp17C;struct
Cyc_Tcenv_CtrlEnv*_tmp17D;struct Cyc_Tcenv_Fenv*_tmp17B=fe;_tmp17C=*_tmp17B;
_tmp17D=_tmp17C.ctrl_env;_tmp17D->fallthru_clause=0;return Cyc_Tcenv_put_fenv(r,
te,fe);}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_loop(struct _RegionHandle*r,struct
Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*continue_dest);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_in_loop(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*continue_dest){
struct Cyc_Tcenv_Fenv _tmp181;struct Cyc_Tcenv_SharedFenv*_tmp182;struct Cyc_List_List*
_tmp183;struct Cyc_RgnOrder_RgnPO*_tmp184;struct Cyc_Tcenv_Bindings*_tmp185;struct
Cyc_Absyn_Stmt*_tmp186;struct Cyc_Tcenv_CtrlEnv*_tmp187;void*_tmp188;void*_tmp189;
int _tmp18A;const char*_tmp340;struct Cyc_Tcenv_Fenv*_tmp180=Cyc_Tcenv_get_fenv(te,((
_tmp340="set_in_loop",_tag_dyneither(_tmp340,sizeof(char),12))));_tmp181=*
_tmp180;_tmp182=_tmp181.shared;_tmp183=_tmp181.type_vars;_tmp184=_tmp181.region_order;
_tmp185=_tmp181.locals;_tmp186=_tmp181.encloser;_tmp187=_tmp181.ctrl_env;_tmp188=(
void*)_tmp181.capability;_tmp189=(void*)_tmp181.curr_rgn;_tmp18A=_tmp181.in_notreadctxt;{
struct Cyc_Tcenv_Stmt_j_struct*_tmp34B;struct Cyc_Tcenv_Stmt_j_struct _tmp34A;
struct Cyc_Tcenv_Stmt_j_struct*_tmp349;struct Cyc_Tcenv_Stmt_j_struct _tmp348;
struct Cyc_Tcenv_CtrlEnv*_tmp347;struct Cyc_Tcenv_CtrlEnv*new_cenv=(_tmp347=
_region_malloc(r,sizeof(*_tmp347)),((_tmp347->ctrl_rgn=r,((_tmp347->continue_stmt=(
void*)((void*)((_tmp34B=_region_malloc(r,sizeof(*_tmp34B)),((_tmp34B[0]=((
_tmp34A.tag=0,((_tmp34A.f1=continue_dest,_tmp34A)))),_tmp34B))))),((_tmp347->break_stmt=(
void*)((void*)((void*)_tmp187->next_stmt)),((_tmp347->next_stmt=(void*)((void*)((
_tmp349=_region_malloc(r,sizeof(*_tmp349)),((_tmp349[0]=((_tmp348.tag=0,((
_tmp348.f1=continue_dest,_tmp348)))),_tmp349))))),((_tmp347->fallthru_clause=(
struct _tuple4*)_tmp187->fallthru_clause,((_tmp347->try_depth=_tmp187->try_depth,
_tmp347)))))))))))));struct Cyc_Tcenv_Fenv*_tmp34C;struct Cyc_Tcenv_Fenv*new_fenv=(
_tmp34C=_region_malloc(r,sizeof(*_tmp34C)),((_tmp34C->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp182,((_tmp34C->type_vars=(struct Cyc_List_List*)_tmp183,((_tmp34C->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp184,((_tmp34C->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp185),((_tmp34C->encloser=(struct Cyc_Absyn_Stmt*)
_tmp186,((_tmp34C->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)new_cenv,((_tmp34C->capability=(
void*)((void*)_tmp188),((_tmp34C->curr_rgn=(void*)((void*)_tmp189),((_tmp34C->in_notreadctxt=(
int)_tmp18A,((_tmp34C->fnrgn=(struct _RegionHandle*)r,_tmp34C)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,new_fenv);}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_try(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_try(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){const char*_tmp34D;struct Cyc_Tcenv_Fenv*
fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(te,((_tmp34D="enter_try",
_tag_dyneither(_tmp34D,sizeof(char),10)))));struct Cyc_Tcenv_Fenv _tmp192;struct
Cyc_Tcenv_CtrlEnv*_tmp193;struct Cyc_Tcenv_Fenv*_tmp191=fe;_tmp192=*_tmp191;
_tmp193=_tmp192.ctrl_env;++ _tmp193->try_depth;return Cyc_Tcenv_put_fenv(r,te,fe);}
int Cyc_Tcenv_get_try_depth(struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_get_try_depth(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmp197;struct Cyc_Tcenv_CtrlEnv*
_tmp198;const char*_tmp34E;struct Cyc_Tcenv_Fenv*_tmp196=Cyc_Tcenv_get_fenv(te,((
_tmp34E="get_try_depth",_tag_dyneither(_tmp34E,sizeof(char),14))));_tmp197=*
_tmp196;_tmp198=_tmp197.ctrl_env;return _tmp198->try_depth;}struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_set_in_switch(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_set_in_switch(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te){const char*
_tmp34F;struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_copy_fenv_new_ctrl(r,Cyc_Tcenv_get_fenv(
te,((_tmp34F="set_in_switch",_tag_dyneither(_tmp34F,sizeof(char),14)))));struct
Cyc_Tcenv_Fenv _tmp19A;struct Cyc_Tcenv_CtrlEnv*_tmp19B;struct Cyc_Tcenv_Fenv*
_tmp199=fe;_tmp19A=*_tmp199;_tmp19B=_tmp19A.ctrl_env;(void*)(_tmp19B->break_stmt=(
void*)((void*)_tmp19B->next_stmt));(void*)(_tmp19B->next_stmt=(void*)((void*)1));
return Cyc_Tcenv_put_fenv(r,te,fe);}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_next(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*j);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_next(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*j){struct Cyc_Tcenv_SharedFenv*
_tmp19F;struct Cyc_List_List*_tmp1A0;struct Cyc_RgnOrder_RgnPO*_tmp1A1;struct Cyc_Tcenv_Bindings*
_tmp1A2;struct Cyc_Absyn_Stmt*_tmp1A3;struct Cyc_Tcenv_CtrlEnv*_tmp1A4;void*
_tmp1A5;void*_tmp1A6;int _tmp1A7;const char*_tmp350;struct Cyc_Tcenv_Fenv _tmp19E=*
Cyc_Tcenv_get_fenv(te,((_tmp350="set_next",_tag_dyneither(_tmp350,sizeof(char),9))));
_tmp19F=_tmp19E.shared;_tmp1A0=_tmp19E.type_vars;_tmp1A1=_tmp19E.region_order;
_tmp1A2=_tmp19E.locals;_tmp1A3=_tmp19E.encloser;_tmp1A4=_tmp19E.ctrl_env;_tmp1A5=(
void*)_tmp19E.capability;_tmp1A6=(void*)_tmp19E.curr_rgn;_tmp1A7=_tmp19E.in_notreadctxt;{
struct Cyc_Tcenv_CtrlEnv*_tmp351;struct Cyc_Tcenv_CtrlEnv*new_cenv=(_tmp351=
_region_malloc(r,sizeof(*_tmp351)),((_tmp351->ctrl_rgn=r,((_tmp351->continue_stmt=(
void*)((void*)((void*)_tmp1A4->continue_stmt)),((_tmp351->break_stmt=(void*)((
void*)((void*)_tmp1A4->break_stmt)),((_tmp351->next_stmt=(void*)j,((_tmp351->fallthru_clause=(
struct _tuple4*)_tmp1A4->fallthru_clause,((_tmp351->try_depth=_tmp1A4->try_depth,
_tmp351)))))))))))));struct Cyc_Tcenv_Fenv*_tmp352;struct Cyc_Tcenv_Fenv*new_fenv=(
_tmp352=_region_malloc(r,sizeof(*_tmp352)),((_tmp352->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp19F,((_tmp352->type_vars=(struct Cyc_List_List*)_tmp1A0,((_tmp352->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp1A1,((_tmp352->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp1A2),((_tmp352->encloser=(struct Cyc_Absyn_Stmt*)
_tmp1A3,((_tmp352->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)new_cenv,((_tmp352->capability=(
void*)((void*)_tmp1A5),((_tmp352->curr_rgn=(void*)((void*)_tmp1A6),((_tmp352->in_notreadctxt=(
int)_tmp1A7,((_tmp352->fnrgn=(struct _RegionHandle*)r,_tmp352)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,new_fenv);}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_label(
struct Cyc_Tcenv_Tenv*te,struct _dyneither_ptr*v,struct Cyc_Absyn_Stmt*s);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_label(struct Cyc_Tcenv_Tenv*te,struct _dyneither_ptr*v,struct Cyc_Absyn_Stmt*
s){struct Cyc_Tcenv_Fenv _tmp1AC;struct Cyc_Tcenv_SharedFenv*_tmp1AD;const char*
_tmp353;struct Cyc_Tcenv_Fenv*_tmp1AB=Cyc_Tcenv_get_fenv(te,((_tmp353="add_label",
_tag_dyneither(_tmp353,sizeof(char),10))));_tmp1AC=*_tmp1AB;_tmp1AD=_tmp1AC.shared;{
struct Cyc_Dict_Dict needed=_tmp1AD->needed_labels;struct Cyc_List_List**sl_opt=((
struct Cyc_List_List**(*)(struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_lookup_opt)(
needed,v);struct _RegionHandle*frgn=_tmp1AD->frgn;if(sl_opt != 0){_tmp1AD->needed_labels=((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*,struct Cyc_Dict_Dict,struct
_dyneither_ptr*))Cyc_Dict_rdelete)(frgn,needed,v);{struct Cyc_List_List*_tmp1AE=*
sl_opt;s->non_local_preds=_tmp1AE;for(0;_tmp1AE != 0;_tmp1AE=_tmp1AE->tl){void*
_tmp1AF=(void*)((struct Cyc_Absyn_Stmt*)_tmp1AE->hd)->r;struct Cyc_Absyn_Stmt*
_tmp1B0;struct Cyc_Absyn_Stmt**_tmp1B1;_LL78: if(_tmp1AF <= (void*)1)goto _LL7A;if(*((
int*)_tmp1AF)!= 7)goto _LL7A;_tmp1B0=((struct Cyc_Absyn_Goto_s_struct*)_tmp1AF)->f2;
_tmp1B1=(struct Cyc_Absyn_Stmt**)&((struct Cyc_Absyn_Goto_s_struct*)_tmp1AF)->f2;
_LL79:*_tmp1B1=(struct Cyc_Absyn_Stmt*)s;goto _LL77;_LL7A:;_LL7B:{const char*
_tmp354;((int(*)(struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp354="Tcenv: add_label backpatching of non-goto",
_tag_dyneither(_tmp354,sizeof(char),42))));}goto _LL77;_LL77:;}}}if(((int(*)(
struct Cyc_Dict_Dict d,struct _dyneither_ptr*k))Cyc_Dict_member)(_tmp1AD->seen_labels,
v)){const char*_tmp358;void*_tmp357[1];struct Cyc_String_pa_struct _tmp356;(_tmp356.tag=
0,((_tmp356.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*v),((_tmp357[0]=&
_tmp356,Cyc_Tcutil_terr(s->loc,((_tmp358="Repeated label: %s",_tag_dyneither(
_tmp358,sizeof(char),19))),_tag_dyneither(_tmp357,sizeof(void*),1)))))));}
_tmp1AD->seen_labels=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct
_dyneither_ptr*k,struct Cyc_Absyn_Stmt*v))Cyc_Dict_insert)(_tmp1AD->seen_labels,v,
s);return te;}}int Cyc_Tcenv_all_labels_resolved(struct Cyc_Tcenv_Tenv*te);int Cyc_Tcenv_all_labels_resolved(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmp1B8;struct Cyc_Tcenv_SharedFenv*
_tmp1B9;const char*_tmp359;struct Cyc_Tcenv_Fenv*_tmp1B7=Cyc_Tcenv_get_fenv(te,((
_tmp359="all_labels_resolved",_tag_dyneither(_tmp359,sizeof(char),20))));_tmp1B8=*
_tmp1B7;_tmp1B9=_tmp1B8.shared;return((int(*)(struct Cyc_Dict_Dict d))Cyc_Dict_is_empty)(
_tmp1B9->needed_labels);}struct Cyc_Absyn_Stmt*Cyc_Tcenv_get_encloser(struct Cyc_Tcenv_Tenv*
te);struct Cyc_Absyn_Stmt*Cyc_Tcenv_get_encloser(struct Cyc_Tcenv_Tenv*te){struct
Cyc_Tcenv_Fenv _tmp1BC;struct Cyc_Absyn_Stmt*_tmp1BD;const char*_tmp35A;struct Cyc_Tcenv_Fenv*
_tmp1BB=Cyc_Tcenv_get_fenv(te,((_tmp35A="get_encloser",_tag_dyneither(_tmp35A,
sizeof(char),13))));_tmp1BC=*_tmp1BB;_tmp1BD=_tmp1BC.encloser;return _tmp1BD;}
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct _RegionHandle*r2,struct Cyc_Tcenv_Tenv*
te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct _RegionHandle*r2,struct
Cyc_Tcenv_Tenv*te){if(te->le == 0){struct Cyc_Tcenv_Tenv*_tmp35B;return(_tmp35B=
_region_malloc(r2,sizeof(*_tmp35B)),((_tmp35B->ns=te->ns,((_tmp35B->ae=te->ae,((
_tmp35B->le=0,((_tmp35B->allow_valueof=1,_tmp35B)))))))));}{struct Cyc_Tcenv_SharedFenv*
_tmp1C0;struct Cyc_List_List*_tmp1C1;struct Cyc_RgnOrder_RgnPO*_tmp1C2;struct Cyc_Tcenv_Bindings*
_tmp1C3;struct Cyc_Absyn_Stmt*_tmp1C4;struct Cyc_Tcenv_CtrlEnv*_tmp1C5;void*
_tmp1C6;void*_tmp1C7;int _tmp1C8;struct Cyc_Tcenv_Fenv _tmp1BF=*((struct Cyc_Tcenv_Fenv*)
_check_null(te->le));_tmp1C0=_tmp1BF.shared;_tmp1C1=_tmp1BF.type_vars;_tmp1C2=
_tmp1BF.region_order;_tmp1C3=_tmp1BF.locals;_tmp1C4=_tmp1BF.encloser;_tmp1C5=
_tmp1BF.ctrl_env;_tmp1C6=(void*)_tmp1BF.capability;_tmp1C7=(void*)_tmp1BF.curr_rgn;
_tmp1C8=_tmp1BF.in_notreadctxt;{struct Cyc_Tcenv_Fenv*_tmp35C;struct Cyc_Tcenv_Fenv*
_tmp1C9=(_tmp35C=_region_malloc(r2,sizeof(*_tmp35C)),((_tmp35C->shared=(struct
Cyc_Tcenv_SharedFenv*)_tmp1C0,((_tmp35C->type_vars=(struct Cyc_List_List*)_tmp1C1,((
_tmp35C->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmp1C2,((_tmp35C->locals=(
struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)_tmp1C3),((_tmp35C->encloser=(
struct Cyc_Absyn_Stmt*)_tmp1C4,((_tmp35C->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((
struct Cyc_Tcenv_CtrlEnv*)_tmp1C5),((_tmp35C->capability=(void*)((void*)_tmp1C6),((
_tmp35C->curr_rgn=(void*)((void*)_tmp1C7),((_tmp35C->in_notreadctxt=(int)_tmp1C8,((
_tmp35C->fnrgn=(struct _RegionHandle*)r2,_tmp35C)))))))))))))))))))));struct Cyc_Tcenv_Tenv*
_tmp35D;return(_tmp35D=_region_malloc(r2,sizeof(*_tmp35D)),((_tmp35D->ns=te->ns,((
_tmp35D->ae=te->ae,((_tmp35D->le=_tmp1C9,((_tmp35D->allow_valueof=1,_tmp35D)))))))));}}}
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_encloser(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Absyn_Stmt*s);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_encloser(struct
_RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s){struct Cyc_Tcenv_SharedFenv*
_tmp1CE;struct Cyc_List_List*_tmp1CF;struct Cyc_RgnOrder_RgnPO*_tmp1D0;struct Cyc_Tcenv_Bindings*
_tmp1D1;struct Cyc_Absyn_Stmt*_tmp1D2;struct Cyc_Tcenv_CtrlEnv*_tmp1D3;void*
_tmp1D4;void*_tmp1D5;int _tmp1D6;const char*_tmp35E;struct Cyc_Tcenv_Fenv _tmp1CD=*
Cyc_Tcenv_get_fenv(te,((_tmp35E="set_encloser",_tag_dyneither(_tmp35E,sizeof(
char),13))));_tmp1CE=_tmp1CD.shared;_tmp1CF=_tmp1CD.type_vars;_tmp1D0=_tmp1CD.region_order;
_tmp1D1=_tmp1CD.locals;_tmp1D2=_tmp1CD.encloser;_tmp1D3=_tmp1CD.ctrl_env;_tmp1D4=(
void*)_tmp1CD.capability;_tmp1D5=(void*)_tmp1CD.curr_rgn;_tmp1D6=_tmp1CD.in_notreadctxt;{
struct Cyc_Tcenv_Fenv*_tmp35F;struct Cyc_Tcenv_Fenv*_tmp1D7=(_tmp35F=
_region_malloc(r,sizeof(*_tmp35F)),((_tmp35F->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp1CE,((_tmp35F->type_vars=(struct Cyc_List_List*)_tmp1CF,((_tmp35F->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp1D0,((_tmp35F->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp1D1),((_tmp35F->encloser=(struct Cyc_Absyn_Stmt*)s,((
_tmp35F->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp1D3),((
_tmp35F->capability=(void*)((void*)_tmp1D4),((_tmp35F->curr_rgn=(void*)((void*)
_tmp1D5),((_tmp35F->in_notreadctxt=(int)_tmp1D6,((_tmp35F->fnrgn=(struct
_RegionHandle*)r,_tmp35F)))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,te,
_tmp1D7);}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(struct _RegionHandle*r,
struct Cyc_Tcenv_Tenv*te,void*rgn,int resetable,int opened);struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_region(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,void*rgn,int
resetable,int opened){struct Cyc_Absyn_Tvar*tv;{void*_tmp1D9=Cyc_Tcutil_compress(
rgn);struct Cyc_Absyn_Tvar*_tmp1DA;_LL7D: if(_tmp1D9 <= (void*)4)goto _LL7F;if(*((
int*)_tmp1D9)!= 1)goto _LL7F;_tmp1DA=((struct Cyc_Absyn_VarType_struct*)_tmp1D9)->f1;
_LL7E: tv=_tmp1DA;goto _LL7C;_LL7F:;_LL80:{const char*_tmp360;tv=((struct Cyc_Absyn_Tvar*(*)(
struct _dyneither_ptr msg))Cyc_Tcenv_env_err)(((_tmp360="bad add region",
_tag_dyneither(_tmp360,sizeof(char),15))));}goto _LL7C;_LL7C:;}{struct Cyc_Tcenv_SharedFenv*
_tmp1DE;struct Cyc_List_List*_tmp1DF;struct Cyc_RgnOrder_RgnPO*_tmp1E0;struct Cyc_Tcenv_Bindings*
_tmp1E1;struct Cyc_Absyn_Stmt*_tmp1E2;struct Cyc_Tcenv_CtrlEnv*_tmp1E3;void*
_tmp1E4;void*_tmp1E5;int _tmp1E6;const char*_tmp361;struct Cyc_Tcenv_Fenv _tmp1DD=*
Cyc_Tcenv_get_fenv(te,((_tmp361="add_region",_tag_dyneither(_tmp361,sizeof(char),
11))));_tmp1DE=_tmp1DD.shared;_tmp1DF=_tmp1DD.type_vars;_tmp1E0=_tmp1DD.region_order;
_tmp1E1=_tmp1DD.locals;_tmp1E2=_tmp1DD.encloser;_tmp1E3=_tmp1DD.ctrl_env;_tmp1E4=(
void*)_tmp1DD.capability;_tmp1E5=(void*)_tmp1DD.curr_rgn;_tmp1E6=_tmp1DD.in_notreadctxt;
_tmp1E0=Cyc_RgnOrder_add_youngest(_tmp1DE->frgn,_tmp1E0,tv,resetable,opened);{
struct Cyc_Absyn_JoinEff_struct _tmp374;struct Cyc_Absyn_AccessEff_struct*_tmp373;
struct Cyc_Absyn_AccessEff_struct _tmp372;struct Cyc_List_List*_tmp371;struct Cyc_List_List*
_tmp370;struct Cyc_Absyn_JoinEff_struct*_tmp36F;_tmp1E4=(void*)((_tmp36F=
_cycalloc(sizeof(*_tmp36F)),((_tmp36F[0]=((_tmp374.tag=20,((_tmp374.f1=((_tmp370=
_cycalloc(sizeof(*_tmp370)),((_tmp370->hd=(void*)((void*)((_tmp373=_cycalloc(
sizeof(*_tmp373)),((_tmp373[0]=((_tmp372.tag=19,((_tmp372.f1=(void*)rgn,_tmp372)))),
_tmp373))))),((_tmp370->tl=((_tmp371=_cycalloc(sizeof(*_tmp371)),((_tmp371->hd=(
void*)_tmp1E4,((_tmp371->tl=0,_tmp371)))))),_tmp370)))))),_tmp374)))),_tmp36F))));}{
struct Cyc_Tcenv_Fenv*_tmp375;struct Cyc_Tcenv_Fenv*_tmp1ED=(_tmp375=
_region_malloc(r,sizeof(*_tmp375)),((_tmp375->shared=(struct Cyc_Tcenv_SharedFenv*)
_tmp1DE,((_tmp375->type_vars=(struct Cyc_List_List*)_tmp1DF,((_tmp375->region_order=(
struct Cyc_RgnOrder_RgnPO*)_tmp1E0,((_tmp375->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp1E1),((_tmp375->encloser=(struct Cyc_Absyn_Stmt*)
_tmp1E2,((_tmp375->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp1E3),((_tmp375->capability=(void*)((void*)_tmp1E4),((_tmp375->curr_rgn=(void*)((
void*)_tmp1E5),((_tmp375->in_notreadctxt=(int)_tmp1E6,((_tmp375->fnrgn=(struct
_RegionHandle*)r,_tmp375)))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,te,
_tmp1ED);}}}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(struct _RegionHandle*r,
struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Tvar*
block_rgn);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(struct _RegionHandle*r,
struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Tvar*
block_rgn){struct Cyc_Tcenv_SharedFenv*_tmp1F1;struct Cyc_List_List*_tmp1F2;struct
Cyc_RgnOrder_RgnPO*_tmp1F3;struct Cyc_Tcenv_Bindings*_tmp1F4;struct Cyc_Absyn_Stmt*
_tmp1F5;struct Cyc_Tcenv_CtrlEnv*_tmp1F6;void*_tmp1F7;void*_tmp1F8;int _tmp1F9;
const char*_tmp376;struct Cyc_Tcenv_Fenv _tmp1F0=*Cyc_Tcenv_get_fenv(te,((_tmp376="new_named_block",
_tag_dyneither(_tmp376,sizeof(char),16))));_tmp1F1=_tmp1F0.shared;_tmp1F2=
_tmp1F0.type_vars;_tmp1F3=_tmp1F0.region_order;_tmp1F4=_tmp1F0.locals;_tmp1F5=
_tmp1F0.encloser;_tmp1F6=_tmp1F0.ctrl_env;_tmp1F7=(void*)_tmp1F0.capability;
_tmp1F8=(void*)_tmp1F0.curr_rgn;_tmp1F9=_tmp1F0.in_notreadctxt;{const char*
_tmp377;struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_copy_fenv_old_ctrl(r,Cyc_Tcenv_get_fenv(
te,((_tmp377="new_block",_tag_dyneither(_tmp377,sizeof(char),10)))));struct Cyc_Absyn_VarType_struct
_tmp37A;struct Cyc_Absyn_VarType_struct*_tmp379;void*block_typ=(void*)((_tmp379=
_cycalloc(sizeof(*_tmp379)),((_tmp379[0]=((_tmp37A.tag=1,((_tmp37A.f1=block_rgn,
_tmp37A)))),_tmp379))));{struct Cyc_List_List*_tmp37B;_tmp1F2=((_tmp37B=_cycalloc(
sizeof(*_tmp37B)),((_tmp37B->hd=block_rgn,((_tmp37B->tl=_tmp1F2,_tmp37B))))));}
Cyc_Tcutil_check_unique_tvars(loc,_tmp1F2);_tmp1F3=Cyc_RgnOrder_add_youngest(
_tmp1F1->frgn,_tmp1F3,block_rgn,0,0);{struct Cyc_Absyn_JoinEff_struct _tmp38E;
struct Cyc_Absyn_AccessEff_struct*_tmp38D;struct Cyc_Absyn_AccessEff_struct _tmp38C;
struct Cyc_List_List*_tmp38B;struct Cyc_List_List*_tmp38A;struct Cyc_Absyn_JoinEff_struct*
_tmp389;_tmp1F7=(void*)((_tmp389=_cycalloc(sizeof(*_tmp389)),((_tmp389[0]=((
_tmp38E.tag=20,((_tmp38E.f1=((_tmp38A=_cycalloc(sizeof(*_tmp38A)),((_tmp38A->hd=(
void*)((void*)((_tmp38D=_cycalloc(sizeof(*_tmp38D)),((_tmp38D[0]=((_tmp38C.tag=
19,((_tmp38C.f1=(void*)block_typ,_tmp38C)))),_tmp38D))))),((_tmp38A->tl=((
_tmp38B=_cycalloc(sizeof(*_tmp38B)),((_tmp38B->hd=(void*)_tmp1F7,((_tmp38B->tl=0,
_tmp38B)))))),_tmp38A)))))),_tmp38E)))),_tmp389))));}_tmp1F8=block_typ;{struct
Cyc_Tcenv_Fenv*_tmp38F;struct Cyc_Tcenv_Fenv*_tmp201=(_tmp38F=_region_malloc(r,
sizeof(*_tmp38F)),((_tmp38F->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp1F1,((
_tmp38F->type_vars=(struct Cyc_List_List*)_tmp1F2,((_tmp38F->region_order=(struct
Cyc_RgnOrder_RgnPO*)_tmp1F3,((_tmp38F->locals=(struct Cyc_Tcenv_Bindings*)((
struct Cyc_Tcenv_Bindings*)_tmp1F4),((_tmp38F->encloser=(struct Cyc_Absyn_Stmt*)
_tmp1F5,((_tmp38F->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)
_tmp1F6),((_tmp38F->capability=(void*)((void*)_tmp1F7),((_tmp38F->curr_rgn=(void*)((
void*)_tmp1F8),((_tmp38F->in_notreadctxt=(int)_tmp1F9,((_tmp38F->fnrgn=(struct
_RegionHandle*)r,_tmp38F)))))))))))))))))))));return Cyc_Tcenv_put_fenv(r,te,
_tmp201);}}}static struct Cyc_Absyn_Eq_kb_struct Cyc_Tcenv_rgn_kb={0,(void*)((void*)
3)};struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(struct _RegionHandle*r,struct Cyc_Position_Segment*
loc,struct Cyc_Tcenv_Tenv*te);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(struct
_RegionHandle*r,struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Tenv*te){struct
Cyc_Absyn_Tvar*t=Cyc_Tcutil_new_tvar((void*)& Cyc_Tcenv_rgn_kb);Cyc_Tcutil_add_tvar_identity(
t);return Cyc_Tcenv_new_named_block(r,loc,te,t);}struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(
struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,struct Cyc_Position_Segment*
loc);struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(struct _RegionHandle*
r,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,struct Cyc_Position_Segment*loc){
struct Cyc_Tcenv_SharedFenv*_tmp209;struct Cyc_List_List*_tmp20A;struct Cyc_RgnOrder_RgnPO*
_tmp20B;struct Cyc_Tcenv_Bindings*_tmp20C;struct Cyc_Absyn_Stmt*_tmp20D;struct Cyc_Tcenv_CtrlEnv*
_tmp20E;void*_tmp20F;void*_tmp210;int _tmp211;const char*_tmp390;struct Cyc_Tcenv_Fenv
_tmp208=*Cyc_Tcenv_get_fenv(te,((_tmp390="new_outlives_constraints",
_tag_dyneither(_tmp390,sizeof(char),25))));_tmp209=_tmp208.shared;_tmp20A=
_tmp208.type_vars;_tmp20B=_tmp208.region_order;_tmp20C=_tmp208.locals;_tmp20D=
_tmp208.encloser;_tmp20E=_tmp208.ctrl_env;_tmp20F=(void*)_tmp208.capability;
_tmp210=(void*)_tmp208.curr_rgn;_tmp211=_tmp208.in_notreadctxt;for(0;cs != 0;cs=
cs->tl){_tmp20B=Cyc_RgnOrder_add_outlives_constraint(_tmp209->frgn,_tmp20B,(*((
struct _tuple0*)cs->hd)).f1,(*((struct _tuple0*)cs->hd)).f2,loc);}{struct Cyc_Tcenv_Fenv*
_tmp391;struct Cyc_Tcenv_Fenv*_tmp212=(_tmp391=_region_malloc(r,sizeof(*_tmp391)),((
_tmp391->shared=(struct Cyc_Tcenv_SharedFenv*)_tmp209,((_tmp391->type_vars=(
struct Cyc_List_List*)_tmp20A,((_tmp391->region_order=(struct Cyc_RgnOrder_RgnPO*)
_tmp20B,((_tmp391->locals=(struct Cyc_Tcenv_Bindings*)((struct Cyc_Tcenv_Bindings*)
_tmp20C),((_tmp391->encloser=(struct Cyc_Absyn_Stmt*)_tmp20D,((_tmp391->ctrl_env=(
struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)_tmp20E),((_tmp391->capability=(
void*)((void*)_tmp20F),((_tmp391->curr_rgn=(void*)((void*)_tmp210),((_tmp391->in_notreadctxt=(
int)_tmp211,((_tmp391->fnrgn=(struct _RegionHandle*)r,_tmp391)))))))))))))))))))));
return Cyc_Tcenv_put_fenv(r,te,_tmp212);}}void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*
te);void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv*
_tmp214=te->le;if(_tmp214 == 0)return(void*)2;{struct Cyc_Tcenv_Fenv*fe=(struct Cyc_Tcenv_Fenv*)
_tmp214;struct Cyc_Tcenv_Fenv _tmp216;void*_tmp217;struct Cyc_Tcenv_Fenv*_tmp215=fe;
_tmp216=*_tmp215;_tmp217=(void*)_tmp216.curr_rgn;return _tmp217;}}void Cyc_Tcenv_check_rgn_accessible(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,void*rgn);void Cyc_Tcenv_check_rgn_accessible(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*loc,void*rgn){const char*
_tmp392;struct Cyc_Tcenv_Fenv*fe=Cyc_Tcenv_get_fenv(te,((_tmp392="check_rgn_accessible",
_tag_dyneither(_tmp392,sizeof(char),21))));struct Cyc_Tcenv_Fenv _tmp219;void*
_tmp21A;struct Cyc_RgnOrder_RgnPO*_tmp21B;struct Cyc_Tcenv_Fenv*_tmp218=fe;_tmp219=*
_tmp218;_tmp21A=(void*)_tmp219.capability;_tmp21B=_tmp219.region_order;if(Cyc_Tcutil_region_in_effect(
0,rgn,_tmp21A) || Cyc_Tcutil_region_in_effect(1,rgn,_tmp21A))return;{struct Cyc_Absyn_AccessEff_struct
_tmp395;struct Cyc_Absyn_AccessEff_struct*_tmp394;if(Cyc_RgnOrder_eff_outlives_eff(
_tmp21B,(void*)((_tmp394=_cycalloc(sizeof(*_tmp394)),((_tmp394[0]=((_tmp395.tag=
19,((_tmp395.f1=(void*)rgn,_tmp395)))),_tmp394)))),_tmp21A))return;}{const char*
_tmp399;void*_tmp398[1];struct Cyc_String_pa_struct _tmp397;(_tmp397.tag=0,((
_tmp397.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
rgn)),((_tmp398[0]=& _tmp397,Cyc_Tcutil_terr(loc,((_tmp399="Expression accesses unavailable region %s",
_tag_dyneither(_tmp399,sizeof(char),42))),_tag_dyneither(_tmp398,sizeof(void*),1)))))));}}
void Cyc_Tcenv_check_rgn_resetable(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,void*rgn);void Cyc_Tcenv_check_rgn_resetable(struct Cyc_Tcenv_Tenv*te,struct
Cyc_Position_Segment*loc,void*rgn){Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);{
struct Cyc_Tcenv_Fenv _tmp224;struct Cyc_RgnOrder_RgnPO*_tmp225;const char*_tmp39A;
struct Cyc_Tcenv_Fenv*_tmp223=Cyc_Tcenv_get_fenv(te,((_tmp39A="check_rgn_resetable",
_tag_dyneither(_tmp39A,sizeof(char),20))));_tmp224=*_tmp223;_tmp225=_tmp224.region_order;{
void*_tmp226=Cyc_Tcutil_compress(rgn);struct Cyc_Absyn_Tvar*_tmp227;_LL82: if(
_tmp226 <= (void*)4)goto _LL84;if(*((int*)_tmp226)!= 1)goto _LL84;_tmp227=((struct
Cyc_Absyn_VarType_struct*)_tmp226)->f1;_LL83: if(!Cyc_RgnOrder_is_region_resetable(
_tmp225,_tmp227)){const char*_tmp39E;void*_tmp39D[1];struct Cyc_String_pa_struct
_tmp39C;(_tmp39C.tag=0,((_tmp39C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(rgn)),((_tmp39D[0]=& _tmp39C,Cyc_Tcutil_terr(loc,((_tmp39E="Region %s is not resetable",
_tag_dyneither(_tmp39E,sizeof(char),27))),_tag_dyneither(_tmp39D,sizeof(void*),1)))))));}
return;_LL84:;_LL85: {const char*_tmp3A1;void*_tmp3A0;(_tmp3A0=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp3A1="check_rgn_resetable",
_tag_dyneither(_tmp3A1,sizeof(char),20))),_tag_dyneither(_tmp3A0,sizeof(void*),0)));}
_LL81:;}}}int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*te,void*rt_a,void*
rt_b);int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*te,void*rt_a,void*rt_b){
struct Cyc_Tcenv_Fenv*_tmp22D=te->le;rt_a=Cyc_Tcutil_compress(rt_a);rt_b=Cyc_Tcutil_compress(
rt_b);if(_tmp22D == 0)return rt_a == (void*)3  || rt_a == (void*)2  && rt_b != (void*)3;{
struct Cyc_Tcenv_Fenv*fe=(struct Cyc_Tcenv_Fenv*)_tmp22D;struct Cyc_Tcenv_Fenv
_tmp22F;struct Cyc_RgnOrder_RgnPO*_tmp230;struct Cyc_Tcenv_Fenv*_tmp22E=fe;_tmp22F=*
_tmp22E;_tmp230=_tmp22F.region_order;{struct Cyc_Absyn_AccessEff_struct _tmp3A4;
struct Cyc_Absyn_AccessEff_struct*_tmp3A3;int _tmp231=Cyc_RgnOrder_effect_outlives(
_tmp230,(void*)((_tmp3A3=_cycalloc(sizeof(*_tmp3A3)),((_tmp3A3[0]=((_tmp3A4.tag=
19,((_tmp3A4.f1=(void*)rt_a,_tmp3A4)))),_tmp3A3)))),rt_b);return _tmp231;}}}
struct _tuple6{void*f1;void*f2;struct Cyc_RgnOrder_RgnPO*f3;struct Cyc_Position_Segment*
f4;};void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,void*eff);void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Position_Segment*loc,void*eff){struct Cyc_Tcenv_Fenv _tmp236;void*
_tmp237;struct Cyc_RgnOrder_RgnPO*_tmp238;struct Cyc_Tcenv_SharedFenv*_tmp239;
const char*_tmp3A5;struct Cyc_Tcenv_Fenv*_tmp235=Cyc_Tcenv_get_fenv(te,((_tmp3A5="check_effect_accessible",
_tag_dyneither(_tmp3A5,sizeof(char),24))));_tmp236=*_tmp235;_tmp237=(void*)
_tmp236.capability;_tmp238=_tmp236.region_order;_tmp239=_tmp236.shared;if(Cyc_Tcutil_subset_effect(
0,eff,_tmp237))return;if(Cyc_RgnOrder_eff_outlives_eff(_tmp238,eff,_tmp237))
return;{struct _RegionHandle*frgn=_tmp239->frgn;struct _tuple6*_tmp3A8;struct Cyc_List_List*
_tmp3A7;_tmp239->delayed_effect_checks=((_tmp3A7=_region_malloc(frgn,sizeof(*
_tmp3A7)),((_tmp3A7->hd=((_tmp3A8=_region_malloc(frgn,sizeof(*_tmp3A8)),((
_tmp3A8->f1=_tmp237,((_tmp3A8->f2=eff,((_tmp3A8->f3=_tmp238,((_tmp3A8->f4=loc,
_tmp3A8)))))))))),((_tmp3A7->tl=_tmp239->delayed_effect_checks,_tmp3A7))))));}}
void Cyc_Tcenv_check_delayed_effects(struct Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_check_delayed_effects(
struct Cyc_Tcenv_Tenv*te){struct Cyc_Tcenv_Fenv _tmp23E;struct Cyc_Tcenv_SharedFenv*
_tmp23F;const char*_tmp3A9;struct Cyc_Tcenv_Fenv*_tmp23D=Cyc_Tcenv_get_fenv(te,((
_tmp3A9="check_delayed_effects",_tag_dyneither(_tmp3A9,sizeof(char),22))));
_tmp23E=*_tmp23D;_tmp23F=_tmp23E.shared;{struct Cyc_List_List*_tmp240=_tmp23F->delayed_effect_checks;
for(0;_tmp240 != 0;_tmp240=_tmp240->tl){struct _tuple6 _tmp242;void*_tmp243;void*
_tmp244;struct Cyc_RgnOrder_RgnPO*_tmp245;struct Cyc_Position_Segment*_tmp246;
struct _tuple6*_tmp241=(struct _tuple6*)_tmp240->hd;_tmp242=*_tmp241;_tmp243=
_tmp242.f1;_tmp244=_tmp242.f2;_tmp245=_tmp242.f3;_tmp246=_tmp242.f4;if(Cyc_Tcutil_subset_effect(
1,_tmp244,_tmp243))continue;if(Cyc_RgnOrder_eff_outlives_eff(_tmp245,_tmp244,
_tmp243))continue;{const char*_tmp3AE;void*_tmp3AD[2];struct Cyc_String_pa_struct
_tmp3AC;struct Cyc_String_pa_struct _tmp3AB;(_tmp3AB.tag=0,((_tmp3AB.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp244)),((_tmp3AC.tag=
0,((_tmp3AC.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
_tmp243)),((_tmp3AD[0]=& _tmp3AC,((_tmp3AD[1]=& _tmp3AB,Cyc_Tcutil_terr(_tmp246,((
_tmp3AE="Capability \n%s\ndoes not cover function's effect\n%s",_tag_dyneither(
_tmp3AE,sizeof(char),51))),_tag_dyneither(_tmp3AD,sizeof(void*),2)))))))))))));}}}}
struct _tuple7{struct Cyc_RgnOrder_RgnPO*f1;struct Cyc_List_List*f2;struct Cyc_Position_Segment*
f3;};void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,struct Cyc_Position_Segment*
loc,struct Cyc_List_List*po);void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*
te,struct Cyc_Position_Segment*loc,struct Cyc_List_List*po){struct Cyc_Tcenv_Fenv*
_tmp24B=te->le;if(_tmp24B == 0){for(0;po != 0;po=po->tl){struct Cyc_Absyn_AccessEff_struct
_tmp3B1;struct Cyc_Absyn_AccessEff_struct*_tmp3B0;if(!Cyc_Tcutil_subset_effect(1,(*((
struct _tuple0*)po->hd)).f1,Cyc_Absyn_empty_effect) || !Cyc_Tcutil_subset_effect(
1,(void*)((_tmp3B0=_cycalloc(sizeof(*_tmp3B0)),((_tmp3B0[0]=((_tmp3B1.tag=19,((
_tmp3B1.f1=(void*)(*((struct _tuple0*)po->hd)).f2,_tmp3B1)))),_tmp3B0)))),Cyc_Absyn_empty_effect)){
const char*_tmp3B4;void*_tmp3B3;(_tmp3B3=0,Cyc_Tcutil_terr(loc,((_tmp3B4="the required region ordering is not satisfied here",
_tag_dyneither(_tmp3B4,sizeof(char),51))),_tag_dyneither(_tmp3B3,sizeof(void*),0)));}}
return;}{struct Cyc_Tcenv_Fenv _tmp251;struct Cyc_RgnOrder_RgnPO*_tmp252;struct Cyc_Tcenv_SharedFenv*
_tmp253;struct Cyc_Tcenv_Fenv*_tmp250=(struct Cyc_Tcenv_Fenv*)_tmp24B;_tmp251=*
_tmp250;_tmp252=_tmp251.region_order;_tmp253=_tmp251.shared;if(!Cyc_RgnOrder_satisfies_constraints(
_tmp252,po,(void*)2,0)){struct _tuple7*_tmp3B7;struct Cyc_List_List*_tmp3B6;
_tmp253->delayed_constraint_checks=((_tmp3B6=_region_malloc(_tmp253->frgn,
sizeof(*_tmp3B6)),((_tmp3B6->hd=((_tmp3B7=_region_malloc(_tmp253->frgn,sizeof(*
_tmp3B7)),((_tmp3B7->f1=_tmp252,((_tmp3B7->f2=po,((_tmp3B7->f3=loc,_tmp3B7)))))))),((
_tmp3B6->tl=_tmp253->delayed_constraint_checks,_tmp3B6))))));}}}void Cyc_Tcenv_check_delayed_constraints(
struct Cyc_Tcenv_Tenv*te);void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*
te){struct Cyc_Tcenv_Fenv _tmp258;struct Cyc_Tcenv_SharedFenv*_tmp259;const char*
_tmp3B8;struct Cyc_Tcenv_Fenv*_tmp257=Cyc_Tcenv_get_fenv(te,((_tmp3B8="check_delayed_constraints",
_tag_dyneither(_tmp3B8,sizeof(char),26))));_tmp258=*_tmp257;_tmp259=_tmp258.shared;{
struct Cyc_List_List*_tmp25A=_tmp259->delayed_constraint_checks;for(0;_tmp25A != 0;
_tmp25A=_tmp25A->tl){struct _tuple7 _tmp25C;struct Cyc_RgnOrder_RgnPO*_tmp25D;
struct Cyc_List_List*_tmp25E;struct Cyc_Position_Segment*_tmp25F;struct _tuple7*
_tmp25B=(struct _tuple7*)_tmp25A->hd;_tmp25C=*_tmp25B;_tmp25D=_tmp25C.f1;_tmp25E=
_tmp25C.f2;_tmp25F=_tmp25C.f3;if(!Cyc_RgnOrder_satisfies_constraints(_tmp25D,
_tmp25E,(void*)2,1)){const char*_tmp3BB;void*_tmp3BA;(_tmp3BA=0,Cyc_Tcutil_terr(
_tmp25F,((_tmp3BB="the required region ordering is not satisfied here",
_tag_dyneither(_tmp3BB,sizeof(char),51))),_tag_dyneither(_tmp3BA,sizeof(void*),0)));}}}}
struct _tuple8{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct
Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(struct _RegionHandle*r,struct Cyc_Position_Segment*
loc,struct Cyc_Absyn_Fndecl*fd);struct Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(struct
_RegionHandle*r,struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Fndecl*fd){struct
Cyc_Tcenv_Bindings*locals=0;struct _dyneither_ptr*_tmp3C8;const char*_tmp3C7;void*
_tmp3C6[1];struct Cyc_String_pa_struct _tmp3C5;struct Cyc_Absyn_Tvar*_tmp3C4;struct
Cyc_Absyn_Tvar*rgn0=(_tmp3C4=_cycalloc(sizeof(*_tmp3C4)),((_tmp3C4->name=((
_tmp3C8=_cycalloc(sizeof(*_tmp3C8)),((_tmp3C8[0]=(struct _dyneither_ptr)((_tmp3C5.tag=
0,((_tmp3C5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*(*fd->name).f2),((
_tmp3C6[0]=& _tmp3C5,Cyc_aprintf(((_tmp3C7="`%s",_tag_dyneither(_tmp3C7,sizeof(
char),4))),_tag_dyneither(_tmp3C6,sizeof(void*),1)))))))),_tmp3C8)))),((_tmp3C4->identity=
Cyc_Tcutil_new_tvar_id(),((_tmp3C4->kind=(void*)((void*)& Cyc_Tcenv_rgn_kb),
_tmp3C4)))))));struct Cyc_List_List*_tmp3C9;struct Cyc_List_List*_tmp262=(_tmp3C9=
_cycalloc(sizeof(*_tmp3C9)),((_tmp3C9->hd=rgn0,((_tmp3C9->tl=fd->tvs,_tmp3C9)))));
Cyc_Tcutil_check_unique_tvars(loc,_tmp262);{struct Cyc_RgnOrder_RgnPO*_tmp263=Cyc_RgnOrder_initial_fn_po(
r,fd->tvs,fd->rgn_po,(void*)((struct Cyc_Core_Opt*)_check_null(fd->effect))->v,
rgn0,loc);struct Cyc_Absyn_VarType_struct _tmp3CC;struct Cyc_Absyn_VarType_struct*
_tmp3CB;void*param_rgn=(void*)((_tmp3CB=_cycalloc(sizeof(*_tmp3CB)),((_tmp3CB[0]=((
_tmp3CC.tag=1,((_tmp3CC.f1=rgn0,_tmp3CC)))),_tmp3CB))));struct Cyc_Core_Opt*
_tmp3CD;struct Cyc_Core_Opt*param_rgn_opt=(_tmp3CD=_cycalloc(sizeof(*_tmp3CD)),((
_tmp3CD->v=(void*)param_rgn,_tmp3CD)));struct Cyc_List_List*_tmp264=0;{struct Cyc_List_List*
_tmp265=fd->args;for(0;_tmp265 != 0;_tmp265=_tmp265->tl){struct Cyc_Absyn_Vardecl
_tmp3D7;union Cyc_Absyn_Nmspace_union _tmp3D6;struct _tuple1*_tmp3D5;struct Cyc_Absyn_Vardecl*
_tmp3D4;struct Cyc_Absyn_Vardecl*_tmp266=(_tmp3D4=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)
* 1),((_tmp3D4[0]=((_tmp3D7.sc=(void*)((void*)2),((_tmp3D7.name=((_tmp3D5=
_cycalloc(sizeof(*_tmp3D5)),((_tmp3D5->f1=(union Cyc_Absyn_Nmspace_union)((union
Cyc_Absyn_Nmspace_union)(((_tmp3D6.Loc_n).tag=0,_tmp3D6))),((_tmp3D5->f2=(*((
struct _tuple8*)_tmp265->hd)).f1,_tmp3D5)))))),((_tmp3D7.tq=(*((struct _tuple8*)
_tmp265->hd)).f2,((_tmp3D7.type=(void*)(*((struct _tuple8*)_tmp265->hd)).f3,((
_tmp3D7.initializer=0,((_tmp3D7.rgn=param_rgn_opt,((_tmp3D7.attributes=0,((
_tmp3D7.escapes=0,_tmp3D7)))))))))))))))),_tmp3D4)));{struct Cyc_List_List _tmp3DA;
struct Cyc_List_List*_tmp3D9;_tmp264=((_tmp3D9=_cycalloc(sizeof(struct Cyc_List_List)
* 1),((_tmp3D9[0]=((_tmp3DA.hd=_tmp266,((_tmp3DA.tl=_tmp264,_tmp3DA)))),_tmp3D9))));}{
struct Cyc_Absyn_Param_b_struct*_tmp3E0;struct Cyc_Absyn_Param_b_struct _tmp3DF;
struct Cyc_Tcenv_Bindings*_tmp3DE;locals=((_tmp3DE=_region_malloc(r,sizeof(*
_tmp3DE)),((_tmp3DE->v=(*((struct _tuple8*)_tmp265->hd)).f1,((_tmp3DE->b=(void*)((
void*)((_tmp3E0=_cycalloc(sizeof(*_tmp3E0)),((_tmp3E0[0]=((_tmp3DF.tag=2,((
_tmp3DF.f1=_tmp266,_tmp3DF)))),_tmp3E0))))),((_tmp3DE->tl=locals,_tmp3DE))))))));}}}
if(fd->cyc_varargs != 0){struct Cyc_Core_Opt*_tmp271;struct Cyc_Absyn_Tqual _tmp272;
void*_tmp273;int _tmp274;struct Cyc_Absyn_VarargInfo _tmp270=*((struct Cyc_Absyn_VarargInfo*)
_check_null(fd->cyc_varargs));_tmp271=_tmp270.name;_tmp272=_tmp270.tq;_tmp273=(
void*)_tmp270.type;_tmp274=_tmp270.inject;if(_tmp271 != 0){void*_tmp275=Cyc_Absyn_dyneither_typ(
_tmp273,param_rgn,_tmp272,Cyc_Absyn_false_conref);struct Cyc_Absyn_Vardecl _tmp3EA;
union Cyc_Absyn_Nmspace_union _tmp3E9;struct _tuple1*_tmp3E8;struct Cyc_Absyn_Vardecl*
_tmp3E7;struct Cyc_Absyn_Vardecl*_tmp276=(_tmp3E7=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)
* 1),((_tmp3E7[0]=((_tmp3EA.sc=(void*)((void*)2),((_tmp3EA.name=((_tmp3E8=
_cycalloc(sizeof(*_tmp3E8)),((_tmp3E8->f1=(union Cyc_Absyn_Nmspace_union)((union
Cyc_Absyn_Nmspace_union)(((_tmp3E9.Loc_n).tag=0,_tmp3E9))),((_tmp3E8->f2=(struct
_dyneither_ptr*)_tmp271->v,_tmp3E8)))))),((_tmp3EA.tq=Cyc_Absyn_empty_tqual(0),((
_tmp3EA.type=(void*)_tmp275,((_tmp3EA.initializer=0,((_tmp3EA.rgn=param_rgn_opt,((
_tmp3EA.attributes=0,((_tmp3EA.escapes=0,_tmp3EA)))))))))))))))),_tmp3E7)));{
struct Cyc_List_List*_tmp3EB;_tmp264=((_tmp3EB=_cycalloc(sizeof(*_tmp3EB)),((
_tmp3EB->hd=_tmp276,((_tmp3EB->tl=_tmp264,_tmp3EB))))));}{struct Cyc_Absyn_Param_b_struct*
_tmp3F1;struct Cyc_Absyn_Param_b_struct _tmp3F0;struct Cyc_Tcenv_Bindings*_tmp3EF;
locals=((_tmp3EF=_region_malloc(r,sizeof(*_tmp3EF)),((_tmp3EF->v=(struct
_dyneither_ptr*)_tmp271->v,((_tmp3EF->b=(void*)((void*)((_tmp3F1=_cycalloc(
sizeof(*_tmp3F1)),((_tmp3F1[0]=((_tmp3F0.tag=2,((_tmp3F0.f1=_tmp276,_tmp3F0)))),
_tmp3F1))))),((_tmp3EF->tl=locals,_tmp3EF))))))));}}else{const char*_tmp3F4;void*
_tmp3F3;(_tmp3F3=0,Cyc_Tcutil_terr(loc,((_tmp3F4="missing name for varargs",
_tag_dyneither(_tmp3F4,sizeof(char),25))),_tag_dyneither(_tmp3F3,sizeof(void*),0)));}}{
struct Cyc_Core_Opt _tmp3F7;struct Cyc_Core_Opt*_tmp3F6;fd->param_vardecls=((
_tmp3F6=_cycalloc(sizeof(struct Cyc_Core_Opt)* 1),((_tmp3F6[0]=((_tmp3F7.v=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp264),
_tmp3F7)),_tmp3F6))));}{struct Cyc_Tcenv_SharedFenv*_tmp415;struct Cyc_Tcenv_CtrlEnv*
_tmp414;struct Cyc_Absyn_JoinEff_struct*_tmp413;struct Cyc_List_List*_tmp412;
struct Cyc_List_List*_tmp411;struct Cyc_Absyn_AccessEff_struct _tmp410;struct Cyc_Absyn_AccessEff_struct*
_tmp40F;struct Cyc_Absyn_JoinEff_struct _tmp40E;struct Cyc_Tcenv_Fenv*_tmp40D;
return(_tmp40D=_region_malloc(r,sizeof(*_tmp40D)),((_tmp40D->shared=(struct Cyc_Tcenv_SharedFenv*)((
_tmp415=_region_malloc(r,sizeof(*_tmp415)),((_tmp415->frgn=r,((_tmp415->return_typ=(
void*)((void*)fd->ret_type),((_tmp415->seen_labels=((struct Cyc_Dict_Dict(*)(
struct _RegionHandle*,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(
r,Cyc_strptrcmp),((_tmp415->needed_labels=((struct Cyc_Dict_Dict(*)(struct
_RegionHandle*,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(
r,Cyc_strptrcmp),((_tmp415->delayed_effect_checks=0,((_tmp415->delayed_constraint_checks=
0,_tmp415)))))))))))))),((_tmp40D->type_vars=(struct Cyc_List_List*)_tmp262,((
_tmp40D->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmp263,((_tmp40D->locals=(
struct Cyc_Tcenv_Bindings*)locals,((_tmp40D->encloser=(struct Cyc_Absyn_Stmt*)fd->body,((
_tmp40D->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((_tmp414=_region_malloc(r,sizeof(*
_tmp414)),((_tmp414->ctrl_rgn=r,((_tmp414->continue_stmt=(void*)((void*)0),((
_tmp414->break_stmt=(void*)((void*)0),((_tmp414->fallthru_clause=0,((_tmp414->next_stmt=(
void*)((void*)2),((_tmp414->try_depth=0,_tmp414)))))))))))))),((_tmp40D->capability=(
void*)((void*)((void*)((_tmp413=_cycalloc(sizeof(*_tmp413)),((_tmp413[0]=((
_tmp40E.tag=20,((_tmp40E.f1=((_tmp412=_cycalloc(sizeof(*_tmp412)),((_tmp412->hd=(
void*)((void*)((_tmp40F=_cycalloc(sizeof(*_tmp40F)),((_tmp40F[0]=((_tmp410.tag=
19,((_tmp410.f1=(void*)param_rgn,_tmp410)))),_tmp40F))))),((_tmp412->tl=((
_tmp411=_cycalloc(sizeof(*_tmp411)),((_tmp411->hd=(void*)((void*)((struct Cyc_Core_Opt*)
_check_null(fd->effect))->v),((_tmp411->tl=0,_tmp411)))))),_tmp412)))))),_tmp40E)))),
_tmp413)))))),((_tmp40D->curr_rgn=(void*)((void*)param_rgn),((_tmp40D->in_notreadctxt=(
int)0,((_tmp40D->fnrgn=(struct _RegionHandle*)r,_tmp40D)))))))))))))))))))));}}}
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(struct Cyc_Position_Segment*loc,struct
Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*fd);struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(
struct Cyc_Position_Segment*loc,struct Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*
fd){struct Cyc_Tcenv_Fenv _tmp296;struct Cyc_Tcenv_Bindings*_tmp297;struct Cyc_RgnOrder_RgnPO*
_tmp298;struct Cyc_List_List*_tmp299;struct Cyc_Tcenv_SharedFenv*_tmp29A;struct
_RegionHandle*_tmp29B;struct Cyc_Tcenv_Fenv*_tmp295=old_fenv;_tmp296=*_tmp295;
_tmp297=_tmp296.locals;_tmp298=_tmp296.region_order;_tmp299=_tmp296.type_vars;
_tmp29A=_tmp296.shared;_tmp29B=_tmp296.fnrgn;{struct _RegionHandle*_tmp29C=
_tmp29A->frgn;struct Cyc_Tcenv_Bindings*_tmp29D=(struct Cyc_Tcenv_Bindings*)
_tmp297;struct _dyneither_ptr*_tmp422;const char*_tmp421;void*_tmp420[1];struct Cyc_String_pa_struct
_tmp41F;struct Cyc_Absyn_Tvar*_tmp41E;struct Cyc_Absyn_Tvar*rgn0=(_tmp41E=
_cycalloc(sizeof(*_tmp41E)),((_tmp41E->name=((_tmp422=_cycalloc(sizeof(*_tmp422)),((
_tmp422[0]=(struct _dyneither_ptr)((_tmp41F.tag=0,((_tmp41F.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)*(*fd->name).f2),((_tmp420[0]=& _tmp41F,Cyc_aprintf(((
_tmp421="`%s",_tag_dyneither(_tmp421,sizeof(char),4))),_tag_dyneither(_tmp420,
sizeof(void*),1)))))))),_tmp422)))),((_tmp41E->identity=Cyc_Tcutil_new_tvar_id(),((
_tmp41E->kind=(void*)((void*)& Cyc_Tcenv_rgn_kb),_tmp41E)))))));{struct Cyc_List_List*
_tmp29E=fd->tvs;for(0;_tmp29E != 0;_tmp29E=_tmp29E->tl){if(Cyc_Tcutil_tvar_kind((
struct Cyc_Absyn_Tvar*)_tmp29E->hd)== (void*)3)_tmp298=Cyc_RgnOrder_add_youngest(
_tmp29C,_tmp298,(struct Cyc_Absyn_Tvar*)_tmp29E->hd,0,0);else{if(Cyc_Tcutil_tvar_kind((
struct Cyc_Absyn_Tvar*)_tmp29E->hd)== (void*)4  || Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)
_tmp29E->hd)== (void*)5){const char*_tmp425;void*_tmp424;(_tmp424=0,((int(*)(
struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp425="non-intuitionistic tvar in nested_fenv",
_tag_dyneither(_tmp425,sizeof(char),39))),_tag_dyneither(_tmp424,sizeof(void*),0)));}}}}
_tmp298=Cyc_RgnOrder_add_youngest(_tmp29C,_tmp298,rgn0,0,0);{struct Cyc_List_List*
_tmp426;struct Cyc_List_List*_tmp2A1=(_tmp426=_cycalloc(sizeof(*_tmp426)),((
_tmp426->hd=rgn0,((_tmp426->tl=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_append)(fd->tvs,_tmp299),_tmp426)))));Cyc_Tcutil_check_unique_tvars(
loc,_tmp2A1);{struct Cyc_Absyn_VarType_struct _tmp429;struct Cyc_Absyn_VarType_struct*
_tmp428;void*param_rgn=(void*)((_tmp428=_cycalloc(sizeof(*_tmp428)),((_tmp428[0]=((
_tmp429.tag=1,((_tmp429.f1=rgn0,_tmp429)))),_tmp428))));struct Cyc_Core_Opt*
_tmp42A;struct Cyc_Core_Opt*param_rgn_opt=(_tmp42A=_cycalloc(sizeof(*_tmp42A)),((
_tmp42A->v=(void*)param_rgn,_tmp42A)));struct Cyc_List_List*_tmp2A2=0;{struct Cyc_List_List*
_tmp2A3=fd->args;for(0;_tmp2A3 != 0;_tmp2A3=_tmp2A3->tl){struct Cyc_Absyn_Vardecl
_tmp434;union Cyc_Absyn_Nmspace_union _tmp433;struct _tuple1*_tmp432;struct Cyc_Absyn_Vardecl*
_tmp431;struct Cyc_Absyn_Vardecl*_tmp2A4=(_tmp431=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)
* 1),((_tmp431[0]=((_tmp434.sc=(void*)((void*)2),((_tmp434.name=((_tmp432=
_cycalloc(sizeof(*_tmp432)),((_tmp432->f1=(union Cyc_Absyn_Nmspace_union)((union
Cyc_Absyn_Nmspace_union)(((_tmp433.Loc_n).tag=0,_tmp433))),((_tmp432->f2=(*((
struct _tuple8*)_tmp2A3->hd)).f1,_tmp432)))))),((_tmp434.tq=(*((struct _tuple8*)
_tmp2A3->hd)).f2,((_tmp434.type=(void*)(*((struct _tuple8*)_tmp2A3->hd)).f3,((
_tmp434.initializer=0,((_tmp434.rgn=param_rgn_opt,((_tmp434.attributes=0,((
_tmp434.escapes=0,_tmp434)))))))))))))))),_tmp431)));{struct Cyc_List_List _tmp437;
struct Cyc_List_List*_tmp436;_tmp2A2=((_tmp436=_cycalloc(sizeof(struct Cyc_List_List)
* 1),((_tmp436[0]=((_tmp437.hd=_tmp2A4,((_tmp437.tl=_tmp2A2,_tmp437)))),_tmp436))));}{
struct Cyc_Absyn_Param_b_struct*_tmp43D;struct Cyc_Absyn_Param_b_struct _tmp43C;
struct Cyc_Tcenv_Bindings*_tmp43B;_tmp29D=(struct Cyc_Tcenv_Bindings*)((_tmp43B=
_region_malloc(_tmp29C,sizeof(*_tmp43B)),((_tmp43B->v=(*((struct _tuple8*)_tmp2A3->hd)).f1,((
_tmp43B->b=(void*)((void*)((_tmp43D=_cycalloc(sizeof(*_tmp43D)),((_tmp43D[0]=((
_tmp43C.tag=2,((_tmp43C.f1=_tmp2A4,_tmp43C)))),_tmp43D))))),((_tmp43B->tl=
_tmp29D,_tmp43B))))))));}}}if(fd->cyc_varargs != 0){struct Cyc_Core_Opt*_tmp2AF;
struct Cyc_Absyn_Tqual _tmp2B0;void*_tmp2B1;int _tmp2B2;struct Cyc_Absyn_VarargInfo
_tmp2AE=*((struct Cyc_Absyn_VarargInfo*)_check_null(fd->cyc_varargs));_tmp2AF=
_tmp2AE.name;_tmp2B0=_tmp2AE.tq;_tmp2B1=(void*)_tmp2AE.type;_tmp2B2=_tmp2AE.inject;
if(_tmp2AF != 0){void*_tmp2B3=Cyc_Absyn_dyneither_typ(_tmp2B1,param_rgn,_tmp2B0,
Cyc_Absyn_false_conref);struct Cyc_Absyn_Vardecl _tmp447;union Cyc_Absyn_Nmspace_union
_tmp446;struct _tuple1*_tmp445;struct Cyc_Absyn_Vardecl*_tmp444;struct Cyc_Absyn_Vardecl*
_tmp2B4=(_tmp444=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)* 1),((_tmp444[0]=((
_tmp447.sc=(void*)((void*)2),((_tmp447.name=((_tmp445=_cycalloc(sizeof(*_tmp445)),((
_tmp445->f1=(union Cyc_Absyn_Nmspace_union)((union Cyc_Absyn_Nmspace_union)(((
_tmp446.Loc_n).tag=0,_tmp446))),((_tmp445->f2=(struct _dyneither_ptr*)_tmp2AF->v,
_tmp445)))))),((_tmp447.tq=Cyc_Absyn_empty_tqual(0),((_tmp447.type=(void*)
_tmp2B3,((_tmp447.initializer=0,((_tmp447.rgn=param_rgn_opt,((_tmp447.attributes=
0,((_tmp447.escapes=0,_tmp447)))))))))))))))),_tmp444)));{struct Cyc_List_List*
_tmp448;_tmp2A2=((_tmp448=_cycalloc(sizeof(*_tmp448)),((_tmp448->hd=_tmp2B4,((
_tmp448->tl=_tmp2A2,_tmp448))))));}{struct Cyc_Absyn_Param_b_struct*_tmp44E;
struct Cyc_Absyn_Param_b_struct _tmp44D;struct Cyc_Tcenv_Bindings*_tmp44C;_tmp29D=(
struct Cyc_Tcenv_Bindings*)((_tmp44C=_region_malloc(_tmp29C,sizeof(*_tmp44C)),((
_tmp44C->v=(struct _dyneither_ptr*)_tmp2AF->v,((_tmp44C->b=(void*)((void*)((
_tmp44E=_cycalloc(sizeof(*_tmp44E)),((_tmp44E[0]=((_tmp44D.tag=2,((_tmp44D.f1=
_tmp2B4,_tmp44D)))),_tmp44E))))),((_tmp44C->tl=_tmp29D,_tmp44C))))))));}}else{
const char*_tmp451;void*_tmp450;(_tmp450=0,Cyc_Tcutil_terr(loc,((_tmp451="missing name for varargs",
_tag_dyneither(_tmp451,sizeof(char),25))),_tag_dyneither(_tmp450,sizeof(void*),0)));}}{
struct Cyc_Core_Opt _tmp454;struct Cyc_Core_Opt*_tmp453;fd->param_vardecls=((
_tmp453=_cycalloc(sizeof(struct Cyc_Core_Opt)* 1),((_tmp453[0]=((_tmp454.v=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp2A2),
_tmp454)),_tmp453))));}{struct Cyc_Tcenv_SharedFenv*_tmp472;struct Cyc_Tcenv_CtrlEnv*
_tmp471;struct Cyc_Absyn_JoinEff_struct*_tmp470;struct Cyc_List_List*_tmp46F;
struct Cyc_List_List*_tmp46E;struct Cyc_Absyn_AccessEff_struct _tmp46D;struct Cyc_Absyn_AccessEff_struct*
_tmp46C;struct Cyc_Absyn_JoinEff_struct _tmp46B;struct Cyc_Tcenv_Fenv*_tmp46A;
return(struct Cyc_Tcenv_Fenv*)((_tmp46A=_region_malloc(_tmp29C,sizeof(*_tmp46A)),((
_tmp46A->shared=(struct Cyc_Tcenv_SharedFenv*)((_tmp472=_region_malloc(_tmp29C,
sizeof(*_tmp472)),((_tmp472->frgn=_tmp29C,((_tmp472->return_typ=(void*)((void*)
fd->ret_type),((_tmp472->seen_labels=((struct Cyc_Dict_Dict(*)(struct
_RegionHandle*,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(
_tmp29C,Cyc_strptrcmp),((_tmp472->needed_labels=((struct Cyc_Dict_Dict(*)(struct
_RegionHandle*,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Dict_rempty)(
_tmp29C,Cyc_strptrcmp),((_tmp472->delayed_effect_checks=0,((_tmp472->delayed_constraint_checks=
0,_tmp472)))))))))))))),((_tmp46A->type_vars=(struct Cyc_List_List*)_tmp2A1,((
_tmp46A->region_order=(struct Cyc_RgnOrder_RgnPO*)_tmp298,((_tmp46A->locals=(
struct Cyc_Tcenv_Bindings*)_tmp29D,((_tmp46A->encloser=(struct Cyc_Absyn_Stmt*)fd->body,((
_tmp46A->ctrl_env=(struct Cyc_Tcenv_CtrlEnv*)((struct Cyc_Tcenv_CtrlEnv*)((_tmp471=
_region_malloc(_tmp29C,sizeof(*_tmp471)),((_tmp471->ctrl_rgn=_tmp29C,((_tmp471->continue_stmt=(
void*)((void*)0),((_tmp471->break_stmt=(void*)((void*)0),((_tmp471->fallthru_clause=
0,((_tmp471->next_stmt=(void*)((void*)2),((_tmp471->try_depth=0,_tmp471))))))))))))))),((
_tmp46A->capability=(void*)((void*)((void*)((_tmp470=_cycalloc(sizeof(*_tmp470)),((
_tmp470[0]=((_tmp46B.tag=20,((_tmp46B.f1=((_tmp46F=_cycalloc(sizeof(*_tmp46F)),((
_tmp46F->hd=(void*)((void*)((_tmp46C=_cycalloc(sizeof(*_tmp46C)),((_tmp46C[0]=((
_tmp46D.tag=19,((_tmp46D.f1=(void*)param_rgn,_tmp46D)))),_tmp46C))))),((_tmp46F->tl=((
_tmp46E=_cycalloc(sizeof(*_tmp46E)),((_tmp46E->hd=(void*)((void*)((struct Cyc_Core_Opt*)
_check_null(fd->effect))->v),((_tmp46E->tl=0,_tmp46E)))))),_tmp46F)))))),_tmp46B)))),
_tmp470)))))),((_tmp46A->curr_rgn=(void*)((void*)param_rgn),((_tmp46A->in_notreadctxt=(
int)0,((_tmp46A->fnrgn=(struct _RegionHandle*)_tmp29B,_tmp46A))))))))))))))))))))));}}}}}