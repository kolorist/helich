#ifndef __HELICH_MACROS_H__
#define __HELICH_MACROS_H__

#define     SIZE_KB(X)                          (X * 1024u)
#define     SIZE_MB(X)                          (SIZE_KB(X) * 1024u)
#define     SIZE_GB(X)                          (SIZE_MB(X) * 1024u)
#define     SIZE_TB(X)                          (SIZE_GB(X) * 1024u)

#define		TO_KB(X)							(X / 1024u)
#define		TO_MB(X)							(TO_KB(X) / 1024u)

// constants
#define     HL_ALIGNMENT                        4

// namespaces
#define     BEGIN_HELICH_NAMESPACE              namespace helich {
#define     END_HELICH_NAMESPACE                }

#endif // __HELICH_MACROS_H__
