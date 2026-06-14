
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <fenv.h>

#include "try_sigsafe.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include <math.h>
#include <float.h>
#include <errno.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


#if 1
#define DPRINT(...)  do {  fprintf(stderr, __VA_ARGS__); } while (0)
#else
#define DPRINT(...)  do {  } while (0)
#endif


#define MAX_SIG_HANDLERS  31


/* gcc doesn't know _Thread_local from C11 yet */
/* from https://stackoverflow.com/questions/18298280/how-to-declare-a-variable-as-thread-local-portably */
#ifdef __GNUC__
# define THREAD_LOCAL_STORE __thread
#elif __STDC_VERSION__ >= 201112L
# define THREAD_LOCAL_STORE _Thread_local
#elif defined(_MSC_VER)
# define THREAD_LOCAL_STORE __declspec(thread)
#else
# error Cannot define THREAD_LOCAL_STORE
#endif


#if defined(_MSC_VER)
#define JMP_BUF_T       jmp_buf
#define SET_JMP(BUF)    setjmp(BUF)
#define LONG_JMP        longjmp

#pragma fenv_access(on)

#define FPUENV_T        fenv_t
#define FE_MASK_T       unsigned

#define GET_EXCEPTS()   fetestexcept(FE_ALL_EXCEPT)

#else
#define JMP_BUF_T       sigjmp_buf
#define SET_JMP(BUF)    sigsetjmp(BUF, 1)
#define LONG_JMP        siglongjmp
#define FPUENV_T        fenv_t
#define FE_MASK_T       int

#pragma STDC FENV_ACCESS ON

#define GET_EXCEPTS()  fegetexcept()
#define SIGNAL_TO_ABBREV   sigabbrev_np
#endif

void show_fp_status(void)
{
  int fe_traps;

#if 0
  printf("MATH_ERRNO     is %s\n", math_errhandling & MATH_ERRNO ? "set" : "not set");
  printf("MATH_ERREXCEPT is %s\n", math_errhandling & MATH_ERREXCEPT ? "set" : "not set");
#endif

#if 0
  printf("current rounding direction:  ");
  switch (fegetround())
  {
    case FE_TONEAREST:  printf ("FE_TONEAREST");  break;
    case FE_DOWNWARD:   printf ("FE_DOWNWARD");   break;
    case FE_UPWARD:     printf ("FE_UPWARD");     break;
    case FE_TOWARDZERO: printf ("FE_TOWARDZERO"); break;
    default:            printf ("unknown");
  };
  printf("\n");
#endif

  printf("current exceptions raised:  ");
  if (fetestexcept(FE_DIVBYZERO))     printf(" FE_DIVBYZERO");
  if (fetestexcept(FE_INEXACT))       printf(" FE_INEXACT");
  if (fetestexcept(FE_INVALID))       printf(" FE_INVALID");
  if (fetestexcept(FE_OVERFLOW))      printf(" FE_OVERFLOW");
  if (fetestexcept(FE_UNDERFLOW))     printf(" FE_UNDERFLOW");
  if (fetestexcept(FE_ALL_EXCEPT)==0) printf(" none");
  printf("\n");

  fe_traps = GET_EXCEPTS();
  printf("current raising traps:      ");
  if (fe_traps & FE_DIVBYZERO)        printf(" FE_DIVBYZERO");
  if (fe_traps & FE_INEXACT)          printf(" FE_INEXACT");
  if (fe_traps & FE_INVALID)          printf(" FE_INVALID");
  if (fe_traps & FE_OVERFLOW)         printf(" FE_OVERFLOW");
  if (fe_traps & FE_UNDERFLOW)        printf(" FE_UNDERFLOW");
  if ((fe_traps & FE_ALL_EXCEPT)==0)  printf(" none");
  printf("\n");
#if 0
  printf("errno %d: %s\n\n", errno, errno ? strerror(errno) : "");
#endif
}


typedef void (*SignalHandlerPointer)(int);

typedef struct
{
  JMP_BUF_T* jumpBuffer;
  void* faultAddr;
  int catched_signal;
  int catched_fp_traps;
  int call_prev_handler;
  int num_prev;
  const int *sig_numbers;
  SignalHandlerPointer old_handlers[MAX_SIG_HANDLERS + 1];
} handler_info_t;

