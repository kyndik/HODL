#include <memory.h>
#include <stdlib.h>

#include "miner.h"
//#include "algo-gate-api.h"
#include "hodl-gate.h"
#include "hodl.h"
#include "hodl-wolf.h"

#define HODL_NSTARTLOC_INDEX 20
#define HODL_NFINALCALC_INDEX 21

static struct work hodl_work;

pthread_barrier_t hodl_barrier;

// All references to this buffer are local to this file, so no args
// need to be passed.
unsigned char *hodl_scratchbuf = NULL;

void hodl_set_target( struct work* work, double diff )
{
     diff_to_target(work->target, diff / 8388608.0 );
}

void hodl_le_build_stratum_request( char* req, struct work* work,
                                    struct stratum_ctx *sctx ) 
{
   uint32_t ntime,       nonce,       nstartloc,       nfinalcalc;
   char     ntimestr[9], noncestr[9], nstartlocstr[9], nfinalcalcstr[9];
   unsigned char *xnonce2str;

   le32enc( &ntime, work->data[ algo_gate.ntime_index ] );
   le32enc( &nonce, work->data[ algo_gate.nonce_index ] );
   bin2hex( ntimestr, (char*)(&ntime), sizeof(uint32_t) );
   bin2hex( noncestr, (char*)(&nonce), sizeof(uint32_t) );
   xnonce2str = abin2hex(work->xnonce2, work->xnonce2_len );
   le32enc( &nstartloc,  work->data[ HODL_NSTARTLOC_INDEX ] );
   le32enc( &nfinalcalc, work->data[ HODL_NFINALCALC_INDEX ] );
   bin2hex( nstartlocstr,  (char*)(&nstartloc),  sizeof(uint32_t) );
   bin2hex( nfinalcalcstr, (char*)(&nfinalcalc), sizeof(uint32_t) );
   sprintf( req, "{\"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"], \"id\":4}",
           rpc_user, work->job_id, xnonce2str, ntimestr, noncestr,
           nstartlocstr, nfinalcalcstr );
   free( xnonce2str );
}

void hodl_build_extraheader( struct work* work, struct stratum_ctx *sctx )
{
   work->data[ algo_gate.ntime_index ] = le32dec( sctx->job.ntime );
   work->data[ algo_gate.nbits_index ] = le32dec( sctx->job.nbits );
   work->data[22] = 0x80000000;
   work->data[31] = 0x00000280;
}

// called only by thread 0, saves a backup of g_work
void hodl_init_nonce( struct work* work, struct work* g_work)
{
   if ( memcmp( hodl_work.data, g_work->data, algo_gate.work_cmp_size ) )
   {
      work_free( &hodl_work );
      work_copy( &hodl_work, g_work );
   }
   hodl_work.data[ algo_gate.nonce_index ] = ( clock() + rand() ) % 9999;
}

// called by every thread, copies the backup to each thread's work.
void hodl_resync_threads( struct work* work )
{
   int nonce_index = algo_gate.nonce_index;
   pthread_barrier_wait( &hodl_barrier );
   if ( memcmp( work->data, hodl_work.data, algo_gate.work_cmp_size ) )
   {
      work_free( work );
      work_copy( work, &hodl_work );
   }
   work->data[ nonce_index ] = swab32( hodl_work.data[ nonce_index ] );
}

bool hodl_do_this_thread( int thr_id )
{
  return ( thr_id == 0 );
}

int hodl_scanhash( int thr_id, struct work* work, uint32_t max_nonce,
                   uint64_t *hashes_done )
{
#if defined(NO_AES_NI) && !defined(__AVX__)
  GetPsuedoRandomData( hodl_scratchbuf, work->data, thr_id );
  pthread_barrier_wait( &hodl_barrier );
  return scanhash_hodl( thr_id, work, max_nonce, hashes_done );
#else
  GenRandomGarbage( hodl_scratchbuf, work->data, thr_id );
  pthread_barrier_wait( &hodl_barrier );
  return scanhash_hodl_wolf( thr_id, work, max_nonce, hashes_done );
#endif
}

bool register_hodl_algo( algo_gate_t* gate )
{
  pthread_barrier_init( &hodl_barrier, NULL, opt_n_threads );
  gate->aes_ni_optimized      = true;
  gate->optimizations         = SSE2_OPT | AES_OPT | AVX_OPT | AVX2_OPT;
  gate->scanhash              = (void*)&hodl_scanhash;
  gate->init_nonce            = (void*)&hodl_init_nonce;
  gate->set_target            = (void*)&hodl_set_target;
  gate->build_stratum_request = (void*)&hodl_le_build_stratum_request;
  gate->build_extraheader     = (void*)&hodl_build_extraheader;
  gate->resync_threads        = (void*)&hodl_resync_threads;
  gate->do_this_thread        = (void*)&hodl_do_this_thread;
  hodl_scratchbuf = (unsigned char*)malloc( 1 << 30 );
  return ( hodl_scratchbuf != NULL );
}

