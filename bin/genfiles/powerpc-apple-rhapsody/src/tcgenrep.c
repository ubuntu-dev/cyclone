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
void*v;};void*Cyc_Core_identity(void*);extern char Cyc_Core_Invalid_argument[21];
struct Cyc_Core_Invalid_argument_struct{char*tag;struct _dyneither_ptr f1;};extern
char Cyc_Core_Failure[12];struct Cyc_Core_Failure_struct{char*tag;struct
_dyneither_ptr f1;};extern char Cyc_Core_Impossible[15];struct Cyc_Core_Impossible_struct{
char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[14];extern char Cyc_Core_Unreachable[
16];struct Cyc_Core_Unreachable_struct{char*tag;struct _dyneither_ptr f1;};extern
char Cyc_Core_Open_Region[16];extern char Cyc_Core_Free_Region[16];int isalnum(int);
struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};struct Cyc_List_List*Cyc_List_list(
struct _dyneither_ptr);int Cyc_List_length(struct Cyc_List_List*x);struct Cyc_List_List*
Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_map_c(
void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[
18];void*Cyc_List_fold_left(void*(*f)(void*,void*),void*accum,struct Cyc_List_List*
x);void*Cyc_List_fold_right_c(void*(*f)(void*,void*,void*),void*,struct Cyc_List_List*
x,void*accum);struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,
struct Cyc_List_List*y);struct Cyc_List_List*Cyc_List_flatten(struct Cyc_List_List*
x);extern char Cyc_List_Nth[8];int Cyc_List_forall(int(*pred)(void*),struct Cyc_List_List*
x);struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);
struct Cyc_List_List*Cyc_List_zip3(struct Cyc_List_List*x,struct Cyc_List_List*y,
struct Cyc_List_List*z);struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*
f2;};struct _tuple0 Cyc_List_split(struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_tabulate(
int n,void*(*f)(int));struct Cyc_List_List*Cyc_List_filter(int(*f)(void*),struct
Cyc_List_List*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc___cycFILE;struct Cyc_Cstdio___abstractFILE;
struct Cyc_String_pa_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_struct{
int tag;unsigned long f1;};struct Cyc_Double_pa_struct{int tag;double f1;};struct Cyc_LongDouble_pa_struct{
int tag;long double f1;};struct Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{
int tag;unsigned long*f1;};struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,
struct _dyneither_ptr);struct Cyc_ShortPtr_sa_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{
int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{
int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct
_dyneither_ptr f1;};struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dyneither_ptr f1;};
int Cyc_printf(struct _dyneither_ptr,struct _dyneither_ptr);extern char Cyc_FileCloseError[
19];extern char Cyc_FileOpenError[18];struct Cyc_FileOpenError_struct{char*tag;
struct _dyneither_ptr f1;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,
void*);struct _RegionHandle*r;struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[12];
extern char Cyc_Dict_Absent[11];struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,
void*));struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);void**Cyc_Dict_lookup_opt(
struct Cyc_Dict_Dict d,void*k);void Cyc_Dict_iter(void(*f)(void*,void*),struct Cyc_Dict_Dict
d);struct _tuple1{void*f1;void*f2;};struct _tuple1*Cyc_Dict_rchoose(struct
_RegionHandle*r,struct Cyc_Dict_Dict d);struct _tuple1*Cyc_Dict_rchoose(struct
_RegionHandle*,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_delete(struct
Cyc_Dict_Dict,void*);unsigned long Cyc_strlen(struct _dyneither_ptr s);int Cyc_strcmp(
struct _dyneither_ptr s1,struct _dyneither_ptr s2);struct _dyneither_ptr Cyc_strconcat(
struct _dyneither_ptr,struct _dyneither_ptr);struct _dyneither_ptr Cyc_strdup(struct
_dyneither_ptr src);struct Cyc_Hashtable_Table;struct Cyc_Hashtable_Table*Cyc_Hashtable_create(
int sz,int(*cmp)(void*,void*),int(*hash)(void*));void Cyc_Hashtable_insert(struct
Cyc_Hashtable_Table*t,void*key,void*val);void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*
t,void*key);struct Cyc_Lineno_Pos{struct _dyneither_ptr logical_file;struct
_dyneither_ptr line;int line_no;int col;};extern char Cyc_Position_Exit[9];struct Cyc_Position_Segment;
struct Cyc_Position_Segment*Cyc_Position_segment_of_abs(int,int);struct Cyc_Position_Error{
struct _dyneither_ptr source;struct Cyc_Position_Segment*seg;void*kind;struct
_dyneither_ptr desc;};extern char Cyc_Position_Nocontext[14];struct Cyc_Typerep_Int_struct{
int tag;int f1;unsigned int f2;};struct Cyc_Typerep_ThinPtr_struct{int tag;
unsigned int f1;void*f2;};struct Cyc_Typerep_FatPtr_struct{int tag;void*f1;};struct
_tuple2{unsigned int f1;struct _dyneither_ptr f2;void*f3;};struct Cyc_Typerep_Struct_struct{
int tag;struct _dyneither_ptr*f1;unsigned int f2;struct _dyneither_ptr f3;};struct
_tuple3{unsigned int f1;void*f2;};struct Cyc_Typerep_Tuple_struct{int tag;
unsigned int f1;struct _dyneither_ptr f2;};struct _tuple4{unsigned int f1;struct
_dyneither_ptr f2;};struct Cyc_Typerep_TUnion_struct{int tag;struct _dyneither_ptr f1;
struct _dyneither_ptr f2;struct _dyneither_ptr f3;};struct Cyc_Typerep_TUnionField_struct{
int tag;struct _dyneither_ptr f1;struct _dyneither_ptr f2;unsigned int f3;struct
_dyneither_ptr f4;};struct _tuple5{struct _dyneither_ptr f1;void*f2;};struct Cyc_Typerep_XTUnion_struct{
int tag;struct _dyneither_ptr f1;struct _dyneither_ptr f2;};struct Cyc_Typerep_Union_struct{
int tag;struct _dyneither_ptr*f1;int f2;struct _dyneither_ptr f3;};struct Cyc_Typerep_Enum_struct{
int tag;struct _dyneither_ptr*f1;int f2;struct _dyneither_ptr f3;};unsigned int Cyc_Typerep_size_type(
void*rep);struct Cyc_timespec{long tv_sec;int tv_nsec;};struct Cyc_tm{int tm_sec;int
tm_min;int tm_hour;int tm_mday;int tm_mon;int tm_year;int tm_wday;int tm_yday;int
tm_isdst;long tm_gmtoff;char*tm_zone;};struct Cyc_Absyn_Loc_n_struct{int tag;};
struct Cyc_Absyn_Rel_n_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{
int tag;struct Cyc_List_List*f1;};union Cyc_Absyn_Nmspace_union{struct Cyc_Absyn_Loc_n_struct
Loc_n;struct Cyc_Absyn_Rel_n_struct Rel_n;struct Cyc_Absyn_Abs_n_struct Abs_n;};
struct _tuple6{union Cyc_Absyn_Nmspace_union f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Conref;
struct Cyc_Absyn_Tqual{int print_const;int q_volatile;int q_restrict;int real_const;
struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Eq_constr_struct{int tag;void*f1;
};struct Cyc_Absyn_Forward_constr_struct{int tag;struct Cyc_Absyn_Conref*f1;};
struct Cyc_Absyn_No_constr_struct{int tag;};union Cyc_Absyn_Constraint_union{struct
Cyc_Absyn_Eq_constr_struct Eq_constr;struct Cyc_Absyn_Forward_constr_struct
Forward_constr;struct Cyc_Absyn_No_constr_struct No_constr;};struct Cyc_Absyn_Conref{
union Cyc_Absyn_Constraint_union v;};struct Cyc_Absyn_Eq_kb_struct{int tag;void*f1;}
;struct Cyc_Absyn_Unknown_kb_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_struct{
int tag;struct Cyc_Core_Opt*f1;void*f2;};struct Cyc_Absyn_Tvar{struct _dyneither_ptr*
name;int identity;void*kind;};struct Cyc_Absyn_Upper_b_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_PtrLoc{struct Cyc_Position_Segment*ptr_loc;struct Cyc_Position_Segment*
rgn_loc;struct Cyc_Position_Segment*zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;
struct Cyc_Absyn_Conref*nullable;struct Cyc_Absyn_Conref*bounds;struct Cyc_Absyn_Conref*
zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_typ;
struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{
struct Cyc_Core_Opt*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{
struct Cyc_List_List*tvars;struct Cyc_Core_Opt*effect;void*ret_typ;struct Cyc_List_List*
args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*
rgn_po;struct Cyc_List_List*attributes;};struct Cyc_Absyn_UnknownTunionInfo{struct
_tuple6*name;int is_xtunion;int is_flat;};struct Cyc_Absyn_UnknownTunion_struct{int
tag;struct Cyc_Absyn_UnknownTunionInfo f1;};struct Cyc_Absyn_KnownTunion_struct{int
tag;struct Cyc_Absyn_Tuniondecl**f1;};union Cyc_Absyn_TunionInfoU_union{struct Cyc_Absyn_UnknownTunion_struct
UnknownTunion;struct Cyc_Absyn_KnownTunion_struct KnownTunion;};struct Cyc_Absyn_TunionInfo{
union Cyc_Absyn_TunionInfoU_union tunion_info;struct Cyc_List_List*targs;struct Cyc_Core_Opt*
rgn;};struct Cyc_Absyn_UnknownTunionFieldInfo{struct _tuple6*tunion_name;struct
_tuple6*field_name;int is_xtunion;};struct Cyc_Absyn_UnknownTunionfield_struct{int
tag;struct Cyc_Absyn_UnknownTunionFieldInfo f1;};struct Cyc_Absyn_KnownTunionfield_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};union Cyc_Absyn_TunionFieldInfoU_union{
struct Cyc_Absyn_UnknownTunionfield_struct UnknownTunionfield;struct Cyc_Absyn_KnownTunionfield_struct
KnownTunionfield;};struct Cyc_Absyn_TunionFieldInfo{union Cyc_Absyn_TunionFieldInfoU_union
field_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownAggr_struct{int tag;
void*f1;struct _tuple6*f2;};struct Cyc_Absyn_KnownAggr_struct{int tag;struct Cyc_Absyn_Aggrdecl**
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
int tag;struct _tuple6*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*
f1;};struct Cyc_Absyn_DynRgnType_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple6*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
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
_tuple6*f1;void*f2;};struct Cyc_Absyn_UnknownId_e_struct{int tag;struct _tuple6*f1;
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
f1;};struct _tuple7{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual f2;void*f3;};
struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple7*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;
int f4;};struct Cyc_Absyn_Struct_e_struct{int tag;struct _tuple6*f1;struct Cyc_List_List*
f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Tunion_e_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*f2;struct Cyc_Absyn_Tunionfield*
f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct _tuple6*f1;struct Cyc_Absyn_Enumdecl*
f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{int tag;
struct _tuple6*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Valueof_e_struct{int tag;void*f1;
};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;void*r;struct Cyc_Position_Segment*
loc;void*annot;};struct Cyc_Absyn_Exp_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};
struct _tuple8{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_struct{
int tag;struct _tuple8 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{int tag;struct
Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct _dyneither_ptr*f1;
struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _tuple8 f2;struct _tuple8 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Fallthru_s_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_struct{
int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_struct{
int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple8 f2;};struct Cyc_Absyn_TryCatch_s_struct{
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
f2;};struct Cyc_Absyn_UnknownId_p_struct{int tag;struct _tuple6*f1;};struct Cyc_Absyn_UnknownCall_p_struct{
int tag;struct _tuple6*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;struct Cyc_Core_Opt*
topt;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*
pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*
body;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Global_b_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Param_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Local_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{void*sc;struct
_tuple6*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;
struct Cyc_Core_Opt*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{
void*sc;int is_inline;struct _tuple6*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
effect;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;struct Cyc_Core_Opt*
cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;
struct Cyc_List_List*attributes;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*
name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*
attributes;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct
Cyc_List_List*rgn_po;struct Cyc_List_List*fields;};struct Cyc_Absyn_Aggrdecl{void*
kind;void*sc;struct _tuple6*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*
impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Tunionfield{struct _tuple6*
name;struct Cyc_List_List*typs;struct Cyc_Position_Segment*loc;void*sc;};struct Cyc_Absyn_Tuniondecl{
void*sc;struct _tuple6*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int
is_xtunion;int is_flat;};struct Cyc_Absyn_Enumfield{struct _tuple6*name;struct Cyc_Absyn_Exp*
tag;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Enumdecl{void*sc;struct
_tuple6*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct
_tuple6*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
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
f2;};struct Cyc_Absyn_Using_d_struct{int tag;struct _tuple6*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ExternC_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*
r;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];int Cyc_Absyn_qvar_cmp(
struct _tuple6*,struct _tuple6*);void*Cyc_Absyn_conref_val(struct Cyc_Absyn_Conref*
x);extern struct Cyc_Absyn_Conref*Cyc_Absyn_true_conref;extern struct Cyc_Absyn_Conref*
Cyc_Absyn_false_conref;extern void*Cyc_Absyn_char_typ;extern void*Cyc_Absyn_uint_typ;
void*Cyc_Absyn_const_string_typ(void*rgn);void*Cyc_Absyn_star_typ(void*t,void*
rgn,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Conref*zero_term);void*Cyc_Absyn_at_typ(
void*t,void*rgn,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Conref*zero_term);void*
Cyc_Absyn_dyneither_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Conref*
zero_term);void*Cyc_Absyn_array_typ(void*elt_type,struct Cyc_Absyn_Tqual tq,struct
Cyc_Absyn_Exp*num_elts,struct Cyc_Absyn_Conref*zero_term,struct Cyc_Position_Segment*
ztloc);struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst_union,struct
Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_unknownid_exp(struct _tuple6*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_unknowncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct
Cyc_Absyn_Exp*,int user_cast,void*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct
Cyc_Absyn_Exp*Cyc_Absyn_sizeoftyp_exp(void*t,struct Cyc_Position_Segment*);struct
Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,void*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_unresolvedmem_exp(struct Cyc_Core_Opt*,struct Cyc_List_List*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*r,
struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(
struct _tuple6*x,void*t,struct Cyc_Absyn_Exp*init);struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(
union Cyc_Absyn_AggrInfoU_union info);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct
Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs: 1;int qvar_to_Cids: 1;int
add_cyc_prefix: 1;int to_VC: 1;int decls_first: 1;int rewrite_temp_tvars: 1;int
print_all_tvars: 1;int print_all_kinds: 1;int print_all_effects: 1;int
print_using_stmts: 1;int print_externC_stmts: 1;int print_full_evars: 1;int
print_zeroterm: 1;int generate_line_directives: 1;int use_curr_namespace: 1;struct Cyc_List_List*
curr_namespace;};struct _dyneither_ptr Cyc_Absynpp_typ2string(void*);struct
_dyneither_ptr Cyc_Absynpp_typ2cstring(void*);struct _tuple9{unsigned int f1;int f2;
};struct _tuple9 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);struct Cyc_Set_Set;
extern char Cyc_Set_Absent[11];struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*
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
_dyneither_ptr ap);void Cyc_Tcutil_terr(struct Cyc_Position_Segment*,struct
_dyneither_ptr fmt,struct _dyneither_ptr ap);void*Cyc_Tcutil_compress(void*t);int
Cyc_Tcutil_typecmp(void*,void*);void*Cyc_Tcutil_substitute(struct Cyc_List_List*,
void*);void Cyc_Tcutil_check_valid_toplevel_type(struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*,void*);struct Cyc_Tcgenrep_RepInfo{struct Cyc_List_List*
decls;struct Cyc_Absyn_Exp*exp;struct Cyc_List_List*dependencies;struct Cyc_Core_Opt*
fwd_decl;int emitted;int is_extern;};struct Cyc_Dict_Dict Cyc_Tcgenrep_empty_typerep_dict();
struct Cyc_Dict_Dict Cyc_Tcgenrep_empty_typerep_dict(){return((struct Cyc_Dict_Dict(*)(
int(*cmp)(void*,void*)))Cyc_Dict_empty)(Cyc_Tcutil_typecmp);}void Cyc_Tcgenrep_print_dict_entry(
void*type,struct Cyc_Tcgenrep_RepInfo*info);void Cyc_Tcgenrep_print_dict_entry(
void*type,struct Cyc_Tcgenrep_RepInfo*info){{const char*_tmp288;void*_tmp287[3];
struct Cyc_String_pa_struct _tmp286;struct Cyc_Int_pa_struct _tmp285;struct Cyc_Int_pa_struct
_tmp284;(_tmp284.tag=1,((_tmp284.f1=(unsigned long)info->emitted,((_tmp285.tag=1,((
_tmp285.f1=(unsigned int)info,((_tmp286.tag=0,((_tmp286.f1=(struct _dyneither_ptr)((
struct _dyneither_ptr)Cyc_Absynpp_typ2string(type)),((_tmp287[0]=& _tmp286,((
_tmp287[1]=& _tmp285,((_tmp287[2]=& _tmp284,Cyc_printf(((_tmp288="(%s,%x:%d,",
_tag_dyneither(_tmp288,sizeof(char),11))),_tag_dyneither(_tmp287,sizeof(void*),3)))))))))))))))))));}{
struct Cyc_List_List*p=info->dependencies;for(0;p != 0;p=p->tl){{const char*_tmp28C;
void*_tmp28B[1];struct Cyc_Int_pa_struct _tmp28A;(_tmp28A.tag=1,((_tmp28A.f1=(
unsigned int)((struct Cyc_Tcgenrep_RepInfo*)p->hd),((_tmp28B[0]=& _tmp28A,Cyc_printf(((
_tmp28C="%x",_tag_dyneither(_tmp28C,sizeof(char),3))),_tag_dyneither(_tmp28B,
sizeof(void*),1)))))));}if(p->tl != 0){const char*_tmp28F;void*_tmp28E;(_tmp28E=0,
Cyc_printf(((_tmp28F=",",_tag_dyneither(_tmp28F,sizeof(char),2))),_tag_dyneither(
_tmp28E,sizeof(void*),0)));}}}{const char*_tmp292;void*_tmp291;(_tmp291=0,Cyc_printf(((
_tmp292=")\n",_tag_dyneither(_tmp292,sizeof(char),3))),_tag_dyneither(_tmp291,
sizeof(void*),0)));}}void Cyc_Tcgenrep_print_typerep_dict(struct Cyc_Dict_Dict dict);
void Cyc_Tcgenrep_print_typerep_dict(struct Cyc_Dict_Dict dict){((void(*)(void(*f)(
void*,struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Dict_Dict d))Cyc_Dict_iter)(Cyc_Tcgenrep_print_dict_entry,
dict);}static int Cyc_Tcgenrep_rephash(struct Cyc_Tcgenrep_RepInfo*ri);static int Cyc_Tcgenrep_rephash(
struct Cyc_Tcgenrep_RepInfo*ri){return(int)ri;}static int Cyc_Tcgenrep_repcmp(
struct Cyc_Tcgenrep_RepInfo*r1,struct Cyc_Tcgenrep_RepInfo*r2);static int Cyc_Tcgenrep_repcmp(
struct Cyc_Tcgenrep_RepInfo*r1,struct Cyc_Tcgenrep_RepInfo*r2){unsigned int r1p;
unsigned int r2p;r1p=(unsigned int)r1;r2p=(unsigned int)r2;if(r1 < r2)return - 1;
else{if(r1 == r2)return 0;else{return 1;}}}static struct Cyc_Absyn_Tunionfield*Cyc_Tcgenrep_getField(
struct Cyc_Absyn_Tuniondecl*td,struct _tuple6*fieldname);static struct Cyc_Absyn_Tunionfield*
Cyc_Tcgenrep_getField(struct Cyc_Absyn_Tuniondecl*td,struct _tuple6*fieldname){if(
td->fields == 0){const char*_tmp295;void*_tmp294;(_tmp294=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp295="Could not find field in tuniondecl",
_tag_dyneither(_tmp295,sizeof(char),35))),_tag_dyneither(_tmp294,sizeof(void*),0)));}
else{struct Cyc_List_List*l=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)
_check_null(td->fields))->v;for(0;l != 0;l=l->tl){if(!Cyc_Absyn_qvar_cmp(((struct
Cyc_Absyn_Tunionfield*)l->hd)->name,fieldname))return(struct Cyc_Absyn_Tunionfield*)
l->hd;}}{const char*_tmp298;void*_tmp297;(_tmp297=0,((int(*)(struct _dyneither_ptr
fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp298="Could not find field in tuniondecl",
_tag_dyneither(_tmp298,sizeof(char),35))),_tag_dyneither(_tmp297,sizeof(void*),0)));}}
static char _tmp10[8]="Typerep";static struct _dyneither_ptr Cyc_Tcgenrep_typerep_nm={
_tmp10,_tmp10,_tmp10 + 8};static struct Cyc_List_List Cyc_Tcgenrep_l2={(void*)& Cyc_Tcgenrep_typerep_nm,
0};static struct Cyc_List_List*Cyc_Tcgenrep_dfsvisit(struct Cyc_Tcgenrep_RepInfo*ri,
struct Cyc_Hashtable_Table*visited);static struct Cyc_List_List*Cyc_Tcgenrep_dfsvisit(
struct Cyc_Tcgenrep_RepInfo*ri,struct Cyc_Hashtable_Table*visited){struct
_handler_cons _tmp11;_push_handler(& _tmp11);{int _tmp13=0;if(setjmp(_tmp11.handler))
_tmp13=1;if(!_tmp13){((int(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Tcgenrep_RepInfo*
key))Cyc_Hashtable_lookup)(visited,ri);{struct Cyc_List_List*_tmp14=0;
_npop_handler(0);return _tmp14;};_pop_handler();}else{void*_tmp12=(void*)
_exn_thrown;void*_tmp16=_tmp12;_LL1: if(_tmp16 != Cyc_Core_Not_found)goto _LL3;_LL2:((
void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Tcgenrep_RepInfo*key,int val))Cyc_Hashtable_insert)(
visited,ri,1);{struct Cyc_List_List*ds=0;{struct Cyc_List_List*l=ri->dependencies;
for(0;l != 0;l=l->tl){ds=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct
Cyc_List_List*y))Cyc_List_imp_append)(ds,Cyc_Tcgenrep_dfsvisit((struct Cyc_Tcgenrep_RepInfo*)
l->hd,visited));}}if(ri->emitted == 0){if(ri->fwd_decl != 0){struct Cyc_Absyn_Decl*
_tmp29B[1];struct Cyc_List_List*_tmp29A;ds=((_tmp29A=_cycalloc(sizeof(*_tmp29A)),((
_tmp29A->hd=((_tmp29B[0]=(struct Cyc_Absyn_Decl*)((struct Cyc_Core_Opt*)
_check_null(ri->fwd_decl))->v,((struct Cyc_List_List*(*)(struct _dyneither_ptr))
Cyc_List_list)(_tag_dyneither(_tmp29B,sizeof(struct Cyc_Absyn_Decl*),1)))),((
_tmp29A->tl=ds,_tmp29A))))));}{struct Cyc_List_List*_tmp29C;struct Cyc_List_List*
_tmp19=(_tmp29C=_cycalloc(sizeof(*_tmp29C)),((_tmp29C->hd=ri->decls,((_tmp29C->tl=
0,_tmp29C)))));ri->emitted=1;return((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_imp_append)(ds,_tmp19);}}else{return ds;}}_LL3:;
_LL4:(void)_throw(_tmp16);_LL0:;}}}static struct Cyc_List_List*Cyc_Tcgenrep_dfs(
struct Cyc_Tcgenrep_RepInfo*ri);static struct Cyc_List_List*Cyc_Tcgenrep_dfs(struct
Cyc_Tcgenrep_RepInfo*ri){struct Cyc_Hashtable_Table*tab=((struct Cyc_Hashtable_Table*(*)(
int sz,int(*cmp)(struct Cyc_Tcgenrep_RepInfo*,struct Cyc_Tcgenrep_RepInfo*),int(*
hash)(struct Cyc_Tcgenrep_RepInfo*)))Cyc_Hashtable_create)(53,Cyc_Tcgenrep_repcmp,
Cyc_Tcgenrep_rephash);struct Cyc_List_List*ds=Cyc_Tcgenrep_dfsvisit(ri,tab);
return((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_flatten)(ds);}
static char _tmp1B[11]="Typestruct";static struct _dyneither_ptr Cyc_Tcgenrep_typestruct_str={
_tmp1B,_tmp1B,_tmp1B + 11};static char _tmp1C[4]="Var";static struct _dyneither_ptr
Cyc_Tcgenrep_var_str={_tmp1C,_tmp1C,_tmp1C + 4};static char _tmp1D[4]="Int";static
struct _dyneither_ptr Cyc_Tcgenrep_int_str={_tmp1D,_tmp1D,_tmp1D + 4};static char
_tmp1E[6]="Float";static struct _dyneither_ptr Cyc_Tcgenrep_float_str={_tmp1E,
_tmp1E,_tmp1E + 6};static char _tmp1F[7]="Double";static struct _dyneither_ptr Cyc_Tcgenrep_double_str={
_tmp1F,_tmp1F,_tmp1F + 7};static char _tmp20[8]="ThinPtr";static struct
_dyneither_ptr Cyc_Tcgenrep_thinptr_str={_tmp20,_tmp20,_tmp20 + 8};static char
_tmp21[7]="FatPtr";static struct _dyneither_ptr Cyc_Tcgenrep_fatptr_str={_tmp21,
_tmp21,_tmp21 + 7};static char _tmp22[6]="Tuple";static struct _dyneither_ptr Cyc_Tcgenrep_tuple_str={
_tmp22,_tmp22,_tmp22 + 6};static char _tmp23[12]="TUnionField";static struct
_dyneither_ptr Cyc_Tcgenrep_tunionfield_str={_tmp23,_tmp23,_tmp23 + 12};static char
_tmp24[7]="Struct";static struct _dyneither_ptr Cyc_Tcgenrep_struct_str={_tmp24,
_tmp24,_tmp24 + 7};static char _tmp25[7]="TUnion";static struct _dyneither_ptr Cyc_Tcgenrep_tunion_str={
_tmp25,_tmp25,_tmp25 + 7};static char _tmp26[8]="XTUnion";static struct
_dyneither_ptr Cyc_Tcgenrep_xtunion_str={_tmp26,_tmp26,_tmp26 + 8};static char
_tmp27[6]="Union";static struct _dyneither_ptr Cyc_Tcgenrep_union_str={_tmp27,
_tmp27,_tmp27 + 6};static char _tmp28[5]="Enum";static struct _dyneither_ptr Cyc_Tcgenrep_enum_str={
_tmp28,_tmp28,_tmp28 + 5};static char _tmp29[7]="name_t";static struct _dyneither_ptr
Cyc_Tcgenrep_name_t_str={_tmp29,_tmp29,_tmp29 + 7};static struct _tuple6*Cyc_Tcgenrep_typerep_name(
struct _dyneither_ptr*name);static struct _tuple6*Cyc_Tcgenrep_typerep_name(struct
_dyneither_ptr*name){union Cyc_Absyn_Nmspace_union _tmp29F;struct _tuple6*_tmp29E;
return(_tmp29E=_cycalloc(sizeof(*_tmp29E)),((_tmp29E->f1=(union Cyc_Absyn_Nmspace_union)(((
_tmp29F.Abs_n).tag=2,(((_tmp29F.Abs_n).f1=(struct Cyc_List_List*)& Cyc_Tcgenrep_l2,
_tmp29F)))),((_tmp29E->f2=name,_tmp29E)))));}static int Cyc_Tcgenrep_gen_id_counter=
0;static struct _dyneither_ptr*Cyc_Tcgenrep_new_gen_id(struct _dyneither_ptr name);
static struct _dyneither_ptr*Cyc_Tcgenrep_new_gen_id(struct _dyneither_ptr name){
struct Cyc_Int_pa_struct _tmp2A9;struct Cyc_String_pa_struct _tmp2A8;void*_tmp2A7[2];
const char*_tmp2A6;struct _dyneither_ptr*_tmp2A5;return(_tmp2A5=_cycalloc(sizeof(*
_tmp2A5)),((_tmp2A5[0]=(struct _dyneither_ptr)((_tmp2A9.tag=1,((_tmp2A9.f1=(
unsigned long)Cyc_Tcgenrep_gen_id_counter ++,((_tmp2A8.tag=0,((_tmp2A8.f1=(struct
_dyneither_ptr)((struct _dyneither_ptr)name),((_tmp2A7[0]=& _tmp2A8,((_tmp2A7[1]=&
_tmp2A9,Cyc_aprintf(((_tmp2A6="_gen%s_%d",_tag_dyneither(_tmp2A6,sizeof(char),10))),
_tag_dyneither(_tmp2A7,sizeof(void*),2)))))))))))))),_tmp2A5)));}static void Cyc_Tcgenrep_print_params(
struct Cyc_List_List*l);static void Cyc_Tcgenrep_print_params(struct Cyc_List_List*l){{
const char*_tmp2AC;void*_tmp2AB;(_tmp2AB=0,Cyc_printf(((_tmp2AC="<",
_tag_dyneither(_tmp2AC,sizeof(char),2))),_tag_dyneither(_tmp2AB,sizeof(void*),0)));}{
struct Cyc_List_List*p=l;for(0;p != 0;p=p->tl){const char*_tmp2B1;void*_tmp2B0[2];
struct Cyc_String_pa_struct _tmp2AF;struct Cyc_Int_pa_struct _tmp2AE;(_tmp2AE.tag=1,((
_tmp2AE.f1=(unsigned long)((int)(p->tl != 0?',':' ')),((_tmp2AF.tag=0,((_tmp2AF.f1=(
struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)p->hd)),((
_tmp2B0[0]=& _tmp2AF,((_tmp2B0[1]=& _tmp2AE,Cyc_printf(((_tmp2B1="%s%c",
_tag_dyneither(_tmp2B1,sizeof(char),5))),_tag_dyneither(_tmp2B0,sizeof(void*),2)))))))))))));}}{
const char*_tmp2B4;void*_tmp2B3;(_tmp2B3=0,Cyc_printf(((_tmp2B4=">\n",
_tag_dyneither(_tmp2B4,sizeof(char),3))),_tag_dyneither(_tmp2B3,sizeof(void*),0)));}}
static void Cyc_Tcgenrep_print_tvars(struct Cyc_List_List*l);static void Cyc_Tcgenrep_print_tvars(
struct Cyc_List_List*l){{const char*_tmp2B7;void*_tmp2B6;(_tmp2B6=0,Cyc_printf(((
_tmp2B7="<",_tag_dyneither(_tmp2B7,sizeof(char),2))),_tag_dyneither(_tmp2B6,
sizeof(void*),0)));}{struct Cyc_List_List*p=l;for(0;p != 0;p=p->tl){const char*
_tmp2BC;void*_tmp2BB[2];struct Cyc_String_pa_struct _tmp2BA;struct Cyc_Int_pa_struct
_tmp2B9;(_tmp2B9.tag=1,((_tmp2B9.f1=(unsigned long)((int)(p->tl != 0?',':' ')),((
_tmp2BA.tag=0,((_tmp2BA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*((
struct Cyc_Absyn_Tvar*)p->hd)->name),((_tmp2BB[0]=& _tmp2BA,((_tmp2BB[1]=& _tmp2B9,
Cyc_printf(((_tmp2BC="%s%c",_tag_dyneither(_tmp2BC,sizeof(char),5))),
_tag_dyneither(_tmp2BB,sizeof(void*),2)))))))))))));}}{const char*_tmp2BF;void*
_tmp2BE;(_tmp2BE=0,Cyc_printf(((_tmp2BF=">\n",_tag_dyneither(_tmp2BF,sizeof(char),
3))),_tag_dyneither(_tmp2BE,sizeof(void*),0)));}}static struct _tuple6*Cyc_Tcgenrep_toplevel_name(
struct _dyneither_ptr*name);static struct _tuple6*Cyc_Tcgenrep_toplevel_name(struct
_dyneither_ptr*name){union Cyc_Absyn_Nmspace_union _tmp2C2;struct _tuple6*_tmp2C1;
return(_tmp2C1=_cycalloc(sizeof(*_tmp2C1)),((_tmp2C1->f1=(union Cyc_Absyn_Nmspace_union)(((
_tmp2C2.Rel_n).tag=1,(((_tmp2C2.Rel_n).f1=0,_tmp2C2)))),((_tmp2C1->f2=(struct
_dyneither_ptr*)name,_tmp2C1)))));}static struct _tuple6*Cyc_Tcgenrep_relative_name(
struct Cyc_List_List*pathl,struct _dyneither_ptr*name);static struct _tuple6*Cyc_Tcgenrep_relative_name(
struct Cyc_List_List*pathl,struct _dyneither_ptr*name){union Cyc_Absyn_Nmspace_union
_tmp2C5;struct _tuple6*_tmp2C4;return(_tmp2C4=_cycalloc(sizeof(*_tmp2C4)),((
_tmp2C4->f1=(union Cyc_Absyn_Nmspace_union)(((_tmp2C5.Rel_n).tag=1,(((_tmp2C5.Rel_n).f1=
pathl,_tmp2C5)))),((_tmp2C4->f2=(struct _dyneither_ptr*)name,_tmp2C4)))));}static
struct Cyc_Absyn_Decl*Cyc_Tcgenrep_gen_decl(struct _dyneither_ptr*name,void*type,
struct Cyc_Absyn_Exp*init,struct Cyc_Position_Segment*seg);static struct Cyc_Absyn_Decl*
Cyc_Tcgenrep_gen_decl(struct _dyneither_ptr*name,void*type,struct Cyc_Absyn_Exp*
init,struct Cyc_Position_Segment*seg){struct _tuple6*qvar=Cyc_Tcgenrep_toplevel_name(
name);struct Cyc_Absyn_Vardecl*vd=Cyc_Absyn_new_vardecl(qvar,type,init);(void*)(
vd->sc=(void*)((void*)0));{struct Cyc_Absyn_Var_d_struct _tmp2C8;struct Cyc_Absyn_Var_d_struct*
_tmp2C7;struct Cyc_Absyn_Var_d_struct*r1=(_tmp2C7=_cycalloc(sizeof(*_tmp2C7)),((
_tmp2C7[0]=((_tmp2C8.tag=0,((_tmp2C8.f1=vd,_tmp2C8)))),_tmp2C7)));void*r2=(void*)
r1;return Cyc_Absyn_new_decl(r2,seg);}}static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_gen_vardecl(
struct _tuple6*name,void*type,struct Cyc_Absyn_Exp*init,void*sc,struct Cyc_Position_Segment*
seg);static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_gen_vardecl(struct _tuple6*name,void*
type,struct Cyc_Absyn_Exp*init,void*sc,struct Cyc_Position_Segment*seg){struct
_tuple6*topname=Cyc_Tcgenrep_toplevel_name((*name).f2);struct Cyc_Absyn_Vardecl*
vd=Cyc_Absyn_new_vardecl(topname,type,init);(void*)(vd->sc=(void*)sc);{struct Cyc_Absyn_Var_d_struct
_tmp2CB;struct Cyc_Absyn_Var_d_struct*_tmp2CA;struct Cyc_Absyn_Var_d_struct*r1=(
_tmp2CA=_cycalloc(sizeof(*_tmp2CA)),((_tmp2CA[0]=((_tmp2CB.tag=0,((_tmp2CB.f1=vd,
_tmp2CB)))),_tmp2CA)));void*r2=(void*)r1;return Cyc_Absyn_new_decl(r2,seg);}}
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_string(struct _dyneither_ptr s,struct
Cyc_Position_Segment*seg);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_string(
struct _dyneither_ptr s,struct Cyc_Position_Segment*seg){union Cyc_Absyn_Cnst_union
_tmp2CC;return Cyc_Absyn_const_exp((union Cyc_Absyn_Cnst_union)((union Cyc_Absyn_Cnst_union)(((
_tmp2CC.String_c).tag=5,(((_tmp2CC.String_c).f1=s,_tmp2CC))))),seg);}static
struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_string_cls(struct Cyc_Position_Segment*seg,
struct _dyneither_ptr*s);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_string_cls(
struct Cyc_Position_Segment*seg,struct _dyneither_ptr*s){union Cyc_Absyn_Cnst_union
_tmp2CD;return Cyc_Absyn_const_exp((union Cyc_Absyn_Cnst_union)((union Cyc_Absyn_Cnst_union)(((
_tmp2CD.String_c).tag=5,(((_tmp2CD.String_c).f1=*s,_tmp2CD))))),seg);}static
struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_qvar_string_cls(struct Cyc_Position_Segment*
seg,struct _tuple6*s);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_qvar_string_cls(
struct Cyc_Position_Segment*seg,struct _tuple6*s){union Cyc_Absyn_Cnst_union _tmp2CE;
return Cyc_Absyn_const_exp((union Cyc_Absyn_Cnst_union)((union Cyc_Absyn_Cnst_union)(((
_tmp2CE.String_c).tag=5,(((_tmp2CE.String_c).f1=*(*s).f2,_tmp2CE))))),seg);}
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_int(int i,struct Cyc_Position_Segment*
seg);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_cnst_int(int i,struct Cyc_Position_Segment*
seg){union Cyc_Absyn_Cnst_union _tmp2CF;return Cyc_Absyn_const_exp((union Cyc_Absyn_Cnst_union)((
union Cyc_Absyn_Cnst_union)(((_tmp2CF.Int_c).tag=2,(((_tmp2CF.Int_c).f1=(void*)((
void*)0),(((_tmp2CF.Int_c).f2=i,_tmp2CF))))))),seg);}static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_cnst_int_cls(struct Cyc_Position_Segment*seg,int i);static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_cnst_int_cls(struct Cyc_Position_Segment*seg,int i){union Cyc_Absyn_Cnst_union
_tmp2D0;return Cyc_Absyn_const_exp((union Cyc_Absyn_Cnst_union)((union Cyc_Absyn_Cnst_union)(((
_tmp2D0.Int_c).tag=2,(((_tmp2D0.Int_c).f1=(void*)((void*)0),(((_tmp2D0.Int_c).f2=
i,_tmp2D0))))))),seg);}static int Cyc_Tcgenrep_size_of2int(void*sz);static int Cyc_Tcgenrep_size_of2int(
void*sz){void*_tmp4E=sz;_LL6: if((int)_tmp4E != 0)goto _LL8;_LL7: return 8;_LL8: if((
int)_tmp4E != 1)goto _LLA;_LL9: return 16;_LLA: if((int)_tmp4E != 2)goto _LLC;_LLB:
return 32;_LLC: if((int)_tmp4E != 3)goto _LLE;_LLD: return 32;_LLE: if((int)_tmp4E != 4)
goto _LL5;_LLF: return 64;_LL5:;}static void*Cyc_Tcgenrep_tunion_typ(struct _tuple6*
name);static void*Cyc_Tcgenrep_tunion_typ(struct _tuple6*name){struct Cyc_Absyn_TunionType_struct
_tmp2E3;union Cyc_Absyn_TunionInfoU_union _tmp2E2;struct Cyc_Absyn_UnknownTunionInfo
_tmp2E1;struct Cyc_Core_Opt*_tmp2E0;struct Cyc_Absyn_TunionInfo _tmp2DF;struct Cyc_Absyn_TunionType_struct*
_tmp2DE;return(void*)((_tmp2DE=_cycalloc(sizeof(*_tmp2DE)),((_tmp2DE[0]=((
_tmp2E3.tag=2,((_tmp2E3.f1=((_tmp2DF.tunion_info=(union Cyc_Absyn_TunionInfoU_union)(((
_tmp2E2.UnknownTunion).tag=0,(((_tmp2E2.UnknownTunion).f1=((_tmp2E1.name=name,((
_tmp2E1.is_xtunion=0,((_tmp2E1.is_flat=0,_tmp2E1)))))),_tmp2E2)))),((_tmp2DF.targs=
0,((_tmp2DF.rgn=((_tmp2E0=_cycalloc(sizeof(*_tmp2E0)),((_tmp2E0->v=(void*)((void*)
2),_tmp2E0)))),_tmp2DF)))))),_tmp2E3)))),_tmp2DE))));}static void*Cyc_Tcgenrep_tunionfield_typ(
struct _tuple6*name,struct _tuple6*fieldname);static void*Cyc_Tcgenrep_tunionfield_typ(
struct _tuple6*name,struct _tuple6*fieldname){struct Cyc_Absyn_TunionFieldType_struct
_tmp2F2;union Cyc_Absyn_TunionFieldInfoU_union _tmp2F1;struct Cyc_Absyn_UnknownTunionFieldInfo
_tmp2F0;struct Cyc_Absyn_TunionFieldInfo _tmp2EF;struct Cyc_Absyn_TunionFieldType_struct*
_tmp2EE;return(void*)((_tmp2EE=_cycalloc(sizeof(*_tmp2EE)),((_tmp2EE[0]=((
_tmp2F2.tag=3,((_tmp2F2.f1=((_tmp2EF.field_info=(union Cyc_Absyn_TunionFieldInfoU_union)(((
_tmp2F1.UnknownTunionfield).tag=0,(((_tmp2F1.UnknownTunionfield).f1=((_tmp2F0.tunion_name=
name,((_tmp2F0.field_name=fieldname,((_tmp2F0.is_xtunion=0,_tmp2F0)))))),_tmp2F1)))),((
_tmp2EF.targs=0,_tmp2EF)))),_tmp2F2)))),_tmp2EE))));}static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_call_exp(struct _tuple6*name,struct Cyc_List_List*args,struct Cyc_Position_Segment*
loc);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_call_exp(struct _tuple6*name,struct
Cyc_List_List*args,struct Cyc_Position_Segment*loc){return Cyc_Absyn_unknowncall_exp(
Cyc_Absyn_unknownid_exp(name,loc),args,loc);}static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_tunion_constructor_decl(
struct _tuple6*tunionname,struct _tuple6*fieldname,struct _tuple6*varname,struct Cyc_List_List*
args,void*sc,struct Cyc_Position_Segment*loc);static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_tunion_constructor_decl(
struct _tuple6*tunionname,struct _tuple6*fieldname,struct _tuple6*varname,struct Cyc_List_List*
args,void*sc,struct Cyc_Position_Segment*loc){void*_tmp5A=Cyc_Tcgenrep_tunionfield_typ(
tunionname,fieldname);struct Cyc_Absyn_Exp*_tmp5B=Cyc_Tcgenrep_call_exp(fieldname,
args,loc);struct Cyc_Absyn_Decl*_tmp5C=Cyc_Tcgenrep_gen_vardecl(varname,_tmp5A,(
struct Cyc_Absyn_Exp*)_tmp5B,sc,loc);return _tmp5C;}struct _tuple10{struct Cyc_Absyn_Tqual
f1;void*f2;};static void*Cyc_Tcgenrep_get_second(struct _tuple10*pair);static void*
Cyc_Tcgenrep_get_second(struct _tuple10*pair){return(*pair).f2;}struct _tuple11{
struct _dyneither_ptr*f1;struct Cyc_Absyn_Exp*f2;};static struct _dyneither_ptr*Cyc_Tcgenrep_get_first(
struct _tuple11*pair);static struct _dyneither_ptr*Cyc_Tcgenrep_get_first(struct
_tuple11*pair){return(*pair).f1;}static char _tmp5D[5]="list";static struct
_dyneither_ptr Cyc_Tcgenrep_list_str={_tmp5D,_tmp5D,_tmp5D + 5};static char _tmp5E[5]="List";
static struct _dyneither_ptr Cyc_Tcgenrep_List_str={_tmp5E,_tmp5E,_tmp5E + 5};struct
_tuple12{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_tuple2_exp_cls(struct Cyc_Position_Segment*loc,struct _tuple12*es);
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_tuple2_exp_cls(struct Cyc_Position_Segment*
loc,struct _tuple12*es){struct _tuple12 _tmp60;struct Cyc_Absyn_Exp*_tmp61;struct Cyc_Absyn_Exp*
_tmp62;struct _tuple12*_tmp5F=es;_tmp60=*_tmp5F;_tmp61=_tmp60.f1;_tmp62=_tmp60.f2;{
struct Cyc_Absyn_Exp*_tmp2F3[2];return Cyc_Absyn_tuple_exp(((_tmp2F3[1]=_tmp62,((
_tmp2F3[0]=_tmp61,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp2F3,sizeof(struct Cyc_Absyn_Exp*),2)))))),loc);}}struct
_tuple13{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_tuple3_exp_cls(struct Cyc_Position_Segment*
loc,struct _tuple13*es);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_tuple3_exp_cls(
struct Cyc_Position_Segment*loc,struct _tuple13*es){struct _tuple13 _tmp65;struct Cyc_Absyn_Exp*
_tmp66;struct Cyc_Absyn_Exp*_tmp67;struct Cyc_Absyn_Exp*_tmp68;struct _tuple13*
_tmp64=es;_tmp65=*_tmp64;_tmp66=_tmp65.f1;_tmp67=_tmp65.f2;_tmp68=_tmp65.f3;{
struct Cyc_Absyn_Exp*_tmp2F4[3];return Cyc_Absyn_tuple_exp(((_tmp2F4[2]=_tmp68,((
_tmp2F4[1]=_tmp67,((_tmp2F4[0]=_tmp66,((struct Cyc_List_List*(*)(struct
_dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp2F4,sizeof(struct Cyc_Absyn_Exp*),
3)))))))),loc);}}static char _tmp6A[5]="NULL";static struct _dyneither_ptr Cyc_Tcgenrep_null_str={
_tmp6A,_tmp6A,_tmp6A + 5};static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_list_exp(struct
Cyc_List_List*l,struct Cyc_Position_Segment*loc);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_list_exp(
struct Cyc_List_List*l,struct Cyc_Position_Segment*loc){if(l == 0)return Cyc_Absyn_null_exp(
loc);return Cyc_Tcgenrep_call_exp(Cyc_Tcgenrep_toplevel_name(& Cyc_Tcgenrep_list_str),
l,loc);}struct _tuple14{void*f1;struct Cyc_Position_Segment*f2;};static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_make_offsetof_exp(struct _tuple14*typeloc,int index);static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_make_offsetof_exp(struct _tuple14*typeloc,int index){struct _tuple14
_tmp6C;void*_tmp6D;struct Cyc_Position_Segment*_tmp6E;struct _tuple14*_tmp6B=
typeloc;_tmp6C=*_tmp6B;_tmp6D=_tmp6C.f1;_tmp6E=_tmp6C.f2;{struct Cyc_Absyn_TupleIndex_struct
_tmp2F7;struct Cyc_Absyn_TupleIndex_struct*_tmp2F6;return Cyc_Absyn_offsetof_exp(
_tmp6D,(void*)((_tmp2F6=_cycalloc_atomic(sizeof(*_tmp2F6)),((_tmp2F6[0]=((
_tmp2F7.tag=1,((_tmp2F7.f1=(unsigned int)index,_tmp2F7)))),_tmp2F6)))),_tmp6E);}}
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_get_and_cast_ri_exp(struct Cyc_Position_Segment*
loc,struct Cyc_Tcgenrep_RepInfo*info);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_get_and_cast_ri_exp(
struct Cyc_Position_Segment*loc,struct Cyc_Tcgenrep_RepInfo*info){return Cyc_Absyn_cast_exp(
Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str)),(
struct Cyc_Absyn_Exp*)_check_null(info->exp),1,(void*)0,loc);}static struct
_tuple11*Cyc_Tcgenrep_gen_id_for_exp(struct Cyc_Absyn_Exp*e);static struct _tuple11*
Cyc_Tcgenrep_gen_id_for_exp(struct Cyc_Absyn_Exp*e){const char*_tmp2FA;struct
_tuple11*_tmp2F9;return(_tmp2F9=_cycalloc(sizeof(*_tmp2F9)),((_tmp2F9->f1=Cyc_Tcgenrep_new_gen_id(((
_tmp2FA="tuple",_tag_dyneither(_tmp2FA,sizeof(char),6)))),((_tmp2F9->f2=e,
_tmp2F9)))));}static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_gen_decl_cls(struct
_tuple14*env,struct _tuple11*name_exp);static struct Cyc_Absyn_Decl*Cyc_Tcgenrep_gen_decl_cls(
struct _tuple14*env,struct _tuple11*name_exp){struct _tuple14 _tmp74;void*_tmp75;
struct Cyc_Position_Segment*_tmp76;struct _tuple14*_tmp73=env;_tmp74=*_tmp73;
_tmp75=_tmp74.f1;_tmp76=_tmp74.f2;{struct _tuple11 _tmp78;struct _dyneither_ptr*
_tmp79;struct Cyc_Absyn_Exp*_tmp7A;struct _tuple11*_tmp77=name_exp;_tmp78=*_tmp77;
_tmp79=_tmp78.f1;_tmp7A=_tmp78.f2;return Cyc_Tcgenrep_gen_decl(_tmp79,_tmp75,(
struct Cyc_Absyn_Exp*)_tmp7A,_tmp76);}}struct _tuple15{struct Cyc_List_List*f1;
struct Cyc_Absyn_Exp*f2;};static struct _tuple15*Cyc_Tcgenrep_null_designator_exp(
struct Cyc_Absyn_Exp*e);static struct _tuple15*Cyc_Tcgenrep_null_designator_exp(
struct Cyc_Absyn_Exp*e){struct _tuple15*_tmp2FB;return(_tmp2FB=_cycalloc(sizeof(*
_tmp2FB)),((_tmp2FB->f1=0,((_tmp2FB->f2=e,_tmp2FB)))));}static struct Cyc_Absyn_Exp*
Cyc_Tcgenrep_arr_init_exp(struct Cyc_List_List*l,struct Cyc_Position_Segment*loc);
static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_arr_init_exp(struct Cyc_List_List*l,struct
Cyc_Position_Segment*loc){struct Cyc_List_List*_tmp7C=((struct Cyc_List_List*(*)(
struct _tuple15*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_null_designator_exp,l);return Cyc_Absyn_unresolvedmem_exp(0,_tmp7C,
loc);}static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_address_exp_cls(struct Cyc_Position_Segment*
loc,struct Cyc_Absyn_Exp*e);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_address_exp_cls(
struct Cyc_Position_Segment*loc,struct Cyc_Absyn_Exp*e){return Cyc_Absyn_address_exp(
e,loc);}static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_unknownid_exp_cls(struct Cyc_Position_Segment*
loc,struct _tuple6*e);static struct Cyc_Absyn_Exp*Cyc_Tcgenrep_unknownid_exp_cls(
struct Cyc_Position_Segment*loc,struct _tuple6*e){return Cyc_Absyn_unknownid_exp(e,
loc);}static int Cyc_Tcgenrep_has_bitfield(struct Cyc_Absyn_Aggrfield*sf);static int
Cyc_Tcgenrep_has_bitfield(struct Cyc_Absyn_Aggrfield*sf){return sf->width != 0;}
static int Cyc_Tcgenrep_add_bitfield_sizes(int total,struct Cyc_Absyn_Aggrfield*sf);
static int Cyc_Tcgenrep_add_bitfield_sizes(int total,struct Cyc_Absyn_Aggrfield*sf){
unsigned int _tmp7E;int _tmp7F;struct _tuple9 _tmp7D=Cyc_Evexp_eval_const_uint_exp((
struct Cyc_Absyn_Exp*)_check_null(sf->width));_tmp7E=_tmp7D.f1;_tmp7F=_tmp7D.f2;
if(!_tmp7F){const char*_tmp2FE;void*_tmp2FD;(_tmp2FD=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp2FE="add_bitfield_sizes: sizeof or offsetof in bitfield size",
_tag_dyneither(_tmp2FE,sizeof(char),56))),_tag_dyneither(_tmp2FD,sizeof(void*),0)));}
return(int)(_tmp7E + total);}static void*Cyc_Tcgenrep_select_structfield_type(
struct Cyc_Absyn_Aggrfield*sf);static void*Cyc_Tcgenrep_select_structfield_type(
struct Cyc_Absyn_Aggrfield*sf){{const char*_tmp2FF;if(Cyc_strcmp((struct
_dyneither_ptr)*sf->name,((_tmp2FF="",_tag_dyneither(_tmp2FF,sizeof(char),1))))
== 0){const char*_tmp302;void*_tmp301;(_tmp301=0,((int(*)(struct _dyneither_ptr fmt,
struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp302="gen(): anonymous (padding) structfield not handled yet",
_tag_dyneither(_tmp302,sizeof(char),55))),_tag_dyneither(_tmp301,sizeof(void*),0)));}}
if(Cyc_Tcgenrep_has_bitfield(sf)){const char*_tmp305;void*_tmp304;(_tmp304=0,((
int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp305="gen(): mixed struct bitfields and union bitfields not handled",
_tag_dyneither(_tmp305,sizeof(char),62))),_tag_dyneither(_tmp304,sizeof(void*),0)));}
return(void*)sf->type;}struct _tuple16{struct _dyneither_ptr*f1;void*f2;};static
struct _tuple16*Cyc_Tcgenrep_select_structfield_nmtype(struct Cyc_Absyn_Aggrfield*
sf);static struct _tuple16*Cyc_Tcgenrep_select_structfield_nmtype(struct Cyc_Absyn_Aggrfield*
sf){{const char*_tmp306;if(Cyc_strcmp((struct _dyneither_ptr)*sf->name,((_tmp306="",
_tag_dyneither(_tmp306,sizeof(char),1))))== 0){const char*_tmp309;void*_tmp308;(
_tmp308=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp309="gen(): anonymous (padding) structfield not handled yet",_tag_dyneither(
_tmp309,sizeof(char),55))),_tag_dyneither(_tmp308,sizeof(void*),0)));}}if(Cyc_Tcgenrep_has_bitfield(
sf)){const char*_tmp30C;void*_tmp30B;(_tmp30B=0,((int(*)(struct _dyneither_ptr fmt,
struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp30C="gen(): mixed struct bitfields and union bitfields not handled",
_tag_dyneither(_tmp30C,sizeof(char),62))),_tag_dyneither(_tmp30B,sizeof(void*),0)));}{
struct _tuple16*_tmp30D;return(_tmp30D=_cycalloc(sizeof(*_tmp30D)),((_tmp30D->f1=
sf->name,((_tmp30D->f2=(void*)sf->type,_tmp30D)))));}}struct _tuple17{int f1;
struct _tuple6*f2;};struct _tuple17*Cyc_Tcgenrep_select_enumfield_tagnm(struct Cyc_Absyn_Enumfield*
ef);struct _tuple17*Cyc_Tcgenrep_select_enumfield_tagnm(struct Cyc_Absyn_Enumfield*
ef){if(ef->tag == 0){const char*_tmp310;void*_tmp30F;(_tmp30F=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp310="Enum tag exp should be filled in by now",
_tag_dyneither(_tmp310,sizeof(char),40))),_tag_dyneither(_tmp30F,sizeof(void*),0)));}{
struct _tuple17*_tmp311;return(_tmp311=_cycalloc(sizeof(*_tmp311)),((_tmp311->f1=(
int)(Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(ef->tag))).f1,((
_tmp311->f2=ef->name,_tmp311)))));}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_update_info(
struct Cyc_Dict_Dict dict,void*type,struct Cyc_List_List*decls,struct Cyc_Absyn_Exp*
exp,struct Cyc_List_List*dependencies,struct Cyc_Core_Opt*fwd_decl);static struct
Cyc_Dict_Dict Cyc_Tcgenrep_update_info(struct Cyc_Dict_Dict dict,void*type,struct
Cyc_List_List*decls,struct Cyc_Absyn_Exp*exp,struct Cyc_List_List*dependencies,
struct Cyc_Core_Opt*fwd_decl){struct Cyc_Tcgenrep_RepInfo**_tmp90=((struct Cyc_Tcgenrep_RepInfo**(*)(
struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt)(dict,type);if(_tmp90 != 0){if((*
_tmp90)->decls != 0){Cyc_Tcgenrep_print_typerep_dict(dict);{const char*_tmp314;
void*_tmp313;(_tmp313=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))
Cyc_Tcutil_impos)(((_tmp314="Updating non-forward declaration",_tag_dyneither(
_tmp314,sizeof(char),33))),_tag_dyneither(_tmp313,sizeof(void*),0)));}}(*_tmp90)->decls=
decls;(*_tmp90)->exp=exp;(*_tmp90)->dependencies=dependencies;return dict;}else{
struct Cyc_Tcgenrep_RepInfo*_tmp315;return((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict
d,void*k,struct Cyc_Tcgenrep_RepInfo*v))Cyc_Dict_insert)(dict,type,((_tmp315=
_cycalloc(sizeof(*_tmp315)),((_tmp315->decls=decls,((_tmp315->exp=exp,((_tmp315->dependencies=
dependencies,((_tmp315->fwd_decl=fwd_decl,((_tmp315->emitted=0,((_tmp315->is_extern=
0,_tmp315)))))))))))))));}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_make_fwd_decl_info(
struct Cyc_Dict_Dict dict,void*type,struct Cyc_Absyn_Exp*exp,struct Cyc_Core_Opt*
fwd_decl,int is_extern);static struct Cyc_Dict_Dict Cyc_Tcgenrep_make_fwd_decl_info(
struct Cyc_Dict_Dict dict,void*type,struct Cyc_Absyn_Exp*exp,struct Cyc_Core_Opt*
fwd_decl,int is_extern){struct Cyc_Tcgenrep_RepInfo**_tmp94=((struct Cyc_Tcgenrep_RepInfo**(*)(
struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt)(dict,type);if(_tmp94 != 0){{
const char*_tmp319;void*_tmp318[1];struct Cyc_String_pa_struct _tmp317;(_tmp317.tag=
0,((_tmp317.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(
type)),((_tmp318[0]=& _tmp317,((int(*)(struct _dyneither_ptr fmt,struct
_dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp319="Repinfo for fwd declaration of %s already exists",
_tag_dyneither(_tmp319,sizeof(char),49))),_tag_dyneither(_tmp318,sizeof(void*),1)))))));}
return dict;}else{struct Cyc_Tcgenrep_RepInfo*_tmp31A;return((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,void*k,struct Cyc_Tcgenrep_RepInfo*v))Cyc_Dict_insert)(dict,
type,((_tmp31A=_cycalloc(sizeof(*_tmp31A)),((_tmp31A->decls=0,((_tmp31A->exp=exp,((
_tmp31A->dependencies=0,((_tmp31A->fwd_decl=fwd_decl,((_tmp31A->emitted=0,((
_tmp31A->is_extern=is_extern,_tmp31A)))))))))))))));}}static struct Cyc_Absyn_Tqual
Cyc_Tcgenrep_tq_none={0,0,0,0,0};static struct _tuple10*Cyc_Tcgenrep_tqual_type(
struct Cyc_Absyn_Tqual*tq,void*type);static struct _tuple10*Cyc_Tcgenrep_tqual_type(
struct Cyc_Absyn_Tqual*tq,void*type){struct _tuple10*_tmp31B;return(_tmp31B=
_cycalloc(sizeof(*_tmp31B)),((_tmp31B->f1=*((struct Cyc_Absyn_Tqual*)_check_null(
tq)),((_tmp31B->f2=type,_tmp31B)))));}static void*Cyc_Tcgenrep_tuple_typ(struct
Cyc_List_List*types);static void*Cyc_Tcgenrep_tuple_typ(struct Cyc_List_List*types){
struct Cyc_List_List*_tmp9A=((struct Cyc_List_List*(*)(struct _tuple10*(*f)(struct
Cyc_Absyn_Tqual*,void*),struct Cyc_Absyn_Tqual*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tqual_type,(struct Cyc_Absyn_Tqual*)& Cyc_Tcgenrep_tq_none,types);
struct Cyc_Absyn_TupleType_struct _tmp31E;struct Cyc_Absyn_TupleType_struct*_tmp31D;
struct Cyc_Absyn_TupleType_struct*tuple_type_base=(_tmp31D=_cycalloc(sizeof(*
_tmp31D)),((_tmp31D[0]=((_tmp31E.tag=9,((_tmp31E.f1=_tmp9A,_tmp31E)))),_tmp31D)));
void*tuple_type=(void*)tuple_type_base;return tuple_type;}static struct _tuple15*
Cyc_Tcgenrep_array_decls(void*type,struct Cyc_List_List*exps,struct Cyc_Position_Segment*
loc);static struct _tuple15*Cyc_Tcgenrep_array_decls(void*type,struct Cyc_List_List*
exps,struct Cyc_Position_Segment*loc){struct Cyc_List_List*_tmp9D=((struct Cyc_List_List*(*)(
struct _tuple11*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_gen_id_for_exp,exps);struct _tuple14*_tmp31F;struct Cyc_List_List*
_tmp9E=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Decl*(*f)(struct _tuple14*,
struct _tuple11*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_gen_decl_cls,((
_tmp31F=_cycalloc(sizeof(*_tmp31F)),((_tmp31F->f1=type,((_tmp31F->f2=loc,_tmp31F)))))),
_tmp9D);struct Cyc_List_List*_tmp9F=((struct Cyc_List_List*(*)(struct
_dyneither_ptr*(*f)(struct _tuple11*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_get_first,
_tmp9D);struct Cyc_List_List*_tmpA0=((struct Cyc_List_List*(*)(struct _tuple6*(*f)(
struct _dyneither_ptr*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_toplevel_name,
_tmp9F);struct Cyc_List_List*_tmpA1=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _tuple6*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_unknownid_exp_cls,loc,_tmpA0);
struct Cyc_List_List*_tmpA2=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(
struct Cyc_Position_Segment*,struct Cyc_Absyn_Exp*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_address_exp_cls,loc,_tmpA1);
struct Cyc_Absyn_Exp*_tmpA3=Cyc_Tcgenrep_arr_init_exp(_tmpA2,loc);const char*
_tmp320;struct _dyneither_ptr*_tmpA4=Cyc_Tcgenrep_new_gen_id(((_tmp320="arr",
_tag_dyneither(_tmp320,sizeof(char),4))));void*_tmpA5=Cyc_Absyn_at_typ(type,(
void*)2,Cyc_Tcgenrep_tq_none,Cyc_Absyn_false_conref);void*_tmpA6=Cyc_Absyn_array_typ(
_tmpA5,Cyc_Tcgenrep_tq_none,0,Cyc_Absyn_false_conref,0);struct Cyc_Absyn_Decl*
_tmpA7=Cyc_Tcgenrep_gen_decl(_tmpA4,_tmpA6,(struct Cyc_Absyn_Exp*)_tmpA3,loc);
struct Cyc_Absyn_Exp*_tmpA8=Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_toplevel_name(
_tmpA4),loc);struct Cyc_Absyn_Decl*_tmp323[1];struct _tuple15*_tmp322;return(
_tmp322=_cycalloc(sizeof(*_tmp322)),((_tmp322->f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp9E,((
_tmp323[0]=_tmpA7,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp323,sizeof(struct Cyc_Absyn_Decl*),1))))),((_tmp322->f2=_tmpA8,
_tmp322)))));}static void*Cyc_Tcgenrep_check_tunionfield_and_get_type(struct Cyc_Absyn_Tunionfield*
tuf);static void*Cyc_Tcgenrep_check_tunionfield_and_get_type(struct Cyc_Absyn_Tunionfield*
tuf){struct Cyc_Absyn_TupleType_struct _tmp32D;struct _tuple10*_tmp32C;struct
_tuple10*_tmp32B[1];struct Cyc_Absyn_TupleType_struct*_tmp32A;return(void*)((
_tmp32A=_cycalloc(sizeof(*_tmp32A)),((_tmp32A[0]=((_tmp32D.tag=9,((_tmp32D.f1=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(((
_tmp32B[0]=((_tmp32C=_cycalloc(sizeof(*_tmp32C)),((_tmp32C->f1=Cyc_Tcgenrep_tq_none,((
_tmp32C->f2=Cyc_Absyn_uint_typ,_tmp32C)))))),((struct Cyc_List_List*(*)(struct
_dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp32B,sizeof(struct _tuple10*),1)))),
tuf->typs),_tmp32D)))),_tmp32A))));}static struct _tuple6*Cyc_Tcgenrep_check_tunionfield_and_get_name(
struct Cyc_Absyn_Tunionfield*tuf);static struct _tuple6*Cyc_Tcgenrep_check_tunionfield_and_get_name(
struct Cyc_Absyn_Tunionfield*tuf){return tuf->name;}struct _tuple18{struct _tuple6*
f1;void*f2;};static struct _tuple18*Cyc_Tcgenrep_check_tunionfield_and_get_nmtype(
struct Cyc_Absyn_Tunionfield*tuf);static struct _tuple18*Cyc_Tcgenrep_check_tunionfield_and_get_nmtype(
struct Cyc_Absyn_Tunionfield*tuf){struct Cyc_Absyn_TupleType_struct*_tmp33C;struct
_tuple10*_tmp33B[1];struct _tuple10*_tmp33A;struct Cyc_Absyn_TupleType_struct
_tmp339;struct _tuple18*_tmp338;return(_tmp338=_cycalloc(sizeof(*_tmp338)),((
_tmp338->f1=tuf->name,((_tmp338->f2=(void*)((_tmp33C=_cycalloc(sizeof(*_tmp33C)),((
_tmp33C[0]=((_tmp339.tag=9,((_tmp339.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_imp_append)(((_tmp33B[0]=((_tmp33A=_cycalloc(
sizeof(*_tmp33A)),((_tmp33A->f1=Cyc_Tcgenrep_tq_none,((_tmp33A->f2=Cyc_Absyn_uint_typ,
_tmp33A)))))),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp33B,sizeof(struct _tuple10*),1)))),tuf->typs),_tmp339)))),
_tmp33C)))),_tmp338)))));}static struct _tuple16*Cyc_Tcgenrep_check_xtunionfield_and_get_name_type(
struct Cyc_Absyn_Tunionfield*tuf);static struct _tuple16*Cyc_Tcgenrep_check_xtunionfield_and_get_name_type(
struct Cyc_Absyn_Tunionfield*tuf){struct Cyc_Absyn_TupleType_struct*_tmp34B;struct
_tuple10*_tmp34A[1];struct _tuple10*_tmp349;struct Cyc_Absyn_TupleType_struct
_tmp348;struct _tuple16*_tmp347;return(_tmp347=_cycalloc(sizeof(*_tmp347)),((
_tmp347->f1=(*tuf->name).f2,((_tmp347->f2=(void*)((_tmp34B=_cycalloc(sizeof(*
_tmp34B)),((_tmp34B[0]=((_tmp348.tag=9,((_tmp348.f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(((_tmp34A[0]=((
_tmp349=_cycalloc(sizeof(*_tmp349)),((_tmp349->f1=Cyc_Tcgenrep_tq_none,((_tmp349->f2=
Cyc_Absyn_star_typ(Cyc_Absyn_char_typ,(void*)2,Cyc_Tcgenrep_tq_none,Cyc_Absyn_true_conref),
_tmp349)))))),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp34A,sizeof(struct _tuple10*),1)))),tuf->typs),_tmp348)))),
_tmp34B)))),_tmp347)))));}static int Cyc_Tcgenrep_filter_empty_tunionfield(struct
Cyc_Absyn_Tunionfield*tuf);static int Cyc_Tcgenrep_filter_empty_tunionfield(struct
Cyc_Absyn_Tunionfield*tuf){return tuf->typs != 0;}static int Cyc_Tcgenrep_filter_nonempty_tunionfield(
struct Cyc_Absyn_Tunionfield*tuf);static int Cyc_Tcgenrep_filter_nonempty_tunionfield(
struct Cyc_Absyn_Tunionfield*tuf){return tuf->typs == 0;}static struct _dyneither_ptr*
Cyc_Tcgenrep_get_tunionfield_name(struct Cyc_Absyn_Tunionfield*tuf);static struct
_dyneither_ptr*Cyc_Tcgenrep_get_tunionfield_name(struct Cyc_Absyn_Tunionfield*tuf){
return(*tuf->name).f2;}struct Cyc_Absyn_Aggrfield*Cyc_Tcgenrep_substitute_structfield_type(
struct Cyc_List_List*subst,struct Cyc_Absyn_Aggrfield*sf);struct Cyc_Absyn_Aggrfield*
Cyc_Tcgenrep_substitute_structfield_type(struct Cyc_List_List*subst,struct Cyc_Absyn_Aggrfield*
sf){struct Cyc_Absyn_Aggrfield*_tmp34C;return(_tmp34C=_cycalloc(sizeof(*_tmp34C)),((
_tmp34C->name=sf->name,((_tmp34C->tq=sf->tq,((_tmp34C->type=(void*)Cyc_Tcutil_substitute(
subst,(void*)sf->type),((_tmp34C->width=sf->width,((_tmp34C->attributes=sf->attributes,
_tmp34C)))))))))));}struct _tuple10*Cyc_Tcgenrep_substitute_tqual_type(struct Cyc_List_List*
subst,struct _tuple10*pair);struct _tuple10*Cyc_Tcgenrep_substitute_tqual_type(
struct Cyc_List_List*subst,struct _tuple10*pair){struct _tuple10 _tmpBD;struct Cyc_Absyn_Tqual
_tmpBE;void*_tmpBF;struct _tuple10*_tmpBC=pair;_tmpBD=*_tmpBC;_tmpBE=_tmpBD.f1;
_tmpBF=_tmpBD.f2;{struct _tuple10*_tmp34D;return(_tmp34D=_cycalloc(sizeof(*
_tmp34D)),((_tmp34D->f1=_tmpBE,((_tmp34D->f2=Cyc_Tcutil_substitute(subst,_tmpBF),
_tmp34D)))));}}struct Cyc_Absyn_Tunionfield*Cyc_Tcgenrep_substitute_tunionfield_type(
struct Cyc_List_List*subst,struct Cyc_Absyn_Tunionfield*tf);struct Cyc_Absyn_Tunionfield*
Cyc_Tcgenrep_substitute_tunionfield_type(struct Cyc_List_List*subst,struct Cyc_Absyn_Tunionfield*
tf){struct Cyc_Absyn_Tunionfield*_tmp34E;return(_tmp34E=_cycalloc(sizeof(*_tmp34E)),((
_tmp34E->name=tf->name,((_tmp34E->typs=((struct Cyc_List_List*(*)(struct _tuple10*(*
f)(struct Cyc_List_List*,struct _tuple10*),struct Cyc_List_List*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_substitute_tqual_type,subst,tf->typs),((_tmp34E->loc=
tf->loc,((_tmp34E->sc=(void*)((void*)tf->sc),_tmp34E)))))))));}void*Cyc_Tcgenrep_monomorphize_type(
void*type);void*Cyc_Tcgenrep_monomorphize_type(void*type){void*_tmpC2=Cyc_Tcutil_compress(
type);struct Cyc_Absyn_AggrInfo _tmpC3;union Cyc_Absyn_AggrInfoU_union _tmpC4;struct
Cyc_List_List*_tmpC5;struct Cyc_Absyn_TunionInfo _tmpC6;union Cyc_Absyn_TunionInfoU_union
_tmpC7;struct Cyc_Absyn_Tuniondecl**_tmpC8;struct Cyc_Absyn_Tuniondecl*_tmpC9;
struct Cyc_List_List*_tmpCA;struct Cyc_Core_Opt*_tmpCB;_LL11: if(_tmpC2 <= (void*)4)
goto _LL15;if(*((int*)_tmpC2)!= 10)goto _LL13;_tmpC3=((struct Cyc_Absyn_AggrType_struct*)
_tmpC2)->f1;_tmpC4=_tmpC3.aggr_info;_tmpC5=_tmpC3.targs;_LL12: {struct Cyc_Absyn_Aggrdecl*
_tmpCC=Cyc_Absyn_get_known_aggrdecl(_tmpC4);struct Cyc_List_List*_tmpCD=_tmpCC->tvs;
if(Cyc_List_length(_tmpC5)!= ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(
_tmpCD)){const char*_tmp353;void*_tmp352[2];struct Cyc_Int_pa_struct _tmp351;struct
Cyc_Int_pa_struct _tmp350;(_tmp350.tag=1,((_tmp350.f1=(unsigned long)((int(*)(
struct Cyc_List_List*x))Cyc_List_length)(_tmpCD),((_tmp351.tag=1,((_tmp351.f1=(
unsigned long)Cyc_List_length(_tmpC5),((_tmp352[0]=& _tmp351,((_tmp352[1]=&
_tmp350,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp353="gen(): number of params %d differs from number of tyvars %d",
_tag_dyneither(_tmp353,sizeof(char),60))),_tag_dyneither(_tmp352,sizeof(void*),2)))))))))))));}{
struct Cyc_List_List*_tmpD2=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_zip)(_tmpCD,_tmpC5);struct Cyc_List_List*fields=0;
if(_tmpCC->impl != 0)fields=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*
f)(struct Cyc_List_List*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_substitute_structfield_type,
_tmpD2,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmpCC->impl))->fields);{
struct Cyc_Absyn_AggrdeclImpl*_tmp354;struct Cyc_Absyn_AggrdeclImpl*_tmpD3=_tmpCC->impl
== 0?0:((_tmp354=_cycalloc(sizeof(*_tmp354)),((_tmp354->exist_vars=0,((_tmp354->rgn_po=
0,((_tmp354->fields=fields,_tmp354))))))));struct Cyc_Absyn_Aggrdecl*_tmp355;
struct Cyc_Absyn_Aggrdecl*ad2=(_tmp355=_cycalloc(sizeof(*_tmp355)),((_tmp355->kind=(
void*)((void*)_tmpCC->kind),((_tmp355->sc=(void*)((void*)_tmpCC->sc),((_tmp355->name=
_tmpCC->name,((_tmp355->tvs=0,((_tmp355->impl=_tmpD3,((_tmp355->attributes=
_tmpCC->attributes,_tmp355)))))))))))));struct Cyc_Absyn_AggrType_struct _tmp364;
union Cyc_Absyn_AggrInfoU_union _tmp363;struct Cyc_Absyn_Aggrdecl**_tmp362;struct
Cyc_Absyn_AggrInfo _tmp361;struct Cyc_Absyn_AggrType_struct*_tmp360;return(void*)((
_tmp360=_cycalloc(sizeof(*_tmp360)),((_tmp360[0]=((_tmp364.tag=10,((_tmp364.f1=((
_tmp361.aggr_info=(union Cyc_Absyn_AggrInfoU_union)(((_tmp363.KnownAggr).tag=1,(((
_tmp363.KnownAggr).f1=((_tmp362=_cycalloc(sizeof(*_tmp362)),((_tmp362[0]=ad2,
_tmp362)))),_tmp363)))),((_tmp361.targs=0,_tmp361)))),_tmp364)))),_tmp360))));}}}
_LL13: if(*((int*)_tmpC2)!= 2)goto _LL15;_tmpC6=((struct Cyc_Absyn_TunionType_struct*)
_tmpC2)->f1;_tmpC7=_tmpC6.tunion_info;if((((((struct Cyc_Absyn_TunionType_struct*)
_tmpC2)->f1).tunion_info).KnownTunion).tag != 1)goto _LL15;_tmpC8=(_tmpC7.KnownTunion).f1;
_tmpC9=*_tmpC8;_tmpCA=_tmpC6.targs;_tmpCB=_tmpC6.rgn;_LL14: {struct Cyc_List_List*
_tmpDB=_tmpC9->tvs;if(Cyc_List_length(_tmpCA)!= ((int(*)(struct Cyc_List_List*x))
Cyc_List_length)(_tmpDB)){const char*_tmp369;void*_tmp368[2];struct Cyc_Int_pa_struct
_tmp367;struct Cyc_Int_pa_struct _tmp366;(_tmp366.tag=1,((_tmp366.f1=(
unsigned long)((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmpDB),((_tmp367.tag=
1,((_tmp367.f1=(unsigned long)Cyc_List_length(_tmpCA),((_tmp368[0]=& _tmp367,((
_tmp368[1]=& _tmp366,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp369="gen(): number of params %d differs from number of tyvars %d",
_tag_dyneither(_tmp369,sizeof(char),60))),_tag_dyneither(_tmp368,sizeof(void*),2)))))))))))));}{
struct Cyc_List_List*_tmpE0=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_zip)(_tmpDB,_tmpCA);struct Cyc_Core_Opt*fields=0;
if(_tmpC9->fields != 0){struct Cyc_Core_Opt*_tmp36A;fields=((_tmp36A=_cycalloc(
sizeof(*_tmp36A)),((_tmp36A->v=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tunionfield*(*
f)(struct Cyc_List_List*,struct Cyc_Absyn_Tunionfield*),struct Cyc_List_List*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_substitute_tunionfield_type,
_tmpE0,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmpC9->fields))->v),
_tmp36A))));}{struct Cyc_Absyn_Tuniondecl*_tmp36B;struct Cyc_Absyn_Tuniondecl*
_tmpE2=(_tmp36B=_cycalloc(sizeof(*_tmp36B)),((_tmp36B->sc=(void*)((void*)_tmpC9->sc),((
_tmp36B->name=_tmpC9->name,((_tmp36B->tvs=0,((_tmp36B->fields=fields,((_tmp36B->is_xtunion=
_tmpC9->is_xtunion,((_tmp36B->is_flat=_tmpC9->is_flat,_tmp36B)))))))))))));
struct Cyc_Absyn_TunionType_struct _tmp37A;union Cyc_Absyn_TunionInfoU_union _tmp379;
struct Cyc_Absyn_Tuniondecl**_tmp378;struct Cyc_Absyn_TunionInfo _tmp377;struct Cyc_Absyn_TunionType_struct*
_tmp376;struct Cyc_Absyn_TunionType_struct*_tmpE3=(_tmp376=_cycalloc(sizeof(*
_tmp376)),((_tmp376[0]=((_tmp37A.tag=2,((_tmp37A.f1=((_tmp377.tunion_info=(union
Cyc_Absyn_TunionInfoU_union)(((_tmp379.KnownTunion).tag=1,(((_tmp379.KnownTunion).f1=((
_tmp378=_cycalloc(sizeof(*_tmp378)),((_tmp378[0]=_tmpE2,_tmp378)))),_tmp379)))),((
_tmp377.targs=_tmpCA,((_tmp377.rgn=_tmpCB,_tmp377)))))),_tmp37A)))),_tmp376)));
struct Cyc_Position_Segment*_tmpE4=Cyc_Position_segment_of_abs(0,0);return(void*)
_tmpE3;}}}_LL15:;_LL16: return type;_LL10:;}struct _dyneither_ptr Cyc_Tcgenrep_make_type_cstring(
void*t);struct _dyneither_ptr Cyc_Tcgenrep_make_type_cstring(void*t){struct
_dyneither_ptr s=Cyc_strdup((struct _dyneither_ptr)Cyc_Absynpp_typ2cstring(t));{
int i=0;for(0;i < Cyc_strlen((struct _dyneither_ptr)s);++ i){if(*((char*)
_check_dyneither_subscript(s,sizeof(char),i))== ' '){char _tmp37D;char _tmp37C;
struct _dyneither_ptr _tmp37B;(_tmp37B=_dyneither_ptr_plus(s,sizeof(char),i),((
_tmp37C=*((char*)_check_dyneither_subscript(_tmp37B,sizeof(char),0)),((_tmp37D='_',((
_get_dyneither_size(_tmp37B,sizeof(char))== 1  && (_tmp37C == '\000'  && _tmp37D != '\000')?
_throw_arraybounds(): 1,*((char*)_tmp37B.curr)=_tmp37D)))))));}else{if(!isalnum((
int)*((char*)_check_dyneither_subscript(s,sizeof(char),i))) && *((char*)
_check_dyneither_subscript(s,sizeof(char),i))!= '_'){char _tmp380;char _tmp37F;
struct _dyneither_ptr _tmp37E;(_tmp37E=_dyneither_ptr_plus(s,sizeof(char),i),((
_tmp37F=*((char*)_check_dyneither_subscript(_tmp37E,sizeof(char),0)),((_tmp380=(
char)('0' + *((char*)_check_dyneither_subscript(s,sizeof(char),i))% 10),((
_get_dyneither_size(_tmp37E,sizeof(char))== 1  && (_tmp37F == '\000'  && _tmp380 != '\000')?
_throw_arraybounds(): 1,*((char*)_tmp37E.curr)=_tmp380)))))));}}}}{const char*
_tmp381;return(struct _dyneither_ptr)Cyc_strconcat((struct _dyneither_ptr)s,((
_tmp381="_rep",_tag_dyneither(_tmp381,sizeof(char),5))));}}struct _tuple19{struct
Cyc_Dict_Dict f1;struct Cyc_Tcgenrep_RepInfo*f2;};static struct _tuple19*Cyc_Tcgenrep_lookupRep(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,
void*type);struct _tuple20{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct
_tuple21{struct Cyc_Tcenv_Tenv*f1;struct Cyc_Position_Segment*f2;};static struct
_tuple20*Cyc_Tcgenrep_lookupRepsCls(struct _tuple21*env,void*type,struct _tuple20*
carry);static struct _tuple20*Cyc_Tcgenrep_lookupRepsCls(struct _tuple21*env,void*
type,struct _tuple20*carry){struct _tuple19 _tmpF3;struct Cyc_Dict_Dict _tmpF4;struct
Cyc_Tcgenrep_RepInfo*_tmpF5;struct _tuple19*_tmpF2=Cyc_Tcgenrep_lookupRep((*env).f1,(*
carry).f1,(*env).f2,type);_tmpF3=*_tmpF2;_tmpF4=_tmpF3.f1;_tmpF5=_tmpF3.f2;{
struct Cyc_List_List*_tmp384;struct _tuple20*_tmp383;return(_tmp383=_cycalloc(
sizeof(*_tmp383)),((_tmp383->f1=_tmpF4,((_tmp383->f2=((_tmp384=_cycalloc(sizeof(*
_tmp384)),((_tmp384->hd=_tmpF5,((_tmp384->tl=(*carry).f2,_tmp384)))))),_tmp383)))));}}
static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTuple(struct Cyc_Tcenv_Tenv*te,
struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,struct _tuple6*varname,
void*sc,void*type,struct Cyc_List_List*types);static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTuple(
struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,
struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*types){struct Cyc_Absyn_Exp*
_tmpF8=Cyc_Absyn_sizeoftyp_exp(type,loc);struct _tuple21*_tmp385;struct _tuple21*
_tmpF9=(_tmp385=_cycalloc(sizeof(*_tmp385)),((_tmp385->f1=te,((_tmp385->f2=loc,
_tmp385)))));struct _tuple20*_tmp386;struct _tuple20*_tmpFA=(_tmp386=_cycalloc(
sizeof(*_tmp386)),((_tmp386->f1=dict,((_tmp386->f2=0,_tmp386)))));struct _tuple20
_tmpFC;struct Cyc_Dict_Dict _tmpFD;struct Cyc_List_List*_tmpFE;struct _tuple20*
_tmpFB=((struct _tuple20*(*)(struct _tuple20*(*f)(struct _tuple21*,void*,struct
_tuple20*),struct _tuple21*,struct Cyc_List_List*x,struct _tuple20*accum))Cyc_List_fold_right_c)(
Cyc_Tcgenrep_lookupRepsCls,_tmpF9,types,_tmpFA);_tmpFC=*_tmpFB;_tmpFD=_tmpFC.f1;
_tmpFE=_tmpFC.f2;dict=_tmpFD;{struct Cyc_List_List*_tmpFF=((struct Cyc_List_List*(*)(
int n,int(*f)(int)))Cyc_List_tabulate)(Cyc_List_length(types),(int(*)(int))Cyc_Core_identity);
struct _tuple14*_tmp387;struct Cyc_List_List*_tmp100=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Exp*(*f)(struct _tuple14*,int),struct _tuple14*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_make_offsetof_exp,((_tmp387=_cycalloc(sizeof(*
_tmp387)),((_tmp387->f1=type,((_tmp387->f2=loc,_tmp387)))))),_tmpFF);struct Cyc_List_List*
_tmp101=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Position_Segment*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,loc,_tmpFE);struct Cyc_List_List*
_tmp102=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _tuple12*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tuple2_exp_cls,loc,((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_zip)(_tmp100,_tmp101));void*_tmp388[2];void*
tuple_type=Cyc_Tcgenrep_tuple_typ(((_tmp388[1]=Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str)),((_tmp388[0]=Cyc_Absyn_uint_typ,Cyc_List_list(
_tag_dyneither(_tmp388,sizeof(void*),2)))))));struct _tuple15 _tmp104;struct Cyc_List_List*
_tmp105;struct Cyc_Absyn_Exp*_tmp106;struct _tuple15*_tmp103=Cyc_Tcgenrep_array_decls(
tuple_type,_tmp102,loc);_tmp104=*_tmp103;_tmp105=_tmp104.f1;_tmp106=_tmp104.f2;{
struct Cyc_Absyn_Exp*_tmp389[2];struct Cyc_Absyn_Decl*_tmp107=Cyc_Tcgenrep_tunion_constructor_decl(
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_tuple_str),varname,((_tmp389[1]=_tmp106,((_tmp389[0]=_tmpF8,((
struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(
_tmp389,sizeof(struct Cyc_Absyn_Exp*),2)))))),sc,loc);struct Cyc_Absyn_Decl*
_tmp38A[1];struct Cyc_List_List*_tmp108=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp105,((_tmp38A[0]=_tmp107,((
struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(
_tmp38A,sizeof(struct Cyc_Absyn_Decl*),1)))));return Cyc_Tcgenrep_update_info(dict,
type,_tmp108,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
varname,loc),loc),_tmpFE,0);}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTunionfield(
struct _tuple6*tname,struct _tuple6*fname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict
dict,struct Cyc_Position_Segment*loc,struct _tuple6*varname,void*sc,void*type,
struct Cyc_List_List*types);static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTunionfield(
struct _tuple6*tname,struct _tuple6*fname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict
dict,struct Cyc_Position_Segment*loc,struct _tuple6*varname,void*sc,void*type,
struct Cyc_List_List*types){struct Cyc_Absyn_Exp*_tmp10F=Cyc_Absyn_sizeoftyp_exp(
type,loc);struct _tuple21*_tmp38B;struct _tuple21*_tmp110=(_tmp38B=_cycalloc(
sizeof(*_tmp38B)),((_tmp38B->f1=te,((_tmp38B->f2=loc,_tmp38B)))));struct _tuple20*
_tmp38C;struct _tuple20*_tmp111=(_tmp38C=_cycalloc(sizeof(*_tmp38C)),((_tmp38C->f1=
dict,((_tmp38C->f2=0,_tmp38C)))));struct _tuple20 _tmp113;struct Cyc_Dict_Dict
_tmp114;struct Cyc_List_List*_tmp115;struct _tuple20*_tmp112=((struct _tuple20*(*)(
struct _tuple20*(*f)(struct _tuple21*,void*,struct _tuple20*),struct _tuple21*,
struct Cyc_List_List*x,struct _tuple20*accum))Cyc_List_fold_right_c)(Cyc_Tcgenrep_lookupRepsCls,
_tmp110,types,_tmp111);_tmp113=*_tmp112;_tmp114=_tmp113.f1;_tmp115=_tmp113.f2;
dict=_tmp114;{struct Cyc_List_List*_tmp116=((struct Cyc_List_List*(*)(int n,int(*f)(
int)))Cyc_List_tabulate)(Cyc_List_length(types),(int(*)(int))Cyc_Core_identity);
struct _tuple14*_tmp38D;struct Cyc_List_List*_tmp117=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Exp*(*f)(struct _tuple14*,int),struct _tuple14*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_make_offsetof_exp,((_tmp38D=_cycalloc(sizeof(*
_tmp38D)),((_tmp38D->f1=type,((_tmp38D->f2=loc,_tmp38D)))))),_tmp116);struct Cyc_List_List*
_tmp118=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Position_Segment*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,loc,_tmp115);struct Cyc_List_List*
_tmp119=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _tuple12*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tuple2_exp_cls,loc,((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_zip)(_tmp117,_tmp118));void*_tmp38E[2];void*
tuple_type=Cyc_Tcgenrep_tuple_typ(((_tmp38E[1]=Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str)),((_tmp38E[0]=Cyc_Absyn_uint_typ,Cyc_List_list(
_tag_dyneither(_tmp38E,sizeof(void*),2)))))));struct Cyc_Absyn_Exp*_tmp11A=Cyc_Tcgenrep_cnst_string(*(*
tname).f2,loc);struct Cyc_Absyn_Exp*_tmp11B=Cyc_Tcgenrep_cnst_string(*(*fname).f2,
loc);struct _tuple15 _tmp11D;struct Cyc_List_List*_tmp11E;struct Cyc_Absyn_Exp*
_tmp11F;struct _tuple15*_tmp11C=Cyc_Tcgenrep_array_decls(tuple_type,_tmp119,loc);
_tmp11D=*_tmp11C;_tmp11E=_tmp11D.f1;_tmp11F=_tmp11D.f2;{struct Cyc_Absyn_Exp*
_tmp38F[4];struct Cyc_Absyn_Decl*_tmp120=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_tunionfield_str),
varname,((_tmp38F[3]=_tmp11F,((_tmp38F[2]=_tmp10F,((_tmp38F[1]=_tmp11B,((_tmp38F[
0]=_tmp11A,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp38F,sizeof(struct Cyc_Absyn_Exp*),4)))))))))),sc,loc);struct
Cyc_Absyn_Decl*_tmp390[1];struct Cyc_List_List*_tmp121=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp11E,((
_tmp390[0]=_tmp120,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp390,sizeof(struct Cyc_Absyn_Decl*),1)))));return Cyc_Tcgenrep_update_info(
dict,type,_tmp121,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
varname,loc),loc),_tmp115,0);}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepStruct(
struct Cyc_Core_Opt*sname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct
Cyc_Position_Segment*loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*
nmtypes);static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepStruct(struct Cyc_Core_Opt*
sname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*nmtypes){struct
Cyc_Absyn_Exp*_tmp128=Cyc_Absyn_sizeoftyp_exp(type,loc);struct _tuple21*_tmp391;
struct _tuple21*_tmp129=(_tmp391=_cycalloc(sizeof(*_tmp391)),((_tmp391->f1=te,((
_tmp391->f2=loc,_tmp391)))));struct _tuple20*_tmp392;struct _tuple20*_tmp12A=(
_tmp392=_cycalloc(sizeof(*_tmp392)),((_tmp392->f1=dict,((_tmp392->f2=0,_tmp392)))));
struct Cyc_List_List*_tmp12C;struct Cyc_List_List*_tmp12D;struct _tuple0 _tmp12B=((
struct _tuple0(*)(struct Cyc_List_List*x))Cyc_List_split)(nmtypes);_tmp12C=_tmp12B.f1;
_tmp12D=_tmp12B.f2;{struct _tuple20 _tmp12F;struct Cyc_Dict_Dict _tmp130;struct Cyc_List_List*
_tmp131;struct _tuple20*_tmp12E=((struct _tuple20*(*)(struct _tuple20*(*f)(struct
_tuple21*,void*,struct _tuple20*),struct _tuple21*,struct Cyc_List_List*x,struct
_tuple20*accum))Cyc_List_fold_right_c)(Cyc_Tcgenrep_lookupRepsCls,_tmp129,
_tmp12D,_tmp12A);_tmp12F=*_tmp12E;_tmp130=_tmp12F.f1;_tmp131=_tmp12F.f2;dict=
_tmp130;{struct Cyc_List_List*_tmp132=((struct Cyc_List_List*(*)(int n,int(*f)(int)))
Cyc_List_tabulate)(Cyc_List_length(_tmp12D),(int(*)(int))Cyc_Core_identity);
struct _tuple14*_tmp393;struct Cyc_List_List*_tmp133=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Exp*(*f)(struct _tuple14*,int),struct _tuple14*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_make_offsetof_exp,((_tmp393=_cycalloc(sizeof(*
_tmp393)),((_tmp393->f1=type,((_tmp393->f2=loc,_tmp393)))))),_tmp132);struct Cyc_List_List*
_tmp134=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _dyneither_ptr*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_cnst_string_cls,loc,_tmp12C);struct Cyc_List_List*_tmp135=((struct
Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,struct Cyc_Tcgenrep_RepInfo*),
struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,
loc,_tmp131);struct Cyc_List_List*_tmp136=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _tuple13*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_tuple3_exp_cls,loc,((struct
Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*
z))Cyc_List_zip3)(_tmp133,_tmp134,_tmp135));void*_tmp394[3];void*tuple_type=Cyc_Tcgenrep_tuple_typ(((
_tmp394[2]=Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str)),((
_tmp394[1]=Cyc_Absyn_const_string_typ((void*)2),((_tmp394[0]=Cyc_Absyn_uint_typ,
Cyc_List_list(_tag_dyneither(_tmp394,sizeof(void*),3)))))))));struct _tuple15
_tmp138;struct Cyc_List_List*_tmp139;struct Cyc_Absyn_Exp*_tmp13A;struct _tuple15*
_tmp137=Cyc_Tcgenrep_array_decls(tuple_type,_tmp136,loc);_tmp138=*_tmp137;
_tmp139=_tmp138.f1;_tmp13A=_tmp138.f2;{struct Cyc_Absyn_Exp*name;if(sname == 0)
name=Cyc_Absyn_null_exp(loc);else{const char*_tmp395;struct _dyneither_ptr*_tmp13B=
Cyc_Tcgenrep_new_gen_id(((_tmp395="name",_tag_dyneither(_tmp395,sizeof(char),5))));
struct Cyc_Absyn_Decl*_tmp13C=Cyc_Tcgenrep_gen_decl(_tmp13B,Cyc_Absyn_const_string_typ((
void*)2),(struct Cyc_Absyn_Exp*)Cyc_Tcgenrep_cnst_string(*(*((struct _tuple6*)
sname->v)).f2,loc),loc);{struct Cyc_List_List*_tmp396;_tmp139=((_tmp396=_cycalloc(
sizeof(*_tmp396)),((_tmp396->hd=_tmp13C,((_tmp396->tl=_tmp139,_tmp396))))));}
name=Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_toplevel_name(
_tmp13B),loc),loc);}{struct Cyc_Absyn_Exp*_tmp397[3];struct Cyc_Absyn_Decl*_tmp13F=
Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_struct_str),varname,((_tmp397[2]=_tmp13A,((
_tmp397[1]=_tmp128,((_tmp397[0]=name,((struct Cyc_List_List*(*)(struct
_dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp397,sizeof(struct Cyc_Absyn_Exp*),
3)))))))),sc,loc);struct Cyc_Absyn_Decl*_tmp398[1];struct Cyc_List_List*_tmp140=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(
_tmp139,((_tmp398[0]=_tmp13F,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp398,sizeof(struct Cyc_Absyn_Decl*),1)))));return Cyc_Tcgenrep_update_info(
dict,type,_tmp140,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
varname,loc),loc),_tmp131,0);}}}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepUnion(
struct Cyc_Core_Opt*uname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct
Cyc_Position_Segment*loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*
nmtypes);static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepUnion(struct Cyc_Core_Opt*
uname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*nmtypes){struct
Cyc_Absyn_Exp*_tmp147=Cyc_Absyn_sizeoftyp_exp(type,loc);struct _tuple21*_tmp399;
struct _tuple21*_tmp148=(_tmp399=_cycalloc(sizeof(*_tmp399)),((_tmp399->f1=te,((
_tmp399->f2=loc,_tmp399)))));struct _tuple20*_tmp39A;struct _tuple20*_tmp149=(
_tmp39A=_cycalloc(sizeof(*_tmp39A)),((_tmp39A->f1=dict,((_tmp39A->f2=0,_tmp39A)))));
struct Cyc_List_List*_tmp14B;struct Cyc_List_List*_tmp14C;struct _tuple0 _tmp14A=((
struct _tuple0(*)(struct Cyc_List_List*x))Cyc_List_split)(nmtypes);_tmp14B=_tmp14A.f1;
_tmp14C=_tmp14A.f2;{struct _tuple20 _tmp14E;struct Cyc_Dict_Dict _tmp14F;struct Cyc_List_List*
_tmp150;struct _tuple20*_tmp14D=((struct _tuple20*(*)(struct _tuple20*(*f)(struct
_tuple21*,void*,struct _tuple20*),struct _tuple21*,struct Cyc_List_List*x,struct
_tuple20*accum))Cyc_List_fold_right_c)(Cyc_Tcgenrep_lookupRepsCls,_tmp148,
_tmp14C,_tmp149);_tmp14E=*_tmp14D;_tmp14F=_tmp14E.f1;_tmp150=_tmp14E.f2;dict=
_tmp14F;{struct Cyc_List_List*_tmp151=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _dyneither_ptr*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_string_cls,loc,
_tmp14B);struct Cyc_List_List*_tmp152=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,loc,
_tmp150);struct Cyc_List_List*_tmp153=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _tuple12*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_tuple2_exp_cls,loc,((struct
Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_zip)(
_tmp151,_tmp152));void*_tmp39B[2];void*_tmp154=Cyc_Tcgenrep_tuple_typ(((_tmp39B[
1]=Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str)),((
_tmp39B[0]=Cyc_Absyn_const_string_typ((void*)2),Cyc_List_list(_tag_dyneither(
_tmp39B,sizeof(void*),2)))))));struct _tuple15 _tmp156;struct Cyc_List_List*_tmp157;
struct Cyc_Absyn_Exp*_tmp158;struct _tuple15*_tmp155=Cyc_Tcgenrep_array_decls(
_tmp154,_tmp153,loc);_tmp156=*_tmp155;_tmp157=_tmp156.f1;_tmp158=_tmp156.f2;{
struct Cyc_Absyn_Exp*name;if(uname == 0)name=Cyc_Absyn_null_exp(loc);else{const
char*_tmp39C;struct _dyneither_ptr*_tmp159=Cyc_Tcgenrep_new_gen_id(((_tmp39C="name",
_tag_dyneither(_tmp39C,sizeof(char),5))));struct Cyc_Absyn_Decl*_tmp15A=Cyc_Tcgenrep_gen_decl(
_tmp159,Cyc_Absyn_const_string_typ((void*)2),(struct Cyc_Absyn_Exp*)Cyc_Tcgenrep_cnst_string(*(*((
struct _tuple6*)uname->v)).f2,loc),loc);{struct Cyc_List_List*_tmp39D;_tmp157=((
_tmp39D=_cycalloc(sizeof(*_tmp39D)),((_tmp39D->hd=_tmp15A,((_tmp39D->tl=_tmp157,
_tmp39D))))));}name=Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_toplevel_name(
_tmp159),loc),loc);}{struct Cyc_Absyn_Exp*_tmp39E[3];struct Cyc_Absyn_Decl*_tmp15D=
Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_union_str),varname,((_tmp39E[2]=_tmp158,((
_tmp39E[1]=_tmp147,((_tmp39E[0]=name,((struct Cyc_List_List*(*)(struct
_dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp39E,sizeof(struct Cyc_Absyn_Exp*),
3)))))))),sc,loc);struct Cyc_Absyn_Decl*_tmp39F[1];return Cyc_Tcgenrep_update_info(
dict,type,((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))
Cyc_List_imp_append)(_tmp157,((_tmp39F[0]=_tmp15D,((struct Cyc_List_List*(*)(
struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp39F,sizeof(struct Cyc_Absyn_Decl*),
1))))),(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
varname,loc),loc),_tmp150,0);}}}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepEnum(
struct _tuple6**ename,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*tagnms);static
struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepEnum(struct _tuple6**ename,struct Cyc_Tcenv_Tenv*
te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,struct _tuple6*varname,
void*sc,void*type,struct Cyc_List_List*tagnms){struct Cyc_Absyn_Exp*_tmp163=Cyc_Absyn_sizeoftyp_exp(
type,loc);struct Cyc_List_List*_tmp165;struct Cyc_List_List*_tmp166;struct _tuple0
_tmp164=((struct _tuple0(*)(struct Cyc_List_List*x))Cyc_List_split)(tagnms);
_tmp165=_tmp164.f1;_tmp166=_tmp164.f2;{struct Cyc_List_List*_tmp167=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,struct _tuple6*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_qvar_string_cls,loc,
_tmp166);struct Cyc_List_List*_tmp168=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,int),struct Cyc_Position_Segment*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_int_cls,loc,_tmp165);struct Cyc_List_List*
_tmp169=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _tuple12*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tuple2_exp_cls,loc,((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_zip)(_tmp168,_tmp167));void*_tmp3A0[2];void*
_tmp16A=Cyc_Tcgenrep_tuple_typ(((_tmp3A0[1]=Cyc_Absyn_const_string_typ((void*)2),((
_tmp3A0[0]=Cyc_Absyn_uint_typ,Cyc_List_list(_tag_dyneither(_tmp3A0,sizeof(void*),
2)))))));struct _tuple15 _tmp16C;struct Cyc_List_List*_tmp16D;struct Cyc_Absyn_Exp*
_tmp16E;struct _tuple15*_tmp16B=Cyc_Tcgenrep_array_decls(_tmp16A,_tmp169,loc);
_tmp16C=*_tmp16B;_tmp16D=_tmp16C.f1;_tmp16E=_tmp16C.f2;{struct Cyc_Absyn_Exp*name;
if(ename == 0)name=Cyc_Absyn_null_exp(loc);else{const char*_tmp3A1;struct
_dyneither_ptr*_tmp16F=Cyc_Tcgenrep_new_gen_id(((_tmp3A1="name",_tag_dyneither(
_tmp3A1,sizeof(char),5))));struct Cyc_Absyn_Decl*_tmp170=Cyc_Tcgenrep_gen_decl(
_tmp16F,Cyc_Absyn_const_string_typ((void*)2),(struct Cyc_Absyn_Exp*)Cyc_Tcgenrep_cnst_string(*(*(*
ename)).f2,loc),loc);{struct Cyc_List_List*_tmp3A2;_tmp16D=((_tmp3A2=_cycalloc(
sizeof(*_tmp3A2)),((_tmp3A2->hd=_tmp170,((_tmp3A2->tl=_tmp16D,_tmp3A2))))));}
name=Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_toplevel_name(
_tmp16F),loc),loc);}{struct Cyc_Absyn_Exp*_tmp3A3[3];struct Cyc_Absyn_Decl*_tmp173=
Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_enum_str),varname,((_tmp3A3[2]=_tmp16E,((
_tmp3A3[1]=_tmp163,((_tmp3A3[0]=name,((struct Cyc_List_List*(*)(struct
_dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3A3,sizeof(struct Cyc_Absyn_Exp*),
3)))))))),sc,loc);struct Cyc_Absyn_Decl*_tmp3A4[1];return Cyc_Tcgenrep_update_info(
dict,type,((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))
Cyc_List_imp_append)(_tmp16D,((_tmp3A4[0]=_tmp173,((struct Cyc_List_List*(*)(
struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3A4,sizeof(struct Cyc_Absyn_Decl*),
1))))),(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
varname,loc),loc),0,0);}}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTunion(
struct _tuple6*tname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*tonames,struct
Cyc_List_List*nmtypes);static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepTunion(
struct _tuple6*tname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*tonames,struct
Cyc_List_List*nmtypes){struct Cyc_List_List*_tmp177=((struct Cyc_List_List*(*)(int
n,int(*f)(int)))Cyc_List_tabulate)(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(
tonames),(int(*)(int))Cyc_Core_identity);struct Cyc_List_List*_tmp178=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,int),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_int_cls,loc,_tmp177);
struct Cyc_List_List*_tmp179=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(
struct Cyc_Position_Segment*,struct _tuple6*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_qvar_string_cls,loc,
tonames);struct Cyc_List_List*_tmp17A=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_zip)(_tmp178,_tmp179);struct Cyc_List_List*
_tmp17B=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _tuple12*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tuple2_exp_cls,loc,_tmp17A);void*_tmp3A5[2];void*_tmp17C=Cyc_Tcgenrep_tuple_typ(((
_tmp3A5[1]=Cyc_Absyn_const_string_typ((void*)2),((_tmp3A5[0]=Cyc_Absyn_uint_typ,
Cyc_List_list(_tag_dyneither(_tmp3A5,sizeof(void*),2)))))));struct _tuple15
_tmp17E;struct Cyc_List_List*_tmp17F;struct Cyc_Absyn_Exp*_tmp180;struct _tuple15*
_tmp17D=Cyc_Tcgenrep_array_decls(_tmp17C,_tmp17B,loc);_tmp17E=*_tmp17D;_tmp17F=
_tmp17E.f1;_tmp180=_tmp17E.f2;{struct _tuple21*_tmp3A6;struct _tuple21*_tmp181=(
_tmp3A6=_cycalloc(sizeof(*_tmp3A6)),((_tmp3A6->f1=te,((_tmp3A6->f2=loc,_tmp3A6)))));
struct _tuple20*_tmp3A7;struct _tuple20*_tmp182=(_tmp3A7=_cycalloc(sizeof(*_tmp3A7)),((
_tmp3A7->f1=dict,((_tmp3A7->f2=0,_tmp3A7)))));struct Cyc_List_List*_tmp184;struct
Cyc_List_List*_tmp185;struct _tuple0 _tmp183=((struct _tuple0(*)(struct Cyc_List_List*
x))Cyc_List_split)(nmtypes);_tmp184=_tmp183.f1;_tmp185=_tmp183.f2;{struct
_tuple20 _tmp187;struct Cyc_Dict_Dict _tmp188;struct Cyc_List_List*_tmp189;struct
_tuple20*_tmp186=((struct _tuple20*(*)(struct _tuple20*(*f)(struct _tuple21*,void*,
struct _tuple20*),struct _tuple21*,struct Cyc_List_List*x,struct _tuple20*accum))Cyc_List_fold_right_c)(
Cyc_Tcgenrep_lookupRepsCls,_tmp181,_tmp185,_tmp182);_tmp187=*_tmp186;_tmp188=
_tmp187.f1;_tmp189=_tmp187.f2;dict=_tmp188;{struct Cyc_List_List*_tmp18A=((struct
Cyc_List_List*(*)(int n,int(*f)(int)))Cyc_List_tabulate)(((int(*)(struct Cyc_List_List*
x))Cyc_List_length)(_tmp189),(int(*)(int))Cyc_Core_identity);struct Cyc_List_List*
_tmp18B=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
int),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_int_cls,
loc,_tmp18A);struct Cyc_List_List*_tmp18C=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _tuple6*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_qvar_string_cls,loc,
_tmp184);struct Cyc_List_List*_tmp18D=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,loc,
_tmp189);struct Cyc_List_List*_tmp18E=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y,struct Cyc_List_List*z))Cyc_List_zip3)(_tmp18B,_tmp18C,
_tmp18D);struct Cyc_List_List*_tmp18F=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _tuple13*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_tuple3_exp_cls,loc,_tmp18E);
void*_tmp3A8[3];void*tuple_type2=Cyc_Tcgenrep_tuple_typ(((_tmp3A8[2]=Cyc_Tcgenrep_tunion_typ(
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str)),((_tmp3A8[1]=Cyc_Absyn_const_string_typ((
void*)2),((_tmp3A8[0]=Cyc_Absyn_uint_typ,Cyc_List_list(_tag_dyneither(_tmp3A8,
sizeof(void*),3)))))))));struct _tuple15 _tmp191;struct Cyc_List_List*_tmp192;
struct Cyc_Absyn_Exp*_tmp193;struct _tuple15*_tmp190=Cyc_Tcgenrep_array_decls(
tuple_type2,_tmp18F,loc);_tmp191=*_tmp190;_tmp192=_tmp191.f1;_tmp193=_tmp191.f2;{
struct Cyc_Absyn_Exp*_tmp194=Cyc_Tcgenrep_cnst_string(*(*tname).f2,loc);struct Cyc_Absyn_Exp*
_tmp3A9[3];struct Cyc_Absyn_Decl*_tmp195=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_tunion_str),
varname,((_tmp3A9[2]=_tmp193,((_tmp3A9[1]=_tmp180,((_tmp3A9[0]=_tmp194,((struct
Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3A9,
sizeof(struct Cyc_Absyn_Exp*),3)))))))),sc,loc);{struct Cyc_Absyn_Decl*_tmp3AA[1];
return Cyc_Tcgenrep_update_info(dict,type,((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp17F,((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp192,((
_tmp3AA[0]=_tmp195,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3AA,sizeof(struct Cyc_Absyn_Decl*),1)))))),(struct Cyc_Absyn_Exp*)
Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(varname,loc),loc),_tmp189,0);}
return dict;}}}}}static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepXTunion(struct
_tuple6*xname,struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*
loc,struct _tuple6*varname,void*sc,void*type,struct Cyc_List_List*nametypes);
static struct Cyc_Dict_Dict Cyc_Tcgenrep_buildRepXTunion(struct _tuple6*xname,struct
Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,struct
_tuple6*varname,void*sc,void*type,struct Cyc_List_List*nametypes){struct Cyc_List_List*
_tmp19D;struct Cyc_List_List*_tmp19E;struct _tuple0 _tmp19C=((struct _tuple0(*)(
struct Cyc_List_List*x))Cyc_List_split)(nametypes);_tmp19D=_tmp19C.f1;_tmp19E=
_tmp19C.f2;{struct _tuple21*_tmp3AB;struct _tuple21*_tmp19F=(_tmp3AB=_cycalloc(
sizeof(*_tmp3AB)),((_tmp3AB->f1=te,((_tmp3AB->f2=loc,_tmp3AB)))));struct _tuple20*
_tmp3AC;struct _tuple20*_tmp1A0=(_tmp3AC=_cycalloc(sizeof(*_tmp3AC)),((_tmp3AC->f1=
dict,((_tmp3AC->f2=0,_tmp3AC)))));struct _tuple20 _tmp1A2;struct Cyc_Dict_Dict
_tmp1A3;struct Cyc_List_List*_tmp1A4;struct _tuple20*_tmp1A1=((struct _tuple20*(*)(
struct _tuple20*(*f)(struct _tuple21*,void*,struct _tuple20*),struct _tuple21*,
struct Cyc_List_List*x,struct _tuple20*accum))Cyc_List_fold_right_c)(Cyc_Tcgenrep_lookupRepsCls,
_tmp19F,_tmp19E,_tmp1A0);_tmp1A2=*_tmp1A1;_tmp1A3=_tmp1A2.f1;_tmp1A4=_tmp1A2.f2;
dict=_tmp1A3;{struct Cyc_List_List*_tmp1A5=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct _dyneither_ptr*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_cnst_string_cls,loc,
_tmp19D);struct Cyc_List_List*_tmp1A6=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*
f)(struct Cyc_Position_Segment*,struct Cyc_Tcgenrep_RepInfo*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcgenrep_get_and_cast_ri_exp,loc,
_tmp1A4);struct Cyc_List_List*_tmp1A7=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_zip)(_tmp1A5,_tmp1A6);struct Cyc_List_List*
_tmp1A8=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Position_Segment*,
struct _tuple12*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Tcgenrep_tuple2_exp_cls,loc,_tmp1A7);struct Cyc_Absyn_Tqual _tmp3AD;void*
name_type=Cyc_Absyn_dyneither_typ(Cyc_Absyn_char_typ,(void*)2,((_tmp3AD.print_const=
1,((_tmp3AD.q_volatile=0,((_tmp3AD.q_restrict=0,((_tmp3AD.real_const=1,((_tmp3AD.loc=
0,_tmp3AD)))))))))),Cyc_Absyn_true_conref);void*_tmp3AE[2];void*tuple_type=Cyc_Tcgenrep_tuple_typ(((
_tmp3AE[1]=Cyc_Tcgenrep_tunion_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str)),((
_tmp3AE[0]=name_type,Cyc_List_list(_tag_dyneither(_tmp3AE,sizeof(void*),2)))))));
struct _tuple15 _tmp1AA;struct Cyc_List_List*_tmp1AB;struct Cyc_Absyn_Exp*_tmp1AC;
struct _tuple15*_tmp1A9=Cyc_Tcgenrep_array_decls(tuple_type,_tmp1A8,loc);_tmp1AA=*
_tmp1A9;_tmp1AB=_tmp1AA.f1;_tmp1AC=_tmp1AA.f2;{struct Cyc_Absyn_Exp*_tmp1AD=Cyc_Tcgenrep_cnst_string(*(*
xname).f2,loc);struct Cyc_Absyn_Exp*_tmp3AF[2];struct Cyc_Absyn_Decl*_tmp1AE=Cyc_Tcgenrep_tunion_constructor_decl(
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_xtunion_str),varname,((_tmp3AF[1]=_tmp1AC,((_tmp3AF[0]=_tmp1AD,((
struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(
_tmp3AF,sizeof(struct Cyc_Absyn_Exp*),2)))))),sc,loc);{struct Cyc_Absyn_Decl*
_tmp3B0[1];return Cyc_Tcgenrep_update_info(dict,type,((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp1AB,((
_tmp3B0[0]=_tmp1AE,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3B0,sizeof(struct Cyc_Absyn_Decl*),1))))),(struct Cyc_Absyn_Exp*)
Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(varname,loc),loc),_tmp1A4,0);}
return dict;}}}}static struct _tuple19*Cyc_Tcgenrep_lookupRep(struct Cyc_Tcenv_Tenv*
te,struct Cyc_Dict_Dict dict,struct Cyc_Position_Segment*loc,void*type);static
struct _tuple19*Cyc_Tcgenrep_lookupRep(struct Cyc_Tcenv_Tenv*te,struct Cyc_Dict_Dict
dict,struct Cyc_Position_Segment*loc,void*type){struct Cyc_Tcgenrep_RepInfo**info=((
struct Cyc_Tcgenrep_RepInfo**(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt)(
dict,type);if(info != 0){if((*info)->is_extern)dict=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict,void*))Cyc_Dict_delete)(dict,type);else{struct _tuple19*
_tmp3B1;struct _tuple19*_tmp1B5=(_tmp3B1=_cycalloc(sizeof(*_tmp3B1)),((_tmp3B1->f1=
dict,((_tmp3B1->f2=*info,_tmp3B1)))));return _tmp1B5;}}{void*_tmp1B7=Cyc_Tcutil_compress(
Cyc_Tcgenrep_monomorphize_type(type));void*_tmp1B8;void*_tmp1B9;int _tmp1BA;
struct Cyc_Absyn_PtrInfo _tmp1BB;struct Cyc_Absyn_ArrayInfo _tmp1BC;void*_tmp1BD;
struct Cyc_Absyn_Tqual _tmp1BE;struct Cyc_Absyn_Exp*_tmp1BF;struct Cyc_Absyn_Conref*
_tmp1C0;struct Cyc_List_List*_tmp1C1;struct _tuple6*_tmp1C2;struct Cyc_List_List*
_tmp1C3;struct Cyc_Absyn_Typedefdecl*_tmp1C4;void**_tmp1C5;struct Cyc_Absyn_Tvar*
_tmp1C6;struct Cyc_Absyn_FnInfo _tmp1C7;struct Cyc_Absyn_TunionInfo _tmp1C8;union Cyc_Absyn_TunionInfoU_union
_tmp1C9;struct Cyc_Absyn_Tuniondecl**_tmp1CA;struct Cyc_Absyn_Tuniondecl*_tmp1CB;
struct Cyc_List_List*_tmp1CC;struct Cyc_Core_Opt*_tmp1CD;struct Cyc_Absyn_TunionFieldInfo
_tmp1CE;struct Cyc_Absyn_AggrInfo _tmp1CF;union Cyc_Absyn_AggrInfoU_union _tmp1D0;
struct Cyc_List_List*_tmp1D1;void*_tmp1D2;struct Cyc_List_List*_tmp1D3;void*
_tmp1D4;struct Cyc_List_List*_tmp1D5;struct Cyc_Absyn_Enumdecl*_tmp1D6;struct Cyc_List_List*
_tmp1D7;_LL18: if(_tmp1B7 <= (void*)4)goto _LL1A;if(*((int*)_tmp1B7)!= 5)goto _LL1A;
_tmp1B8=(void*)((struct Cyc_Absyn_IntType_struct*)_tmp1B7)->f1;_tmp1B9=(void*)((
struct Cyc_Absyn_IntType_struct*)_tmp1B7)->f2;_LL19: {const char*_tmp3B2;struct
_tuple6*_tmp1D8=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((_tmp3B2="rep",
_tag_dyneither(_tmp3B2,sizeof(char),4)))));struct Cyc_Absyn_Exp*_tmp3B3[2];struct
Cyc_Absyn_Decl*_tmp1D9=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_int_str),
_tmp1D8,((_tmp3B3[1]=Cyc_Tcgenrep_cnst_int(Cyc_Tcgenrep_size_of2int(_tmp1B9),loc),((
_tmp3B3[0]=Cyc_Tcgenrep_cnst_int(_tmp1B8 == (void*)0?1: 0,loc),((struct Cyc_List_List*(*)(
struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3B3,sizeof(struct Cyc_Absyn_Exp*),
2)))))),(void*)0,loc);{struct Cyc_Absyn_Decl*_tmp3B4[1];dict=Cyc_Tcgenrep_update_info(
dict,type,((_tmp3B4[0]=_tmp1D9,((struct Cyc_List_List*(*)(struct _dyneither_ptr))
Cyc_List_list)(_tag_dyneither(_tmp3B4,sizeof(struct Cyc_Absyn_Decl*),1)))),(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp1D8,loc),
loc),0,0);}goto _LL17;}_LL1A: if((int)_tmp1B7 != 1)goto _LL1C;_LL1B: dict=Cyc_Tcgenrep_update_info(
dict,type,0,(struct Cyc_Absyn_Exp*)Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_float_str),loc),0,0);goto _LL17;_LL1C: if(_tmp1B7 <= (void*)4)goto
_LL26;if(*((int*)_tmp1B7)!= 6)goto _LL1E;_tmp1BA=((struct Cyc_Absyn_DoubleType_struct*)
_tmp1B7)->f1;_LL1D: dict=Cyc_Tcgenrep_update_info(dict,type,0,(struct Cyc_Absyn_Exp*)
Cyc_Absyn_unknownid_exp(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_double_str),loc),
0,0);goto _LL17;_LL1E: if(*((int*)_tmp1B7)!= 4)goto _LL20;_tmp1BB=((struct Cyc_Absyn_PointerType_struct*)
_tmp1B7)->f1;_LL1F:{void*_tmp1DD=Cyc_Absyn_conref_val((_tmp1BB.ptr_atts).bounds);
struct Cyc_Absyn_Exp*_tmp1DE;_LL51: if(_tmp1DD <= (void*)1)goto _LL53;if(*((int*)
_tmp1DD)!= 0)goto _LL53;_tmp1DE=((struct Cyc_Absyn_Upper_b_struct*)_tmp1DD)->f1;
_LL52: {const char*_tmp3B5;struct _tuple6*_tmp1DF=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((
_tmp3B5="rep",_tag_dyneither(_tmp3B5,sizeof(char),4)))));struct Cyc_Core_Opt*
_tmp3B6;struct Cyc_Core_Opt*_tmp1E0=(_tmp3B6=_cycalloc(sizeof(*_tmp3B6)),((
_tmp3B6->v=Cyc_Tcgenrep_gen_vardecl(_tmp1DF,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_thinptr_str)),
0,(void*)3,loc),_tmp3B6)));dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp1DF,loc),
loc),_tmp1E0,0);{struct _tuple19 _tmp1E2;struct Cyc_Dict_Dict _tmp1E3;struct Cyc_Tcgenrep_RepInfo*
_tmp1E4;struct _tuple19*_tmp1E1=Cyc_Tcgenrep_lookupRep(te,dict,loc,(void*)_tmp1BB.elt_typ);
_tmp1E2=*_tmp1E1;_tmp1E3=_tmp1E2.f1;_tmp1E4=_tmp1E2.f2;dict=_tmp1E3;{struct Cyc_Absyn_Exp*
_tmp3B7[2];struct Cyc_Absyn_Decl*_tmp1E5=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_thinptr_str),
_tmp1DF,((_tmp3B7[1]=(struct Cyc_Absyn_Exp*)_check_null(_tmp1E4->exp),((_tmp3B7[0]=
_tmp1DE,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3B7,sizeof(struct Cyc_Absyn_Exp*),2)))))),(void*)0,loc);{
struct Cyc_Tcgenrep_RepInfo*_tmp3B9[1];struct Cyc_Absyn_Decl*_tmp3B8[1];dict=Cyc_Tcgenrep_update_info(
dict,type,((_tmp3B8[0]=_tmp1E5,((struct Cyc_List_List*(*)(struct _dyneither_ptr))
Cyc_List_list)(_tag_dyneither(_tmp3B8,sizeof(struct Cyc_Absyn_Decl*),1)))),(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp1DF,loc),
loc),((_tmp3B9[0]=_tmp1E4,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3B9,sizeof(struct Cyc_Tcgenrep_RepInfo*),1)))),0);}goto _LL50;}}}
_LL53: if((int)_tmp1DD != 0)goto _LL50;_LL54: {const char*_tmp3BA;struct _tuple6*
_tmp1EB=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((_tmp3BA="rep",
_tag_dyneither(_tmp3BA,sizeof(char),4)))));struct Cyc_Core_Opt*_tmp3BB;struct Cyc_Core_Opt*
_tmp1EC=(_tmp3BB=_cycalloc(sizeof(*_tmp3BB)),((_tmp3BB->v=Cyc_Tcgenrep_gen_vardecl(
_tmp1EB,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_fatptr_str)),0,(void*)3,loc),_tmp3BB)));
dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp1EB,loc),loc),_tmp1EC,0);{struct _tuple19 _tmp1EE;
struct Cyc_Dict_Dict _tmp1EF;struct Cyc_Tcgenrep_RepInfo*_tmp1F0;struct _tuple19*
_tmp1ED=Cyc_Tcgenrep_lookupRep(te,dict,loc,(void*)_tmp1BB.elt_typ);_tmp1EE=*
_tmp1ED;_tmp1EF=_tmp1EE.f1;_tmp1F0=_tmp1EE.f2;dict=_tmp1EF;{struct Cyc_Absyn_Exp*
_tmp3BC[1];struct Cyc_Absyn_Decl*_tmp1F1=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_fatptr_str),
_tmp1EB,((_tmp3BC[0]=(struct Cyc_Absyn_Exp*)_check_null(_tmp1F0->exp),((struct Cyc_List_List*(*)(
struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3BC,sizeof(struct Cyc_Absyn_Exp*),
1)))),(void*)0,loc);{struct Cyc_Tcgenrep_RepInfo*_tmp3BE[1];struct Cyc_Absyn_Decl*
_tmp3BD[1];dict=Cyc_Tcgenrep_update_info(dict,type,((_tmp3BD[0]=_tmp1F1,((struct
Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3BD,
sizeof(struct Cyc_Absyn_Decl*),1)))),(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp1EB,loc),loc),((_tmp3BE[0]=_tmp1F0,((struct Cyc_List_List*(*)(
struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp3BE,sizeof(struct Cyc_Tcgenrep_RepInfo*),
1)))),0);}goto _LL50;}}}_LL50:;}goto _LL17;_LL20: if(*((int*)_tmp1B7)!= 7)goto _LL22;
_tmp1BC=((struct Cyc_Absyn_ArrayType_struct*)_tmp1B7)->f1;_tmp1BD=(void*)_tmp1BC.elt_type;
_tmp1BE=_tmp1BC.tq;_tmp1BF=_tmp1BC.num_elts;_tmp1C0=_tmp1BC.zero_term;_LL21: if(
_tmp1BF == 0){const char*_tmp3C1;void*_tmp3C0;(_tmp3C0=0,((int(*)(struct
_dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp3C1="tcgenrep:At this point, array bounds must be constants",
_tag_dyneither(_tmp3C1,sizeof(char),55))),_tag_dyneither(_tmp3C0,sizeof(void*),0)));}{
struct _tuple19 _tmp1FA;struct Cyc_Dict_Dict _tmp1FB;struct Cyc_Tcgenrep_RepInfo*
_tmp1FC;struct _tuple19*_tmp1F9=Cyc_Tcgenrep_lookupRep(te,dict,loc,_tmp1BD);
_tmp1FA=*_tmp1F9;_tmp1FB=_tmp1FA.f1;_tmp1FC=_tmp1FA.f2;dict=_tmp1FB;{const char*
_tmp3C2;struct _tuple6*_tmp1FD=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((
_tmp3C2="rep",_tag_dyneither(_tmp3C2,sizeof(char),4)))));struct Cyc_Absyn_Exp*
_tmp3C3[2];struct Cyc_Absyn_Decl*_tmp1FE=Cyc_Tcgenrep_tunion_constructor_decl(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_thinptr_str),
_tmp1FD,((_tmp3C3[1]=(struct Cyc_Absyn_Exp*)_check_null(_tmp1FC->exp),((_tmp3C3[0]=(
struct Cyc_Absyn_Exp*)_tmp1BF,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3C3,sizeof(struct Cyc_Absyn_Exp*),2)))))),(void*)0,loc);{
struct Cyc_Tcgenrep_RepInfo*_tmp3C5[1];struct Cyc_Absyn_Decl*_tmp3C4[1];dict=Cyc_Tcgenrep_update_info(
dict,type,((_tmp3C4[0]=_tmp1FE,((struct Cyc_List_List*(*)(struct _dyneither_ptr))
Cyc_List_list)(_tag_dyneither(_tmp3C4,sizeof(struct Cyc_Absyn_Decl*),1)))),(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp1FD,loc),
loc),((_tmp3C5[0]=_tmp1FC,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(
_tag_dyneither(_tmp3C5,sizeof(struct Cyc_Tcgenrep_RepInfo*),1)))),0);}goto _LL17;}}
_LL22: if(*((int*)_tmp1B7)!= 9)goto _LL24;_tmp1C1=((struct Cyc_Absyn_TupleType_struct*)
_tmp1B7)->f1;_LL23: {const char*_tmp3C6;struct _tuple6*_tmp203=Cyc_Tcgenrep_toplevel_name(
Cyc_Tcgenrep_new_gen_id(((_tmp3C6="rep",_tag_dyneither(_tmp3C6,sizeof(char),4)))));
struct Cyc_Core_Opt*_tmp3C7;struct Cyc_Core_Opt*_tmp204=(_tmp3C7=_cycalloc(sizeof(*
_tmp3C7)),((_tmp3C7->v=Cyc_Tcgenrep_gen_vardecl(_tmp203,Cyc_Tcgenrep_tunionfield_typ(
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_tuple_str)),0,(void*)3,loc),_tmp3C7)));dict=Cyc_Tcgenrep_make_fwd_decl_info(
dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
_tmp203,loc),loc),_tmp204,0);{struct Cyc_List_List*_tmp205=((struct Cyc_List_List*(*)(
void*(*f)(struct _tuple10*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_get_second,
_tmp1C1);dict=Cyc_Tcgenrep_buildRepTuple(te,dict,loc,_tmp203,(void*)0,type,
_tmp205);goto _LL17;}}_LL24: if(*((int*)_tmp1B7)!= 16)goto _LL26;_tmp1C2=((struct
Cyc_Absyn_TypedefType_struct*)_tmp1B7)->f1;_tmp1C3=((struct Cyc_Absyn_TypedefType_struct*)
_tmp1B7)->f2;_tmp1C4=((struct Cyc_Absyn_TypedefType_struct*)_tmp1B7)->f3;_tmp1C5=((
struct Cyc_Absyn_TypedefType_struct*)_tmp1B7)->f4;_LL25: if(_tmp1C5 == 0){const char*
_tmp3CA;void*_tmp3C9;(_tmp3C9=0,((int(*)(struct _dyneither_ptr fmt,struct
_dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp3CA="gen(): can't gen for abstract typedef",
_tag_dyneither(_tmp3CA,sizeof(char),38))),_tag_dyneither(_tmp3C9,sizeof(void*),0)));}{
struct _tuple19 _tmp20B;struct Cyc_Dict_Dict _tmp20C;struct Cyc_Tcgenrep_RepInfo*
_tmp20D;struct _tuple19*_tmp20A=Cyc_Tcgenrep_lookupRep(te,dict,loc,*_tmp1C5);
_tmp20B=*_tmp20A;_tmp20C=_tmp20B.f1;_tmp20D=_tmp20B.f2;dict=((struct Cyc_Dict_Dict(*)(
struct Cyc_Dict_Dict d,void*k,struct Cyc_Tcgenrep_RepInfo*v))Cyc_Dict_insert)(
_tmp20C,type,_tmp20D);goto _LL17;}_LL26: if((int)_tmp1B7 != 0)goto _LL28;_LL27:{
const char*_tmp3CD;void*_tmp3CC;(_tmp3CC=0,Cyc_Tcutil_terr(loc,((_tmp3CD="found void in gen() expression",
_tag_dyneither(_tmp3CD,sizeof(char),31))),_tag_dyneither(_tmp3CC,sizeof(void*),0)));}
goto _LL17;_LL28: if(_tmp1B7 <= (void*)4)goto _LL32;if(*((int*)_tmp1B7)!= 0)goto
_LL2A;_LL29:{const char*_tmp3D0;void*_tmp3CF;(_tmp3CF=0,Cyc_Tcutil_terr(loc,((
_tmp3D0="found evar in gen() expression",_tag_dyneither(_tmp3D0,sizeof(char),31))),
_tag_dyneither(_tmp3CF,sizeof(void*),0)));}goto _LL17;_LL2A: if(*((int*)_tmp1B7)!= 
1)goto _LL2C;_tmp1C6=((struct Cyc_Absyn_VarType_struct*)_tmp1B7)->f1;_LL2B:{const
char*_tmp3D3;void*_tmp3D2;(_tmp3D2=0,Cyc_Tcutil_terr(loc,((_tmp3D3="found tyvar in gen() expression",
_tag_dyneither(_tmp3D3,sizeof(char),32))),_tag_dyneither(_tmp3D2,sizeof(void*),0)));}
goto _LL17;_LL2C: if(*((int*)_tmp1B7)!= 8)goto _LL2E;_tmp1C7=((struct Cyc_Absyn_FnType_struct*)
_tmp1B7)->f1;_LL2D:{const char*_tmp3D6;void*_tmp3D5;(_tmp3D5=0,Cyc_Tcutil_terr(
loc,((_tmp3D6="found function type in gen() expression",_tag_dyneither(_tmp3D6,
sizeof(char),40))),_tag_dyneither(_tmp3D5,sizeof(void*),0)));}goto _LL17;_LL2E:
if(*((int*)_tmp1B7)!= 14)goto _LL30;_LL2F: goto _LL31;_LL30: if(*((int*)_tmp1B7)!= 
15)goto _LL32;_LL31: goto _LL33;_LL32: if((int)_tmp1B7 != 3)goto _LL34;_LL33: goto _LL35;
_LL34: if((int)_tmp1B7 != 2)goto _LL36;_LL35: goto _LL37;_LL36: if(_tmp1B7 <= (void*)4)
goto _LL38;if(*((int*)_tmp1B7)!= 19)goto _LL38;_LL37: goto _LL39;_LL38: if(_tmp1B7 <= (
void*)4)goto _LL3A;if(*((int*)_tmp1B7)!= 20)goto _LL3A;_LL39: goto _LL3B;_LL3A: if(
_tmp1B7 <= (void*)4)goto _LL3C;if(*((int*)_tmp1B7)!= 21)goto _LL3C;_LL3B:{const char*
_tmp3D9;void*_tmp3D8;(_tmp3D8=0,Cyc_Tcutil_terr(loc,((_tmp3D9="gen(): unhandled region, handle or effect type",
_tag_dyneither(_tmp3D9,sizeof(char),47))),_tag_dyneither(_tmp3D8,sizeof(void*),0)));}
goto _LL17;_LL3C: if(_tmp1B7 <= (void*)4)goto _LL3E;if(*((int*)_tmp1B7)!= 2)goto
_LL3E;_tmp1C8=((struct Cyc_Absyn_TunionType_struct*)_tmp1B7)->f1;_tmp1C9=_tmp1C8.tunion_info;
if((((((struct Cyc_Absyn_TunionType_struct*)_tmp1B7)->f1).tunion_info).KnownTunion).tag
!= 1)goto _LL3E;_tmp1CA=(_tmp1C9.KnownTunion).f1;_tmp1CB=*_tmp1CA;_tmp1CC=_tmp1C8.targs;
_tmp1CD=_tmp1C8.rgn;_LL3D: if(_tmp1CB->tvs != 0){const char*_tmp3DC;void*_tmp3DB;(
_tmp3DB=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp3DC="gen(): tunion type variables not handled yet",_tag_dyneither(_tmp3DC,
sizeof(char),45))),_tag_dyneither(_tmp3DB,sizeof(void*),0)));}if(_tmp1CB->fields
== 0){struct _dyneither_ptr*_tmp3DD;struct _tuple6*_tmp21A=Cyc_Tcgenrep_toplevel_name(((
_tmp3DD=_cycalloc(sizeof(*_tmp3DD)),((_tmp3DD[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp3DD)))));struct Cyc_Core_Opt*_tmp3DE;struct Cyc_Core_Opt*_tmp21B=(
_tmp3DE=_cycalloc(sizeof(*_tmp3DE)),((_tmp3DE->v=Cyc_Tcgenrep_gen_vardecl(
_tmp21A,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_tunion_str)),0,(void*)3,loc),_tmp3DE)));
dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp21A,loc),loc),_tmp21B,1);goto _LL17;}if(!_tmp1CB->is_xtunion){
struct _dyneither_ptr*_tmp3DF;struct _tuple6*_tmp21E=Cyc_Tcgenrep_toplevel_name(((
_tmp3DF=_cycalloc(sizeof(*_tmp3DF)),((_tmp3DF[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp3DF)))));struct Cyc_Core_Opt*_tmp3E0;struct Cyc_Core_Opt*_tmp21F=(
_tmp3E0=_cycalloc(sizeof(*_tmp3E0)),((_tmp3E0->v=Cyc_Tcgenrep_gen_vardecl(
_tmp21E,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_tunion_str)),0,(void*)3,loc),_tmp3E0)));
dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp21E,loc),loc),_tmp21F,0);{struct Cyc_List_List*
_tmp220=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp1CB->fields))->v;
struct Cyc_List_List*_tmp221=((struct Cyc_List_List*(*)(int(*f)(struct Cyc_Absyn_Tunionfield*),
struct Cyc_List_List*x))Cyc_List_filter)(Cyc_Tcgenrep_filter_empty_tunionfield,
_tmp220);struct Cyc_List_List*_tmp222=((struct Cyc_List_List*(*)(int(*f)(struct Cyc_Absyn_Tunionfield*),
struct Cyc_List_List*x))Cyc_List_filter)(Cyc_Tcgenrep_filter_nonempty_tunionfield,
_tmp220);struct Cyc_List_List*_tmp223=((struct Cyc_List_List*(*)(struct _tuple18*(*
f)(struct Cyc_Absyn_Tunionfield*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_check_tunionfield_and_get_nmtype,
_tmp221);struct Cyc_List_List*_tmp224=((struct Cyc_List_List*(*)(struct _tuple6*(*f)(
struct Cyc_Absyn_Tunionfield*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_check_tunionfield_and_get_name,
_tmp222);dict=Cyc_Tcgenrep_buildRepTunion(_tmp1CB->name,te,dict,loc,_tmp21E,(
void*)2,type,_tmp224,_tmp223);}}else{struct _dyneither_ptr*_tmp3E1;struct _tuple6*
_tmp227=Cyc_Tcgenrep_toplevel_name(((_tmp3E1=_cycalloc(sizeof(*_tmp3E1)),((
_tmp3E1[0]=Cyc_Tcgenrep_make_type_cstring(type),_tmp3E1)))));struct Cyc_Core_Opt*
_tmp3E2;struct Cyc_Core_Opt*_tmp228=(_tmp3E2=_cycalloc(sizeof(*_tmp3E2)),((
_tmp3E2->v=Cyc_Tcgenrep_gen_vardecl(_tmp227,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_xtunion_str)),
0,(void*)3,loc),_tmp3E2)));dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp227,loc),
loc),_tmp228,0);{struct Cyc_List_List*_tmp229=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)
_check_null(_tmp1CB->fields))->v;struct Cyc_List_List*_tmp22A=((struct Cyc_List_List*(*)(
int(*f)(struct Cyc_Absyn_Tunionfield*),struct Cyc_List_List*x))Cyc_List_filter)(
Cyc_Tcgenrep_filter_empty_tunionfield,_tmp229);struct Cyc_List_List*_tmp22B=((
struct Cyc_List_List*(*)(struct _tuple16*(*f)(struct Cyc_Absyn_Tunionfield*),struct
Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_check_xtunionfield_and_get_name_type,
_tmp22A);dict=Cyc_Tcgenrep_buildRepXTunion(_tmp1CB->name,te,dict,loc,_tmp227,(
void*)2,type,_tmp22B);}}goto _LL17;_LL3E: if(_tmp1B7 <= (void*)4)goto _LL40;if(*((
int*)_tmp1B7)!= 2)goto _LL40;_LL3F: {const char*_tmp3E5;void*_tmp3E4;(_tmp3E4=0,((
int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp3E5="gen(): tunion must be resolved by now",_tag_dyneither(_tmp3E5,sizeof(
char),38))),_tag_dyneither(_tmp3E4,sizeof(void*),0)));}_LL40: if(_tmp1B7 <= (void*)
4)goto _LL42;if(*((int*)_tmp1B7)!= 3)goto _LL42;_tmp1CE=((struct Cyc_Absyn_TunionFieldType_struct*)
_tmp1B7)->f1;_LL41: if(_tmp1CE.targs != 0){const char*_tmp3E8;void*_tmp3E7;(_tmp3E7=
0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp3E8="gen(): tunionfield type parameters not handled yet",_tag_dyneither(
_tmp3E8,sizeof(char),51))),_tag_dyneither(_tmp3E7,sizeof(void*),0)));}{union Cyc_Absyn_TunionFieldInfoU_union
_tmp232=_tmp1CE.field_info;struct Cyc_Absyn_Tuniondecl*_tmp233;struct Cyc_Absyn_Tunionfield*
_tmp234;_LL56: if((_tmp232.KnownTunionfield).tag != 1)goto _LL58;_tmp233=(_tmp232.KnownTunionfield).f1;
_tmp234=(_tmp232.KnownTunionfield).f2;_LL57: {struct Cyc_List_List*_tmp235=((
struct Cyc_List_List*(*)(void*(*f)(struct _tuple10*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_get_second,_tmp234->typs);struct Cyc_List_List*_tmp3E9;struct Cyc_List_List*
_tmp236=(_tmp3E9=_cycalloc(sizeof(*_tmp3E9)),((_tmp3E9->hd=(void*)Cyc_Absyn_uint_typ,((
_tmp3E9->tl=_tmp235,_tmp3E9)))));const char*_tmp3EA;struct _tuple6*_tmp237=Cyc_Tcgenrep_toplevel_name(
Cyc_Tcgenrep_new_gen_id(((_tmp3EA="rep",_tag_dyneither(_tmp3EA,sizeof(char),4)))));
dict=Cyc_Tcgenrep_buildRepTunionfield(_tmp233->name,_tmp234->name,te,dict,loc,
_tmp237,(void*)0,type,_tmp236);goto _LL55;}_LL58:;_LL59: {const char*_tmp3ED;void*
_tmp3EC;(_tmp3EC=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp3ED="gen(): tunionfield must be known at this point",_tag_dyneither(_tmp3ED,
sizeof(char),47))),_tag_dyneither(_tmp3EC,sizeof(void*),0)));}_LL55:;}goto _LL17;
_LL42: if(_tmp1B7 <= (void*)4)goto _LL44;if(*((int*)_tmp1B7)!= 10)goto _LL44;_tmp1CF=((
struct Cyc_Absyn_AggrType_struct*)_tmp1B7)->f1;_tmp1D0=_tmp1CF.aggr_info;_tmp1D1=
_tmp1CF.targs;_LL43: {struct Cyc_Absyn_Aggrdecl*_tmp23C=Cyc_Absyn_get_known_aggrdecl(
_tmp1D0);if(_tmp23C->impl != 0  && ((struct Cyc_Absyn_AggrdeclImpl*)_check_null(
_tmp23C->impl))->exist_vars != 0){const char*_tmp3F0;void*_tmp3EF;(_tmp3EF=0,((int(*)(
struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp3F0="gen(): existential type variables not yet supported",
_tag_dyneither(_tmp3F0,sizeof(char),52))),_tag_dyneither(_tmp3EF,sizeof(void*),0)));}
if((void*)_tmp23C->kind == (void*)0){struct Cyc_Absyn_Aggrdecl*_tmp23F=_tmp23C;if(
_tmp23F->impl == 0){struct _dyneither_ptr*_tmp3F1;struct _tuple6*_tmp240=Cyc_Tcgenrep_toplevel_name(((
_tmp3F1=_cycalloc(sizeof(*_tmp3F1)),((_tmp3F1[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp3F1)))));struct Cyc_Core_Opt*_tmp3F2;struct Cyc_Core_Opt*_tmp241=(
_tmp3F2=_cycalloc(sizeof(*_tmp3F2)),((_tmp3F2->v=Cyc_Tcgenrep_gen_vardecl(
_tmp240,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_struct_str)),0,(void*)3,loc),_tmp3F2)));
dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp240,loc),loc),_tmp241,1);goto _LL17;}if(((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp23F->impl))->fields != 0  && ((int(*)(int(*pred)(struct Cyc_Absyn_Aggrfield*),
struct Cyc_List_List*x))Cyc_List_forall)(Cyc_Tcgenrep_has_bitfield,((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp23F->impl))->fields)){int _tmp244=((int(*)(int(*f)(int,struct Cyc_Absyn_Aggrfield*),
int accum,struct Cyc_List_List*x))Cyc_List_fold_left)(Cyc_Tcgenrep_add_bitfield_sizes,
0,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp23F->impl))->fields);int
_tmp245=_tmp244 / 8 + (_tmp244 % 8 == 0?0: 1);struct Cyc_List_List*chars=0;{int i=0;
for(0;i < _tmp245;++ i){struct _tuple10*_tmp3F5;struct Cyc_List_List*_tmp3F4;chars=((
_tmp3F4=_cycalloc(sizeof(*_tmp3F4)),((_tmp3F4->hd=((_tmp3F5=_cycalloc(sizeof(*
_tmp3F5)),((_tmp3F5->f1=Cyc_Tcgenrep_tq_none,((_tmp3F5->f2=Cyc_Absyn_char_typ,
_tmp3F5)))))),((_tmp3F4->tl=chars,_tmp3F4))))));}}{struct Cyc_Absyn_TupleType_struct
_tmp3F8;struct Cyc_Absyn_TupleType_struct*_tmp3F7;void*base_type=(void*)((_tmp3F7=
_cycalloc(sizeof(*_tmp3F7)),((_tmp3F7[0]=((_tmp3F8.tag=9,((_tmp3F8.f1=chars,
_tmp3F8)))),_tmp3F7))));struct _tuple19 _tmp249;struct Cyc_Dict_Dict _tmp24A;struct
Cyc_Tcgenrep_RepInfo*_tmp24B;struct _tuple19*_tmp248=Cyc_Tcgenrep_lookupRep(te,
dict,loc,base_type);_tmp249=*_tmp248;_tmp24A=_tmp249.f1;_tmp24B=_tmp249.f2;dict=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Tcgenrep_RepInfo*v))
Cyc_Dict_insert)(_tmp24A,type,_tmp24B);}}else{struct _dyneither_ptr*_tmp3F9;
struct _tuple6*_tmp24E=Cyc_Tcgenrep_toplevel_name(((_tmp3F9=_cycalloc(sizeof(*
_tmp3F9)),((_tmp3F9[0]=Cyc_Tcgenrep_make_type_cstring(type),_tmp3F9)))));struct
Cyc_Core_Opt*_tmp3FA;struct Cyc_Core_Opt*_tmp24F=(_tmp3FA=_cycalloc(sizeof(*
_tmp3FA)),((_tmp3FA->v=Cyc_Tcgenrep_gen_vardecl(_tmp24E,Cyc_Tcgenrep_tunionfield_typ(
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_struct_str)),0,(void*)3,loc),_tmp3FA)));dict=Cyc_Tcgenrep_make_fwd_decl_info(
dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(
_tmp24E,loc),loc),_tmp24F,0);{struct Cyc_List_List*_tmp250=((struct Cyc_List_List*(*)(
struct _tuple16*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_select_structfield_nmtype,((struct Cyc_Absyn_AggrdeclImpl*)
_check_null(_tmp23F->impl))->fields);struct Cyc_Core_Opt*_tmp3FB;dict=Cyc_Tcgenrep_buildRepStruct(((
_tmp3FB=_cycalloc(sizeof(*_tmp3FB)),((_tmp3FB->v=_tmp23F->name,_tmp3FB)))),te,
dict,loc,_tmp24E,(void*)2,type,_tmp250);}}}else{struct Cyc_Absyn_Aggrdecl*_tmp254=
_tmp23C;if(_tmp254->tvs != 0){const char*_tmp3FE;void*_tmp3FD;(_tmp3FD=0,((int(*)(
struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((_tmp3FE="gen: unions with parameters not yet supported",
_tag_dyneither(_tmp3FE,sizeof(char),46))),_tag_dyneither(_tmp3FD,sizeof(void*),0)));}
if(_tmp254->impl == 0){struct _dyneither_ptr*_tmp3FF;struct _tuple6*_tmp257=Cyc_Tcgenrep_toplevel_name(((
_tmp3FF=_cycalloc(sizeof(*_tmp3FF)),((_tmp3FF[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp3FF)))));struct Cyc_Core_Opt*_tmp400;struct Cyc_Core_Opt*_tmp258=(
_tmp400=_cycalloc(sizeof(*_tmp400)),((_tmp400->v=Cyc_Tcgenrep_gen_vardecl(
_tmp257,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_typestruct_str),
Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_union_str)),0,(void*)3,loc),_tmp400)));
dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(
Cyc_Absyn_unknownid_exp(_tmp257,loc),loc),_tmp258,1);goto _LL17;}{struct Cyc_List_List*
_tmp25B=((struct Cyc_List_List*(*)(struct _tuple16*(*f)(struct Cyc_Absyn_Aggrfield*),
struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_select_structfield_nmtype,((
struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp254->impl))->fields);struct
_dyneither_ptr*_tmp401;struct _tuple6*_tmp25C=Cyc_Tcgenrep_toplevel_name(((
_tmp401=_cycalloc(sizeof(*_tmp401)),((_tmp401[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp401)))));struct Cyc_Core_Opt*_tmp402;dict=Cyc_Tcgenrep_buildRepUnion(((
_tmp402=_cycalloc(sizeof(*_tmp402)),((_tmp402->v=_tmp254->name,_tmp402)))),te,
dict,loc,_tmp25C,(void*)2,type,_tmp25B);}}goto _LL17;}_LL44: if(_tmp1B7 <= (void*)4)
goto _LL46;if(*((int*)_tmp1B7)!= 11)goto _LL46;_tmp1D2=(void*)((struct Cyc_Absyn_AnonAggrType_struct*)
_tmp1B7)->f1;if((int)_tmp1D2 != 0)goto _LL46;_tmp1D3=((struct Cyc_Absyn_AnonAggrType_struct*)
_tmp1B7)->f2;_LL45: if(_tmp1D3 != 0  && ((int(*)(int(*pred)(struct Cyc_Absyn_Aggrfield*),
struct Cyc_List_List*x))Cyc_List_forall)(Cyc_Tcgenrep_has_bitfield,_tmp1D3)){int
_tmp25F=((int(*)(int(*f)(int,struct Cyc_Absyn_Aggrfield*),int accum,struct Cyc_List_List*
x))Cyc_List_fold_left)(Cyc_Tcgenrep_add_bitfield_sizes,0,_tmp1D3);int _tmp260=
_tmp25F / 8 + (_tmp25F % 8 == 0?0: 1);void*base_type=Cyc_Absyn_array_typ(Cyc_Absyn_char_typ,
Cyc_Tcgenrep_tq_none,(struct Cyc_Absyn_Exp*)Cyc_Tcgenrep_cnst_int(_tmp260,loc),
Cyc_Absyn_true_conref,0);struct _tuple19 _tmp262;struct Cyc_Dict_Dict _tmp263;struct
Cyc_Tcgenrep_RepInfo*_tmp264;struct _tuple19*_tmp261=Cyc_Tcgenrep_lookupRep(te,
dict,loc,base_type);_tmp262=*_tmp261;_tmp263=_tmp262.f1;_tmp264=_tmp262.f2;dict=((
struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Tcgenrep_RepInfo*v))
Cyc_Dict_insert)(_tmp263,type,_tmp264);}else{struct Cyc_List_List*_tmp265=((
struct Cyc_List_List*(*)(struct _tuple16*(*f)(struct Cyc_Absyn_Aggrfield*),struct
Cyc_List_List*x))Cyc_List_map)(Cyc_Tcgenrep_select_structfield_nmtype,_tmp1D3);
const char*_tmp403;struct _tuple6*_tmp266=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((
_tmp403="rep",_tag_dyneither(_tmp403,sizeof(char),4)))));dict=Cyc_Tcgenrep_buildRepStruct(
0,te,dict,loc,_tmp266,(void*)0,type,_tmp265);}goto _LL17;_LL46: if(_tmp1B7 <= (void*)
4)goto _LL48;if(*((int*)_tmp1B7)!= 11)goto _LL48;_tmp1D4=(void*)((struct Cyc_Absyn_AnonAggrType_struct*)
_tmp1B7)->f1;if((int)_tmp1D4 != 1)goto _LL48;_tmp1D5=((struct Cyc_Absyn_AnonAggrType_struct*)
_tmp1B7)->f2;_LL47: {struct Cyc_List_List*_tmp268=((struct Cyc_List_List*(*)(
struct _tuple16*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_select_structfield_nmtype,_tmp1D5);const char*_tmp404;struct _tuple6*
_tmp269=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((_tmp404="rep",
_tag_dyneither(_tmp404,sizeof(char),4)))));dict=Cyc_Tcgenrep_buildRepUnion(0,te,
dict,loc,_tmp269,(void*)0,type,_tmp268);goto _LL17;}_LL48: if(_tmp1B7 <= (void*)4)
goto _LL4A;if(*((int*)_tmp1B7)!= 12)goto _LL4A;_tmp1D6=((struct Cyc_Absyn_EnumType_struct*)
_tmp1B7)->f2;_LL49: if(_tmp1D6 == 0){const char*_tmp407;void*_tmp406;(_tmp406=0,((
int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp407="gen(): enum declaration must be present by now",_tag_dyneither(_tmp407,
sizeof(char),47))),_tag_dyneither(_tmp406,sizeof(void*),0)));}{struct Cyc_Absyn_Enumdecl
_tmp26D=*_tmp1D6;if(_tmp26D.fields == 0){struct _dyneither_ptr*_tmp408;struct
_tuple6*_tmp26E=Cyc_Tcgenrep_toplevel_name(((_tmp408=_cycalloc(sizeof(*_tmp408)),((
_tmp408[0]=Cyc_Tcgenrep_make_type_cstring(type),_tmp408)))));struct Cyc_Core_Opt*
_tmp409;struct Cyc_Core_Opt*_tmp26F=(_tmp409=_cycalloc(sizeof(*_tmp409)),((
_tmp409->v=Cyc_Tcgenrep_gen_vardecl(_tmp26E,Cyc_Tcgenrep_tunionfield_typ(Cyc_Tcgenrep_typerep_name(&
Cyc_Tcgenrep_typestruct_str),Cyc_Tcgenrep_typerep_name(& Cyc_Tcgenrep_enum_str)),
0,(void*)3,loc),_tmp409)));dict=Cyc_Tcgenrep_make_fwd_decl_info(dict,type,(
struct Cyc_Absyn_Exp*)Cyc_Absyn_address_exp(Cyc_Absyn_unknownid_exp(_tmp26E,loc),
loc),_tmp26F,1);goto _LL17;}{struct Cyc_List_List*_tmp272=((struct Cyc_List_List*(*)(
struct _tuple17*(*f)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_select_enumfield_tagnm,(struct Cyc_List_List*)(_tmp26D.fields)->v);
struct _dyneither_ptr*_tmp40A;struct _tuple6*_tmp273=Cyc_Tcgenrep_toplevel_name(((
_tmp40A=_cycalloc(sizeof(*_tmp40A)),((_tmp40A[0]=Cyc_Tcgenrep_make_type_cstring(
type),_tmp40A)))));dict=Cyc_Tcgenrep_buildRepEnum((struct _tuple6**)& _tmp26D.name,
te,dict,loc,_tmp273,(void*)2,type,_tmp272);goto _LL17;}}_LL4A: if(_tmp1B7 <= (void*)
4)goto _LL4C;if(*((int*)_tmp1B7)!= 13)goto _LL4C;_tmp1D7=((struct Cyc_Absyn_AnonEnumType_struct*)
_tmp1B7)->f1;_LL4B: {struct Cyc_List_List*_tmp275=((struct Cyc_List_List*(*)(
struct _tuple17*(*f)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*x))Cyc_List_map)(
Cyc_Tcgenrep_select_enumfield_tagnm,_tmp1D7);const char*_tmp40B;struct _tuple6*
_tmp276=Cyc_Tcgenrep_toplevel_name(Cyc_Tcgenrep_new_gen_id(((_tmp40B="rep",
_tag_dyneither(_tmp40B,sizeof(char),4)))));dict=Cyc_Tcgenrep_buildRepEnum(0,te,
dict,loc,_tmp276,(void*)0,type,_tmp275);goto _LL17;}_LL4C: if(_tmp1B7 <= (void*)4)
goto _LL4E;if(*((int*)_tmp1B7)!= 18)goto _LL4E;_LL4D: {const char*_tmp40E;void*
_tmp40D;(_tmp40D=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp40E="gen() for tag_t<-> not yet supported",_tag_dyneither(_tmp40E,sizeof(
char),37))),_tag_dyneither(_tmp40D,sizeof(void*),0)));}_LL4E: if(_tmp1B7 <= (void*)
4)goto _LL17;if(*((int*)_tmp1B7)!= 17)goto _LL17;_LL4F: {const char*_tmp411;void*
_tmp410;(_tmp410=0,((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(((
_tmp411="gen() for valueof_t<-> not yet supported",_tag_dyneither(_tmp411,
sizeof(char),41))),_tag_dyneither(_tmp410,sizeof(void*),0)));}_LL17:;}{struct
_tuple19*_tmp412;return(_tmp412=_cycalloc(sizeof(*_tmp412)),((_tmp412->f1=dict,((
_tmp412->f2=((struct Cyc_Tcgenrep_RepInfo*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup)(
dict,type),_tmp412)))));}}static int Cyc_Tcgenrep_not_emitted_filter(struct Cyc_Tcgenrep_RepInfo*
ri);static int Cyc_Tcgenrep_not_emitted_filter(struct Cyc_Tcgenrep_RepInfo*ri){
return ri->emitted == 0;}static void Cyc_Tcgenrep_mark_emitted(struct Cyc_Tcgenrep_RepInfo*
ri);static void Cyc_Tcgenrep_mark_emitted(struct Cyc_Tcgenrep_RepInfo*ri){ri->emitted=
1;}struct _tuple22{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Exp*
f3;};struct _tuple22 Cyc_Tcgenrep_tcGenrep(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*
ge,struct Cyc_Position_Segment*loc,void*type,struct Cyc_Dict_Dict dict);struct
_tuple22 Cyc_Tcgenrep_tcGenrep(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Genv*ge,
struct Cyc_Position_Segment*loc,void*type,struct Cyc_Dict_Dict dict){Cyc_Tcutil_check_valid_toplevel_type(
loc,te,type);Cyc_Tcutil_check_valid_toplevel_type(loc,te,type);{struct _tuple19
_tmp27E;struct Cyc_Dict_Dict _tmp27F;struct Cyc_Tcgenrep_RepInfo*_tmp280;struct
_tuple19*_tmp27D=Cyc_Tcgenrep_lookupRep(te,dict,loc,type);_tmp27E=*_tmp27D;
_tmp27F=_tmp27E.f1;_tmp280=_tmp27E.f2;{struct Cyc_List_List*_tmp281=Cyc_Tcgenrep_dfs(
_tmp280);struct _tuple22 _tmp413;return(_tmp413.f1=_tmp27F,((_tmp413.f2=_tmp281,((
_tmp413.f3=(struct Cyc_Absyn_Exp*)_check_null(_tmp280->exp),_tmp413)))));}}}