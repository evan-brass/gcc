#include <stdio.h>
#include <openacc.h>
#include <alloca.h>
#include <string.h>
#include <gomp-constants.h>
#include <stdlib.h>

#if 0
#define DEBUG(DIM, IDX, VAL) \
  fprintf (stderr, "%sdist[%d] = %d\n", (DIM), (IDX), (VAL))
#else
#define DEBUG(DIM, IDX, VAL)
#endif

#define N (32*32*32)

int
check (const char *dim, int *dist, int dimsize)
{
  int ix;
  int exit = 0;

  for (ix = 0; ix < dimsize; ix++)
    {
      DEBUG(dim, ix, dist[ix]);
      if (dist[ix] < (N) / (dimsize + 0.5)
	  || dist[ix] > (N) / (dimsize - 0.5))
	{
	  fprintf (stderr, "did not distribute to %ss (%d not between %d "
		   "and %d)\n", dim, dist[ix], (int) ((N) / (dimsize + 0.5)),
		   (int) ((N) / (dimsize - 0.5)));
	  exit |= 1;
	}
    }

  return exit;
}

int main ()
{
  int ary[N];
  int ix;
  int exit = 0;
  int gangsize = 0, workersize = 0, vectorsize = 0;
  int *gangdist, *workerdist, *vectordist;

  for (ix = 0; ix < N;ix++)
    ary[ix] = -1;

#pragma acc parallel num_gangs(32) num_workers(32) vector_length(32) \
	    copy(ary) copyout(gangsize, workersize, vectorsize)
  {
#pragma acc loop gang worker vector
    for (unsigned ix = 0; ix < N; ix++)
      {
	int g, w, v;

	g = __builtin_goacc_parlevel_id (GOMP_DIM_GANG);
	w = __builtin_goacc_parlevel_id (GOMP_DIM_WORKER);
	v = __builtin_goacc_parlevel_id (GOMP_DIM_VECTOR);

	ary[ix] = (g << 16) | (w << 8) | v;
      }

    gangsize = __builtin_goacc_parlevel_size (GOMP_DIM_GANG);
    workersize = __builtin_goacc_parlevel_size (GOMP_DIM_WORKER);
    vectorsize = __builtin_goacc_parlevel_size (GOMP_DIM_VECTOR);
  }

  gangdist = (int *) alloca (gangsize * sizeof (int));
  workerdist = (int *) alloca (workersize * sizeof (int));
  vectordist = (int *) alloca (vectorsize * sizeof (int));
  memset (gangdist, 0, gangsize * sizeof (int));
  memset (workerdist, 0, workersize * sizeof (int));
  memset (vectordist, 0, vectorsize * sizeof (int));

  /* Test that work is shared approximately equally amongst each active
     gang/worker/vector.  */
  for (ix = 0; ix < N; ix++)
    {
      int g = (ary[ix] >> 16) & 255;
      int w = (ary[ix] >> 8) & 255;
      int v = ary[ix] & 255;

      gangdist[g]++;
      workerdist[w]++;
      vectordist[v]++;
    }

  exit = check ("gang", gangdist, gangsize);
  exit |= check ("worker", workerdist, workersize);
  exit |= check ("vector", vectordist, vectorsize);

  return exit;
}
