# ============================================================
# Tuned Parameters — Final (2026-06-24)
#
# Matches NIKE_security_estimator.sage PRESETS
# Cost models: MATZOV, CoreSVP classical, CoreSVP quantum
# Mode: both (rough + full)
#
# Security bits (MATZOV / CoreSVP cl. / CoreSVP q.):
#   NIKE-128:  156.69 / 156.69 / 143.66
#   NIKE-192:  221.61 / 221.61 / 202.45
#   NIKE-256:  303.10 / 303.10 / 275.07
#   NIKE-384:  425.23 / 425.23 / 388.80
#   NIKE-512: 1162.60 / 1157.72 / 1110.32  (n=4096)
# ============================================================

PRESETS = {
    "NIKE-128": dict(
        level=128,  n=1024,
        q=2**16,    p_pk=2**15, p_u=2**15, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=2,   r_discret=2,
    ),
    "NIKE-192": dict(
        level=192,  n=1024,
        q=2**14,    p_pk=2**11, p_u=2**11, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=3,   r_discret=2,
    ),
    "NIKE-256": dict(
        level=256,  n=1024,
        q=2**13,    p_pk=2**8,  p_u=2**8,  p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    ),
    "NIKE-384": dict(
        level=384,  n=2048,
        q=2**16,    p_pk=2**13, p_u=2**13, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    ),
    "NIKE-512": dict(
        level=512,  n=4096,
        q=2**13,    p_pk=2**10, p_u=2**10, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    ),
}
