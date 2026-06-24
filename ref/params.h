#ifndef PARAMS_H
#define PARAMS_H

#ifndef PARAM_N
#define PARAM_N 1024
#endif

#ifndef PARAM_K
#define PARAM_K 2   /* eta_s=eta_r=2: centered binomial(4)-2, range [-2,2] */
#endif

#ifndef PARAM_Q
#define PARAM_Q 65536 /* 2^16 */
#endif

#ifndef LOG2Q
#define LOG2Q 16
#endif

#define POLY_BYTES (2 * PARAM_N)  /* 2 bytes per coefficient */
#define NEWHOPE_SEEDBYTES 32
#define NEWHOPE_RECBYTES 256

#define NEWHOPE_SENDABYTES (POLY_BYTES + NEWHOPE_SEEDBYTES)
#define NEWHOPE_SENDBBYTES (POLY_BYTES + NEWHOPE_RECBYTES)

#endif