#if defined(_MSC_VER)
static __declspec(thread) handler_info_t* ptr_handler_info = NULL;
static int ctrl_handler_called = 0;
#else
static THREAD_LOCAL_STORE handler_info_t* ptr_handler_info = NULL;
#endif


#if defined(_MSC_VER)

static const char* SIGNAL_TO_ABBREV(int sig)
{
  if (sig == SIGABRT)
    return "SIGABRT";
  else if (sig == SIGABRT_COMPAT)
    return "SIGABRT_COMPAT";
  else if (sig == SIGFPE)
    return "SIGFPE";
  else if (sig == SIGILL)
    return "SIGILL";
  else if (sig == SIGINT)
    return "SIGINT";
  else if (sig == SIGSEGV)
    return "SIGSEGV";
  else if (sig == SIGTERM)
    return "SIGTERM";
  else
    return "SIG_UK";
}

#endif


static int dflt_call_prev_handler(int signal)
{
  if ( signal == SIGSEGV )
    return 0;
  else if ( signal == SIGILL )
    return 0;
  else if ( signal == SIGINT )
    return 0;
  /* SIGABRT, SIGTERM */
  return 1;
}

static int FEToFaultTraps(FE_MASK_T fe_mask);

static void sigsafe_handler(int signal)
{
  // handler_info_t* phandler = (handler_info_t*)atomic_load( &ptr_handler_info );
  handler_info_t* phandler = ptr_handler_info;
  if ( phandler )
  {
    JMP_BUF_T* jumpBuffer = phandler->jumpBuffer;

    if (signal == SIGFPE) {
      int fe_traps = GET_EXCEPTS();
      phandler->catched_fp_traps = FEToFaultTraps(fe_traps);
    }

    if ( phandler->call_prev_handler && dflt_call_prev_handler(signal) )
    {
      for ( int k = 0; k < phandler->num_prev; ++k ) {
        if ( phandler->sig_numbers[k] == signal ) {
          (phandler->old_handlers[k])(signal);
          break;
        }
      }
      if ( signal == SIGFPE && phandler->old_handlers[MAX_SIG_HANDLERS] ) {
        (phandler->old_handlers[MAX_SIG_HANDLERS])(signal);
      }
    }
    phandler->catched_signal = signal;
    LONG_JMP(*jumpBuffer, 1 );
  }
  abort();
}

int sigsafe_clear_interrupt()
{
#ifdef _MSC_VER
  int value = ctrl_handler_called;
  ctrl_handler_called = 0;
  return value;
#else
  return 0;
#endif
}


void sigsafe_check_interrupt(int clear_interrupt)
{
#ifdef _MSC_VER
  if (ctrl_handler_called)
  {
    if (clear_interrupt)
      ctrl_handler_called = 0;
    /* sigsafe_handler(SIGINT); */
    handler_info_t* phandler = ptr_handler_info;
    if (phandler)
    {
      JMP_BUF_T* jumpBuffer = phandler->jumpBuffer;
      phandler->catched_signal = SIGINT;
      LONG_JMP(*jumpBuffer, 1);
    }
  }
#endif
}


#ifdef _MSC_VER

static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
  /* fprintf(stderr, "Windows CtrlHandler(%d)\n", (int)fdwCtrlType); */

  switch (fdwCtrlType)
  {
    /* Handle the CTRL - C signal. */
  case CTRL_C_EVENT:
    /*
    https://stackoverflow.com/questions/63415756/do-windows-apps-support-sigint-for-user-defined-signal-handlers
    following also doesn't work, cause this is called from a new thread!
    thus, neither LONG_JMP - nor global thread local access to ptr_handler_info possible!
    handler_info_t* phandler = ptr_handler_info;
    if (phandler)
    {
      JMP_BUF_T* jumpBuffer = phandler->jumpBuffer;
      phandler->catched_signal = SIGINT;
      LONG_JMP(*jumpBuffer, 1);
    }
    */
    ctrl_handler_called = 1;
    return TRUE;

    /* CTRL - CLOSE: confirm that the user wants to exit. */
  case CTRL_CLOSE_EVENT:
    ctrl_handler_called = 1;
    return TRUE;

    /* Pass other signals to the next handler. */
  case CTRL_BREAK_EVENT:
    return FALSE;

  case CTRL_LOGOFF_EVENT:
    return FALSE;

  case CTRL_SHUTDOWN_EVENT:
    return FALSE;

  default:
    return FALSE;
  }
}

