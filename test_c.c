
#include <try_sigsafe.h>
#include <stdio.h>
#include <string.h>

#include <fenv.h>
#include <math.h>
#include <float.h>
#include <errno.h>


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

// cross-platform sleep function
// https://stackoverflow.com/a/28827188/1656706
void sleep_ms(int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}


const char* op_desc(int op)
{
  if (op == 1)
    return "segv";
  else if (op == 2)
    return "nop";
  else if (op == 3)
    return "int/0";
  else if (op == 4)
    return "float/0.0F";
  else if (op == 5)
    return "wait_ctrl+c";
  else if (op >= 100)
  {
    while (op >= 200)
      op -= 100;
    switch (op)
    {
    case 100:  return "1/0.0F";
    case 101:  return "1/3.0";
    case 102:  return "sqrt(-1)";
    case 103:  return "FLT_MAX*2";
    case 104:  return "FLT_MIN/x";
    }
  }
  return "?";
}

void stuff(void *userdata)
{
  int op = *(int*)userdata;
  fprintf(stderr, "\n** stuff(op=%d) ***\n", op);

  if (op == 1)
  {
    int * zptr = NULL;
    while (1)
    {
      fprintf(stderr, "try core at %p ..\n", zptr);
      *zptr++ = -1;
    }
  }
  else if (op == 2)
  {
    // rp.restore_with_status( 3 );
  }
  else if (op == 3)
  {
    int nom = op;
    int den = nom - 3;
    int r;
    fprintf(stderr, "\ninteger division by zero isn't catched on all platforms!\n");
    fprintf(stderr, "r = nom / den = ..\n");
    r = nom / den;
    fprintf(stderr, "r = nom / den = %d\n", r);
  }
  else if (op == 4)
  {
    float nom = (float)op;
    float den = (float)(nom - 4);
    float r;
    show_fp_status();
    fprintf(stderr, "r = nom / den = ..\n");
    r = nom / den;
    fprintf(stderr, "r = nom / den = %f\n", r);
  }
  else if (op == 5)
  {
    sigsafe_clear_interrupt();
    fprintf(stderr, "press Ctrl+C in next 2 secs ..\n");
    for (int k = 0; k < 100; ++k)
    {
      sleep_ms(20);
      sigsafe_check_interrupt(1);  // Windows MSVC needs help!
    }
    fprintf(stderr, "end of sleeps()\n");
  }
  else if (op >= 100)
  {
    /* Perform some computations which raise exceptions. */
    float nom, den, r;
    double dnom, dden, dr;
    unsigned ru;
    while (op >= 200)
      op -= 100;
    show_fp_status();
    switch (op)
    {
    case 100:
      nom = 1.0F;
      den = 0.0F;
      printf("1.0/0.0     = ..\n");
      r = nom / den;
      printf("1.0/0.0     = %f\n", r);  /* FE_DIVBYZERO */
      break;
    case 101:
      nom = 1.0F;
      den = 3.0F;
      dnom = 1.0;
      dden = 3.0;
      printf("1.0/3.0     = ..\n");
      /* r = nom / den; */
      dr = dnom / dden;
      printf("1.0/3.0     = %f\n", dr);  /* FE_INEXACT */
      break;
    case 102:
      nom = -1.0F;
      printf("sqrt(-1)    = ..\n");
      r = sqrt(nom);
      printf("sqrt(-1)    = %f\n", r);  /* FE_INVALID */
      break;
    case 103:
      nom = FLT_MAX;
      printf("FLT_MAX*2.0 = ..\n");
      r = 2.0F * nom;
      printf("FLT_MAX*2.0 = %f\n", r);  /* FE_INEXACT FE_OVERFLOW  */
      break;
    case 104:
      nom = FLT_MIN;
      den = 1024.0F * 1024.0F * 4.0F;    /* -> 0x2 */
      den = 1024.0F * 1024.0F * 16.0F;   /* -> 0x0 */
      den = 1024.0F * 1024.0F * 15.95F;  /* -> 0x1 */
      den = 1024.0F * 1024.0F * 8.0F;    /* -> 0x1 */
      printf("FLT_MIN %g / %f= ..\n", nom, den);
      r = nom / den;
      ru = 0;
      memcpy( &ru, &r, sizeof(float) );
      printf("FLT_MIN %g / %f = %g (=0x%x)\n", nom, den, r, ru);  /* FE_UNDERFLOW  */
      break;
#if 0
      printf("nextafter(DBL_MIN/pow(2.0,52),0.0) = %.1f\n",
           nextafter(DBL_MIN/pow(2.0,52),0.0));   /* FE_INEXACT FE_UNDERFLOW */
#endif
      break;
    }
  }

}


