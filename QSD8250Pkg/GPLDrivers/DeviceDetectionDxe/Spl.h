#include <Device/DeviceType.h>
#define SPL_ADRESS 0x1004

#define LEO_HSPL_142 "SPL-1.42.HSPL"
#define LEO_HSPL_148 "SPL-1.48.HSPL"
#define LEO_HSPL_166 "SPL-1.66.HSPL"
#define LEO_HSPL_172 "SPL-1.72.HSPL"
#define LEO_HSPL_208 "SPL-2.08.HSPL"

#define SCHUBERT_HSPL_31 "HSPL3.1.2250.0"

DeviceType ParseSPLVersion(IN CHAR8* spl_version);