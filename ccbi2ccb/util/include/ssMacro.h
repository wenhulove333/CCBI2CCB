#ifndef __SSMACROS_H_
#define __SSMACROS_H_

#define SS_HOST_IS_BIG_ENDIAN (bool)(*(unsigned short *)"\0\xff" < 0x100)
#define SS_SWAP32(i) ((i & 0x000000ff) << 24 | (i & 0x0000ff00) << 8 | (i & 0x00ff0000) >>8 | (i & 0xff000000) >> 24)
#define SS_SWAP16(i) ((i & 0x00ff) << 8 | (i & 0xff00) >> 8)
#define SS_SWAP_INT32_LITTLE_TO_HOST(i) ((SS_HOST_IS_BIG_ENDIAN == true) ? SS_SWAP32(i) : (i))
#define SS_SWAP_INT16_LITTLE_TO_HOST(i) ((SS_HOST_IS_BIG_ENDIAN == true) ? SS_SWAP16(i) : (i))
#define SS_SWAP_INT32_BIG_TO_HOST(i) ((SS_HOST_IS_BIG_ENDIAN == true) ? (i) : SS_SWAP32(i))
#define SS_SWAP_INT16_BIG_TO_HOST(i) ((SS_HOST_IS_BIG_ENDIAN == true) ? (i) : SS_SWAP16(i))

#endif