int main(void)
{
  int arg = 2;
  int catched_signo;

  int r = try_sigsafe(stuff, &arg, SFH_DEFAULT, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);

  arg = 1;
  r = try_sigsafe(stuff, &arg, SFH_DEFAULT | SFH_SEGV, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);

  arg = 4;
  r = try_sigsafe(stuff, &arg, SFH_DEFAULT, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);

  arg = 4;
  r = try_sigsafe(stuff, &arg, SFH_DEFAULT, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);

#if 1
  arg = 5;
  r = try_sigsafe(stuff, &arg, SFH_DEFAULT, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
#endif

  for ( int k = 100; k <= 300; k += 100 )
  {
    int trapped[16];
    int errnoed[16];
    int n_trapped = 0;
    int n_errno = 0;
    int sfhs = SFH_DEFAULT;
    int sfts = SFT_DEFAULT;
    if ( k == 200 )
      sfts = sfts;
    if ( k == 300 )
      sfts = SFT_FP_TRAPS;

    fprintf(stderr, "\n\n************************************\n\n");

    arg = k + 0;
    r = try_sigsafe(stuff, &arg, sfhs, sfts, &catched_signo);
    fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
    show_fp_status();
    if (r)
      trapped[n_trapped++] = arg;
    if (errno)
      errnoed[n_errno++] = arg;

    arg = k + 1;
    r = try_sigsafe(stuff, &arg, sfhs, sfts | SFT_INEXACT | SFT_UNDERFLOW, &catched_signo);
    fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
    show_fp_status();
    if (r)
      trapped[n_trapped++] = arg;
    if (errno)
      errnoed[n_errno++] = arg;

    arg = k + 2;
    r = try_sigsafe(stuff, &arg, sfhs, sfts, &catched_signo);
    fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
    show_fp_status();
    if (r)
      trapped[n_trapped++] = arg;
    if (errno)
      errnoed[n_errno++] = arg;

    arg = k + 3;
    r = try_sigsafe(stuff, &arg, sfhs, sfts, &catched_signo);
    fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
    show_fp_status();
    if (r)
      trapped[n_trapped++] = arg;
    if (errno)
      errnoed[n_errno++] = arg;

    arg = k + 4;
    r = try_sigsafe(stuff, &arg, sfhs, sfts | SFT_INEXACT | SFT_UNDERFLOW, &catched_signo);
    fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);
    show_fp_status();
    if (r)
      trapped[n_trapped++] = arg;
    if (errno)
      errnoed[n_errno++] = arg;

    fprintf(stderr, "\ntrapped ops    for %d ..: ", k);
    for ( int j = 0; j < n_trapped; ++j )
      fprintf(stderr, "%d:%s  ", trapped[j], op_desc(trapped[j]));
    fprintf(stderr, "\n");

    fprintf(stderr, "ops with errno for %d ..: ", k);
    for ( int j = 0; j < n_errno; ++j )
      fprintf(stderr, "%d:%s  ", errnoed[j], op_desc(errnoed[j]));
    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\nfinished FPU tests\n");

  arg = 3;
  r = try_sigsafe(stuff, &arg, SFH_DEFAULT, 0, &catched_signo);
  fprintf(stderr, "returned sig %d and ret %d\n\n", catched_signo, r);

  return r;
}