#endif


static FE_MASK_T FaultTrapsToFE(int fp_traps_mask)
{
  FE_MASK_T fe_mask = 0;
#if defined(_MSC_VER)
  if (fp_traps_mask & SFT_DIVBYZERO)  fe_mask |= _EM_ZERODIVIDE;
  if (fp_traps_mask & SFT_INEXACT)    fe_mask |= _EM_INEXACT;
  if (fp_traps_mask & SFT_INVALID)    fe_mask |= _EM_INVALID;
  if (fp_traps_mask & SFT_OVERFLOW)   fe_mask |= _EM_OVERFLOW;
  if (fp_traps_mask & SFT_UNDERFLOW)  fe_mask |= _EM_UNDERFLOW;
#else
  if (fp_traps_mask & SFT_DIVBYZERO)  fe_mask |= FE_DIVBYZERO;
  if (fp_traps_mask & SFT_INEXACT)    fe_mask |= FE_INEXACT;
  if (fp_traps_mask & SFT_INVALID)    fe_mask |= FE_INVALID;
  if (fp_traps_mask & SFT_OVERFLOW)   fe_mask |= FE_OVERFLOW;
  if (fp_traps_mask & SFT_UNDERFLOW)  fe_mask |= FE_UNDERFLOW;
#endif
  return fe_mask;
}

static int FEToFaultTraps(FE_MASK_T fe_mask)
{
  int fp_traps_mask = 0;
#if defined(_MSC_VER)
  if (fe_mask & _EM_ZERODIVIDE)       fp_traps_mask |= SFT_DIVBYZERO;
  if (fe_mask & _EM_INEXACT)          fp_traps_mask |= SFT_INEXACT;
  if (fe_mask & _EM_INVALID)          fp_traps_mask |= SFT_INVALID;
  if (fe_mask & _EM_OVERFLOW)         fp_traps_mask |= SFT_OVERFLOW;
  if (fe_mask & _EM_UNDERFLOW)        fp_traps_mask |= SFT_UNDERFLOW;
#else
  if (fe_mask & FE_DIVBYZERO)         fp_traps_mask |= SFT_DIVBYZERO;
  if (fe_mask & FE_INEXACT)           fp_traps_mask |= SFT_INEXACT;
  if (fe_mask & FE_INVALID)           fp_traps_mask |= SFT_INVALID;
  if (fe_mask & FE_OVERFLOW)          fp_traps_mask |= SFT_OVERFLOW;
  if (fe_mask & FE_UNDERFLOW)         fp_traps_mask |= SFT_UNDERFLOW;
#endif
  return fp_traps_mask;
}


static int SaveEnableFPtraps(int fp_traps_mask, FPUENV_T * save_fe_env, FE_MASK_T* prev_fe_traps)
{
  int fe_get_ret = fegetenv(save_fe_env);
  feclearexcept(FE_ALL_EXCEPT);
#if defined(_MSC_VER)
  _controlfp_s(prev_fe_traps, FaultTrapsToFE(fp_traps_mask), _MCW_EM);
#else
  *prev_fe_traps = feenableexcept(FaultTrapsToFE(fp_traps_mask));
#endif
  return fe_get_ret;
}

static void RestoreFPtraps(int saved_ret, const FPUENV_T* saved_fe_env, const FE_MASK_T* prev_fe_traps)
{
#if defined(_MSC_VER)
  FE_MASK_T prev_control;
  _controlfp_s(&prev_control, *prev_fe_traps, _MCW_EM);
#else
  fedisableexcept(FE_ALL_EXCEPT);
  feenableexcept(*prev_fe_traps);
#endif
  if (!saved_ret)
    fesetenv(saved_fe_env);
}




