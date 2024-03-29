#include "devices/timer.h"
#include <float.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* List of sleeping threads. */
static struct list timer_list;

static int counter = 0;

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void)
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
  list_init (&timer_list);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void)
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1))
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (high_bit | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void)
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then)
{
  return timer_ticks () - then;
}

/* Comparing list elements. */
static bool end_time_less (const struct list_elem *a, const struct list_elem *b, void *aux)
{
  struct timer_elem *e1 = list_entry (a, struct timer_elem, elem);
  struct timer_elem *e2 = list_entry (b, struct timer_elem, elem);

  return e1->end_time < e2->end_time;
}

/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks)
{
  enum intr_level old_level;
  struct timer_elem element;

  ASSERT (intr_get_level () == INTR_ON);

  /* Add thread to a list with its end time and block it. */
  old_level = intr_disable ();
	element.thread = thread_current ();
  element.end_time = timer_ticks () + ticks;
  list_insert_ordered (&timer_list, &element.elem, end_time_less, NULL);
  thread_block ();
  intr_set_level (old_level);
}

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms)
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us)
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns)
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms)
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us)
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns)
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void)
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Calculates recent_cpu according to this formula:
   recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice */
static void
calc_recent_cpu(struct thread *t, void *aux UNUSED) {
	struct float32 real = multiply_int(thread_get_real_load_avg (), 2);
	int recent_cpu = to_int(add_int(multiply_int(divide(real, add_int(real, 1)), t->recent_cpu), t->nice), true);
  t->recent_cpu = recent_cpu;
}

/* Calculates priority according to this formula:
   priority = PRI_MAX - (recent_cpu / 4) - (nice * 2) */
static void
calc_priority(struct thread *t, void *aux UNUSED) {

	struct float32 real = to_float(t->recent_cpu);
	struct float32 real4 = to_float(4);
	real = divide(real, real4);
	int calc_p = to_int(multiply_int((subtract_int(add_int(real, t->nice * 2), PRI_MAX)), -1), false);
  t->priority = calc_p;
}


/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();

  /* Wakes up sleeping threads whose time has passed. */
  while (!list_empty(&timer_list))
  {
    struct timer_elem *element = list_entry (list_front(&timer_list), struct timer_elem, elem);
    if (ticks >= element->end_time)
    {
       thread_unblock (element->thread);
       list_pop_front(&timer_list);
    }
    else {
      break;
    }
  }

  if (thread_mlfqs)
  {
    struct float32 real;
    struct float32 real60 = to_float(60);
    struct float32 real59 = to_float(59);
    struct float32 real1 = to_float(1);

    if(timer_ticks () % TIMER_FREQ == 0)
    {
      /* Calculates load_avg according to this formula:
         load_avg = (59/60)*load_avg + (1/60)*ready_threads */
    	int ready_threads = ready_queue_length();
      if (!is_idle_thread (thread_current ()))
        ready_threads++;
    	real = multiply(divide(real59, real60), thread_get_real_load_avg ());
      struct float32 load_avg =  add(real, multiply_int(divide(real1, real60), ready_threads));
    	thread_set_load_avg(load_avg);

      thread_foreach (calc_recent_cpu, NULL);
    }

    if(timer_ticks () % 4 == 0)
    {
    	thread_foreach (calc_priority, NULL);
			intr_yield_on_return ();
    }
  }
}


/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops)
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops)
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */
      timer_sleep (ticks);
    }
  else
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom);
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
}
