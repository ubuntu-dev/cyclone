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
static _INLINE unsigned char *
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
static _INLINE unsigned char *
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
static _INLINE unsigned char *
_check_dynforward_subscript(struct _dynforward_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dynforward_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_curr = _cus_arr.curr;
  unsigned char *_cus_ans = _cus_curr + _cus_elt_sz * _cus_index;
  if (!_cus_arr.last_plus_one) _throw_null();
  if (_cus_ans < _cus_curr || _cus_ans >= _cus_arr.last_plus_one)
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
  unsigned char *_cus_curr = _cus_arr.curr; \
  unsigned char *_cus_ans = _cus_curr + _cus_elt_sz * _cus_index; \
  if (!_cus_arr.last_plus_one) _throw_null(); \
  if (_cus_ans < _cus_curr || _cus_ans >= _cus_arr.last_plus_one) \
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
static _INLINE struct _dynforward_ptr
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
static _INLINE struct _dyneither_ptr *
_init_dyneither_ptr(struct _dyneither_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((unsigned char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
static _INLINE struct _dynforward_ptr *
_init_dynforward_ptr(struct _dynforward_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dynforward_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->curr = _itarr;
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
static _INLINE unsigned char *
_untag_dyneither_ptr(struct _dyneither_ptr arr, 
                     unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
    _throw_arraybounds();
  return _curr;
}
static _INLINE unsigned char *
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
static _INLINE unsigned
_get_dyneither_size(struct _dyneither_ptr arr,unsigned elt_sz) {
  struct _dyneither_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
static _INLINE unsigned
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
static _INLINE struct _dyneither_ptr
_dyneither_ptr_plus(struct _dyneither_ptr arr,unsigned elt_sz,int change) {
  struct _dyneither_ptr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
/* Here we have to worry about wrapping around, so if we go past the
 * end, we set the end to 0. */
static _INLINE struct _dynforward_ptr
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
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,
                            int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
static _INLINE struct _dynforward_ptr
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
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus_post(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  struct _dyneither_ptr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
static _INLINE struct _dynforward_ptr
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

/* the next three routines swap [x] and [y]; not thread safe! */
static _INLINE void _swap_word(void *x, void *y) {
  unsigned long *lx = (unsigned long *)x, *ly = (unsigned long *)y, tmp;
  tmp = *lx;
  *lx = *ly;
  *ly = tmp;
}
static _INLINE void _swap_dynforward(struct _dynforward_ptr *x, 
				    struct _dynforward_ptr *y) {
  struct _dynforward_ptr tmp = *x;
  *x = *y;
  *y = tmp;
}
static _INLINE void _swap_dyneither(struct _dyneither_ptr *x, 
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
18];extern char Cyc_List_Nth[8];struct Cyc_Iter_Iter{void*env;int(*next)(void*env,
void*dest);};int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc___cycFILE;
struct Cyc_Cstdio___abstractFILE;struct Cyc_String_pa_struct{int tag;struct
_dynforward_ptr f1;};struct Cyc_Int_pa_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_struct{
int tag;double f1;};struct Cyc_LongDouble_pa_struct{int tag;long double f1;};struct
Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{int tag;
unsigned long*f1;};struct Cyc_ShortPtr_sa_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{
int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{
int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct
_dynforward_ptr f1;};struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _dynforward_ptr f1;};
int Cyc_getw(struct Cyc___cycFILE*);extern char Cyc_FileCloseError[19];extern char Cyc_FileOpenError[
18];struct Cyc_FileOpenError_struct{char*tag;struct _dynforward_ptr f1;};struct Cyc_Dict_T;
struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;struct Cyc_Dict_T*
t;};extern char Cyc_Dict_Present[12];extern char Cyc_Dict_Absent[11];struct Cyc_Dict_Dict
Cyc_Dict_empty(int(*cmp)(void*,void*));struct Cyc_Dict_Dict Cyc_Dict_rempty(struct
_RegionHandle*,int(*cmp)(void*,void*));struct Cyc_Dict_Dict Cyc_Dict_rshare(struct
_RegionHandle*,struct Cyc_Dict_Dict);int Cyc_Dict_is_empty(struct Cyc_Dict_Dict d);
int Cyc_Dict_cardinality(struct Cyc_Dict_Dict d);int Cyc_Dict_member(struct Cyc_Dict_Dict
d,void*k);struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
struct Cyc_Dict_Dict Cyc_Dict_insert_new(struct Cyc_Dict_Dict d,void*k,void*v);
struct Cyc_Dict_Dict Cyc_Dict_inserts(struct Cyc_Dict_Dict d,struct Cyc_List_List*l);
struct Cyc_Dict_Dict Cyc_Dict_singleton(int(*cmp)(void*,void*),void*k,void*v);
struct Cyc_Dict_Dict Cyc_Dict_rsingleton(struct _RegionHandle*,int(*cmp)(void*,void*),
void*k,void*v);void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);void**Cyc_Dict_lookup_opt(
struct Cyc_Dict_Dict d,void*k);int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*k,
void**ans);void*Cyc_Dict_fold(void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d,
void*accum);void*Cyc_Dict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,
struct Cyc_Dict_Dict d,void*accum);void Cyc_Dict_app(void*(*f)(void*,void*),struct
Cyc_Dict_Dict d);void Cyc_Dict_app_c(void*(*f)(void*,void*,void*),void*env,struct
Cyc_Dict_Dict d);void Cyc_Dict_iter(void(*f)(void*,void*),struct Cyc_Dict_Dict d);
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
void Cyc_Dict_iter2(void(*f)(void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict
d2);void Cyc_Dict_iter2_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict
d1,struct Cyc_Dict_Dict d2);void*Cyc_Dict_fold2_c(void*(*f)(void*,void*,void*,void*,
void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2,void*accum);struct
Cyc_Dict_Dict Cyc_Dict_rcopy(struct _RegionHandle*,struct Cyc_Dict_Dict);struct Cyc_Dict_Dict
Cyc_Dict_copy(struct Cyc_Dict_Dict);struct Cyc_Dict_Dict Cyc_Dict_map(void*(*f)(
void*),struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rmap(struct
_RegionHandle*,void*(*f)(void*),struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_map_c(
void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rmap_c(
struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d);
struct Cyc_Dict_Dict Cyc_Dict_union_two_c(void*(*f)(void*,void*,void*,void*),void*
env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict Cyc_Dict_intersect(
void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct
Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(void*,void*,void*,void*),void*env,
struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);int Cyc_Dict_forall_c(int(*f)(void*,
void*,void*),void*env,struct Cyc_Dict_Dict d);int Cyc_Dict_forall_intersect(int(*f)(
void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct _tuple0{
void*f1;void*f2;};struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict
d);struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*,struct Cyc_Dict_Dict d);
struct Cyc_List_List*Cyc_Dict_to_list(struct Cyc_Dict_Dict d);struct Cyc_List_List*
Cyc_Dict_rto_list(struct _RegionHandle*,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict
Cyc_Dict_filter(int(*f)(void*,void*),struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict
Cyc_Dict_rfilter(struct _RegionHandle*,int(*f)(void*,void*),struct Cyc_Dict_Dict d);
struct Cyc_Dict_Dict Cyc_Dict_filter_c(int(*f)(void*,void*,void*),void*env,struct
Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rfilter_c(struct _RegionHandle*,int(*
f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_difference(
struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict Cyc_Dict_rdifference(
struct _RegionHandle*,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict
Cyc_Dict_delete(struct Cyc_Dict_Dict,void*);struct Cyc_Dict_Dict Cyc_Dict_rdelete(
struct _RegionHandle*,struct Cyc_Dict_Dict,void*);struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(
struct Cyc_Dict_Dict,void*);struct Cyc_Iter_Iter Cyc_Dict_make_iter(struct
_RegionHandle*rgn,struct Cyc_Dict_Dict d);void*Cyc_Dict_marshal(struct
_RegionHandle*rgn,void*env,void*(*write_key)(void*,struct Cyc___cycFILE*,void*),
void*(*write_val)(void*,struct Cyc___cycFILE*,void*),struct Cyc___cycFILE*fp,
struct Cyc_Dict_Dict dict);struct Cyc_Dict_Dict Cyc_Dict_unmarshal(struct
_RegionHandle*rgn,void**env,int(*cmp)(void*,void*),void*(*read_key)(void**,
struct Cyc___cycFILE*),void*(*read_val)(void**,struct Cyc___cycFILE*),struct Cyc___cycFILE*
fp);char Cyc_Dict_Absent[11]="\000\000\000\000Absent\000";char Cyc_Dict_Present[12]="\000\000\000\000Present\000";
struct Cyc_Dict_T{void*color;struct Cyc_Dict_T*left;struct Cyc_Dict_T*right;struct
_tuple0 key_val;};struct Cyc_Dict_Dict;struct Cyc_Dict_Dict Cyc_Dict_rempty(struct
_RegionHandle*r,int(*comp)(void*,void*));struct Cyc_Dict_Dict Cyc_Dict_rempty(
struct _RegionHandle*r,int(*comp)(void*,void*)){struct Cyc_Dict_Dict _tmp121;return(
_tmp121.rel=comp,((_tmp121.r=r,((_tmp121.t=0,_tmp121)))));}struct Cyc_Dict_Dict
Cyc_Dict_empty(int(*comp)(void*,void*));struct Cyc_Dict_Dict Cyc_Dict_empty(int(*
comp)(void*,void*)){return Cyc_Dict_rempty(Cyc_Core_heap_region,comp);}struct Cyc_Dict_Dict
Cyc_Dict_rsingleton(struct _RegionHandle*r,int(*comp)(void*,void*),void*key,void*
data);struct Cyc_Dict_Dict Cyc_Dict_rsingleton(struct _RegionHandle*r,int(*comp)(
void*,void*),void*key,void*data){struct Cyc_Dict_T*_tmp127;struct _tuple0 _tmp126;
struct Cyc_Dict_Dict _tmp125;return(_tmp125.rel=comp,((_tmp125.r=r,((_tmp125.t=((
_tmp127=_region_malloc(r,sizeof(*_tmp127)),((_tmp127->color=(void*)((void*)1),((
_tmp127->left=0,((_tmp127->right=0,((_tmp127->key_val=((_tmp126.f1=key,((_tmp126.f2=
data,_tmp126)))),_tmp127)))))))))),_tmp125)))));}struct Cyc_Dict_Dict Cyc_Dict_singleton(
int(*comp)(void*,void*),void*key,void*data);struct Cyc_Dict_Dict Cyc_Dict_singleton(
int(*comp)(void*,void*),void*key,void*data){return Cyc_Dict_rsingleton(Cyc_Core_heap_region,
comp,key,data);}struct Cyc_Dict_Dict Cyc_Dict_rshare(struct _RegionHandle*r,struct
Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rshare(struct _RegionHandle*r,struct
Cyc_Dict_Dict d){struct Cyc_Dict_Dict _tmp128;return(_tmp128.rel=d.rel,((_tmp128.r=
r,((_tmp128.t=(struct Cyc_Dict_T*)d.t,_tmp128)))));}int Cyc_Dict_is_empty(struct
Cyc_Dict_Dict d);int Cyc_Dict_is_empty(struct Cyc_Dict_Dict d){return d.t == 0;}int Cyc_Dict_member(
struct Cyc_Dict_Dict d,void*key);int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*key){
int(*_tmp5)(void*,void*)=d.rel;struct Cyc_Dict_T*_tmp6=d.t;while(_tmp6 != 0){int
_tmp7=_tmp5(key,(_tmp6->key_val).f1);if(_tmp7 < 0)_tmp6=_tmp6->left;else{if(_tmp7
> 0)_tmp6=_tmp6->right;else{return 1;}}}return 0;}void*Cyc_Dict_lookup(struct Cyc_Dict_Dict
d,void*key);void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*key){int(*_tmp8)(void*,
void*)=d.rel;struct Cyc_Dict_T*_tmp9=d.t;while(_tmp9 != 0){int _tmpA=_tmp8(key,(
_tmp9->key_val).f1);if(_tmpA < 0)_tmp9=_tmp9->left;else{if(_tmpA > 0)_tmp9=_tmp9->right;
else{return(_tmp9->key_val).f2;}}}(int)_throw((void*)Cyc_Dict_Absent);}void**Cyc_Dict_lookup_opt(
struct Cyc_Dict_Dict d,void*key);void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,
void*key){int(*_tmpB)(void*,void*)=d.rel;struct Cyc_Dict_T*_tmpC=d.t;while(_tmpC
!= 0){int _tmpD=_tmpB(key,(_tmpC->key_val).f1);if(_tmpD < 0)_tmpC=_tmpC->left;
else{if(_tmpD > 0)_tmpC=_tmpC->right;else{return(void**)&(_tmpC->key_val).f2;}}}
return 0;}int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*key,void**ans_place);
int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*key,void**ans_place){int(*
_tmpE)(void*,void*)=d.rel;struct Cyc_Dict_T*_tmpF=d.t;while(_tmpF != 0){int _tmp10=
_tmpE(key,(_tmpF->key_val).f1);if(_tmp10 < 0)_tmpF=_tmpF->left;else{if(_tmp10 > 0)
_tmpF=_tmpF->right;else{*ans_place=(_tmpF->key_val).f2;return 1;}}}return 0;}
struct _tuple1{void*f1;struct Cyc_Dict_T*f2;struct Cyc_Dict_T*f3;struct _tuple0 f4;};
static struct Cyc_Dict_T*Cyc_Dict_balance(struct _RegionHandle*r,struct _tuple1 quad);
static struct Cyc_Dict_T*Cyc_Dict_balance(struct _RegionHandle*r,struct _tuple1 quad){
struct _tuple1 _tmp11=quad;void*_tmp12;struct Cyc_Dict_T*_tmp13;struct Cyc_Dict_T
_tmp14;void*_tmp15;struct Cyc_Dict_T*_tmp16;struct Cyc_Dict_T _tmp17;void*_tmp18;
struct Cyc_Dict_T*_tmp19;struct Cyc_Dict_T*_tmp1A;struct _tuple0 _tmp1B;struct Cyc_Dict_T*
_tmp1C;struct _tuple0 _tmp1D;struct Cyc_Dict_T*_tmp1E;struct _tuple0 _tmp1F;void*
_tmp20;struct Cyc_Dict_T*_tmp21;struct Cyc_Dict_T _tmp22;void*_tmp23;struct Cyc_Dict_T*
_tmp24;struct Cyc_Dict_T*_tmp25;struct Cyc_Dict_T _tmp26;void*_tmp27;struct Cyc_Dict_T*
_tmp28;struct Cyc_Dict_T*_tmp29;struct _tuple0 _tmp2A;struct _tuple0 _tmp2B;struct Cyc_Dict_T*
_tmp2C;struct _tuple0 _tmp2D;void*_tmp2E;struct Cyc_Dict_T*_tmp2F;struct Cyc_Dict_T*
_tmp30;struct Cyc_Dict_T _tmp31;void*_tmp32;struct Cyc_Dict_T*_tmp33;struct Cyc_Dict_T
_tmp34;void*_tmp35;struct Cyc_Dict_T*_tmp36;struct Cyc_Dict_T*_tmp37;struct _tuple0
_tmp38;struct Cyc_Dict_T*_tmp39;struct _tuple0 _tmp3A;struct _tuple0 _tmp3B;void*
_tmp3C;struct Cyc_Dict_T*_tmp3D;struct Cyc_Dict_T*_tmp3E;struct Cyc_Dict_T _tmp3F;
void*_tmp40;struct Cyc_Dict_T*_tmp41;struct Cyc_Dict_T*_tmp42;struct Cyc_Dict_T
_tmp43;void*_tmp44;struct Cyc_Dict_T*_tmp45;struct Cyc_Dict_T*_tmp46;struct _tuple0
_tmp47;struct _tuple0 _tmp48;struct _tuple0 _tmp49;void*_tmp4A;struct Cyc_Dict_T*
_tmp4B;struct Cyc_Dict_T*_tmp4C;struct _tuple0 _tmp4D;_LL1: _tmp12=_tmp11.f1;if((int)
_tmp12 != 1)goto _LL3;_tmp13=_tmp11.f2;if(_tmp13 == 0)goto _LL3;_tmp14=*_tmp13;
_tmp15=(void*)_tmp14.color;if((int)_tmp15 != 0)goto _LL3;_tmp16=_tmp14.left;if(
_tmp16 == 0)goto _LL3;_tmp17=*_tmp16;_tmp18=(void*)_tmp17.color;if((int)_tmp18 != 0)
goto _LL3;_tmp19=_tmp17.left;_tmp1A=_tmp17.right;_tmp1B=_tmp17.key_val;_tmp1C=
_tmp14.right;_tmp1D=_tmp14.key_val;_tmp1E=_tmp11.f3;_tmp1F=_tmp11.f4;_LL2: {
struct Cyc_Dict_T*_tmp12D;struct Cyc_Dict_T*_tmp12C;struct Cyc_Dict_T*_tmp12B;
return(_tmp12B=_region_malloc(r,sizeof(*_tmp12B)),((_tmp12B->color=(void*)((void*)
0),((_tmp12B->left=((_tmp12C=_region_malloc(r,sizeof(*_tmp12C)),((_tmp12C->color=(
void*)((void*)1),((_tmp12C->left=_tmp19,((_tmp12C->right=_tmp1A,((_tmp12C->key_val=
_tmp1B,_tmp12C)))))))))),((_tmp12B->right=((_tmp12D=_region_malloc(r,sizeof(*
_tmp12D)),((_tmp12D->color=(void*)((void*)1),((_tmp12D->left=_tmp1C,((_tmp12D->right=
_tmp1E,((_tmp12D->key_val=_tmp1F,_tmp12D)))))))))),((_tmp12B->key_val=_tmp1D,
_tmp12B)))))))));}_LL3: _tmp20=_tmp11.f1;if((int)_tmp20 != 1)goto _LL5;_tmp21=
_tmp11.f2;if(_tmp21 == 0)goto _LL5;_tmp22=*_tmp21;_tmp23=(void*)_tmp22.color;if((
int)_tmp23 != 0)goto _LL5;_tmp24=_tmp22.left;_tmp25=_tmp22.right;if(_tmp25 == 0)
goto _LL5;_tmp26=*_tmp25;_tmp27=(void*)_tmp26.color;if((int)_tmp27 != 0)goto _LL5;
_tmp28=_tmp26.left;_tmp29=_tmp26.right;_tmp2A=_tmp26.key_val;_tmp2B=_tmp22.key_val;
_tmp2C=_tmp11.f3;_tmp2D=_tmp11.f4;_LL4: {struct Cyc_Dict_T*_tmp132;struct Cyc_Dict_T*
_tmp131;struct Cyc_Dict_T*_tmp130;return(_tmp130=_region_malloc(r,sizeof(*_tmp130)),((
_tmp130->color=(void*)((void*)0),((_tmp130->left=((_tmp131=_region_malloc(r,
sizeof(*_tmp131)),((_tmp131->color=(void*)((void*)1),((_tmp131->left=_tmp24,((
_tmp131->right=_tmp28,((_tmp131->key_val=_tmp2B,_tmp131)))))))))),((_tmp130->right=((
_tmp132=_region_malloc(r,sizeof(*_tmp132)),((_tmp132->color=(void*)((void*)1),((
_tmp132->left=_tmp29,((_tmp132->right=_tmp2C,((_tmp132->key_val=_tmp2D,_tmp132)))))))))),((
_tmp130->key_val=_tmp2A,_tmp130)))))))));}_LL5: _tmp2E=_tmp11.f1;if((int)_tmp2E != 
1)goto _LL7;_tmp2F=_tmp11.f2;_tmp30=_tmp11.f3;if(_tmp30 == 0)goto _LL7;_tmp31=*
_tmp30;_tmp32=(void*)_tmp31.color;if((int)_tmp32 != 0)goto _LL7;_tmp33=_tmp31.left;
if(_tmp33 == 0)goto _LL7;_tmp34=*_tmp33;_tmp35=(void*)_tmp34.color;if((int)_tmp35
!= 0)goto _LL7;_tmp36=_tmp34.left;_tmp37=_tmp34.right;_tmp38=_tmp34.key_val;
_tmp39=_tmp31.right;_tmp3A=_tmp31.key_val;_tmp3B=_tmp11.f4;_LL6: {struct Cyc_Dict_T*
_tmp137;struct Cyc_Dict_T*_tmp136;struct Cyc_Dict_T*_tmp135;return(_tmp135=
_region_malloc(r,sizeof(*_tmp135)),((_tmp135->color=(void*)((void*)0),((_tmp135->left=((
_tmp136=_region_malloc(r,sizeof(*_tmp136)),((_tmp136->color=(void*)((void*)1),((
_tmp136->left=_tmp2F,((_tmp136->right=_tmp36,((_tmp136->key_val=_tmp3B,_tmp136)))))))))),((
_tmp135->right=((_tmp137=_region_malloc(r,sizeof(*_tmp137)),((_tmp137->color=(
void*)((void*)1),((_tmp137->left=_tmp37,((_tmp137->right=_tmp39,((_tmp137->key_val=
_tmp3A,_tmp137)))))))))),((_tmp135->key_val=_tmp38,_tmp135)))))))));}_LL7: _tmp3C=
_tmp11.f1;if((int)_tmp3C != 1)goto _LL9;_tmp3D=_tmp11.f2;_tmp3E=_tmp11.f3;if(
_tmp3E == 0)goto _LL9;_tmp3F=*_tmp3E;_tmp40=(void*)_tmp3F.color;if((int)_tmp40 != 0)
goto _LL9;_tmp41=_tmp3F.left;_tmp42=_tmp3F.right;if(_tmp42 == 0)goto _LL9;_tmp43=*
_tmp42;_tmp44=(void*)_tmp43.color;if((int)_tmp44 != 0)goto _LL9;_tmp45=_tmp43.left;
_tmp46=_tmp43.right;_tmp47=_tmp43.key_val;_tmp48=_tmp3F.key_val;_tmp49=_tmp11.f4;
_LL8: {struct Cyc_Dict_T*_tmp13C;struct Cyc_Dict_T*_tmp13B;struct Cyc_Dict_T*
_tmp13A;return(_tmp13A=_region_malloc(r,sizeof(*_tmp13A)),((_tmp13A->color=(void*)((
void*)0),((_tmp13A->left=((_tmp13B=_region_malloc(r,sizeof(*_tmp13B)),((_tmp13B->color=(
void*)((void*)1),((_tmp13B->left=_tmp3D,((_tmp13B->right=_tmp41,((_tmp13B->key_val=
_tmp49,_tmp13B)))))))))),((_tmp13A->right=((_tmp13C=_region_malloc(r,sizeof(*
_tmp13C)),((_tmp13C->color=(void*)((void*)1),((_tmp13C->left=_tmp45,((_tmp13C->right=
_tmp46,((_tmp13C->key_val=_tmp47,_tmp13C)))))))))),((_tmp13A->key_val=_tmp48,
_tmp13A)))))))));}_LL9: _tmp4A=_tmp11.f1;_tmp4B=_tmp11.f2;_tmp4C=_tmp11.f3;_tmp4D=
_tmp11.f4;_LLA: {struct Cyc_Dict_T*_tmp13D;return(_tmp13D=_region_malloc(r,
sizeof(*_tmp13D)),((_tmp13D->color=(void*)_tmp4A,((_tmp13D->left=_tmp4B,((
_tmp13D->right=_tmp4C,((_tmp13D->key_val=_tmp4D,_tmp13D)))))))));}_LL0:;}static
struct Cyc_Dict_T*Cyc_Dict_ins(struct _RegionHandle*r,int(*rel)(void*,void*),
struct _tuple0 key_val,struct Cyc_Dict_T*t);static struct Cyc_Dict_T*Cyc_Dict_ins(
struct _RegionHandle*r,int(*rel)(void*,void*),struct _tuple0 key_val,struct Cyc_Dict_T*
t){struct Cyc_Dict_T*_tmp5B=t;struct Cyc_Dict_T _tmp5C;void*_tmp5D;struct Cyc_Dict_T*
_tmp5E;struct Cyc_Dict_T*_tmp5F;struct _tuple0 _tmp60;_LLC: if(_tmp5B != 0)goto _LLE;
_LLD: {struct Cyc_Dict_T*_tmp13E;return(_tmp13E=_region_malloc(r,sizeof(*_tmp13E)),((
_tmp13E->color=(void*)((void*)0),((_tmp13E->left=0,((_tmp13E->right=0,((_tmp13E->key_val=
key_val,_tmp13E)))))))));}_LLE: if(_tmp5B == 0)goto _LLB;_tmp5C=*_tmp5B;_tmp5D=(
void*)_tmp5C.color;_tmp5E=_tmp5C.left;_tmp5F=_tmp5C.right;_tmp60=_tmp5C.key_val;
_LLF: {int _tmp62=rel(key_val.f1,_tmp60.f1);if(_tmp62 < 0){struct _tuple1 _tmp13F;
return Cyc_Dict_balance(r,((_tmp13F.f1=_tmp5D,((_tmp13F.f2=Cyc_Dict_ins(r,rel,
key_val,_tmp5E),((_tmp13F.f3=_tmp5F,((_tmp13F.f4=_tmp60,_tmp13F)))))))));}else{
if(_tmp62 > 0){struct _tuple1 _tmp140;return Cyc_Dict_balance(r,((_tmp140.f1=_tmp5D,((
_tmp140.f2=_tmp5E,((_tmp140.f3=Cyc_Dict_ins(r,rel,key_val,_tmp5F),((_tmp140.f4=
_tmp60,_tmp140)))))))));}else{struct Cyc_Dict_T*_tmp141;return(_tmp141=
_region_malloc(r,sizeof(*_tmp141)),((_tmp141->color=(void*)_tmp5D,((_tmp141->left=
_tmp5E,((_tmp141->right=_tmp5F,((_tmp141->key_val=key_val,_tmp141)))))))));}}}
_LLB:;}struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*key,void*
data);struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*key,void*data){
struct _tuple0 _tmp142;struct Cyc_Dict_T*_tmp66=Cyc_Dict_ins(d.r,d.rel,((_tmp142.f1=
key,((_tmp142.f2=data,_tmp142)))),d.t);(void*)(((struct Cyc_Dict_T*)_check_null(
_tmp66))->color=(void*)((void*)1));{struct Cyc_Dict_Dict _tmp143;struct Cyc_Dict_Dict
_tmp67=(_tmp143.rel=d.rel,((_tmp143.r=d.r,((_tmp143.t=_tmp66,_tmp143)))));return
_tmp67;}}struct Cyc_Dict_Dict Cyc_Dict_insert_new(struct Cyc_Dict_Dict d,void*key,
void*data);struct Cyc_Dict_Dict Cyc_Dict_insert_new(struct Cyc_Dict_Dict d,void*key,
void*data){if(Cyc_Dict_member(d,key))(int)_throw((void*)Cyc_Dict_Absent);return
Cyc_Dict_insert(d,key,data);}struct Cyc_Dict_Dict Cyc_Dict_inserts(struct Cyc_Dict_Dict
d,struct Cyc_List_List*kds);struct Cyc_Dict_Dict Cyc_Dict_inserts(struct Cyc_Dict_Dict
d,struct Cyc_List_List*kds){for(0;kds != 0;kds=kds->tl){d=Cyc_Dict_insert(d,(((
struct _tuple0*)kds->hd)[_check_known_subscript_notnull(1,0)]).f1,(((struct
_tuple0*)kds->hd)[_check_known_subscript_notnull(1,0)]).f2);}return d;}static void*
Cyc_Dict_fold_tree(void*(*f)(void*,void*,void*),struct Cyc_Dict_T*t,void*accum);
static void*Cyc_Dict_fold_tree(void*(*f)(void*,void*,void*),struct Cyc_Dict_T*t,
void*accum){struct Cyc_Dict_T _tmp6B;struct Cyc_Dict_T*_tmp6C;struct Cyc_Dict_T*
_tmp6D;struct _tuple0 _tmp6E;void*_tmp6F;void*_tmp70;struct Cyc_Dict_T*_tmp6A=t;
_tmp6B=*_tmp6A;_tmp6C=_tmp6B.left;_tmp6D=_tmp6B.right;_tmp6E=_tmp6B.key_val;
_tmp6F=_tmp6E.f1;_tmp70=_tmp6E.f2;if(_tmp6C != 0)accum=Cyc_Dict_fold_tree(f,(
struct Cyc_Dict_T*)_tmp6C,accum);accum=f(_tmp6F,_tmp70,accum);if(_tmp6D != 0)accum=
Cyc_Dict_fold_tree(f,(struct Cyc_Dict_T*)_tmp6D,accum);return accum;}void*Cyc_Dict_fold(
void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d,void*accum);void*Cyc_Dict_fold(
void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d,void*accum){if(d.t == 0)return
accum;return Cyc_Dict_fold_tree(f,(struct Cyc_Dict_T*)d.t,accum);}static void*Cyc_Dict_fold_tree_c(
void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_T*t,void*accum);
static void*Cyc_Dict_fold_tree_c(void*(*f)(void*,void*,void*,void*),void*env,
struct Cyc_Dict_T*t,void*accum){struct Cyc_Dict_T _tmp72;struct Cyc_Dict_T*_tmp73;
struct Cyc_Dict_T*_tmp74;struct _tuple0 _tmp75;void*_tmp76;void*_tmp77;struct Cyc_Dict_T*
_tmp71=t;_tmp72=*_tmp71;_tmp73=_tmp72.left;_tmp74=_tmp72.right;_tmp75=_tmp72.key_val;
_tmp76=_tmp75.f1;_tmp77=_tmp75.f2;if(_tmp73 != 0)accum=Cyc_Dict_fold_tree_c(f,env,(
struct Cyc_Dict_T*)_tmp73,accum);accum=f(env,_tmp76,_tmp77,accum);if(_tmp74 != 0)
accum=Cyc_Dict_fold_tree_c(f,env,(struct Cyc_Dict_T*)_tmp74,accum);return accum;}
void*Cyc_Dict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict
d,void*accum);void*Cyc_Dict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,
struct Cyc_Dict_Dict d,void*accum){if(d.t == 0)return accum;return Cyc_Dict_fold_tree_c(
f,env,(struct Cyc_Dict_T*)d.t,accum);}static void Cyc_Dict_app_tree(void*(*f)(void*,
void*),struct Cyc_Dict_T*t);static void Cyc_Dict_app_tree(void*(*f)(void*,void*),
struct Cyc_Dict_T*t){struct Cyc_Dict_T _tmp79;struct Cyc_Dict_T*_tmp7A;struct Cyc_Dict_T*
_tmp7B;struct _tuple0 _tmp7C;void*_tmp7D;void*_tmp7E;struct Cyc_Dict_T*_tmp78=t;
_tmp79=*_tmp78;_tmp7A=_tmp79.left;_tmp7B=_tmp79.right;_tmp7C=_tmp79.key_val;
_tmp7D=_tmp7C.f1;_tmp7E=_tmp7C.f2;if(_tmp7A != 0)Cyc_Dict_app_tree(f,(struct Cyc_Dict_T*)
_tmp7A);f(_tmp7D,_tmp7E);if(_tmp7B != 0)Cyc_Dict_app_tree(f,(struct Cyc_Dict_T*)
_tmp7B);}void Cyc_Dict_app(void*(*f)(void*,void*),struct Cyc_Dict_Dict d);void Cyc_Dict_app(
void*(*f)(void*,void*),struct Cyc_Dict_Dict d){if(d.t != 0)Cyc_Dict_app_tree(f,(
struct Cyc_Dict_T*)d.t);}static void Cyc_Dict_app_tree_c(void*(*f)(void*,void*,void*),
void*env,struct Cyc_Dict_T*t);static void Cyc_Dict_app_tree_c(void*(*f)(void*,void*,
void*),void*env,struct Cyc_Dict_T*t){struct Cyc_Dict_T _tmp80;struct Cyc_Dict_T*
_tmp81;struct Cyc_Dict_T*_tmp82;struct _tuple0 _tmp83;void*_tmp84;void*_tmp85;
struct Cyc_Dict_T*_tmp7F=t;_tmp80=*_tmp7F;_tmp81=_tmp80.left;_tmp82=_tmp80.right;
_tmp83=_tmp80.key_val;_tmp84=_tmp83.f1;_tmp85=_tmp83.f2;if(_tmp81 != 0)Cyc_Dict_app_tree_c(
f,env,(struct Cyc_Dict_T*)_tmp81);f(env,_tmp84,_tmp85);if(_tmp82 != 0)Cyc_Dict_app_tree_c(
f,env,(struct Cyc_Dict_T*)_tmp82);}void Cyc_Dict_app_c(void*(*f)(void*,void*,void*),
void*env,struct Cyc_Dict_Dict d);void Cyc_Dict_app_c(void*(*f)(void*,void*,void*),
void*env,struct Cyc_Dict_Dict d){if(d.t != 0)Cyc_Dict_app_tree_c(f,env,(struct Cyc_Dict_T*)
d.t);}static void Cyc_Dict_iter_tree(void(*f)(void*,void*),struct Cyc_Dict_T*t);
static void Cyc_Dict_iter_tree(void(*f)(void*,void*),struct Cyc_Dict_T*t){struct Cyc_Dict_T
_tmp87;struct Cyc_Dict_T*_tmp88;struct Cyc_Dict_T*_tmp89;struct _tuple0 _tmp8A;void*
_tmp8B;void*_tmp8C;struct Cyc_Dict_T*_tmp86=t;_tmp87=*_tmp86;_tmp88=_tmp87.left;
_tmp89=_tmp87.right;_tmp8A=_tmp87.key_val;_tmp8B=_tmp8A.f1;_tmp8C=_tmp8A.f2;if(
_tmp88 != 0)Cyc_Dict_iter_tree(f,(struct Cyc_Dict_T*)_tmp88);f(_tmp8B,_tmp8C);if(
_tmp89 != 0)Cyc_Dict_iter_tree(f,(struct Cyc_Dict_T*)_tmp89);}void Cyc_Dict_iter(
void(*f)(void*,void*),struct Cyc_Dict_Dict d);void Cyc_Dict_iter(void(*f)(void*,
void*),struct Cyc_Dict_Dict d){if(d.t != 0)Cyc_Dict_iter_tree(f,(struct Cyc_Dict_T*)
d.t);}static void Cyc_Dict_iter_tree_c(void(*f)(void*,void*,void*),void*env,struct
Cyc_Dict_T*t);static void Cyc_Dict_iter_tree_c(void(*f)(void*,void*,void*),void*
env,struct Cyc_Dict_T*t){struct Cyc_Dict_T _tmp8E;struct Cyc_Dict_T*_tmp8F;struct Cyc_Dict_T*
_tmp90;struct _tuple0 _tmp91;void*_tmp92;void*_tmp93;struct Cyc_Dict_T*_tmp8D=t;
_tmp8E=*_tmp8D;_tmp8F=_tmp8E.left;_tmp90=_tmp8E.right;_tmp91=_tmp8E.key_val;
_tmp92=_tmp91.f1;_tmp93=_tmp91.f2;if(_tmp8F != 0)Cyc_Dict_iter_tree_c(f,env,(
struct Cyc_Dict_T*)_tmp8F);f(env,_tmp92,_tmp93);if(_tmp90 != 0)Cyc_Dict_iter_tree_c(
f,env,(struct Cyc_Dict_T*)_tmp90);}void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),
void*env,struct Cyc_Dict_Dict d);void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),
void*env,struct Cyc_Dict_Dict d){if(d.t != 0)Cyc_Dict_iter_tree_c(f,env,(struct Cyc_Dict_T*)
d.t);}static void Cyc_Dict_count_elem(int*cnt,void*a,void*b);static void Cyc_Dict_count_elem(
int*cnt,void*a,void*b){*cnt=*cnt + 1;}int Cyc_Dict_cardinality(struct Cyc_Dict_Dict
d);int Cyc_Dict_cardinality(struct Cyc_Dict_Dict d){int num=0;((void(*)(void(*f)(int*,
void*,void*),int*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c)(Cyc_Dict_count_elem,&
num,d);return num;}struct _tuple2{void(*f1)(void*,void*);struct Cyc_Dict_Dict f2;};
static void Cyc_Dict_iter2_f(struct _tuple2*env,void*a,void*b1);static void Cyc_Dict_iter2_f(
struct _tuple2*env,void*a,void*b1){struct _tuple2 _tmp95;void(*_tmp96)(void*,void*);
struct Cyc_Dict_Dict _tmp97;struct _tuple2*_tmp94=env;_tmp95=*_tmp94;_tmp96=_tmp95.f1;
_tmp97=_tmp95.f2;_tmp96(b1,Cyc_Dict_lookup(_tmp97,a));}void Cyc_Dict_iter2(void(*
f)(void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);void Cyc_Dict_iter2(
void(*f)(void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){struct _tuple2
_tmp144;struct _tuple2 _tmp98=(_tmp144.f1=f,((_tmp144.f2=d2,_tmp144)));((void(*)(
void(*f)(struct _tuple2*,void*,void*),struct _tuple2*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c)(
Cyc_Dict_iter2_f,& _tmp98,d1);}struct _tuple3{void(*f1)(void*,void*,void*);struct
Cyc_Dict_Dict f2;void*f3;};static void Cyc_Dict_iter2_c_f(struct _tuple3*env,void*a,
void*b1);static void Cyc_Dict_iter2_c_f(struct _tuple3*env,void*a,void*b1){struct
_tuple3 _tmp9B;void(*_tmp9C)(void*,void*,void*);struct Cyc_Dict_Dict _tmp9D;void*
_tmp9E;struct _tuple3*_tmp9A=env;_tmp9B=*_tmp9A;_tmp9C=_tmp9B.f1;_tmp9D=_tmp9B.f2;
_tmp9E=_tmp9B.f3;_tmp9C(_tmp9E,b1,Cyc_Dict_lookup(_tmp9D,a));}void Cyc_Dict_iter2_c(
void(*f)(void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict
d2);void Cyc_Dict_iter2_c(void(*f)(void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict
d1,struct Cyc_Dict_Dict d2){struct _tuple3 _tmp145;struct _tuple3 _tmp9F=(_tmp145.f1=f,((
_tmp145.f2=d2,((_tmp145.f3=inner_env,_tmp145)))));((void(*)(void(*f)(struct
_tuple3*,void*,void*),struct _tuple3*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c)(
Cyc_Dict_iter2_c_f,& _tmp9F,d1);}struct _tuple4{void*(*f1)(void*,void*,void*,void*,
void*);struct Cyc_Dict_Dict f2;void*f3;};static void*Cyc_Dict_fold2_c_f(struct
_tuple4*env,void*a,void*b1,void*accum);static void*Cyc_Dict_fold2_c_f(struct
_tuple4*env,void*a,void*b1,void*accum){struct _tuple4 _tmpA2;void*(*_tmpA3)(void*,
void*,void*,void*,void*);struct Cyc_Dict_Dict _tmpA4;void*_tmpA5;struct _tuple4*
_tmpA1=env;_tmpA2=*_tmpA1;_tmpA3=_tmpA2.f1;_tmpA4=_tmpA2.f2;_tmpA5=_tmpA2.f3;
return _tmpA3(_tmpA5,a,b1,Cyc_Dict_lookup(_tmpA4,a),accum);}void*Cyc_Dict_fold2_c(
void*(*f)(void*,void*,void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict d1,
struct Cyc_Dict_Dict d2,void*accum);void*Cyc_Dict_fold2_c(void*(*f)(void*,void*,
void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2,
void*accum){struct _tuple4 _tmp146;struct _tuple4 _tmpA6=(_tmp146.f1=f,((_tmp146.f2=
d2,((_tmp146.f3=inner_env,_tmp146)))));return((void*(*)(void*(*f)(struct _tuple4*,
void*,void*,void*),struct _tuple4*env,struct Cyc_Dict_Dict d,void*accum))Cyc_Dict_fold_c)(
Cyc_Dict_fold2_c_f,& _tmpA6,d1,accum);}static struct Cyc_Dict_T*Cyc_Dict_copy_tree(
struct _RegionHandle*r2,struct Cyc_Dict_T*t);static struct Cyc_Dict_T*Cyc_Dict_copy_tree(
struct _RegionHandle*r2,struct Cyc_Dict_T*t){if(t == 0)return 0;else{void*_tmpA9;
struct Cyc_Dict_T*_tmpAA;struct Cyc_Dict_T*_tmpAB;struct _tuple0 _tmpAC;struct Cyc_Dict_T
_tmpA8=*t;_tmpA9=(void*)_tmpA8.color;_tmpAA=_tmpA8.left;_tmpAB=_tmpA8.right;
_tmpAC=_tmpA8.key_val;{struct Cyc_Dict_T*_tmpAD=Cyc_Dict_copy_tree(r2,_tmpAA);
struct Cyc_Dict_T*_tmpAE=Cyc_Dict_copy_tree(r2,_tmpAB);struct Cyc_Dict_T*_tmp147;
return(_tmp147=_region_malloc(r2,sizeof(*_tmp147)),((_tmp147->color=(void*)
_tmpA9,((_tmp147->left=_tmpAD,((_tmp147->right=_tmpAE,((_tmp147->key_val=_tmpAC,
_tmp147)))))))));}}}struct Cyc_Dict_Dict Cyc_Dict_rcopy(struct _RegionHandle*r2,
struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rcopy(struct _RegionHandle*r2,
struct Cyc_Dict_Dict d){struct Cyc_Dict_Dict _tmp148;return(_tmp148.rel=d.rel,((
_tmp148.r=r2,((_tmp148.t=Cyc_Dict_copy_tree(r2,d.t),_tmp148)))));}struct Cyc_Dict_Dict
Cyc_Dict_copy(struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_copy(struct Cyc_Dict_Dict
d){return Cyc_Dict_rcopy(Cyc_Core_heap_region,d);}static struct Cyc_Dict_T*Cyc_Dict_map_tree(
struct _RegionHandle*r,void*(*f)(void*),struct Cyc_Dict_T*t);static struct Cyc_Dict_T*
Cyc_Dict_map_tree(struct _RegionHandle*r,void*(*f)(void*),struct Cyc_Dict_T*t){
struct Cyc_Dict_T _tmpB2;void*_tmpB3;struct Cyc_Dict_T*_tmpB4;struct Cyc_Dict_T*
_tmpB5;struct _tuple0 _tmpB6;void*_tmpB7;void*_tmpB8;struct Cyc_Dict_T*_tmpB1=t;
_tmpB2=*_tmpB1;_tmpB3=(void*)_tmpB2.color;_tmpB4=_tmpB2.left;_tmpB5=_tmpB2.right;
_tmpB6=_tmpB2.key_val;_tmpB7=_tmpB6.f1;_tmpB8=_tmpB6.f2;{struct Cyc_Dict_T*_tmpB9=
_tmpB4 == 0?0: Cyc_Dict_map_tree(r,f,(struct Cyc_Dict_T*)_tmpB4);void*_tmpBA=f(
_tmpB8);struct Cyc_Dict_T*_tmpBB=_tmpB5 == 0?0: Cyc_Dict_map_tree(r,f,(struct Cyc_Dict_T*)
_tmpB5);struct _tuple0 _tmp14B;struct Cyc_Dict_T*_tmp14A;return(_tmp14A=
_region_malloc(r,sizeof(*_tmp14A)),((_tmp14A->color=(void*)_tmpB3,((_tmp14A->left=
_tmpB9,((_tmp14A->right=_tmpBB,((_tmp14A->key_val=((_tmp14B.f1=_tmpB7,((_tmp14B.f2=
_tmpBA,_tmp14B)))),_tmp14A)))))))));}}struct Cyc_Dict_Dict Cyc_Dict_rmap(struct
_RegionHandle*r,void*(*f)(void*),struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rmap(
struct _RegionHandle*r,void*(*f)(void*),struct Cyc_Dict_Dict d){if(d.t == 0){struct
Cyc_Dict_Dict _tmp14C;return(_tmp14C.rel=d.rel,((_tmp14C.r=r,((_tmp14C.t=0,
_tmp14C)))));}{struct Cyc_Dict_Dict _tmp14D;return(_tmp14D.rel=d.rel,((_tmp14D.r=r,((
_tmp14D.t=Cyc_Dict_map_tree(r,f,(struct Cyc_Dict_T*)d.t),_tmp14D)))));}}struct Cyc_Dict_Dict
Cyc_Dict_map(void*(*f)(void*),struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_map(
void*(*f)(void*),struct Cyc_Dict_Dict d){return Cyc_Dict_rmap(Cyc_Core_heap_region,
f,d);}static struct Cyc_Dict_T*Cyc_Dict_map_tree_c(struct _RegionHandle*r,void*(*f)(
void*,void*),void*env,struct Cyc_Dict_T*t);static struct Cyc_Dict_T*Cyc_Dict_map_tree_c(
struct _RegionHandle*r,void*(*f)(void*,void*),void*env,struct Cyc_Dict_T*t){struct
Cyc_Dict_T _tmpC1;void*_tmpC2;struct Cyc_Dict_T*_tmpC3;struct Cyc_Dict_T*_tmpC4;
struct _tuple0 _tmpC5;void*_tmpC6;void*_tmpC7;struct Cyc_Dict_T*_tmpC0=t;_tmpC1=*
_tmpC0;_tmpC2=(void*)_tmpC1.color;_tmpC3=_tmpC1.left;_tmpC4=_tmpC1.right;_tmpC5=
_tmpC1.key_val;_tmpC6=_tmpC5.f1;_tmpC7=_tmpC5.f2;{struct Cyc_Dict_T*_tmpC8=_tmpC3
== 0?0: Cyc_Dict_map_tree_c(r,f,env,(struct Cyc_Dict_T*)_tmpC3);void*_tmpC9=f(env,
_tmpC7);struct Cyc_Dict_T*_tmpCA=_tmpC4 == 0?0: Cyc_Dict_map_tree_c(r,f,env,(struct
Cyc_Dict_T*)_tmpC4);struct _tuple0 _tmp150;struct Cyc_Dict_T*_tmp14F;return(_tmp14F=
_region_malloc(r,sizeof(*_tmp14F)),((_tmp14F->color=(void*)_tmpC2,((_tmp14F->left=
_tmpC8,((_tmp14F->right=_tmpCA,((_tmp14F->key_val=((_tmp150.f1=_tmpC6,((_tmp150.f2=
_tmpC9,_tmp150)))),_tmp14F)))))))));}}struct Cyc_Dict_Dict Cyc_Dict_rmap_c(struct
_RegionHandle*r,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict
Cyc_Dict_rmap_c(struct _RegionHandle*r,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict
d){if(d.t == 0){struct Cyc_Dict_Dict _tmp151;return(_tmp151.rel=d.rel,((_tmp151.r=r,((
_tmp151.t=0,_tmp151)))));}{struct Cyc_Dict_Dict _tmp152;return(_tmp152.rel=d.rel,((
_tmp152.r=r,((_tmp152.t=Cyc_Dict_map_tree_c(r,f,env,(struct Cyc_Dict_T*)d.t),
_tmp152)))));}}struct Cyc_Dict_Dict Cyc_Dict_map_c(void*(*f)(void*,void*),void*env,
struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_map_c(void*(*f)(void*,void*),
void*env,struct Cyc_Dict_Dict d){return Cyc_Dict_rmap_c(Cyc_Core_heap_region,f,env,
d);}struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict d);
struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict d){if(d.t
== 0)(int)_throw((void*)Cyc_Dict_Absent);{struct _tuple0*_tmp153;return(_tmp153=
_region_malloc(r,sizeof(*_tmp153)),((_tmp153->f1=((d.t)->key_val).f1,((_tmp153->f2=((
d.t)->key_val).f2,_tmp153)))));}}static int Cyc_Dict_forall_tree_c(int(*f)(void*,
void*,void*),void*env,struct Cyc_Dict_T*t);static int Cyc_Dict_forall_tree_c(int(*f)(
void*,void*,void*),void*env,struct Cyc_Dict_T*t){struct Cyc_Dict_T _tmpD1;struct Cyc_Dict_T*
_tmpD2;struct Cyc_Dict_T*_tmpD3;struct _tuple0 _tmpD4;void*_tmpD5;void*_tmpD6;
struct Cyc_Dict_T*_tmpD0=t;_tmpD1=*_tmpD0;_tmpD2=_tmpD1.left;_tmpD3=_tmpD1.right;
_tmpD4=_tmpD1.key_val;_tmpD5=_tmpD4.f1;_tmpD6=_tmpD4.f2;return((_tmpD2 == 0  || 
Cyc_Dict_forall_tree_c(f,env,(struct Cyc_Dict_T*)_tmpD2)) && f(env,_tmpD5,_tmpD6))
 && (_tmpD3 == 0  || Cyc_Dict_forall_tree_c(f,env,(struct Cyc_Dict_T*)_tmpD3));}int
Cyc_Dict_forall_c(int(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);int
Cyc_Dict_forall_c(int(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d){if(d.t
== 0)return 1;return Cyc_Dict_forall_tree_c(f,env,(struct Cyc_Dict_T*)d.t);}struct
_tuple5{int(*f1)(void*,void*,void*);struct Cyc_Dict_Dict f2;};static int Cyc_Dict_forall_intersect_f(
struct _tuple5*env,void*a,void*b);static int Cyc_Dict_forall_intersect_f(struct
_tuple5*env,void*a,void*b){struct _tuple5 _tmpD8;int(*_tmpD9)(void*,void*,void*);
struct Cyc_Dict_Dict _tmpDA;struct _tuple5*_tmpD7=env;_tmpD8=*_tmpD7;_tmpD9=_tmpD8.f1;
_tmpDA=_tmpD8.f2;if(Cyc_Dict_member(_tmpDA,a))return _tmpD9(a,b,Cyc_Dict_lookup(
_tmpDA,a));return 1;}int Cyc_Dict_forall_intersect(int(*f)(void*,void*,void*),
struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);int Cyc_Dict_forall_intersect(int(*f)(
void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){struct _tuple5
_tmp154;struct _tuple5 _tmpDB=(_tmp154.f1=f,((_tmp154.f2=d2,_tmp154)));return((int(*)(
int(*f)(struct _tuple5*,void*,void*),struct _tuple5*env,struct Cyc_Dict_Dict d))Cyc_Dict_forall_c)(
Cyc_Dict_forall_intersect_f,& _tmpDB,d1);}struct _tuple6{void*(*f1)(void*,void*,
void*,void*);void*f2;};static struct Cyc_Dict_Dict*Cyc_Dict_union_f(struct _tuple6*
env,void*a,void*b,struct Cyc_Dict_Dict*d1);static struct Cyc_Dict_Dict*Cyc_Dict_union_f(
struct _tuple6*env,void*a,void*b,struct Cyc_Dict_Dict*d1){if(Cyc_Dict_member(*d1,a)){
void*_tmpDD=Cyc_Dict_lookup(*d1,a);void*_tmpDE=((*env).f1)((*env).f2,a,_tmpDD,b);
if(_tmpDE != _tmpDD)*d1=Cyc_Dict_insert(*d1,a,_tmpDE);return d1;}*d1=Cyc_Dict_insert(*
d1,a,b);return d1;}struct Cyc_Dict_Dict Cyc_Dict_union_two_c(void*(*f)(void*,void*,
void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict
Cyc_Dict_union_two_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict
d1,struct Cyc_Dict_Dict d2){if((int)d1.t == (int)d2.t)return d1;if(d1.t == 0)return d2;
if(d2.t == 0)return d1;{struct _tuple6 _tmp155;struct _tuple6 _tmpDF=(_tmp155.f1=f,((
_tmp155.f2=env,_tmp155)));((struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(
struct _tuple6*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple6*env,struct Cyc_Dict_Dict
d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c)(Cyc_Dict_union_f,& _tmpDF,d2,& d1);
return d1;}}struct Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(void*,void*,void*,
void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);static void _tmp15A(
struct Cyc_Dict_Dict*d2,unsigned int*_tmp159,unsigned int*_tmp158,struct Cyc_Dict_T***
_tmp157){for(*_tmp159=0;*_tmp159 < *_tmp158;(*_tmp159)++){(*_tmp157)[*_tmp159]=(
struct Cyc_Dict_T*)(*d2).t;}}static void _tmp160(struct _dynforward_ptr*queue,struct
Cyc_Dict_Dict*d2,unsigned int*_tmp15F,unsigned int*_tmp15E,struct Cyc_Dict_T***
_tmp15C){for(*_tmp15F=0;*_tmp15F < *_tmp15E;(*_tmp15F)++){(*_tmp15C)[*_tmp15F]=*
_tmp15F < _get_dynforward_size(*queue,sizeof(struct Cyc_Dict_T*))?*((struct Cyc_Dict_T**)
_check_dynforward_subscript(*queue,sizeof(struct Cyc_Dict_T*),(int)*_tmp15F)):(
struct Cyc_Dict_T*)(*d2).t;}}struct Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(
void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
if((int)d1.t == (int)d2.t  || d2.t == 0)return d2;{struct Cyc_Dict_T*_tmpE1=0;{struct
_RegionHandle _tmpE2=_new_region("temp");struct _RegionHandle*temp=& _tmpE2;
_push_region(temp);{unsigned int _tmp159;unsigned int _tmp158;struct Cyc_Dict_T**
_tmp157;unsigned int _tmp156;struct _dynforward_ptr queue=_tag_dynforward(((_tmp156=(
unsigned int)16,((_tmp157=(struct Cyc_Dict_T**)_region_malloc(temp,_check_times(
sizeof(struct Cyc_Dict_T*),_tmp156)),((((_tmp158=_tmp156,_tmp15A(& d2,& _tmp159,&
_tmp158,& _tmp157))),_tmp157)))))),sizeof(struct Cyc_Dict_T*),(unsigned int)16);
int ind=0;while(ind != - 1){struct Cyc_Dict_T _tmpE4;struct Cyc_Dict_T*_tmpE5;struct
Cyc_Dict_T*_tmpE6;struct _tuple0 _tmpE7;void*_tmpE8;void*_tmpE9;struct Cyc_Dict_T*
_tmpE3=*((struct Cyc_Dict_T**)_check_dynforward_subscript(queue,sizeof(struct Cyc_Dict_T*),
ind --));_tmpE4=*_tmpE3;_tmpE5=_tmpE4.left;_tmpE6=_tmpE4.right;_tmpE7=_tmpE4.key_val;
_tmpE8=_tmpE7.f1;_tmpE9=_tmpE7.f2;if(ind + 2 >= _get_dynforward_size(queue,sizeof(
struct Cyc_Dict_T*))){unsigned int _tmp15F;unsigned int _tmp15E;struct
_dynforward_ptr _tmp15D;struct Cyc_Dict_T**_tmp15C;unsigned int _tmp15B;queue=((
_tmp15B=_get_dynforward_size(queue,sizeof(struct Cyc_Dict_T*))* 2,((_tmp15C=(
struct Cyc_Dict_T**)_region_malloc(temp,_check_times(sizeof(struct Cyc_Dict_T*),
_tmp15B)),((_tmp15D=_tag_dynforward(_tmp15C,sizeof(struct Cyc_Dict_T*),_tmp15B),((((
_tmp15E=_tmp15B,_tmp160(& queue,& d2,& _tmp15F,& _tmp15E,& _tmp15C))),_tmp15D))))))));}
if(_tmpE5 != 0)*((struct Cyc_Dict_T**)_check_dynforward_subscript(queue,sizeof(
struct Cyc_Dict_T*),++ ind))=(struct Cyc_Dict_T*)_tmpE5;if(_tmpE6 != 0)*((struct Cyc_Dict_T**)
_check_dynforward_subscript(queue,sizeof(struct Cyc_Dict_T*),++ ind))=(struct Cyc_Dict_T*)
_tmpE6;if(Cyc_Dict_member(d1,_tmpE8)){struct _tuple0 _tmp161;_tmpE1=Cyc_Dict_ins(
d2.r,d2.rel,((_tmp161.f1=_tmpE8,((_tmp161.f2=f(env,_tmpE8,Cyc_Dict_lookup(d1,
_tmpE8),_tmpE9),_tmp161)))),_tmpE1);}}};_pop_region(temp);}{struct Cyc_Dict_Dict
_tmp162;return(_tmp162.rel=d2.rel,((_tmp162.r=d2.r,((_tmp162.t=_tmpE1,_tmp162)))));}}}
static void*Cyc_Dict_intersect_f(void*(*f)(void*,void*,void*),void*a,void*b1,void*
b2);static void*Cyc_Dict_intersect_f(void*(*f)(void*,void*,void*),void*a,void*b1,
void*b2){return f(a,b1,b2);}struct Cyc_Dict_Dict Cyc_Dict_intersect(void*(*f)(void*,
void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict Cyc_Dict_intersect(
void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){return((
struct Cyc_Dict_Dict(*)(void*(*f)(void*(*)(void*,void*,void*),void*,void*,void*),
void*(*env)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2))Cyc_Dict_intersect_c)(
Cyc_Dict_intersect_f,f,d1,d2);}static struct Cyc_List_List*Cyc_Dict_to_list_f(
struct _RegionHandle*r,void*k,void*v,struct Cyc_List_List*accum);static struct Cyc_List_List*
Cyc_Dict_to_list_f(struct _RegionHandle*r,void*k,void*v,struct Cyc_List_List*accum){
struct _tuple0*_tmp165;struct Cyc_List_List*_tmp164;return(_tmp164=_region_malloc(
r,sizeof(*_tmp164)),((_tmp164->hd=((_tmp165=_region_malloc(r,sizeof(*_tmp165)),((
_tmp165->f1=k,((_tmp165->f2=v,_tmp165)))))),((_tmp164->tl=accum,_tmp164)))));}
struct Cyc_List_List*Cyc_Dict_rto_list(struct _RegionHandle*r,struct Cyc_Dict_Dict d);
struct Cyc_List_List*Cyc_Dict_rto_list(struct _RegionHandle*r,struct Cyc_Dict_Dict d){
return((struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _RegionHandle*,
void*,void*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_Dict_Dict d,
struct Cyc_List_List*accum))Cyc_Dict_fold_c)(Cyc_Dict_to_list_f,r,d,0);}struct Cyc_List_List*
Cyc_Dict_to_list(struct Cyc_Dict_Dict d);struct Cyc_List_List*Cyc_Dict_to_list(
struct Cyc_Dict_Dict d){return Cyc_Dict_rto_list(Cyc_Core_heap_region,d);}struct
_tuple7{int(*f1)(void*,void*);struct _RegionHandle*f2;};static struct Cyc_Dict_Dict*
Cyc_Dict_filter_f(struct _tuple7*env,void*x,void*y,struct Cyc_Dict_Dict*acc);
static struct Cyc_Dict_Dict*Cyc_Dict_filter_f(struct _tuple7*env,void*x,void*y,
struct Cyc_Dict_Dict*acc){struct _tuple7 _tmpF6;int(*_tmpF7)(void*,void*);struct
_RegionHandle*_tmpF8;struct _tuple7*_tmpF5=env;_tmpF6=*_tmpF5;_tmpF7=_tmpF6.f1;
_tmpF8=_tmpF6.f2;if(_tmpF7(x,y))*acc=Cyc_Dict_insert(*acc,x,y);return acc;}struct
Cyc_Dict_Dict Cyc_Dict_rfilter(struct _RegionHandle*r2,int(*f)(void*,void*),struct
Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_rfilter(struct _RegionHandle*r2,int(*
f)(void*,void*),struct Cyc_Dict_Dict d){struct _tuple7 _tmp166;struct _tuple7 _tmpF9=(
_tmp166.f1=f,((_tmp166.f2=r2,_tmp166)));struct Cyc_Dict_Dict _tmpFA=Cyc_Dict_rempty(
r2,d.rel);return*((struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(struct
_tuple7*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple7*env,struct Cyc_Dict_Dict
d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c)(Cyc_Dict_filter_f,& _tmpF9,d,&
_tmpFA);}struct Cyc_Dict_Dict Cyc_Dict_filter(int(*f)(void*,void*),struct Cyc_Dict_Dict
d);struct Cyc_Dict_Dict Cyc_Dict_filter(int(*f)(void*,void*),struct Cyc_Dict_Dict d){
return Cyc_Dict_rfilter(Cyc_Core_heap_region,f,d);}struct _tuple8{int(*f1)(void*,
void*,void*);void*f2;struct _RegionHandle*f3;};static struct Cyc_Dict_Dict*Cyc_Dict_filter_c_f(
struct _tuple8*env,void*x,void*y,struct Cyc_Dict_Dict*acc);static struct Cyc_Dict_Dict*
Cyc_Dict_filter_c_f(struct _tuple8*env,void*x,void*y,struct Cyc_Dict_Dict*acc){
struct _tuple8 _tmpFD;int(*_tmpFE)(void*,void*,void*);void*_tmpFF;struct
_RegionHandle*_tmp100;struct _tuple8*_tmpFC=env;_tmpFD=*_tmpFC;_tmpFE=_tmpFD.f1;
_tmpFF=_tmpFD.f2;_tmp100=_tmpFD.f3;if(_tmpFE(_tmpFF,x,y))*acc=Cyc_Dict_insert(*
acc,x,y);return acc;}struct Cyc_Dict_Dict Cyc_Dict_rfilter_c(struct _RegionHandle*r2,
int(*f)(void*,void*,void*),void*f_env,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict
Cyc_Dict_rfilter_c(struct _RegionHandle*r2,int(*f)(void*,void*,void*),void*f_env,
struct Cyc_Dict_Dict d){struct _tuple8 _tmp167;struct _tuple8 _tmp101=(_tmp167.f1=f,((
_tmp167.f2=f_env,((_tmp167.f3=r2,_tmp167)))));struct Cyc_Dict_Dict _tmp102=Cyc_Dict_rempty(
r2,d.rel);return*((struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(struct
_tuple8*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple8*env,struct Cyc_Dict_Dict
d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c)(Cyc_Dict_filter_c_f,& _tmp101,d,&
_tmp102);}struct Cyc_Dict_Dict Cyc_Dict_filter_c(int(*f)(void*,void*,void*),void*
f_env,struct Cyc_Dict_Dict d);struct Cyc_Dict_Dict Cyc_Dict_filter_c(int(*f)(void*,
void*,void*),void*f_env,struct Cyc_Dict_Dict d){return Cyc_Dict_rfilter_c(Cyc_Core_heap_region,
f,f_env,d);}static int Cyc_Dict_difference_f(struct Cyc_Dict_Dict*d,void*x,void*y);
static int Cyc_Dict_difference_f(struct Cyc_Dict_Dict*d,void*x,void*y){return !Cyc_Dict_member(*
d,x);}struct Cyc_Dict_Dict Cyc_Dict_rdifference(struct _RegionHandle*r2,struct Cyc_Dict_Dict
d1,struct Cyc_Dict_Dict d2);struct Cyc_Dict_Dict Cyc_Dict_rdifference(struct
_RegionHandle*r2,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){return((struct Cyc_Dict_Dict(*)(
struct _RegionHandle*r2,int(*f)(struct Cyc_Dict_Dict*,void*,void*),struct Cyc_Dict_Dict*
f_env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c)(r2,Cyc_Dict_difference_f,& d2,d1);}
struct Cyc_Dict_Dict Cyc_Dict_difference(struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict
d2);struct Cyc_Dict_Dict Cyc_Dict_difference(struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict
d2){return Cyc_Dict_rdifference(Cyc_Core_heap_region,d1,d2);}struct _tuple9{int(*
f1)(void*,void*);void*f2;};static int Cyc_Dict_delete_f(struct _tuple9*env,void*x,
void*y);static int Cyc_Dict_delete_f(struct _tuple9*env,void*x,void*y){struct
_tuple9 _tmp105;int(*_tmp106)(void*,void*);void*_tmp107;struct _tuple9*_tmp104=env;
_tmp105=*_tmp104;_tmp106=_tmp105.f1;_tmp107=_tmp105.f2;return _tmp106(_tmp107,x)
!= 0;}struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct _RegionHandle*r2,struct Cyc_Dict_Dict
d,void*x);struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct _RegionHandle*r2,struct Cyc_Dict_Dict
d,void*x){if(!Cyc_Dict_member(d,x))return Cyc_Dict_rcopy(r2,d);{struct _tuple9
_tmp168;struct _tuple9 _tmp108=(_tmp168.f1=d.rel,((_tmp168.f2=x,_tmp168)));return((
struct Cyc_Dict_Dict(*)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),
struct _tuple9*f_env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c)(r2,Cyc_Dict_delete_f,&
_tmp108,d);}}struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(struct Cyc_Dict_Dict d,void*
x);struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(struct Cyc_Dict_Dict d,void*x){if(!Cyc_Dict_member(
d,x))return d;{struct _tuple9 _tmp169;struct _tuple9 _tmp10A=(_tmp169.f1=d.rel,((
_tmp169.f2=x,_tmp169)));return((struct Cyc_Dict_Dict(*)(struct _RegionHandle*r2,
int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d))
Cyc_Dict_rfilter_c)(d.r,Cyc_Dict_delete_f,& _tmp10A,d);}}struct Cyc_Dict_Dict Cyc_Dict_delete(
struct Cyc_Dict_Dict d,void*x);struct Cyc_Dict_Dict Cyc_Dict_delete(struct Cyc_Dict_Dict
d,void*x){return Cyc_Dict_rdelete(Cyc_Core_heap_region,d,x);}struct _tuple10{
struct _dynforward_ptr f1;int f2;};int Cyc_Dict_iter_f(struct _tuple10*stk,struct
_tuple0*dest);int Cyc_Dict_iter_f(struct _tuple10*stk,struct _tuple0*dest){struct
_tuple10 _tmp10D;struct _dynforward_ptr _tmp10E;int _tmp10F;int*_tmp110;struct
_tuple10*_tmp10C=stk;_tmp10D=*_tmp10C;_tmp10E=_tmp10D.f1;_tmp10F=_tmp10D.f2;
_tmp110=(int*)&(*_tmp10C).f2;{int _tmp111=*_tmp110;if(_tmp111 == - 1)return 0;{
struct Cyc_Dict_T*_tmp112=*((struct Cyc_Dict_T**)_check_dynforward_subscript(
_tmp10E,sizeof(struct Cyc_Dict_T*),_tmp111));*dest=((struct Cyc_Dict_T*)
_check_null(_tmp112))->key_val;-- _tmp111;if((unsigned int)_tmp112->left)*((
struct Cyc_Dict_T**)_check_dynforward_subscript(_tmp10E,sizeof(struct Cyc_Dict_T*),
++ _tmp111))=_tmp112->left;if((unsigned int)_tmp112->right)*((struct Cyc_Dict_T**)
_check_dynforward_subscript(_tmp10E,sizeof(struct Cyc_Dict_T*),++ _tmp111))=
_tmp112->right;*_tmp110=_tmp111;return 1;}}}struct Cyc_Iter_Iter Cyc_Dict_make_iter(
struct _RegionHandle*rgn,struct Cyc_Dict_Dict d);static void _tmp16F(unsigned int*
_tmp16E,unsigned int*_tmp16D,struct Cyc_Dict_T***_tmp16B,struct Cyc_Dict_T**
_tmp113){for(*_tmp16E=0;*_tmp16E < *_tmp16D;(*_tmp16E)++){(*_tmp16B)[*_tmp16E]=*
_tmp113;}}struct Cyc_Iter_Iter Cyc_Dict_make_iter(struct _RegionHandle*rgn,struct
Cyc_Dict_Dict d){int half_max_size=1;struct Cyc_Dict_T*_tmp113=d.t;while(_tmp113 != 
0){_tmp113=_tmp113->left;++ half_max_size;}_tmp113=d.t;{unsigned int _tmp16E;
unsigned int _tmp16D;struct _dynforward_ptr _tmp16C;struct Cyc_Dict_T**_tmp16B;
unsigned int _tmp16A;struct _dynforward_ptr _tmp114=(_tmp16A=(unsigned int)(2 * 
half_max_size),((_tmp16B=(struct Cyc_Dict_T**)_region_malloc(rgn,_check_times(
sizeof(struct Cyc_Dict_T*),_tmp16A)),((_tmp16C=_tag_dynforward(_tmp16B,sizeof(
struct Cyc_Dict_T*),_tmp16A),((((_tmp16D=_tmp16A,_tmp16F(& _tmp16E,& _tmp16D,&
_tmp16B,& _tmp113))),_tmp16C)))))));struct _tuple10*_tmp172;struct Cyc_Iter_Iter
_tmp171;return(_tmp171.env=(void*)((_tmp172=_region_malloc(rgn,sizeof(*_tmp172)),((
_tmp172->f1=_tmp114,((_tmp172->f2=(unsigned int)_tmp113?0: - 1,_tmp172)))))),((
_tmp171.next=(int(*)(void*env,void*dest))Cyc_Dict_iter_f,_tmp171)));}}void*Cyc_Dict_marshal(
struct _RegionHandle*rgn,void*env,void*(*write_key)(void*,struct Cyc___cycFILE*,
void*),void*(*write_val)(void*,struct Cyc___cycFILE*,void*),struct Cyc___cycFILE*
fp,struct Cyc_Dict_Dict dict);void*Cyc_Dict_marshal(struct _RegionHandle*rgn,void*
env,void*(*write_key)(void*,struct Cyc___cycFILE*,void*),void*(*write_val)(void*,
struct Cyc___cycFILE*,void*),struct Cyc___cycFILE*fp,struct Cyc_Dict_Dict dict){
struct Cyc_List_List*dict_list=Cyc_Dict_rto_list(rgn,dict);int len=((int(*)(struct
Cyc_List_List*x))Cyc_List_length)(dict_list);{struct Cyc_Core_Failure_struct
_tmp178;const char*_tmp177;struct Cyc_Core_Failure_struct*_tmp176;(int)_throw((
void*)((_tmp176=_cycalloc(sizeof(*_tmp176)),((_tmp176[0]=((_tmp178.tag=Cyc_Core_Failure,((
_tmp178.f1=((_tmp177="Dict::marshal: Write failure",_tag_dynforward(_tmp177,
sizeof(char),_get_zero_arr_size_char(_tmp177,29)))),_tmp178)))),_tmp176)))));}
while(dict_list != 0){env=((void*(*)(void*,struct Cyc___cycFILE*,struct _tuple0*))
write_key)(env,fp,(struct _tuple0*)dict_list->hd);env=((void*(*)(void*,struct Cyc___cycFILE*,
struct _tuple0*))write_val)(env,fp,(struct _tuple0*)dict_list->hd);dict_list=
dict_list->tl;}return env;}struct Cyc_Dict_Dict Cyc_Dict_unmarshal(struct
_RegionHandle*rgn,void**env,int(*cmp)(void*,void*),void*(*read_key)(void**,
struct Cyc___cycFILE*),void*(*read_val)(void**,struct Cyc___cycFILE*),struct Cyc___cycFILE*
fp);struct Cyc_Dict_Dict Cyc_Dict_unmarshal(struct _RegionHandle*rgn,void**env,int(*
cmp)(void*,void*),void*(*read_key)(void**,struct Cyc___cycFILE*),void*(*read_val)(
void**,struct Cyc___cycFILE*),struct Cyc___cycFILE*fp){struct Cyc_Dict_Dict dict=Cyc_Dict_empty(
cmp);int len=Cyc_getw(fp);if(len == - 1){struct Cyc_Core_Failure_struct _tmp17E;const
char*_tmp17D;struct Cyc_Core_Failure_struct*_tmp17C;(int)_throw((void*)((_tmp17C=
_cycalloc(sizeof(*_tmp17C)),((_tmp17C[0]=((_tmp17E.tag=Cyc_Core_Failure,((
_tmp17E.f1=((_tmp17D="Dict::unmarshal: list length is -1",_tag_dynforward(
_tmp17D,sizeof(char),_get_zero_arr_size_char(_tmp17D,35)))),_tmp17E)))),_tmp17C)))));}{
int i=0;for(0;i < len;++ i){void*key=read_key(env,fp);void*val=read_val(env,fp);
dict=Cyc_Dict_insert(dict,key,val);}}return dict;}