int try_sigsafe( void (*callback)(void *userdata), void* userData, int fault_handlers_mask, int fp_traps_mask, int *catched_signal )
{
  int sig_numbers[6];
  int n_sig_numbers = 0;

  if (fault_handlers_mask & SFH_ABRT)
    sig_numbers[n_sig_numbers++] = SIGABRT;
  if (fault_handlers_mask & SFH_SEGV)
    sig_numbers[n_sig_numbers++] = SIGSEGV;
  if (fault_handlers_mask & SFH_ILL)
    sig_numbers[n_sig_numbers++] = SIGILL;
  if (fault_handlers_mask & SFH_INT)
    sig_numbers[n_sig_numbers++] = SIGINT;
  if (fault_handlers_mask & SFH_TERM)
    sig_numbers[n_sig_numbers++] = SIGTERM;

  return try_sigsafe_ex( callback, userData, fp_traps_mask, catched_signal, sig_numbers, n_sig_numbers );
}


int try_sigsafe_ex( void (*callback)(void *userdata), void* userData, int fp_traps_mask, int *catched_signal, const int * handle_sig_numbers, int num_signos )
{
  typedef void (*SignalHandlerPointer)(int);
  JMP_BUF_T  jumpBuffer;
  handler_info_t handler_info = { &jumpBuffer, NULL, -1, 0 };
  handler_info_t* prev_handler_info;
  FPUENV_T save_fe_env;
  int fe_get_ret;
  FE_MASK_T prev_fe_traps;
  int restore_status;
  int signo_idx;
  int have_fpe_in_list = 0;

  handler_info.call_prev_handler = 1;
  handler_info.num_prev = num_signos;
  handler_info.sig_numbers = handle_sig_numbers;
  handler_info.old_handlers[MAX_SIG_HANDLERS] = NULL;

  if ( num_signos > MAX_SIG_HANDLERS )
    num_signos = MAX_SIG_HANDLERS;

  // prev_handler_info = (handler_info_t*)atomic_exchange( &ptr_handler_info, &handler_info );
  prev_handler_info = ptr_handler_info;
  ptr_handler_info = &handler_info;

  for ( signo_idx = 0; signo_idx < num_signos; ++signo_idx )
  {
    handler_info.old_handlers[signo_idx] = signal(handle_sig_numbers[signo_idx], sigsafe_handler);
    if ( handle_sig_numbers[signo_idx] == SIGFPE )
      have_fpe_in_list = 1;
#ifdef _MSC_VER
    if (handle_sig_numbers[signo_idx] == SIGINT)
      SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif
  }
  if (!have_fpe_in_list && (fp_traps_mask & SFT_FP_TRAPS)) {
    // DPRINT("setting up fpe_handler\n");
    handler_info.old_handlers[MAX_SIG_HANDLERS] = signal(SIGFPE, sigsafe_handler);
  }
  if (fp_traps_mask) {
    fe_get_ret = SaveEnableFPtraps(fp_traps_mask, &save_fe_env, &prev_fe_traps);
  }

  restore_status = SET_JMP(jumpBuffer);
  if (!restore_status)
  {
    // DPRINT("running callback ..\n");
    errno = 0;
    callback(userData);

#ifdef _MSC_VER
    // check once at end
    sigsafe_check_interrupt(0);
#endif
  }

  DPRINT("back from callback .. with errno %d (%s), catched sig %d (%s) and restore_status = %d\n",
    errno, errno ? strerror(errno) : "",
    handler_info.catched_signal,
    handler_info.catched_signal >= 0 ? SIGNAL_TO_ABBREV(handler_info.catched_signal) : "",
    restore_status);

  if (fp_traps_mask) {
    RestoreFPtraps(fe_get_ret, &save_fe_env, &prev_fe_traps);
  }
  if (!have_fpe_in_list && (fp_traps_mask & SFT_FP_TRAPS)) {
    // DPRINT("uninstall fpe_handler\n");
    signal(SIGFPE, handler_info.old_handlers[MAX_SIG_HANDLERS]);
  }

  for ( signo_idx = num_signos -1; signo_idx >= 0; --signo_idx )
  {
    signal(handle_sig_numbers[signo_idx], handler_info.old_handlers[signo_idx]);
#ifdef _MSC_VER
    if (handle_sig_numbers[signo_idx] == SIGINT)
      SetConsoleCtrlHandler(CtrlHandler, FALSE);
#endif
  }

  if (catched_signal)
    *catched_signal = handler_info.catched_signal;

  // atomic_exchange( &ptr_handler_info, prev_handler_info );
  ptr_handler_info = prev_handler_info;

  return restore_status;
}
