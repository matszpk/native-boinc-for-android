#ifndef _MSC_INTTYPES_H
#define _MSC_INTTYPES_H

#define PRId32 "d"
#define PRIu32 "u"
#define PRId64 "I64d"
#define PRIu64 "I64u"

#ifdef _WIN64
#define PRIdFAST32 "I64d"
#define PRIuFAST32 "I64u"
#else
#define PRIdFAST32 "d"
#define PRIuFAST32 "u"
#endif


#define SCNd32 "d"
#define SCNu32 "u"
#define SCNd64 "I64d"
#define SCNu64 "I64u"

#ifdef _WIN64
#define SCNdFAST32 "I64d"
#define SCNuFAST32 "I64u"
#else
#define SCNdFAST32 "d"
#define SCNuFAST32 "u"
#endif

#endif /* _MSC_INTTYPES_H */
